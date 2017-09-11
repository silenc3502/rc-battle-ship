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

/******************************************************************************
 * This vecadd_md example computes on all available ACCELERATOR devices,
 * while vecadd example only computes on the first ACCELERATOR device.
 *****************************************************************************/
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "ocl_util.h"

using namespace cl;
using namespace std;
// OpenCL 커널. 각 작업 항목은 c의 한 요소를 처리합니다.
const char * kernelStr =
    "kernel void VectorAdd(global const short4* a, \n"
    "                      global const short4* b, \n"
    "                      global short4* c) \n"
    "{\n" 
    "    int id = get_global_id(0);\n" // 글로벌 스레드 ID 가져 오기
    "    c[id] = a[id] + b[id];\n"
    "}\n";

const int NumElements     = 8*1024;
const int NumWorkGroups   = 256;
const int VectorElements  = 4;
const int NumVecElements  = NumElements / VectorElements;
const int WorkGroupSize   = NumVecElements / NumWorkGroups;

int gettime_ms()
{
    struct timespec tp; //시간 구조체

// struct timespec {
 //           time_t   tv_sec;        /* seconds */
  //          long     tv_nsec;       /* nanoseconds */
  //  };


    if (clock_gettime(CLOCK_MONOTONIC, &tp) != 0) /*clock_gettime 함수는 리눅스 환경에서 코딩할때 특정 구간내에서 실행되는 코드의 정밀한 
        시간차를 구하려고 할 때 사용할 수 있는 함수 clock_gettime 함수는 나노초 단위까지의 시간을 구할 수 있다.  CLOCK_MONOTONIC :단조 시계 (특정 시점에서 흐른 시간, 대체적으로 부팅후 시간) 
        부팅됐는지 안됐는지 확인 하는 거 같다.*/


        clock_gettime(CLOCK_REALTIME, &tp); /*시스템 전역의 실제 시계(The UNIX Epoch 시간)*/

    return tp.tv_nsec / 1000 + tp.tv_sec * 1000000; //시간값 왜 반환하는거지?
}

int main(int argc, char *argv[]) /* argc 는 정수형 값으로 명령 행 인자의 개수이다. argv는 문자열 배열 포인터로
 배열의 첫번째 요소, argv[0]의 내용은 실행되는 프로그램의 완전한 경로 명칭(FUll path name)이다. */
{
   cl_int err     = CL_SUCCESS;
   int    bufsize = NumElements * sizeof(cl_short);
   int num_errors = 0;
   int          d = 0;
   int start_time, end_time;

   try 
   {
     Context context(CL_DEVICE_TYPE_ACCELERATOR);
     std::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
     int num_devices = devices.size(); //devices.size 를 num_devices 에 넣었다.
     if (NumVecElements % num_devices != 0 || 
         (NumVecElements / num_devices) % WorkGroupSize != 0)
     {
        cerr << "ERROR: Cannot evenly distribute data across devices!" << endl;
        exit(-1);
     }
     int d_bufsize = bufsize / num_devices;
     int d_Elements = NumElements / num_devices;
     int d_VecElements = NumVecElements / num_devices;
	//Program: OpenCL 프로그램은 일련의 커널로 구성됩니다. 프로그램에는 __kernel 함수 및
	//상수 데이터에 의해 호출되는 보조 함수가 포함될 수도 있습니다
     Program::Sources    source(1, std::make_pair(kernelStr,strlen(kernelStr))); //
     Program             program = Program(context, source);
     program.build(devices); 
     Kernel kernel(program, "VectorAdd");

     for (d = 0; d < num_devices; ++d)
     {
        std::string str;
        devices[d].getInfo(CL_DEVICE_NAME, &str); //디바이스 정보 호출
        cout << "DEVICE " << d << ": " << str << endl << endl;
     }

  /* Method 1: Use ReadBuffer/WriteBuffer APIs */
  {
     cout << "=== Method 1: Using ReadBuffer/WriteBuffer APIs ===" << endl; 
     cl_short *srcA   = (cl_short *) malloc(bufsize);
     cl_short *srcB   = (cl_short *) malloc(bufsize);
     cl_short *dst    = (cl_short *) malloc(bufsize);
     cl_short *Golden = (cl_short *) malloc(bufsize);
     if (!srcA || !srcB || !dst || !Golden)
     {
        cout << "Unable to allocate memory for data! (ReadBuffer API)" << endl;
        exit(-1);
     }

     start_time = gettime_ms();
     for (int i=0; i < NumElements; ++i) 
     { 
        srcA[i]   = srcB[i] = i<<2;  //값을 초기화한다. (딱히 중요한건 아닌듯)
        Golden[i] = srcB[i] + srcA[i]; 
        dst[i]    = 0;
     }


     std::vector<Buffer*> bufAs, bufBs, bufDs; // 이건 배열이였다. http://shaeod.tistory.com/464 사이트 참고
     std::vector<CommandQueue*> Qs; //객체 생성 하는 선언
     std::vector<Event*> ev1s, ev2s, ev3s, ev4s;
     for (d = 0; d < num_devices; ++d)
     {
        bufAs.push_back(new Buffer(context, CL_MEM_READ_ONLY, d_bufsize)); // push_back(k) - 벡터에 맨 마지막에 k라는 원소를 추가한다. 이때 기존의 capacity보다 클 경우에는 메모리를 재할당 받게 된다.
        bufBs.push_back(new Buffer(context, CL_MEM_READ_ONLY, d_bufsize)); // 새로운 버퍼 객제를 생성 하는데 flags(CL_MEM_READ_ONLY) 는 버퍼 객체를 할당하기 위해 사용되어야하는 메모리 영역 및 사용 방법과 
        //같은 할당 사용 정보를 지정하는 데 사용되는 비트 필드다. 
        bufDs.push_back(new Buffer(context, CL_MEM_WRITE_ONLY, d_bufsize)); // CL_MEM_WRITE_ONLY 플래그는 메모리 객체가 커널에 의해 쓰여지지만 읽혀지지 않도록 지정 
        //커널 내부에서 CL_MEM_WRITE_ONLY로 생성 된 버퍼 또는 이미지 객체를 읽는 것은 정의되지 않습니다.
        //CL_MEM_WRITE_ONLY 플래그는 메모리 객체가 커널에 의해 읽고 쓰여지도록 지정. 이것은 기본값
        Qs.push_back(new CommandQueue(context, devices[d], // 특정 장치에 명령 대기열을 만든다.
                                      CL_QUEUE_PROFILING_ENABLE)); //properties(CL_QUEUE_PROFILING_ENABLE)는 명령대기열의 등록정보 목록을 지정하는데 properties는 비트필드다. 
        //CL_QUEUE_PROFILING_ENABLE 말고도 다른 명령어로 특성을 결정할 수있다. 
        //CL_QUEUE_PROFILING_ENABLE 는 명령 대기열에서 명령 프로파일 링을 활성화 또는 비활성화를 결정 할 수 있다.  
        ev1s.push_back(new Event());
        ev2s.push_back(new Event());
        ev3s.push_back(new Event());
        ev4s.push_back(new Event());
     }

     for (d = 0; d < num_devices; ++d)
     {
        kernel.setArg(0, *bufAs[d]);
        kernel.setArg(1, *bufBs[d]);
        kernel.setArg(2, *bufDs[d]);
        Qs[d]->enqueueWriteBuffer(*bufAs[d], CL_FALSE, 0, d_bufsize, // enqueueWriteBuffer : 호스트 메모리에서 버퍼 객체에 쓰기 명령을 인큐 <-아마 여기서 더하기를 해주는게 아닌가 싶음
                                  (void *)(((cl_char *) srcA) + d * d_bufsize),
                                  NULL, ev1s[d]);
        Qs[d]->enqueueWriteBuffer(*bufBs[d], CL_FALSE, 0, d_bufsize,
                                  (void *)(((cl_char *) srcB) + d * d_bufsize),
                                  NULL, ev2s[d]);
        Qs[d]->enqueueNDRangeKernel(kernel, NullRange, NDRange(d_VecElements), // 장치에 커널을 실행하는 명령을 대기열에 둡니다.
                                    NDRange(WorkGroupSize), NULL, ev3s[d]);
        Qs[d]->enqueueReadBuffer(*bufDs[d], CL_FALSE, 0, d_bufsize, //버퍼 객체에서 호스트 메모리로 읽는 명령을 enqueue 합니다.
                                 (void *)(((cl_char *) dst) + d * d_bufsize),
                                 NULL, ev4s[d]);
     }

     for (d = 0; d < num_devices; ++d)
        ev4s[d]->wait(); // 대기로 넘어감
     for (int i=0; i < NumElements; ++i) // 내가 어떤 연산을 + 에서 * 로 바껐을때 떴었다. 기억이안남;; 곱하기만 하면 일치하지않는거같다. 
        if (Golden[i] != dst[i]) 
        { 
            num_errors += 1;
            if (num_errors < 10)
                cout << "Failed at Element " << i << ": " 
                     << Golden[i] << " != " << dst[i] << endl; 
        }
     end_time = gettime_ms(); 
     cout << "Method 1:  " << end_time - start_time << " micro seconds" << endl; // 계산한 시간을 보여준다. 

     for (d = 0; d < num_devices; ++d)
     {
        cout << "DEVICE " << d << ":" << endl;
        ocl_event_times(*ev1s[d], "Write BufA ");
        ocl_event_times(*ev2s[d], "Write BufB ");
        ocl_event_times(*ev3s[d], "Kernel Exec");
        ocl_event_times(*ev4s[d], "Read BufDst");
     }
     if (num_errors)  cout << "Fail with " << num_errors << " errors!" << endl;
     else             cout << "Success!" << endl; 

     free(srcA); // 다시 초기화 시킨다.
     free(srcB);
     free(Golden);
     free(dst);
     for (d = 0; d < num_devices; ++d) //다시 원상태로초기화 시키는게 아닌가 싶음
     {
         delete bufAs[d];
         delete bufBs[d];
         delete bufDs[d];
         delete Qs[d];
         delete ev1s[d];
         delete ev2s[d];
         delete ev3s[d];
         delete ev4s[d];
     }
  }  /* end Method 1 */

  /* Method 2: Use MapBuffer/UnmapMemObject APIs */
  {
     cout << "\n\n=== Method 2: Using MapBuffer/UnmapBuffer APIs ===" << endl; 
     cl_short *h_bufA, *h_bufB;
     cl_short *mGolden = (cl_short *) malloc(bufsize);
     cl_short **h_dst = (cl_short **) malloc(num_devices * sizeof(cl_short *));
     if (!mGolden || !h_dst)
     {
        cout << "Unable to allocate memory for data! (MapBuffer API)" << endl;
        exit(-1);
     }
     
     start_time = gettime_ms();
     std::vector<Buffer*> mbufAs, mbufBs, mbufDs;
     std::vector<CommandQueue*> mQs;
     std::vector<Event*> mev1s, mev2s, mev3s, mev4s, mev5s, mev6s, mev7s;
     for (d = 0; d < num_devices; ++d)
     {
        mbufAs.push_back(new Buffer(context, CL_MEM_READ_ONLY, d_bufsize));
        mbufBs.push_back(new Buffer(context, CL_MEM_READ_ONLY, d_bufsize));
        mbufDs.push_back(new Buffer(context, CL_MEM_WRITE_ONLY, d_bufsize));
        mQs.push_back(new CommandQueue(context, devices[d],
                                      CL_QUEUE_PROFILING_ENABLE));
        mev1s.push_back(new Event());
        mev2s.push_back(new Event());
        mev3s.push_back(new Event());
        mev4s.push_back(new Event());
        mev5s.push_back(new Event());
        mev6s.push_back(new Event());
        mev7s.push_back(new Event());
     }
     for (d = 0; d < num_devices; ++d)
     {
        cl_short *h_bufA = (cl_short *) mQs[d]->enqueueMapBuffer(*mbufAs[d],  // 버퍼에 의해 주어진 버퍼 객체의 영역을 호스트 주소 공간에 매핑하는 명령을 대기열에 추가하고이 매핑 된 영역에 대한 포인터를 반환합니다.
                         CL_FALSE, CL_MAP_WRITE, 0, d_bufsize, NULL, mev1s[d]);
        cl_short *h_bufB = (cl_short *) mQs[d]->enqueueMapBuffer(*mbufBs[d], 
                         CL_FALSE, CL_MAP_WRITE, 0, d_bufsize, NULL, mev2s[d]);
        mev1s[d]->wait();
        for (int i = 0; i < d_Elements; i++)
          h_bufA[i] = (d * d_Elements + i)<<2;
        mQs[d]->enqueueUnmapMemObject(*mbufAs[d], h_bufA, NULL, mev3s[d]); // 메모리 객체의 이전에 매핑 된 영역을 매핑 해제하는 명령을 대기열에 추가합니다. 처음에 매핑해서 계산하고  해제를해주면서 출력을 하는거같다.
        mev2s[d]->wait();
        for (int i = 0; i < d_Elements; i++)
        {
          h_bufB[i] = (d * d_Elements + i)<<2;
          mGolden[d*d_Elements + i] = h_bufA[i] + h_bufB[i];
        }
        mQs[d]->enqueueUnmapMemObject(*mbufBs[d], h_bufB, NULL, mev4s[d]);
     }
     for (d = 0; d < num_devices; ++d)
     {
        kernel.setArg(0, *mbufAs[d]);
        kernel.setArg(1, *mbufBs[d]);
        kernel.setArg(2, *mbufDs[d]);
        mev3s[d]->wait();
        mQs[d]->enqueueNDRangeKernel(kernel, NullRange, NDRange(d_VecElements),
                                    NDRange(WorkGroupSize), NULL, mev5s[d]);
        h_dst[d] = (cl_short *) mQs[d]->enqueueMapBuffer(*mbufDs[d], CL_FALSE,
                                    CL_MAP_READ, 0, d_bufsize, NULL, mev6s[d]);
     }
     num_errors = 0;
     for (d = 0; d < num_devices; ++d)
     {
        mev6s[d]->wait();
        for (int i = 0; i < d_Elements; ++i)
          if (mGolden[d*d_Elements + i] != h_dst[d][i]) 
          { 
            num_errors += 1;
            if (num_errors < 10)
                cout << "Failed at Element " << i << ": " 
                 << mGolden[d*d_Elements + i] << " != " << h_dst[d][i] << endl;
          }
        mQs[d]->enqueueUnmapMemObject(*mbufDs[d], h_dst[d], NULL, mev7s[d]);
     }

     for (d = 0; d < num_devices; ++d)
        mev7s[d]->wait();
     end_time = gettime_ms(); // 시간 종료
     cout << "Method 2:  " << end_time - start_time << " micro seconds" << endl;

     for (d = 0; d < num_devices; ++d)
     {
        cout << "DEVICE " << d << ":" << endl;
        ocl_event_times(*mev1s[d], "Map   BufA ");
        ocl_event_times(*mev2s[d], "Map   BufB ");
        ocl_event_times(*mev3s[d], "Unmap BufA");
        ocl_event_times(*mev4s[d], "Unmap BufB");
        ocl_event_times(*mev5s[d], "Kernel Exec");
        ocl_event_times(*mev6s[d], "Map   BufDst");
        ocl_event_times(*mev7s[d], "Unmap BufDst");
     }
     if (num_errors)  cout << "Fail with " << num_errors << " errors!" << endl;
     else             cout << "Success!" << endl; 

     free(mGolden);
     free(h_dst);
     for (d = 0; d < num_devices; ++d)
     {
         delete mbufAs[d];
         delete mbufBs[d];
         delete mbufDs[d];
         delete mQs[d];
         delete mev1s[d];
         delete mev2s[d];
         delete mev3s[d];
         delete mev4s[d];
         delete mev5s[d];
         delete mev6s[d];
         delete mev7s[d];
     }
  }  /* end Mehtod 2 */

   }
   catch (Error err) 
   { cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl; }
}