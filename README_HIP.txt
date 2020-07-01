After the clone of the repository, git checkout hipify1
To build and run, cd mcx/src and make
the binary is located at mcx/bin
To run examples cd mcx/example/benchmark
for example, runing benchmark1 ./benchmark1.sh

The first issue is related to shared memory declaration. To reproduce the problem,
git checkout 350f2b3fd3150878807948e4f571226bc3062937
Then compile and run the program
The program compiles sucessfully but at runtime a page fault occurs in the 
kernel. After debuging, I found the error happens when it first access the
shared memory and moving the shared memory declaration inside the kernel function
solves this problem

The second issue happens when a function get called. The line of code is
https://github.com/yuhuibao/mcx/blob/master/src/mcx_core.cu#L1493. To reproduce
the problem, git checkout 00641e06de7f406c95f0acf43a7ef87c13493864
At runtime a page fault occurs in the kernel. The function prototype is
__device__ inline void transmit(MCXdir *v, float n1, float n2,int flipdir)
writing to variable v causes the page fault. After replacing the type MCXdir
with float4, the problem is gone. Type MCXdir is a user_defined type
https://github.com/yuhuibao/mcx/blob/hipify1/src/mcx_core.h#L51:46

The third issue occurs at host code and happens when testing with two different
AMD GPUs. To reproduce the problem, git checkout 0bd5f34e58fc2fc49ed00a5bbfb4190cc5ad5f78
At runtime, the program throws a user_defined error which indicates no GPU divices
are found but after further checks the program can print out device information.
If you run mcx/bin/mcx -L, the GPU info is print out.
The problem arises when the hipGetDeviceProperties gets called in a loop for the
second time, it changes the value of another user_defined variable in the loop.

The 1 and 2 issues are found when working with ROCm3.0 on a server with two identical AMD gpus.
The 3 issue are found when working with ROCm3.3 on a server with two different AMD gpus.

