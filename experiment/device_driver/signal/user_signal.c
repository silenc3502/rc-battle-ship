#include <stdio.h>
/** To create a thread - task-id for the Signal Handler **/
#include <pthread.h>
/** For signal information handler **/
#include <signal.h>
/** For the pthread **/
#include <linux/unistd.h>
/** file open, read , write **/
#include <fcntl.h>
/** strlen, etc */
#include <string.h>
/** For signals **/
#include <sys/types.h>
#include <unistd.h>
/* for error number **/
#include <errno.h>

/** This is same as the proc file created in the kernel space **/
#define PROCFS_FILE_NAME_PATH "/proc/eSataPlugStat"
/** This signal number matches with the kernel space signal number **/
#define SIG_ESATA_PLUG_OUT (45)
/** Proc File handler - I will use this to write the task-id, also I will use this for blocking the thread **/
int configfd = -1;
/** Sig action to handle the signal handler **/
struct sigaction sigPlugIn;

/** Prototype to init **/
void init();

/** The signal handler for the signal - SIG_ESATA_PLUG_OUT - sent by the Kernel space **/
void sigHandlerEvent(int unUsedInt, siginfo_t * punUsedPtr, void * punUsedVPtr)
{
	/** It is more or less like an interrupt - please do minimal here **/
	/** In fact post the message to the task - and return **/
	printf("Caught the signal ...\r\n");
}

void init()
{
	char bufTaskId[10];
	sigPlugIn.sa_sigaction = sigHandlerEvent;
	sigPlugIn.sa_flags = SA_SIGINFO;

	/**Open Read Write only - write for task-id and read for blocking it **/
	configfd = open(PROCFS_FILE_NAME_PATH, O_RDWR);

	/** file descriptor check **/
	if(configfd < 0)
	{
		printf("%s: Failed to open PROC FIle eSata\n", __FUNCTION__);
	}

	/** get Task-ID **/
	sprintf(bufTaskId, "%i", (int)syscall(__NR_gettid));

	printf("The Task-ID is %d\r\n", (int)syscall(__NR_gettid));

	/** write the TaskId to the proc file **/

	if (write(configfd, bufTaskId, strlen(bufTaskId) + 1) < 0)
	{
		printf("%s: Failed to write PROC FIle eSata\n", __FUNCTION__);
	}

	/** Install the signal handler now **/
	/** Null - passing no parameters to the context **/
	sigaction(SIG_ESATA_PLUG_OUT, &sigPlugIn, NULL);

}

void *Thread_Function(void *thread_arg)
{
	unsigned char readbuff;
	int length = 0;

	/** Initialize and install the signal handler **/
	init();

	/** forever loop **/
	/** when a signal is sent the kernel read() driver here wakes up, and move out
	 * from the wait queue - interruptible **/
	/** The length will be return zero when the signal is handled by the sigaction handler- the character driver on sending the signal
	 * will awake the interruptible read driver, and it will return 0 - otherwise it will be blocked **/
	do {
		length = read(configfd, &readbuff, 1);
		//printf("Looping now..Ideally should be a blocking call\r\n");
		//printf("The value of lenght is %d \r\n",length);
	} while (length == 0 || errno == EINTR);

	/** Once the kernel module will be rmmod, the length will be -1 **/
	printf("The length value is %d\r\n",length);
	/** Forever loop .. as driver blocks read call forever **/
	printf("closing file now..\r\n");
	close(configfd);

}

/** Main function - User space land **/
int main()
{
	pid_t thread_pid_val = getpid();
	pthread_t thread_id;

	printf("The PID is %d\r\n", getpid());
	/** create thread **/
	pthread_create(&thread_id,NULL,Thread_Function,NULL);
	/** join so that application doesn't exit until the thread is running **/
	pthread_join(thread_id, NULL);

	return 0;
}
