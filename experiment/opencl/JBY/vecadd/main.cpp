/******************************************************************************
 * Copyright (c) 2013-2014, Texas Instruments Incorporated - http://www.ti.com/
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *       * Neither the name of Texas Instruments Incorporated nor the
 *         names of its contributors may be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include "ocl_util.h"

#ifdef _TI_RTOS
#include "kernel.dsp_h"
#include "../rtos_main.c"
#endif

using namespace cl;
using namespace std;

// Source can be embedded here, or read in from external kernel.cl
const char * kernelStr =
    "kernel void VectorAdd(global const short4* a, \n"
    "                      global const short4* b, \n"
    "                      global short4* c) \n"
    "{\n"
    "    int id = get_global_id(0);\n"
    "    c[id] = a[id] + b[id];\n"
    "}\n";

const int NumElements     = 8*1024*1024;  // 8 Mb
const int NumWorkGroups   = 256;
const int VectorElements  = 4;
const int NumVecElements  = NumElements / VectorElements;
const int WorkGroupSize   = NumVecElements / NumWorkGroups;


#ifdef _TI_RTOS
void ocl_main(UArg arg0, UArg arg1)
{
   int    argc = (int)     arg0;
   char **argv = (char **) arg1;
#else
#define RETURN(x) return x
int main(int argc, char *argv[])
{
#endif
   cl_int err     = CL_SUCCESS; //CL/cl.hpp 소스 참조
   int    bufsize = sizeof(int) * NumElements; //컴파일러에 따라 int 크기가 다르다 cl 은 플랫폼에 따라 성능이 달라지니까  sizeof(int) 로 선언해준게 아닌가 싶다.8.182*0.003906 = 31.997952 

   cl_short *srcA   = (cl_short *)__malloc_ddr(bufsize);
   cl_short *srcB   = (cl_short *)__malloc_ddr(bufsize);// cpu랑 다른 플랫폼이랑 공유할수있는 버퍼공간 생성
   cl_short *dst    = (cl_short *)__malloc_ddr(bufsize);
   cl_short *Golden = (cl_short *)__malloc_ddr(bufsize);
   assert(srcA != NULL && srcB != NULL && dst != NULL && Golden != NULL);

   for (int i=0; i < NumElements; ++i) // 공간을 옮겨준다. 왜? 옮겨주는거지?
   {
       srcA[i]   = srcB[i] = i<<2; 
       Golden[i] = srcB[i] + srcA[i];
       dst[i]    = 0;
   }

   try
   {
     Context context(CL_DEVICE_TYPE_ACCELERATOR);// dsp 그래픽카드에 붙는다. 
     std::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>(); //cl_context_devices 정보를 생성

     int d = 0;
     std::string str;
     devices[d].getInfo(CL_DEVICE_NAME, &str); //디바이스의 정보를 가져온다.
     cout << "DEVICE: " << str << endl << endl;

     cout << "Offloading vector addition of " << NumElements/1024;
     cout << "K elements..." << endl << endl;

     Buffer bufA   (context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                             bufsize, srcA); //cl_mem_read_only 이 읽는 속도가 더 빨라진다. cl_mem_use_host_ptr:host_ptr이 가르키는 메모리 주소를 그대로 사용한다고 명시(이미 host에 메모리가 할당된상태) 
     Buffer bufB   (context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                             bufsize, srcB);
     Buffer bufDst (context, CL_MEM_WRITE_ONLY| CL_MEM_USE_HOST_PTR,
                             bufsize, dst);
#ifndef _TI_RTOS
     Program::Sources    source(1, std::make_pair(kernelStr,strlen(kernelStr)));
     Program             program = Program(context, source);
#else
     Program::Binaries   binary(1, make_pair(kernel_dsp_bin,
                                             sizeof(kernel_dsp_bin)));
     Program             program = Program(context, devices, binary);
#endif
     program.build(devices);

 Kernel kernel(program, "VectorAdd");
     kernel.setArg(0, bufA); //순서대로 인자를 출력 a, b, dst
     kernel.setArg(1, bufB);
     kernel.setArg(2, bufDst);

     Event ev1;// ev1 이 발생

     CommandQueue Q(context, devices[d], CL_QUEUE_PROFILING_ENABLE); //순서대로 출력을 하기 위해선 대기큐룰 통해 다음에 출력되는것들을 대기시켜줘야한다.

     Q.enqueueNDRangeKernel(kernel, NullRange, NDRange(NumVecElements),
                            NDRange(WorkGroupSize), NULL, &ev1); // 큐에 cl 프로그램 커널을 인큐하고 실행한다. nullrange ndrange ndrange는 차례대로 오프셋 글로벌 로컬사이즈인데 오프셋은없고 ,전체글로벌은 numvecelements을표시 로컬은 workgroupsize 표시(맞나?)
     ev1.wait(); //ev1 대기

     ocl_event_times(ev1, "Kernel Exec"); // event_time : 트리거의 조건 영역과 실행 영역이 실행되는 시점을 지정하며 BEFORE, AFTER, DEFERRED가 있다.

     __free_ddr(srcA); //ddr 사이즈를 자유롭게 부여??
     __free_ddr(srcB);
   }
   catch (Error err) //에러뜨면 잡아주는곳
   {
     cerr << "ERROR: " << err.what() << "(" << err.err() << ", "
          << ocl_decode_error(err.err()) << ")" << endl;
   }

   for (int i=0; i < NumElements; ++i)
       if (Golden[i] != dst[i]) // golden 이랑 dst 랑 안맞으면  출력
       {
           cout << "Failed at Element " << i << ": "
                << Golden[i] << " != " << dst[i] << endl;
           RETURN(-1);
       }

   __free_ddr(dst);
   __free_ddr(Golden);

   cout << "Success!" << endl;

   RETURN(0);
}
                                                                                                                                                                                                                             


