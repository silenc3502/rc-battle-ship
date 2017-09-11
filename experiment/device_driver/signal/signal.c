#include <linux/init.h>
#include <linux/module.h> /** needed by all modules **/
#include <linux/kernel.h>  /** This is for KERN_ALERT **/
/** To send a signal **/
#include <linux/sched.h>
#include <asm/siginfo.h>
#include <linux/pid_namespace.h>
#include <linux/pid.h>
#include <linux/timer.h>  /** Linux Timer **/
/** for copy_to_user **/
#include <asm/uaccess.h>
#include <linux/rcupdate.h>     //rcu_read_lock
#include <linux/proc_fs.h>    /** This is for procfs **/
/** spin lock essential here **/
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

/** This is the user defined signal- I have chosen arbitrary number as a signal number **/
/** This number should match in the user space application to catch this signal **/
#define SIG_ESATA_PLUG_OUT (45)
/** This is the procfs file for the process/thread to register it's thread-id **/
/** To send the signal, we need the task-id, which is a destination **/
#define PROCFS_FILE_NAME  "eSataPlugStat"
/** Maximum proc size - of no use here much **/
#define PROCFS_MAX_SIZE   (4096)

/** This is the EsataTask - task struct used as a destination to send the signal **/
struct task_struct *eSataTask = NULL;
/** This is the signalinfo struct - used to fill up the information - the signal to send **/
struct siginfo eSataInfo;
/** The proc_dir_entry to create the procfs system **/
static struct proc_dir_entry *eSataProcFile = NULL;
/** This is the wait- queue, I want the calling thread to put in the interruptible sleep **/
/** Please note- when the signal is send, the thread comes out from the interruptible sleep  to serve signal via sigHandler **/
/** If it is put in the non-interruptible sleep, then the calling thread will not be able to catch any signals **/
static wait_queue_head_t signalsDriverQueue;
/** block always - flag - I needed a flag to make read blocking ***/
static int read_unblock_flag=0;

/** Note current is a global variable representing the current Task - struct task_struct**/
//static struct task_struct *MycurrentTask = NULL;

/** Standard timer - doesn't offer accuracy required for real time applications **/

static struct timer_list modTimer;
/** I want to send the signal for 5 times. Thus, I am using counter for that purpose **/
/** Every time on Timer callback, I restart the timer- if counter < 4 **/
/** However, I check this condition on execution of the timer - it is like do - while() **/
/** Thus, even checking counter < 4, it executes actually 5 times - 0 till 4 **/
static int counter = 0;

/** One should avoid re-starting the timer again in the timer callback **/
/** The design should be state based and should avoid this situation **/
/** On a particular state, it should again restart the timer **/
void modTimerCallback(unsigned long data)
{
	/** jiffies is a global variable - ticks of the kernel **/
	int ret;
	printk("modTimerCallback..called..%ld\n", jiffies);

	if( eSataTask != NULL )
	{
		printk("Sending Signal \r\\n");

		if(send_sig_info(SIG_ESATA_PLUG_OUT, &eSataInfo, eSataTask) < 0)    //send the signal
		{
			/** I get this error, when the application cannot receive the signal - the application is dead - the task-id doesn't exist **/
			printk("error sending ESATA hotplug-out signal\n");
		}

	}

	if( counter < 4 )

	{

		ret = mod_timer(&modTimer,jiffies+msecs_to_jiffies(20000));
		if( ret < 0)
		{
			printk("Timer restart failed..%d..\r\n",counter);
		}
		counter++;
	}

}

static ssize_t esataProcfsWrite(struct file *file, const char *buffer, size_t len, loff_t * off)
{
	int pid = 0;
	static char procfsBuffer[PROCFS_MAX_SIZE];
	static unsigned long procfsBufferSize = 0;

	if ( len > PROCFS_MAX_SIZE )
	{
		printk("Looks Odd, the procfs size not sufficient\r\n");
		/** anyway - fix it with the size we have fixed in kernel space **/
		procfsBufferSize = PROCFS_MAX_SIZE;
	}
	else
	{
		/** otherwise set the length **/
		procfsBufferSize = len;

	}

	/** copy the buffer - userSpace to the kernel space buffer - procfsBuffer **/
	/** the syntax looks similar to memcpy - destination, src, and the size **/
	if ( copy_from_user(procfsBuffer, buffer, procfsBufferSize) )
	{
		return -EFAULT;
	}
	/** get the number from procfsBuffer as an integer in the pid **/
	sscanf(procfsBuffer, "%d", &pid);
	/** This is the value set by the user space application - it called the write driver call**/
	printk("The task-id is %d\r\n", pid);
	/** What was the size return */
	printk(KERN_INFO "procfs_write: write %lu bytes\n", procfsBufferSize);

	/** Prepare the Signal now ****/
	memset(&eSataInfo, 0, sizeof(struct siginfo));

	/** si_code - for user to user - it is always - SI_QUEUE, however in real time also - we can go for SI_QUEUE **/
	/** With SI_QUEUE - we don't miss any signals **/
	/** With SI_KERNEL - we can lose signals - if it is not served **/
	eSataInfo.si_code =  SI_KERNEL;
	/*** Arbitrarily chosen - 32 bits **/
	eSataInfo.si_int = 1234;
	/** lock the task - so that you get the correct pid from the task provided **/
	/** If you don't lock, and the kernel schedules to other task, then the information may be invalid **/
	rcu_read_lock();
	eSataTask = pid_task(find_pid_ns(pid, &init_pid_ns), PIDTYPE_PID);
	printk("The EsataTask value is %p\r\n", eSataTask);

	if(eSataTask == NULL)
	{
		printk("no such pid-- ERROR\r\n");
		rcu_read_unlock();
		return -ENODEV;
	}
	rcu_read_unlock();
	return procfsBufferSize;
}

/** This is the read function to get the data **/
//static ssize_t esataProcfsWrite(struct file *file, const char *buffer, size_t len, loff_t * off)

static ssize_t esataProcfsRead( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	/** Put read on a blocking call forever **/
	printk("Block the read call forever\r\n");
	/** One should make this interruptible, otherwise,,it will be on a long sleep **/
	/** If I do  wait_event. I see that the task is in the sleep mode ..Sl in ps -auxwww */
	/** Interruptible, the read call is unblocked once the signal is generated by the kernel***/
	/** However, I will return 0 here, This means that there is no data **/
	/** The user application - thread after unblocking, should just loop-without doing anything, if it sees return value 0 **/
	wait_event_interruptible(signalsDriverQueue, read_unblock_flag != 0);
	return 0;
}

static struct file_operations esataFileOps =
{
	.read = esataProcfsRead,
	.write = esataProcfsWrite,
	.open = NULL,
	.release = NULL
};

static int signal_init(void)
{
	int ret = 0;

	init_waitqueue_head(&signalsDriverQueue);

	printk(KERN_ALERT "Welcome signal init-- \r\n");

	/** creates the proc file - the mode is 0644 - permission bits - 0 means octal - and the value for ugo is 6,4,4 **/
	/** The NULL means the proc file will be created under /proc only **/
	eSataProcFile = proc_create(PROCFS_FILE_NAME,0644,NULL,&esataFileOps);

	if (eSataProcFile == NULL)
	{
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
				PROCFS_FILE_NAME);
		return -ENOMEM;
	}

	printk(KERN_INFO "/proc/%s created\n", PROCFS_FILE_NAME);

	/** setup the timer to shoot the signal to user space **/
	/** Remember the kernel code should first insmod before user space application is initialized **/
	/** It always happens the same way - first kernel, and then user space application **/
	setup_timer(&modTimer, modTimerCallback, 0);
	/** setup the timer to fire in 5 seconds **/
	ret = mod_timer(&modTimer,jiffies+msecs_to_jiffies(20000));
	if( ret < 0)
	{
		printk("Failed in setting the modTimer\r\n");
		return ret;
	}

	/** return 0 to tell that insmod was successful **/
	return ret;
}

static void signal_exit(void)
{
	printk(KERN_ALERT "Goodbye..signal- KERNEL TO USER SPACE \r\n");
	read_unblock_flag = 1;
	/** don't block the user application anymore **/
	wake_up_interruptible( &signalsDriverQueue );

	if(del_timer(&modTimer))
	{
		printk("ERROR in deleting the timer\r\n");
	}

	remove_proc_entry(PROCFS_FILE_NAME, NULL);
}

module_init(signal_init);
module_exit(signal_exit);
