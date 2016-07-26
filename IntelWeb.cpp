// ELYSE'S FILE


#include "IntelWeb.h"
#include "DiskMultiMap.h"
#include <string>
#include <queue>
#include <unordered_map>
#include <set>

IntelWeb::IntelWeb()
{
}

IntelWeb::~IntelWeb()
{
    goForward.close();
    goReverse.close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
    if (goForward.createNew(filePrefix + "Forward", maxDataItems) && goReverse.createNew(filePrefix + "Reverse", maxDataItems))
        return true;
    
    close();
    return false;
}

bool IntelWeb::openExisting(const std::string& filePrefix)
{
    if (goForward.openExisting(filePrefix + "Forward") && goReverse.openExisting(filePrefix + "Reverse"))
        return true;
    
    close();
    return false;
}

void IntelWeb::close()
{
    goForward.close();
    goReverse.close();
}

bool IntelWeb::ingest(const std::string& telemetryFile)
{
    ifstream inf(telemetryFile);
    if (!inf)
    {
        cout << "couldn't open file" << endl;
        return false;
    }
    
    string line;
    string computer;
    string key, value;
    while(getline(inf, line))
    {
        istringstream iss(line);
        //int incrementer;
        iss >> computer >> key >> value;
        goForward.insert(key, value, computer);
        goReverse.insert(value, key, computer);
    }
    
    return true;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& interactions)
{
    badEntitiesFound.clear();
    interactions.clear();
    
    // compute prevalence
    queue<string> strQueue;
    unordered_map<string, int> prevalence;
    unordered_map<string, int>::iterator mapIter;
    
    for (int i = 0; i < indicators.size(); i++)
        strQueue.push(indicators[i]);
    
    while (!strQueue.empty())
    {
        string entityToCheck = strQueue.front();
        strQueue.pop();
        
        mapIter = prevalence.find(entityToCheck);
        
        cout << "first while loop to check prevalences" << endl;
        
        if (mapIter == prevalence.end())
        {
            int prevalenceOfEntity = 0;
            
            DiskMultiMap::Iterator iter = goForward.search(entityToCheck);
            
            while (iter.isValid())
            {
                prevalenceOfEntity++;
                strQueue.push((*iter).value);
                ++iter;
            }
            
            iter = goReverse.search(entityToCheck);
            
            while (iter.isValid())
            {
                prevalenceOfEntity++;
                strQueue.push((*iter).value);
                ++iter;
            }
            
            prevalence[entityToCheck] = prevalenceOfEntity;
        }
    } // prevalence should all be calculated
    
    
    
    // look for bad entities
    unordered_map<string, int> badEntitiesMap;
    
    for (int i = 0; i < indicators.size(); i++)
        strQueue.push(indicators[i]);
    
    while (!strQueue.empty())
    {
        string entityToCheck = strQueue.front();
        strQueue.pop();
        
        cout << "second while loop" << endl;
        
        mapIter = badEntitiesMap.find(entityToCheck);
        
        if (mapIter == badEntitiesMap.end())
        {
            DiskMultiMap::Iterator iter = goForward.search(entityToCheck);
            
            while (iter.isValid())
            {
                if (prevalence[(*iter).value] < minPrevalenceToBeGood)
                    strQueue.push((*iter).value);
                ++iter;
            }
            
            iter = goReverse.search(entityToCheck);
            
            while (iter.isValid())
            {
                if (prevalence[(*iter).value] < minPrevalenceToBeGood)
                    strQueue.push((*iter).value);
                ++iter;
            }
            
            badEntitiesMap[entityToCheck] = 0;
        }
    }
    
    for (mapIter = badEntitiesMap.begin(); mapIter != badEntitiesMap.end(); mapIter++)
        badEntitiesFound.push_back((*mapIter).first);
    sort(badEntitiesFound.begin(), badEntitiesFound.end());
    
    
    // find bad interactions
    set<InteractionTuple> badInteractionsSet;
    
    for (mapIter = badEntitiesMap.begin(); mapIter != badEntitiesMap.end(); mapIter++)
    {
        string badEntity = (*mapIter).first;
        
        DiskMultiMap::Iterator iter = goForward.search(badEntity);
        
        cout << "third while loop" << endl;
        
        while (iter.isValid())
        {
            MultiMapTuple mmt = (*iter);
            InteractionTuple IT;
            IT.context = mmt.context;
            IT.from = mmt.key;
            IT.to = mmt.value;
            
            badInteractionsSet.insert(IT);
        }
        
        iter = goReverse.search(badEntity);
        
        while (iter.isValid())
        {
            MultiMapTuple mmt = (*iter);
            InteractionTuple IT;
            IT.context = mmt.context;
            IT.from = mmt.value;
            IT.to = mmt.key;
            
            badInteractionsSet.insert(IT);
        }
    }
    
    set<InteractionTuple>::iterator setIter;
    for (setIter = badInteractionsSet.begin(); setIter != badInteractionsSet.end(); setIter++)
        interactions.push_back((*setIter));
    
    return badEntitiesFound.size();
    
    /*
     for (unordered_map<string, int>::iterator p = computer.begin(); p != computer.end();p++)
     {
     /////////////////////////////
     //associations forward
     DiskMultiMap::Iterator it1 = goForward.search(p->first);
     ////////////////////////////
     //associations backwards
     DiskMultiMap::Iterator it2 = goReverse.search(p->first);
     do {
     MultiMapTuple m1 = *it1;
     MultiMapTuple m2 = *it2;
     for (int i = 0; i < indicators.size(); i++)
     {
     if (m1.value == indicators.at(i))
     {
     //malicious entity found
     //check presendence, if is under minprevelancetobegood then add
     //if not then continue;
     badEntitiesFound.push_back(m1.context);
     }
     if (m2.value == indicators.at(i))
     {//check presendence, if is under minprevelancetobegood then add
     //if not then continue;
     
     badEntitiesFound.push_back(m2.context);
     }
     }
     ++it1;
     ++it2;
     } while (it1.isValid() && it2.isValid());
     }
     
     
     return 0;//this doesnt work
     */
}

bool IntelWeb::purge(const std::string& entity)
{
    // find all InteractionTuples w/ entity as a key by iterating through goForward
    
    // make queue to store Interaction
    // delete the interactiontuple from the goForward using the erase
    // delete the interactiontuple from goReverse using the same tuple but w/ the key as value and viceversa
    
    // do the same thing for goReverse
    
    // return true if you delete at least one thing
    
    
    //queue<string> strQueue;
    
    /*
     DiskMultiMap::Iterator iter = goForward.search(entityToCheck);
     
     while (iter.isValid())
     {
     prevalenceOfEntity++;
     strQueue.push((*iter).value);
     ++iter;
     }
     */
    
    queue<InteractionTuple> interactionsToDelete;
    
    DiskMultiMap::Iterator iter = goForward.search(entity);
    
    bool atLeastOneDeleted = false;
    
    while (iter.isValid())
    {
        atLeastOneDeleted = true;
        MultiMapTuple mmt = (*iter);
        InteractionTuple IT;
        IT.context = mmt.context;
        IT.from = mmt.key;
        IT.to = mmt.value;
        
        interactionsToDelete.push(IT);
        ++iter;
        //goForward.erase(IT.from, IT.to, IT.context);
    }
    
    while (!interactionsToDelete.empty())
    {
        InteractionTuple entityToCheck = interactionsToDelete.front();
        interactionsToDelete.pop();
        
        goForward.erase(entityToCheck.from, entityToCheck.to, entityToCheck.context);
        goReverse.erase(entityToCheck.to, entityToCheck.from, entityToCheck.context);
        
        atLeastOneDeleted = true;
    }
    
    iter = goReverse.search(entity);
    
    while (iter.isValid())
    {
        atLeastOneDeleted = true;
        MultiMapTuple mmt = (*iter);
        InteractionTuple IT;
        IT.context = mmt.context;
        IT.from = mmt.value;
        IT.to = mmt.key;
        
        interactionsToDelete.push(IT);
        ++iter;
        //goReverse.erase(IT.from, IT.to, IT.context);
    }
    
    while (!interactionsToDelete.empty())
    {
        InteractionTuple entityToCheck = interactionsToDelete.front();
        interactionsToDelete.pop();
        
        goReverse.erase(entityToCheck.from, entityToCheck.to, entityToCheck.context);
        goForward.erase(entityToCheck.to, entityToCheck.from, entityToCheck.context);
        
        atLeastOneDeleted = true;
    }
    
    if (atLeastOneDeleted == true)
        return true;
    
    return false;
    
    /*while (!strQueue.empty())
     {
     string entityToCheck = strQueue.front();
     strQueue.pop();
     }
     
     goForward.erase(entity);*/
}


