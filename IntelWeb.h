// ELYSE'S FILE


#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
class IntelWeb
{
public:
    IntelWeb();
    ~IntelWeb();
    bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
    bool openExisting(const std::string& filePrefix);
    void close();
    bool ingest(const std::string& telemetryFile);
    unsigned int crawl(const std::vector<std::string>& indicators,
                       unsigned int minPrevalenceToBeGood,
                       std::vector<std::string>& badEntitiesFound,
                       std::vector<InteractionTuple>& interactions
                       );
    bool purge(const std::string& entity);
    
private:
    // Your private member declarations will go here
    //unordered_map<string, int> ;
    /*
     purge function : find in forward, then remember it and erase in reverse
     then find in reverse, then remember it and erase in forward
     */
    
    
    DiskMultiMap goForward;
    DiskMultiMap goReverse;
};

inline
bool operator<(const InteractionTuple& a, const InteractionTuple& b)
{
    if (a.from < b.from)
        return true;
    else
        return false;
    
    if (a.to < b.to)
        return true;
    else
        return false;
    
    if (a.context < b.context)
        return true;
    else
        return false;
    
    return false;
}

#endif // INTELWEB_H_
