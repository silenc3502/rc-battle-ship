#include <linux/init.h>
#include <linux/module.h> /** needed by all modules **/
#include <linux/kernel.h>  /** This is for KERN_ALERT **/
#include <linux/timer.h>  /** Linux Timer **/

MODULE_LICENSE("GPL");

/** Standard timer - doesn't offer accuracy required for real time applications **/

static struct timer_list modTimer;

/** One should avoid re-starting the timer again in the timer callback **/
/** The design should be state based and should avoid this situation **/
/** On a particular state, it should again restart the timer **/
void modTimerCallback(unsigned long data)
{
	/** jiffies is a global variable - ticks of the kernel **/
	printk("modTimerCallback..called..%ld\n", jiffies);
	printk("Restart the mod timer now\r\n");

}

static int modTimer_init(void)
{
	int ret = -1;
	printk(KERN_ALERT "Hello modTimer\n");
	/** setup timer **/
	setup_timer(&modTimer, modTimerCallback, 0);
	/** setup the timer to fire in 5 seconds **/
	ret = mod_timer(&modTimer,jiffies+msecs_to_jiffies(5000));
	if( ret == 1)
		printk("Timer callback error\r\n");

	return 0;
}

static void modTimer_exit(void)
{
	printk(KERN_ALERT "Goodbye..modTimer\n");

	/*** delete timer **/
	if(del_timer(&modTimer))
	{
		printk("ERROR in deleting the timer\r\n");
	}

}

module_init(modTimer_init);
module_exit(modTimer_exit);
