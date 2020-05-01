/***************************************************************************//**
**  \mainpage Monte Carlo eXtreme - GPU accelerated Monte Carlo Photon Migration
**
**  \author Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2009-2018
**
**  \section sref Reference:
**  \li \c (\b Fang2009) Qianqian Fang and David A. Boas, 
**          <a href="http://www.opticsinfobase.org/abstract.cfm?uri=oe-17-22-20178">
**          "Monte Carlo Simulation of Photon Migration in 3D Turbid Media Accelerated 
**          by Graphics Processing Units,"</a> Optics Express, 17(22) 20178-20190 (2009).
**  \li \c (\b Yu2018) Leiming Yu, Fanny Nina-Paravecino, David Kaeli, and Qianqian Fang,
**          "Scalable and massively parallel Monte Carlo photon transport
**           simulations for heterogeneous computing platforms," J. Biomed. Optics, 23(1), 010504, 2018.
**
**  \section slicense License
**          GPL v3, see LICENSE.txt for details
*******************************************************************************/

/***************************************************************************//**
\file    mcextreme.c

@brief   << MCX main program >>
*******************************************************************************/

#include <stdio.h>
#include "tictoc.h"
#include "mcx_utils.h"
#include "mcx_core.h"
#ifdef _OPENMP
  #include <omp.h>
#endif
#define CUDA_ASSERT(a)      mcx_cu_assess((a),__FILE__,__LINE__) 
#define LEN 512
#include <stdio.h>
#include <stdlib.h>
__constant__ int value[LEN];
__global__ void get(int* Ad)
{
        int tid = blockIdx.x*blockDim.x+threadIdx.x;
        Ad[tid] = value[tid];
}


void main_test()
{
        int *A, *B, *Ad;
        A = (int*)malloc(LEN*sizeof(int));
        B = (int*)malloc(LEN*sizeof(int));
        for(int i=0; i<LEN; i++){
                A[i]=-1*i;
                B[i]=0;
        }
        hipMalloc((void**)&Ad,LEN*sizeof(int));
        hipMemcpyToSymbol(HIP_SYMBOL(value),A,LEN*sizeof(int),0,hipMemcpyHostToDevice);
        hipLaunchKernelGGL(get, dim3(1), dim3(LEN), 0, 0, Ad);
        hipMemcpy(B,Ad,LEN*sizeof(int),hipMemcpyDeviceToHost);

        int error=0;
        for(int i=0; i<LEN; i++){
                if(A[i] != B[i]){
                        error = 1;
                        printf("Error starts element %d, %d != %d\n",i,A[i],B[i]);
                }
                if(error)
                        break;
        }
        if(error == 0){
                printf("No errors\n");
        }
        free(A);
        free(B);
        hipFree(Ad);
}
//_constant__ float4 gprop1[4000];
__constant__ int gprop2[100];
int main (int argc, char *argv[]) {
     main_test();
     /*! structure to store all simulation parameters 
      */
     int prop[100];
     for(int i=0;i<100;i++){
          prop[i] = 1;
     }
     CUDA_ASSERT(hipMemcpyToSymbol(HIP_SYMBOL(gprop2), prop,  100*sizeof(int), 0, hipMemcpyHostToDevice));
     Config  mcxconfig;            /** mcxconfig: structure to store all simulation parameters */
     GPUInfo *gpuinfo=NULL;        /** gpuinfo: structure to store GPU information */
     unsigned int activedev=0;     /** activedev: count of total active GPUs to be used */

     /** 
        To start an MCX simulation, we first create a simulation configuration and
	set all elements to its default settings.
      */
     mcx_initcfg(&mcxconfig);

     /** 
        Then, we parse the full command line parameters and set user specified settings
      */
     mcx_parsecmd(argc,argv,&mcxconfig);
     
     Config* cfg = &mcxconfig;
     CUDA_ASSERT(hipMemcpyToSymbol(HIP_SYMBOL(gprop2), prop,  100*sizeof(int), 0, hipMemcpyHostToDevice));
     CUDA_ASSERT(hipMemcpyToSymbol(HIP_SYMBOL(gprop1), cfg->prop,  cfg->medianum*sizeof(Medium), 0, hipMemcpyHostToDevice));
    //printf("rv is %d\n",rv);
     CUDA_ASSERT(hipMemcpyToSymbol(HIP_SYMBOL(gprop1), cfg->detpos,  cfg->detnum*sizeof(float4), cfg->medianum*sizeof(Medium), hipMemcpyHostToDevice));
     /** The next step, we identify gpu number and query all GPU info */
     if(!(activedev=mcx_list_gpu(&mcxconfig,&gpuinfo))){
         mcx_error(-1,"No GPU device found\n",__FILE__,__LINE__);
     }
     
#ifdef _OPENMP
     /** 
        Now we are ready to launch one thread for each involked GPU to run the simulation 
      */
     omp_set_num_threads(activedev);
     #pragma omp parallel
     {
#endif

     /** 
        This line runs the main MCX simulation for each GPU inside each thread 
      */
     mcx_run_simulation(&mcxconfig,gpuinfo); 

#ifdef _OPENMP
     }
#endif

     /** 
        Once simulation is complete, we clean up the allocated memory in config and gpuinfo, and exit 
      */
     mcx_cleargpuinfo(&gpuinfo);
     mcx_clearcfg(&mcxconfig);
     return 0;
}
