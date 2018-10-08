#include <iostream>
#include <iterator>
#include <string>
#include <cpr/cpr.h>
#include <json.hpp>
#include <regex>
#include <vector>
#include <tuple>

using json = nlohmann::json;

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

std::vector<std::string> getItemLinks(std::string page, std::regex item_regex, std::regex href_regex, std::vector<std::string> item_links, std::string prefix){
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
        std::cout << result << std::endl;
    }
    std::string link = prefix + result;
    return link;
}


json getItemInfo(std::string url, std::vector<itemRegexType> info_page_regex){
    json info;
    std::string page = getPage(url);
    info["url"] = url;
    for (auto i = info_page_regex.begin(); i != info_page_regex.end(); ++i){
        std::string key = std::get<0>(*i);
        std::string value = getValueFromString(page, std::get<2>(*i), std::get<1>(*i));
        info[key] = value;
    }
    return info;
}

std::vector<itemRegexType> getItemInfoRegex(std::string website){
    //get values from file!
    std::regex name_regex("<h1 class=\"product-name\">([^<]+)</h1>");
    // std::regex description_regex("<body onload=\"resizeIframe\((.*)\);\" onresize=\"resizeIframe\((.*)\);\">((.|\n)+(</body>))"); 
    std::regex photo_url_regex("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
    std::regex price_regex("<p class=\"sales-price\">([^<]+)</p>");
    std::regex installments_price_regex("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    std::tuple<std::string,std::string,std::regex>("name", ">", name_regex);
    std::vector<itemRegexType> info_regex = {
        itemRegexType("name",1, name_regex),
        // itemRegexType("description",">",description_regex),
        itemRegexType("photo_url", 2, photo_url_regex),
        itemRegexType("price", 1, price_regex),
        itemRegexType("installments_price", 1, installments_price_regex),
    };
    return info_regex;
}

int main(int argc, char** argv) {
    std::vector<std::string> item_links;
    std::vector<std::string> page_links;
    //get from std::cin
    std::string url = argv[1];
    //get from choosen website regex file
    std::regex item_regex("<a class=\"card-product-url\" href=\"([^\"]+[\"])");
    std::regex next_page_regex("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
    std::regex replace_step_regex("amp;");
    std::regex href_regex("(/produto/.+)(?=\")");
    std::regex website_regex("https://(([^/]+))/");
    std::vector<itemRegexType> info_page_regex = getItemInfoRegex(website);
    std::string website = getValueFromString(url, website_regex);

    website = "https://" + website;
    while(url != website){
        std::string page = getPage(url);
        std::vector<std::string> new_item_links = getItemLinks(page, item_regex, href_regex, item_links, website);
        item_links.insert(item_links.end(), new_item_links.begin(), new_item_links.end());
        url = getNextPageLink(page, next_page_regex, replace_step_regex, website);
        std::cout << url << std::endl;
    }

    json res;
    //this loop will become the tasks for consumer
    for (auto i = item_links.begin(); i != item_links.end(); ++i){
        std::cout << getItemInfo(*i, info_page_regex).dump() << std::endl;
    }
}
