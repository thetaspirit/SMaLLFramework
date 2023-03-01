
# SMaLL Framework

A portable high performance machine learning library framework.

## Build Instructions

### Specify µArch

To architecture specific features such as SIMD and NEON, please choose one of the available architectures specified in  `./config.h`

```c
#define REF 0
#define ZEN2 1
#define ARM 2

```
The µarch is specified when building with CMake using the `CMAKE_UARCH ` parameter.

The reference build (REF) is written in purely in C and should work on any architecture with a C compiler.

### Modify Compiler Flags

Depending on the architecture and the compiler you are using, the compiler flags in `./CMakeLists.txt` might need to be modified

```c
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(ContextDependentFusion)
include(CTest)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_CXX_COMPILER g++) #<--- Specify C++ compiler
set(CMAKE_C_COMPILER gcc) #<--- Specify C compiler

#Compiler Flags 
SET(GCC_AVX_COMPILE_FLAGS "-fopenmp -mavx2 -mfma -O3 -fpermissive -march=native")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_AVX_COMPILE_FLAGS}")
```

### Build using CMake

Create a build folder from the Top Level directory and run CMake.  Currently µarch can be REF (the default), ZEN2, or ARM.

```bash
$ mkdir build
$ cd build
$ cmake ../ -DCMAKE_UARCH=<µarch>
$ make
```

This *should* generate 3 executables

- `test_interface_relu.x`
- `test_interface_pool.x`
- `test_interface_conv.x`

Each executable accepts an input size (C,H,W) and a  filter size where applicable and outputs something like the following

```bash
$ ./test_interface_conv.x 256  30 30  3 1 v 192
layer 0 
µArch: zen2 
W_ob =  6 
C_ob = 16 
SIMD = 8 
3463381.00      2140101.00 (time in nanoseconds)
```

### Test your build

The Experiments folder has some scripts to run a variety of sizes with the framework

- `Experiments/script_VGG_almost.sh` Runs layer sizes similar to VGGnet.
- `Experiments/test_all.sh` Runs the 3 executables generated above through the VGGnet test. 
- `Experiments/sweep_sizes.sh` Runs different problem sizes to test the loops



## Supported Features

3 DNN Layers

- Convolution Layer
- MaxPooling Layer
- ReLU Activation
