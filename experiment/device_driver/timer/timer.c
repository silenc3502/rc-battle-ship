#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

static void mykmod_timer_handler(unsigned long data);
static unsigned long onesec;

DEFINE_TIMER(mytimer, mykmod_timer_handler, 0, 0);

static void
mykmod_timer_handler(unsigned long data)
{
        unsigned long j = jiffies;
        pr_info("Timer %u jiffies\n", (unsigned)j);

        mod_timer(&mytimer, jiffies + onesec);
}


int init_module(void)
{
        unsigned long j = jiffies;

        onesec = msecs_to_jiffies(1000 * 1);

        pr_info("Loaded %u/%u jiffies before\n", (unsigned)j, (unsigned)onesec);

        mod_timer(&mytimer, jiffies + onesec);

        pr_info("Loaded %u jiffies after \n", (unsigned)j);

        return 0;
}

void cleanup_module(void)
{
        del_timer(&mytimer);
        pr_info("Exit\n");
}

MODULE_LICENSE("GPL");
