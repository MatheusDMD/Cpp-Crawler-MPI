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
#define NUM_THREADS_CONSUMERS 1

// ("<a class=\"a-link-normal s-access-detail-page  s-color-twister-title-link a-text-normal\"");
typedef std::tuple<std::string, int,std::regex> itemRegexType;

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
        std::smatch match = *i;  
        std::string link = "";     
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
    //get values from file!
    std::regex name_regex("<h1 class=\"product-name\">([^<]+)</h1>");
    // std::regex description_regex("<body onload=\"resizeIframe\((.*)\);\" onresize=\"resizeIframe\((.*)\);\">((.|\n)+(</body>))"); 
    std::regex photo_url_regex("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
    std::regex price_regex("<p class=\"sales-price\">([^<]+)</p>");
    std::regex installments_price_regex("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    std::tuple<std::string,std::string,std::regex>("name", ">", name_regex);
    std::list<itemRegexType> info_regex = {
        itemRegexType("name",1, name_regex),
        // itemRegexType("description",">",description_regex),
        itemRegexType("photo_url", 2, photo_url_regex),
        itemRegexType("price", 1, price_regex),
        itemRegexType("installments_price", 1, installments_price_regex),
    };
    return info_regex;
}

void getItemLinksFromCategory(std::list<std::string>& item_links, bool& isLinkCollectorDone, Semaphore& accessItemList, Semaphore& availableItemList, std::string& website, std::string& url, std::regex& item_regex, std::regex& href_regex, std::regex& next_page_regex, std::regex& replace_step_regex){
    while(url != website){
        std::string page = getPage(url);
        std::list<std::string> new_item_links = getItemLinks(page, item_regex, href_regex, item_links, website);

        accessItemList.acquire();
        for (auto i = new_item_links.begin(); i != new_item_links.end(); ++i){
            item_links.push_back(*i);
            availableItemList.release();
            // std::cout << *i << std::endl;
        }
        std::cout << "getItemLinksFromCategory - PUSHED LINKS" << std::endl;
        accessItemList.release();

        // item_links.insert(item_links.end(), new_item_links.begin(), new_item_links.end());
        url = getNextPageLink(page, next_page_regex, replace_step_regex, website);
    }
    std::cout << std::endl << "getItemLinksFromCategory - DONE" << std::endl ;
    isLinkCollectorDone = true;
}

void getItemPagesFromLinks(std::list<std::string>& item_links,  int num_producers, bool& isPageCollectorDone, bool& isLinkCollectorDone, std::list<std::string>& item_pages, Semaphore& accessItemList, Semaphore& availableItemList, Semaphore& accessPageList, Semaphore& availablePageList){
    while(true){
        std::string url;
        availableItemList.acquire();
        accessItemList.acquire();
        if(isLinkCollectorDone && item_links.empty()){
            break;
        }else{
            
            url = item_links.front();
            std::cout << "getItemPagesFromLinks - POPPED-URL" << std::endl;
            item_links.pop_front();
            if(isLinkCollectorDone && item_links.empty()){
                for(auto i = 0; i < num_producers; i++){
                    accessItemList.release();
                    availableItemList.release();
                }
            }
        }
        accessItemList.release();
        
        std::string page = getPage(url);

        accessPageList.acquire();
            item_pages.push_back(page);
            availablePageList.release();
            std::cout << "getItemPagesFromLinks - ADDED PAGE" << std::endl;   
        accessPageList.release();
    }
    std::cout << std::endl << "getItemPagesFromLinks - DONE" << std::endl;
    isPageCollectorDone = true;
}

void getItemInfoFromItemPage(json& res, int num_consumers, bool& isPageCollectorDone, std::list<std::string>& item_pages, Semaphore& accessPageList, Semaphore& availablePageList, std::list<itemRegexType>& info_page_regex){
    while(true){
        std::string page;
        availablePageList.acquire();
        accessPageList.acquire();
            if(isPageCollectorDone && item_pages.empty()){
                break;
            }else{
                page = item_pages.front();
                item_pages.pop_front();
                std::cout << "getItemInfoFromItemPage - PAGE ANALYSIS" << std::endl;
                if(isPageCollectorDone && item_pages.empty()){
                    for(auto i = 0; i < num_consumers; i++){
                        availablePageList.release();
                        accessPageList.release();
                    }
                }
            }
        accessPageList.release();

        json info;
        // info["url"] = url;
        for (auto i = info_page_regex.begin(); i != info_page_regex.end(); ++i){
            std::string key = std::get<0>(*i);
            std::string value = getValueFromString(page, std::get<2>(*i), std::get<1>(*i));
            info[key] = value;
        }
        std::cout << "getItemInfoFromItemPage - JSON PUSHED" << std::endl;
        res.push_back(info);
    }
    std::cout << std::endl << res.dump() << std::endl;
}

int main(int argc, char** argv) {
    Semaphore accessItemList(1);
    Semaphore availableItemList(0);
    Semaphore accessPageList(1);
    Semaphore availablePageList(0);
    bool isLinkCollectorDone = false;
    bool isPageCollectorDone = false;
    int num_producers = NUM_THREADS_PRODUCERS;
    int num_consumers = NUM_THREADS_CONSUMERS;
    
    std::list<std::string> item_links;
    std::list<std::string> item_pages;
    //get from std::cin
    std::string url = argv[1];
    json res;
    //get from choosen website regex file
    std::regex item_regex("<a class=\"card-product-url\" href=\"([^\"]+[\"])");
    std::regex next_page_regex("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
    std::regex replace_step_regex("amp;");
    std::regex href_regex("(/produto/.+)(?=\")");
    std::regex website_regex("https://(([^/]+))/");

    std::string website = getValueFromString(url, website_regex);
    std::list<itemRegexType> info_page_regex = getItemInfoRegex(website);
    website = "https://" + website;

    if(std::getenv("NUM_THREADS_PRODUCERS")) num_producers = atoi(std::getenv("NUM_THREADS_PRODUCERS"));
    if(std::getenv("NUM_THREADS_CONSUMERS")) num_consumers = atoi(std::getenv("NUM_THREADS_CONSUMERS"));

    std::thread producer_threads[num_producers];
    std::thread consumer_threads[num_consumers];

    //CREATE THREADS
    std::thread itemLinkCollector(getItemLinksFromCategory, std::ref(item_links), std::ref(isLinkCollectorDone), std::ref(accessItemList), std::ref(availableItemList), std::ref(website), std::ref(url), std::ref(item_regex), std::ref(href_regex), std::ref(next_page_regex), std::ref(replace_step_regex));

    for(int i = 0; i < num_producers; i++){
        producer_threads[i] = std::thread(getItemPagesFromLinks, std::ref(item_links), std::ref(num_producers), std::ref(isPageCollectorDone), std::ref(isLinkCollectorDone), std::ref(item_pages),  std::ref(accessItemList), std::ref(availableItemList),  std::ref(accessPageList), std::ref(availablePageList));
    }
    for(int i = 0; i < num_consumers; i++){
        consumer_threads[i] = std::thread(getItemInfoFromItemPage, std::ref(res), std::ref(num_consumers), std::ref(isPageCollectorDone), std::ref(item_pages), std::ref(accessPageList), std::ref(availablePageList), std::ref(info_page_regex));
    }

    //JOIN THREADS
    itemLinkCollector.join();

    for (int i = 0; i < num_producers; ++i) {
        producer_threads[i].join();
    }      
    for (int i = 0; i < num_consumers; ++i) {
        consumer_threads[i].join();
    }        

    return 0;
}