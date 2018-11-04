#include <iostream>
#include <iterator>
#include <string>
#include <cpr/cpr.h>
#include <json.hpp>
#include <regex>
#include <vector>
#include <tuple>
#include <chrono>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>

#define JSON_RESULT 0
#define TIME_DOWNLOAD 1
#define TIME_PROCESS 2
#define LINK_VECTOR 3
#define END_PROCESS 4

namespace mpi = boost::mpi;
using json = nlohmann::json;

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
        // std::cout << result << std::endl;
    }
    std::string link = prefix + result;
    return link;
}


json getItemInfo(std::string page, std::string url, std::vector<itemRegexType> info_page_regex){
    json info;
    info["url"] = url;
    for (auto i = info_page_regex.begin(); i != info_page_regex.end(); ++i){
        std::string key = std::get<0>(*i);
        std::string value = getValueFromString(page, std::get<2>(*i), std::get<1>(*i));
        info[key] = value;
    }
    return info;
}

std::vector<itemRegexType> getItemInfoRegex(){
    //get values from file!
    std::regex name_regex("<h1 class=\"product-name\">([^<]+)</h1>");
    std::regex description_regex("<div><noframes>((.|\n)+)</noframes><iframe"); ; 
    std::regex photo_url_regex("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
    std::regex price_regex("<p class=\"sales-price\">([^<]+)</p>");
    std::regex installments_price_regex("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    std::regex url_regex("<link rel=\"alternate\" href=\"([^\"]+)\" hreflang=\"pt-br\"");
    std::regex categoty_regex("<h1 class=\"category-title\">([^<]+)</h1>");
    std::tuple<std::string,std::string,std::regex>("name", ">", name_regex);
    std::vector<itemRegexType> info_regex = {
        itemRegexType("nome",1, name_regex),
        itemRegexType("descricao",1,description_regex),
        itemRegexType("foto", 2, photo_url_regex),
        itemRegexType("preco", 1, price_regex),
        itemRegexType("preco_num_parecelas", 1, installments_price_regex),
        itemRegexType("url", 1, url_regex),
        itemRegexType("categoria", 1, categoty_regex),
    };
    return info_regex;
}

int main(int argc, char** argv) {
    //MPI variables
    mpi::environment env(argc, argv);
    mpi::communicator world;

    if (world.rank() == 0) {
        std::chrono::high_resolution_clock::time_point totalT1 = std::chrono::high_resolution_clock::now();

        std::vector<std::string> item_links;
        std::vector<std::string> page_links;
        //Time measures
        double totalDownloadDuration = 0;
        double totalProcessDuration = 0;
        double totalDuration = 0;
        //Count
        int totalCount = 0;
        //get from std::cin
        std::string url = argv[1];
        //get from choosen website regex file
        std::regex item_regex("<a class=\"card-product-url\" href=\"([^\"]+[\"])");
        std::regex next_page_regex("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
        std::regex replace_step_regex("amp;");
        std::regex href_regex("(/produto/.+)(?=\")");
        std::regex website_regex("https://(([^/]+))/");
        std::string website = getValueFromString(url, website_regex);
        website = "https://" + website;

        while(url != website){
            std::cerr << url << std::endl;
            std::cerr << website << std::endl;
            std::chrono::high_resolution_clock::time_point downloadT1 = std::chrono::high_resolution_clock::now(); 
                std::string page = getPage(url);
                std::cerr << "after" << std::endl;
            std::chrono::high_resolution_clock::time_point downloadT2 = std::chrono::high_resolution_clock::now();
            totalDownloadDuration += std::chrono::duration_cast<std::chrono::milliseconds>( downloadT2 - downloadT1 ).count();

            std::chrono::high_resolution_clock::time_point processT1 = std::chrono::high_resolution_clock::now(); 
                item_links = getItemLinks(page, item_regex, href_regex, item_links, website);
                std::cerr << "after links" << std::endl;
                int size = item_links.size()/world.size();
                int uneven_division = 0;
                if((item_links.size() % 2 == 0 && world.size() % 2 != 0) || (item_links.size() % 2 != 0 && world.size() % 2 == 0)){
                    uneven_division = 1;
                    std::cerr << "uneven_division" << std::endl;
                }
                for(int i = 0, j = 1; i < item_links.size() - size - uneven_division; i = i + size, j++){
                    std::vector<std::string> process_item_links;
                    if(i >= item_links.size() - size - uneven_division){
                        std::cerr << "last" << std::endl;
                        process_item_links = std::vector<std::string>(item_links.begin()+ i, item_links.begin() + i + size + 1);
                    }else{
                        process_item_links = std::vector<std::string>(item_links.begin()+ i, item_links.begin() + i + size);
                    }
                    std::cerr << j << std::endl;
                    totalCount += 1;
                    world.send(j, LINK_VECTOR, process_item_links);
                }
                url = getNextPageLink(page, next_page_regex, replace_step_regex, website);
            std::chrono::high_resolution_clock::time_point processT2 = std::chrono::high_resolution_clock::now(); 
            totalProcessDuration += std::chrono::duration_cast<std::chrono::milliseconds>( processT2 - processT1 ).count();
            std::cerr << "Page downloaded - RANK:" << world.rank() << std::endl;
        }
         for(int process_n = 1; process_n < world.size(); process_n++){
            std::vector<std::string> empty_vector;
            world.send(process_n, LINK_VECTOR, empty_vector);
        }

        std::string final_json;
        for(int process_n = 1; process_n < world.size(); process_n++){
            std::string process_json;
            double localDownloadDuration = 0;
            double localProcessDuration = 0;

            world.recv(process_n,JSON_RESULT,   process_json);
            world.recv(process_n,TIME_DOWNLOAD, localDownloadDuration);
            world.recv(process_n,TIME_PROCESS,  localProcessDuration);

            totalDownloadDuration += localDownloadDuration;
            totalProcessDuration += localProcessDuration;
            final_json += process_json;
            std::cerr << "JSON RECEBIDO - " << process_n << std::endl;
        }
        std::chrono::high_resolution_clock::time_point totalT2 = std::chrono::high_resolution_clock::now(); 
        totalDuration += std::chrono::duration_cast<std::chrono::milliseconds>( totalT2 - totalT1 ).count();
       
        std::cout << final_json << std::endl;
        std::cerr << totalDuration << std::endl;
        std::cerr << totalDownloadDuration << std::endl;
        std::cerr << totalProcessDuration << std::endl;
        std::cerr << totalDuration/totalCount << std::endl;

    } else {
        std::vector<itemRegexType> info_page_regex = getItemInfoRegex();
        double localDownloadDuration = 0;
        double localProcessDuration = 0;
        json res;
        while(true){            
            std::vector<std::string> item_links;
            world.recv(0,LINK_VECTOR,item_links);
            if(item_links.empty()){
                std::cerr << "END - " << world.rank() << std::endl;
                break;
            }else{
                for (auto i = item_links.begin(); i != item_links.end(); ++i){
                    json info;
                    std::chrono::high_resolution_clock::time_point download2T1 = std::chrono::high_resolution_clock::now(); 
                        std::string page = getPage(*i);
                    std::chrono::high_resolution_clock::time_point download2T2 = std::chrono::high_resolution_clock::now(); 
                    localDownloadDuration += std::chrono::duration_cast<std::chrono::milliseconds>( download2T2 - download2T1 ).count();

                    std::chrono::high_resolution_clock::time_point process2T1 = std::chrono::high_resolution_clock::now(); 
                        res.push_back(getItemInfo(page, *i, info_page_regex));
                    std::chrono::high_resolution_clock::time_point process2T2 = std::chrono::high_resolution_clock::now(); 
                    localProcessDuration += std::chrono::duration_cast<std::chrono::milliseconds>( process2T2 - process2T1 ).count();
                    std::cerr << "Page processed - RANK:" << world.rank() << std::endl;
                }
            }
        }
        world.send(0,JSON_RESULT,res.dump());
        std::cerr << "DONE Page process - RANK:" << world.rank() << std::endl;
        world.send(0,TIME_DOWNLOAD,localDownloadDuration);
        world.send(0,TIME_PROCESS,localProcessDuration);
    }
    
    return 0;

    // std::cout << res.dump() << std::endl;
    // std::cerr << totalDownloadDuration << std::endl;
    // std::cerr << totalProcessDuration << std::endl;
    // std::cerr << totalDuration << std::endl;
    // std::cerr << totalCount << std::endl;
}