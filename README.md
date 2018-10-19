# C++ Crawler
Project for SuperComputing class @ Insper
## Project
This project implements a C++ Crawler for most of B2W websites (One of the biggest Brazilian Online Marketplace).

The concept is that, given a *category url* from one of the websites, the program returns a *JSON* with the content from the products contained in that category.

## Parameters
Product Name
Product Description
Product Price
Product Price Installments
Product Image URL
Product Category

### Valid URLs:
The accepted URLs are Category only URLs. From these websites:

- https://www.submarino.com.br[https://www.submarino.com.br]
- https://www.americanas.com.br/[https://www.americanas.com.br/]
- https://www.shoptime.com.br/[https://www.shoptime.com.br]

Should Specify in this model
```
https://www.<website>.com.br/categoria/<specific_category>
```
#### examples
```
https://www.submarino.com.br/categoria/brinquedos/hoverboard/f/preco-2579.0:3224.0?ordenacao=topSelling&origem=omega

https://www.americanas.com.br/categoria/brinquedos/bonecos/f/marca-bandai/genero-n%C3%A3o%20se%20aplica/genero-unisex?ordenacao=topSelling&origem=omega
```

## Building

This project and C++ Requests both use CMake. The first step is to make sure all of the submodules are initialized:
```
git submodule update --init --recursive
```
Then make a build directory and do a typical CMake build from there:
```
mkdir build
cd build
cmake ..
make
```

## Running the crawler
Inside the build folder execute either the parallel version by running following commands:
```
./crawler <website_url> <num_producer_threads> <num_consumer_threads>
```
or the sequential version by running:
```
./crawler_sequential <website_url>
```
*website_url* - Is the URL to the Category page as describe in the section: "Valid URLs".
*num_producer_threads* - Is an integer describing the number of threads dedicated to the Producers Threads
*num_consumer_threads* - Is an integer describing the number of threads dedicated to the Consumer Threads

# Program Analysis
While this project has the purpose of collecting data from an online marketplace. The goal is to compare the performance and memory consumption from the *Sequential* and the *Parallel* implementations.

## Performance Analysis
To do so, run the python3 code:
```
python3 analysis.py
```
This program will run the crawler for a small number of pages, comparing the time consumption from the two models and the increment of threads on the parallel model.

## Memory Consumption
To analyze memory consumption, I decided to use python's memory-profiler:
To install the module run:
```
pip3 install memory_profiler
```

This will test and plot the results for the parallel model:
```
mprof run ./crawler https://www.submarino.com.br/categoria/brinquedos/hoverboard/f/preco-2579.0:3224.0?ordenacao=topSelling&origem=omega
mprof plot
mprof clean
```

This will test and plot the results for the sequential model:
```
mprof run ./crawler_sequential https://www.submarino.com.br/categoria/brinquedos/hoverboard/f/preco-2579.0:3224.0?ordenacao=topSelling&origem=omega
mprof plot
mprof clean
```

## Next-Steps

- Implement a better model for accepting new websites;
- Restructure code;

This project starts from a c++ example from cpr-example[https://github.com/whoshuu/cpr-example]
