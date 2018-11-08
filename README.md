# C++ Crawler
Project for SuperComputing class @ Insper
## Project
This project implements a C++ Crawler for most of B2W websites (One of the biggest Brazilian Online Marketplace).

The concept is that, given a *category url* from one of the websites, the program returns a *JSON* with the content from the products contained in that category.

## Parameters
Product Name
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

## Running the crawler on the cluster
Run the command:
```
mpiexec -n <N> -hostfile <hosts> ./crawler <website_url> 
```

- *N* - Number of processes running in all machines. For this program the number must be less than or equal to the number of products + 1 (because of master node)
- *hosts* - The file that contains the ip values for the machines in the cluster

- *website_url* - Is the URL to the Category page as describe in the section: "Valid URLs".

## Next-Steps

- Implement a better model for accepting new websites;
- Restructure code;

This project starts from a c++ example from cpr-example[https://github.com/whoshuu/cpr-example]
