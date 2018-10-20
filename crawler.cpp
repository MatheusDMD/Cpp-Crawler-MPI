#include <iostream>
#include <iterator>
#include <string>
#include <cpr/cpr.h>
#include <json.hpp>
#include <regex>
#include <list>
#include <tuple>
#include <thread>
#include <chrono>
#include "Semaphore.cpp"
using json = nlohmann::json;

#define NUM_THREADS_PRODUCERS 5
#define NUM_THREADS_CONSUMERS 5
#define SUBMARINO  "www.submarino.com.br"
#define AMERICANAS "www.americanas.com.br"
#define SHOPTIME   "www.shoptime.com.br"

// ("<a class=\"a-link-normal s-access-detail-page  s-color-twister-title-link a-text-normal\"");
typedef std::tuple<std::string, int,std::regex> itemRegexType;

bool isURLSupported(std::string website){
    bool isSupported = false;
    if(website == SUBMARINO || website == AMERICANAS || website == SHOPTIME)
        isSupported = true;
    return isSupported;
}

std::string getPage(std::string url){
    auto response = cpr::Get(cpr::Url{url});
    return response.text;
}

std::string getValueFromString(std::string component_str, std::regex href_regex, int index = 1){
    std::smatch base_match;
    if (std::regex_search(component_str, base_match, href_regex)) {
        return base_match[index];
    }else{
        return "";
    }
}

std::list<std::string> getItemLinks(std::string page, std::regex item_regex, std::regex href_regex, std::list<std::string> item_links, std::string prefix){
    auto words_begin = std::sregex_iterator(page.begin(), page.end(), item_regex);
    auto words_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match  = *i;  
        std::string link   = "";     
        std::string result = getValueFromString(match.str(), href_regex);
        link = prefix + result;
        item_links.push_back(link);
    } 
    return item_links;
}

std::string getNextPageLink(std::string page, std::regex next_page_regex, std::regex replace_step_regex, std::string prefix){
    std::string result = getValueFromString(page, next_page_regex);
    if(result == ""){
        std::string result = "";
    }else {
        result = std::regex_replace (result ,replace_step_regex, "");
    }
    std::string link = prefix + result;
    return link;
}

std::list<itemRegexType> getItemInfoRegex(std::string website){
    std::regex name_regex;
    std::regex description_regex;
    std::regex photo_url_regex;
    std::regex price_regex;
    std::regex installments_price_regex;

    if(website == SUBMARINO){
        name_regex               = std::regex("<h1 class=\"product-name\">([^<]+)</h1>");
        description_regex        = std::regex("<div><noframes>((.|\n)+)</noframes><iframe"); 
        photo_url_regex          = std::regex("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
        price_regex              = std::regex("<p class=\"sales-price\">([^<]+)</p>");
        installments_price_regex = std::regex("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    }else if(website == AMERICANAS){
        name_regex               = std::regex("<h1 class=\"product-name\">([^<]+)</h1>");
        description_regex        = std::regex("<div><noframes>((.|\n)+)</noframes><iframe"); 
        photo_url_regex          = std::regex("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
        price_regex              = std::regex("<p class=\"sales-price\">([^<]+)</p>");
        installments_price_regex = std::regex("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    }else if(website == SHOPTIME){
        name_regex               = std::regex("<h1 class=\"product-name\">([^<]+)</h1>");
        description_regex        = std::regex("<div><noframes>((.|\n)+)</noframes><iframe"); 
        photo_url_regex          = std::regex("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
        price_regex              = std::regex("<p class=\"sales-price\">([^<]+)</p>");
        installments_price_regex = std::regex("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    }
    std::list<itemRegexType> info_regex = {
        itemRegexType("name",1, name_regex),
        itemRegexType("description",1,description_regex),
        itemRegexType("photo_url", 2, photo_url_regex),
        itemRegexType("price", 1, price_regex),
        itemRegexType("installments_price", 1, installments_price_regex),
    };
    return info_regex;
}

std::vector<std::regex> getPagesRegex(std::string website){
    std::regex item_regex;
    std::regex next_page_regex;
    std::regex replace_step_regex;
    std::regex href_regex;
    
    if(website == SUBMARINO){
        item_regex         = std::regex("<a class=\"card-product-url\" href=\"([^\"]+[\"])");
        next_page_regex    = std::regex("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
        replace_step_regex = std::regex("amp;");
        href_regex         = std::regex("(/produto/.+)(?=\")");
    }else if(website == AMERICANAS){
        item_regex         = std::regex("<a class=\"card-product-url\" href=\"([^\"]+[\"])");
        next_page_regex    = std::regex("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
        replace_step_regex = std::regex("amp;");
        href_regex         = std::regex("(/produto/.+)(?=\")");
    }else if(website == SHOPTIME){
        item_regex         = std::regex("<a class=\"card-product-url\" href=\"([^\"]+[\"])");
        next_page_regex    = std::regex("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
        replace_step_regex = std::regex("amp;");
        href_regex         = std::regex("(/produto/.+)(?=\")");
    }

    std::vector<std::regex> page_regex = {
        item_regex,
        next_page_regex,
        replace_step_regex,
        href_regex,
    };
    return page_regex;
}

void getItemLinksFromCategory(std::list<std::string>& item_links, bool& isLinkCollectorDone, double& totalDownloadDuration, Semaphore& accessDownloadTime, Semaphore&  accessProcessTime, double& totalProcessDuration, Semaphore& accessItemList, Semaphore& availableItemList, std::string& website, std::string& url, std::regex& item_regex, std::regex& href_regex, std::regex& next_page_regex, std::regex& replace_step_regex,int& totalCount){
    double localDownloadDuration = 0, localProcessDuration = 0;
    while(url != website){
        std::chrono::high_resolution_clock::time_point downloadT1 = std::chrono::high_resolution_clock::now(); 
        std::string page = getPage(url);
        std::chrono::high_resolution_clock::time_point downloadT2 = std::chrono::high_resolution_clock::now();   
        localDownloadDuration += std::chrono::duration_cast<std::chrono::milliseconds>( downloadT2 - downloadT1 ).count();
        std::chrono::high_resolution_clock::time_point processT1 = std::chrono::high_resolution_clock::now(); 
        std::list<std::string> new_item_links = getItemLinks(page, item_regex, href_regex, item_links, website);

        accessItemList.acquire();
        for (auto i = new_item_links.begin(); i != new_item_links.end(); ++i){
            item_links.push_back(*i);
            availableItemList.release();
            totalCount += 1;
        }
        std::cerr << "getItemLinksFromCategory - PUSHED LINKS" << std::endl;
        accessItemList.release();

        url = getNextPageLink(page, next_page_regex, replace_step_regex, website);
        std::chrono::high_resolution_clock::time_point processT2 = std::chrono::high_resolution_clock::now(); 
        localProcessDuration += std::chrono::duration_cast<std::chrono::milliseconds>( processT2 - processT1 ).count();
    }
    accessDownloadTime.acquire();
        totalDownloadDuration += localDownloadDuration;
    accessDownloadTime.release();
    accessProcessTime.acquire();
        totalProcessDuration += localProcessDuration;
    accessProcessTime.release();
    std::cerr << std::endl << "getItemLinksFromCategory - DONE" << std::endl ;
    isLinkCollectorDone = true;
}

void getItemPagesFromLinks(std::list<std::string>& item_links,  int num_producers, bool& isPageCollectorDone, bool& isLinkCollectorDone, double& totalDownloadDuration, Semaphore& accessDownloadTime, std::list<std::string>& item_pages, Semaphore& accessItemList, Semaphore& availableItemList, Semaphore& accessPageList, Semaphore& availablePageList){
    double localDownloadDuration = 0;
    while(true){
        std::string url;
        availableItemList.acquire();
        accessItemList.acquire();
        if(isLinkCollectorDone && item_links.empty()){
            break;
        }else{
            
            url = item_links.front();
            std::cerr << "getItemPagesFromLinks - POPPED-URL" << std::endl;
            item_links.pop_front();
            if(isLinkCollectorDone && item_links.empty()){
                for(auto i = 0; i < num_producers; i++){
                    accessItemList.release();
                    availableItemList.release();
                }
            }
        }
        accessItemList.release();
            std::chrono::high_resolution_clock::time_point downloadT1 = std::chrono::high_resolution_clock::now(); 
            std::string page = getPage(url);
            std::chrono::high_resolution_clock::time_point downloadT2 = std::chrono::high_resolution_clock::now();   
            localDownloadDuration += std::chrono::duration_cast<std::chrono::milliseconds>( downloadT2 - downloadT1 ).count();
        accessPageList.acquire();
            item_pages.push_back(page);
            availablePageList.release();
            std::cerr << "getItemPagesFromLinks - ADDED PAGE" << std::endl;   
        accessPageList.release();
    }
    accessDownloadTime.acquire();
        totalDownloadDuration += localDownloadDuration;
    accessDownloadTime.release();
    std::cerr << std::endl << "getItemPagesFromLinks - DONE" << std::endl;
    isPageCollectorDone = true;
}

void getItemInfoFromItemPage(json& res, Semaphore& accessJSON, int num_consumers, bool& isPageCollectorDone, std::list<std::string>& item_pages, Semaphore& accessPageList, Semaphore& availablePageList, std::list<itemRegexType>& info_page_regex, Semaphore&  accessProcessTime, double& totalProcessDuration){
    double localProcessDuration = 0;
    while(true){
        std::string page;
        availablePageList.acquire();
        accessPageList.acquire();
            if(isPageCollectorDone && item_pages.empty()){
                break;
            }else{
                page = item_pages.front();
                item_pages.pop_front();
                std::cerr << "getItemInfoFromItemPage - PAGE ANALYSIS" << std::endl;
                if(isPageCollectorDone && item_pages.empty()){
                    // std::cerr << std::endl << res.dump() << std::endl;
                    for(auto i = 0; i < num_consumers; i++){
                        availablePageList.release();
                        accessPageList.release();
                    }
                }
            }
        accessPageList.release();

        std::chrono::high_resolution_clock::time_point processT1 = std::chrono::high_resolution_clock::now(); 
        json info;
        for (auto i = info_page_regex.begin(); i != info_page_regex.end(); ++i){
            std::string key = std::get<0>(*i);
            std::string value = getValueFromString(page, std::get<2>(*i), std::get<1>(*i));
            info[key] = value;
        }
        std::chrono::high_resolution_clock::time_point processT2 = std::chrono::high_resolution_clock::now(); 
        localProcessDuration += std::chrono::duration_cast<std::chrono::milliseconds>( processT2 - processT1 ).count();
        std::cerr << "getItemInfoFromItemPage - JSON PUSHED" << std::endl;
        accessJSON.acquire();
            res.push_back(info);
        accessJSON.release();
    }
    accessProcessTime.acquire();
        totalProcessDuration += localProcessDuration;
    accessProcessTime.release();
}

int main(int argc, char** argv) {
    //Semaphores and Thread controls
    Semaphore accessItemList(1);
    Semaphore availableItemList(0);
    Semaphore accessPageList(1);
    Semaphore availablePageList(0);
    Semaphore accessDownloadTime(1);
    Semaphore accessProcessTime(1);
    Semaphore accessTotalTime(1);
    Semaphore accessJSON(1);
    bool isLinkCollectorDone = false;
    bool isPageCollectorDone = false;
    //Thread numbers
    int num_producers, num_consumers;
    num_producers = NUM_THREADS_PRODUCERS;
    num_consumers = NUM_THREADS_CONSUMERS;
    if (argc >= 3){
        if(std::stoi( argv[2]) >= 1){
            num_producers = std::stoi( argv[2]);
        }else{
            std::cerr << std::endl  << argv[2] << " -\033[1;31 number of producer threads is less than 1 \033[0m" << std::endl  << std::endl;
        }
        if(argc > 3){
            if(std::stoi( argv[3]) >= 1){
                num_consumers = std::stoi( argv[3]);
            }else{
                std::cerr << std::endl  << argv[3] << " -\033[1;31m number of consumer threads  is less than 1 \033[0m" << std::endl  << std::endl;
            }
        }
    }else{
        num_producers = NUM_THREADS_PRODUCERS;
        num_consumers = NUM_THREADS_CONSUMERS;
    }
    //Time measures
    double totalDownloadDuration = 0;
    double totalProcessDuration = 0;
    double totalDuration = 0;
    //Count
    int totalCount = 0;
    //Shared Lists
    std::list<std::string> item_links;
    std::list<std::string> item_pages;
    //Final JSON
    json res;
    //input URL
    std::string url = argv[1];
    std::regex website_regex("https://(([^/]+))/");
    std::string website = getValueFromString(url, website_regex);

    if(isURLSupported(website)){
        //Program Time Measure
        std::chrono::high_resolution_clock::time_point programT1 = std::chrono::high_resolution_clock::now(); 

        std::cerr <<  std::endl << website << " -\033[1;32m URL supported\033[0m" <<  std::endl << std::endl;

        std::vector<std::regex> page_regex       = getPagesRegex(website);
        std::regex item_regex                    = page_regex[0];
        std::regex next_page_regex               = page_regex[1];
        std::regex replace_step_regex            = page_regex[2];
        std::regex href_regex                    = page_regex[3];
        std::list<itemRegexType> info_page_regex = getItemInfoRegex(website);
        website = "https://" + website;

        if(std::getenv("NUM_THREADS_PRODUCERS")) num_producers = atoi(std::getenv("NUM_THREADS_PRODUCERS"));
        if(std::getenv("NUM_THREADS_CONSUMERS")) num_consumers = atoi(std::getenv("NUM_THREADS_CONSUMERS"));

        std::thread producer_threads[num_producers];
        std::thread consumer_threads[num_consumers];

        //CREATE THREADS
        std::thread itemLinkCollector(getItemLinksFromCategory, std::ref(item_links), std::ref(isLinkCollectorDone), std::ref(totalDownloadDuration), std::ref(accessDownloadTime), std::ref(accessProcessTime), std::ref(totalProcessDuration), std::ref(accessItemList), std::ref(availableItemList), std::ref(website), std::ref(url), std::ref(item_regex), std::ref(href_regex), std::ref(next_page_regex), std::ref(replace_step_regex), std::ref(totalCount));
        for(int i = 0; i < num_producers; i++)
            producer_threads[i] = std::thread(getItemPagesFromLinks, std::ref(item_links), std::ref(num_producers), std::ref(isPageCollectorDone), std::ref(isLinkCollectorDone), std::ref(totalDownloadDuration), std::ref(accessDownloadTime), std::ref(item_pages),  std::ref(accessItemList), std::ref(availableItemList),  std::ref(accessPageList), std::ref(availablePageList));
        for(int i = 0; i < num_consumers; i++)
            consumer_threads[i] = std::thread(getItemInfoFromItemPage, std::ref(res), std::ref(accessJSON), std::ref(num_consumers), std::ref(isPageCollectorDone), std::ref(item_pages), std::ref(accessPageList), std::ref(availablePageList), std::ref(info_page_regex), std::ref(accessProcessTime),std::ref(totalProcessDuration));

        //JOIN THREADS
        itemLinkCollector.join();
        for (int i = 0; i < num_producers; ++i) 
            producer_threads[i].join();
        for (int i = 0; i < num_consumers; ++i)
            consumer_threads[i].join();
        
        //output JSON
        std::cout << res << std::endl;
        std::cout << "JSON" << std::endl;

        //Program Time Measure
        std::chrono::high_resolution_clock::time_point programT2 = std::chrono::high_resolution_clock::now(); 
        totalDuration += std::chrono::duration_cast<std::chrono::milliseconds>( programT2 - programT1 ).count();

        std::cerr << totalDownloadDuration << std::endl;
        std::cerr << totalProcessDuration << std::endl;
        std::cerr << totalDuration << std::endl;
        std::cerr << totalCount << std::endl;
    }else{
        std::cerr << std::endl  << website << " -\033[1;31m URL not supported\033[0m" << std::endl  << std::endl;
        std::cerr << "Please use one of the supported URLS:" << std::endl;
        std::cerr << AMERICANAS << std::endl;
        std::cerr << SUBMARINO  << std::endl;
        std::cerr << SHOPTIME   << std::endl;
        std::cerr << std::endl;
        return -1;
    }
    

    return 0;
}