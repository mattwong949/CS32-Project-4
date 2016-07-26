//  DiskMultiMap.cpp

#define _CRT_SECURE_NO_DEPRECATE

#include "DiskMultiMap.h"
#include <functional>
#include <string>
#include <cstring>

#include <iostream>

unsigned int DiskMultiMap::getHash(std::string s)
{
    std::hash<std::string> str_hash;
    
    headerNode head;
    m_bf.read(head, 0);
    
    return str_hash(s) % head.m_numBuckets;
}


DiskMultiMap::DiskMultiMap()
{ }

DiskMultiMap::~DiskMultiMap()
{
    close();
} // m_bf's destructor closes the file

bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets)
{
    close();
    
    if (m_bf.createNew(filename))
    {
        // add header data and buckets to the hash map
        headerNode head;
        head.m_numBuckets = numBuckets;
        head.posOfRecentDeleteNode = 0;
        head.posOfNextAddedNode = sizeof(headerNode) + numBuckets*sizeof(hashNode);
        m_bf.write(head, 0);
        
        // add "array" of hashNodes into header
        hashNode hNode;
        BinaryFile::Offset place = sizeof(headerNode);
        for (int i = 0; i < numBuckets; i++)
        {
            m_bf.write(hNode, place);
            place += sizeof(hashNode);
        }
        
        return true;
    }
    return false; // failed to create file
}

bool DiskMultiMap::openExisting(const std::string& filename)
{
    close();
    return m_bf.openExisting(filename);
}

void DiskMultiMap::close()
{
    if (m_bf.isOpen())
        m_bf.close();
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{
    if (key.length() > MAX_STRING_LENGTH || value.length() > MAX_STRING_LENGTH || context.length() > MAX_STRING_LENGTH)
        return false;
    
    // find specific hash value to key
    unsigned int hashValue = getHash(key);
    
    // convert strings into cstrings
    const char* ckey = key.c_str();
    const char* cvalue = value.c_str();
    const char* ccontext = context.c_str();
    
    headerNode head;
    m_bf.read(head, 0);
    
    hashNode hNode;
    m_bf.read(hNode, sizeof(headerNode) + sizeof(hashNode) * hashValue);
    
    // create dataNode to insert
    dataNode nodeToAdd;
    strcpy(nodeToAdd.key, ckey);
    strcpy(nodeToAdd.value, cvalue);
    strcpy(nodeToAdd.context, ccontext);
    nodeToAdd.next = hNode.next;
    
    BinaryFile::Offset addressToInsert;
    
    if (head.posOfRecentDeleteNode != 0) // if there is a deleted node whose space we can use
    {
        addressToInsert = head.posOfRecentDeleteNode; // dataNode will occupy deleted space
        
        deleteNode delNode;
        m_bf.read(delNode, head.posOfRecentDeleteNode);
        
        head.posOfRecentDeleteNode = delNode.next; // update the headNode's 'pointer'
    }
    else // otherwise add the node to the next open spot by expanding the binary file
    {
        addressToInsert = head.posOfNextAddedNode; // dataNode will occupy new space
        
        head.posOfNextAddedNode += sizeof(dataNode); // update the headNode's 'pointer'
    }
    
    // write in the dataNode
    m_bf.write(nodeToAdd, addressToInsert);
    
    // update the headerNode
    m_bf.write(head, 0);
    
    hNode.next = addressToInsert;
    m_bf.write(hNode, sizeof(headerNode) + sizeof(hashNode) * hashValue);
    
    return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key)
{
    if (key.length() > MAX_STRING_LENGTH)
    {
        Iterator failedIterator;
        return failedIterator;
    }
    
    // find specific hash value to key
    unsigned int hashValue = getHash(key);
    
    // convert string into cstring
    const char* ckey = key.c_str();
    
    hashNode hNode;
    m_bf.read(hNode, sizeof(headerNode) + sizeof(hashNode) * hashValue);
    
    // determine if key exists in map
    bool foundAtLeastOne = false;
    BinaryFile::Offset currPos = hNode.next;
    while (currPos != 0 && !foundAtLeastOne)
    {
        dataNode data;
        m_bf.read(data, currPos);
        if (strcmp(data.key, ckey) == 0)
        {
            foundAtLeastOne = true;
            break;
        }
        currPos = data.next;
    }
    
    if (foundAtLeastOne)
    {
        BinaryFile::Offset addressOfIterator = currPos; // position iterator at first dataNode of bucket
        
        Iterator goodIterator(this, addressOfIterator);
        return goodIterator;
    }
    else // couldn't find key
    {
        Iterator failedIterator;
        return failedIterator;
    }
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
    if (key.length() > MAX_STRING_LENGTH || value.length() > MAX_STRING_LENGTH || context.length() > MAX_STRING_LENGTH)
        return 0;
    
    // find specific hash value to key
    unsigned int hashValue = getHash(key);
    
    // convert strings into cstrings
    const char* ckey = key.c_str();
    const char* cvalue = value.c_str();
    const char* ccontext = context.c_str();
    
    headerNode head;
    m_bf.read(head, 0);
    
    hashNode hNode;
    m_bf.read(hNode, sizeof(headerNode) + sizeof(hashNode) * hashValue);
    
    // determine if key exists in bucket
    bool foundAtLeastOne = false;
    BinaryFile::Offset currPos = hNode.next;
    while (currPos != 0 && !foundAtLeastOne)
    {
        dataNode data;
        m_bf.read(data, currPos);
        if (strcmp(data.key, ckey) == 0)
            foundAtLeastOne = true;
        currPos = data.next;
    }
    
    if (foundAtLeastOne) // key is within this linked list of data
    {
        int numRemoved = 0;
        
        dataNode currNode;
        dataNode prevNode;
        
        BinaryFile::Offset currPos = hNode.next;
        BinaryFile::Offset prevPos = 0;
        
        while (currPos != 0)
        {
            m_bf.read(currNode, currPos);
            
            if (strcmp(currNode.key, ckey) != 0 || strcmp(currNode.value, cvalue) != 0 || strcmp(currNode.context, ccontext) != 0)
            {
                prevPos = currPos;
                currPos = currNode.next;
            }
            else // found the correct data to erase
            {
                // unhook dataNode from linked list
                BinaryFile::Offset toBeRemoved = currPos;
                currPos = currNode.next;
                
                if (prevPos == 0) // dataNode is the first in the bucket
                {
                    // update hashNode
                    hNode.next = currPos;
                    
                    m_bf.write(hNode, sizeof(headerNode) + sizeof(hashNode) * hashValue);
                }
                else // dataNode is in middle of the bucket
                {
                    // update prevNode
                    m_bf.read(prevNode, prevPos);
                    prevNode.next = currPos;
                    m_bf.write(prevNode, prevPos);
                }
                
                // add in delete node
                deleteNode delNode;
                delNode.next = head.posOfRecentDeleteNode;
                m_bf.write(delNode, toBeRemoved);
                
                // update head node
                head.posOfRecentDeleteNode = toBeRemoved;
                m_bf.write(head, 0);
                
                numRemoved++;
            }
        }
        return numRemoved;
    }
    else // could not find key
        return 0;
}


// ********** ITERATOR **************


DiskMultiMap::Iterator::Iterator() // default iterator is not valid
{
    m_isValid = false;
    m_address = 0;
    m_dmm = nullptr;
    updateMMT();
}

DiskMultiMap::Iterator::Iterator(DiskMultiMap* dmm, BinaryFile::Offset newAddress)
{
    m_dmm = dmm;
    m_address = newAddress;
    
    if (m_address == 0) // not a valid address
        m_isValid = false;
    else
        m_isValid = true;
    
    updateMMT(); // retrieve the appropriate MultiMapTuple from the hashmap
}

bool DiskMultiMap::Iterator::isValid() const
{
    return m_isValid;
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
    if (m_isValid)
    {
        const char* ckey = m_MMT.key.c_str();
        
        dataNode data;
        m_dmm->m_bf.read(data, m_address);
        
        m_address = data.next; // advance the iterator to the next dataNode
        
        while (m_address != 0) // while not at the end of the linked list
        {
            m_dmm->m_bf.read(data, m_address);
            
            if (strcmp(data.key, ckey) != 0) // if we haven't found the correct key
            {
                m_address = data.next; // advance the iterator to the next dataNode
            }
            else
                break;
        }
        
        if (m_address == 0) // no more dataNodes to iterate to, so the iterator becomes invalid
            m_isValid = false;
        
        updateMMT(); // update the MultiMapTuple
    }
    return (*this);
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
    return m_MMT; // return the "cached" MultiMapTuple
}

void DiskMultiMap::Iterator::updateMMT() // update the "cached" information
{
    if (m_isValid) // read in data from hashmap and update the member variable MMT
    {
        dataNode data;
        m_dmm->m_bf.read(data, m_address);
        
        m_MMT.key = data.key;
        m_MMT.value = data.value;
        m_MMT.context = data.context;
    }
    else // iterator is not valid and thus its MultiMapTuple is all empty strings
    {
        m_MMT.key = "";
        m_MMT.value = "";
        m_MMT.context = "";
    }
}
