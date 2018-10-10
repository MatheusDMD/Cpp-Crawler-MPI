# Cpp-Crawler

Project for SuperComputing class @ Insper

This project starts from a c++ example from cpr-example[https://github.com/whoshuu/cpr-example]

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