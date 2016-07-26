// DiskMultiMap.h

#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include <cstring>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

const int MAX_STRING_LENGTH = 120;

class DiskMultiMap
{
public:
    
    class Iterator
    {
    public:
        Iterator();
        Iterator(DiskMultiMap* dmm, BinaryFile::Offset newAddress);
        
        bool isValid() const;
        Iterator& operator++();
        MultiMapTuple operator*();
        
    private:
        // private member function
        void updateMMT();
        
        // private member variables
        bool m_isValid;
        BinaryFile::Offset m_address;
        DiskMultiMap* m_dmm;
        MultiMapTuple m_MMT;
    };
    
    DiskMultiMap();
    ~DiskMultiMap();
    bool createNew(const std::string& filename, unsigned int numBuckets);
    bool openExisting(const std::string& filename);
    void close();
    bool insert(const std::string& key, const std::string& value, const std::string& context);
    Iterator search(const std::string& key);
    int erase(const std::string& key, const std::string& value, const std::string& context);
    
private:
    // private functions
    unsigned int getHash(std::string s); // uses <functional> to return a hash value
    
    // private structs
    struct hashNode // the 'dummy nodes' that start off all buckets
    {
        hashNode()
        {
            next = 0;
        }
        
        BinaryFile::Offset next;
    };
    
    struct headerNode // at address 0
    {
        unsigned int m_numBuckets;
        BinaryFile::Offset posOfRecentDeleteNode;
        BinaryFile::Offset posOfNextAddedNode;
    };
    
    struct deleteNode // is written over any deleted dataNodes, forms a linked list of deleteNodes
    {
        BinaryFile::Offset next;
    };
    
    struct dataNode // forms a linked list that starts at a hashNode
    {
        BinaryFile::Offset next;
        
        char key[MAX_STRING_LENGTH + 1];
        char value[MAX_STRING_LENGTH + 1];
        char context[MAX_STRING_LENGTH + 1];
    };
    
    // private member variables
    BinaryFile m_bf;
    
};

#endif // DISKMULTIMAP_H_