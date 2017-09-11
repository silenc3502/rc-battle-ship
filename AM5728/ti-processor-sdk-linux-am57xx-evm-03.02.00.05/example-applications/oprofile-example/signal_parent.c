/*
 * Module: signal_parent.c
 *
 * Description: Example program used to demonstrate OProfile
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#define BUF_SIZE (4096*256)

volatile static int signal_parent = 0;

void mysig(int sig)
{
    int status;
    pid_t pid;
        
    printf("Signal %d \n", sig);

    if (sig == SIGALRM)
    {
       signal_parent = 1;
    }
    if (sig == SIGCLD)
    {
       // harvest terminated DEFUNCT child process
       pid = waitpid(-1, &status, WNOHANG); 
       printf(" Child Process(%d) terminated with a status of %d\n",
            pid, status); 
    }
}

/*  In main, we tell the child process to wait for ten seconds
    before sending a SIGALRM signal to its parent.  */

int main()
{
    int pid;
    int sum;
    char * buf;
    int i,j;

    printf("child process starting\n");

    if((pid = fork()) == 0) {
      buf = (char *) malloc(BUF_SIZE);
      if(buf == NULL)
      {
        exit(1);
      }
      for(j=0;j<10;j++)
      {
        for(i=0;i<BUF_SIZE;i++)
        {
          buf[i] = i;
          sum = (int)buf[i];
        }
      }
    // Child process
        sleep(9);
    // Wake up parent process with a SIGALRM
        free (buf);
        kill(getppid(), SIGALRM);
        exit(0);
    }

/*  The parent process arranges to catch SIGALRM with a call to signal
    and then waits for the inevitable.  */

    (void) signal(SIGALRM, mysig);
    (void) signal(SIGCLD, mysig);
 
    do
    {
#ifdef USE_SIGNALS
        printf("Parent blocks and waits for signal from child\n");
        pause(); // Wait for any signal call
#endif
    } while(!signal_parent);
    
    printf("Got signal from child\n");

    printf("done\n");
    exit(0);
}
