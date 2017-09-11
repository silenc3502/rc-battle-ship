#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "test_pruss_intc_mapping.h"
#include "prussdrv.h"

//#define DEBUG

#define DEFAULT_NUM_EVENTS 100
#define DEFAULT_TEST_TIMER_PERIOD 50000
extern int clock_nanosleep(clockid_t __clock_id, int __flags,
                           __const struct timespec *__req,
                           struct timespec *__rem);

#define USEC_PER_SEC		1000000
#define NSEC_PER_SEC		1000000000
typedef void (*bsp_function_handler) (void);

/* Global variables */
unsigned int num_events;
unsigned int test_timer_period;
char pruss_device_name[16];
char pru0_firmware_file_name[128];
char pru1_firmware_file_name[128];

void *pruss_drv_handle;

static pthread_t bsp_systimer_thread = (pthread_t) NULL;
static volatile int shutdown_sys_timer = 0, pru0_isr_exit_flag = 0, pru1_isr_exit_flag = 0;
static int count = 0, pru0_event_count = 0, pru1_event_count = 0;
volatile struct timespec pru0_send_time, pru1_send_time;
volatile uint64_t pru0_accumulated_time_diff=1, pru1_accumulated_time_diff=0;
/* Struct to transfer parameters to the thread */
struct thread_param {
    int prio;
    int policy;
    int interval;
    int clock;
    bsp_function_handler callbackfn;
};

static struct thread_param systimer_thread_param;

static inline void tsnorm(struct timespec *ts)
{
    while (ts->tv_nsec >= NSEC_PER_SEC) {
        ts->tv_nsec -= NSEC_PER_SEC;
        ts->tv_sec++;
    }
}

static inline int64_t calcdiff(struct timespec t1, struct timespec t2)
{
    int64_t diff;

    diff = USEC_PER_SEC * (long long)((int) t1.tv_sec - (int) t2.tv_sec);
    diff += ((int) t1.tv_nsec - (int) t2.tv_nsec) / 1000;
    return diff;
}

static inline int64_t calcdiff_ns(struct timespec t1, struct timespec t2)
{
    int64_t diff;

    diff = NSEC_PER_SEC * (int64_t)((int) t1.tv_sec - (int) t2.tv_sec);
    diff += ((int) t1.tv_nsec - (int) t2.tv_nsec);
    return diff;
}

void *timer_thread(void *param)
{
    struct timespec now, next, interval;
    struct thread_param *par = param;
    struct sched_param schedp;

    interval.tv_sec = par->interval / USEC_PER_SEC;
    interval.tv_nsec = (par->interval % USEC_PER_SEC) * 1000;

    memset(&schedp, 0, sizeof(schedp));
    schedp.sched_priority = par->prio;
    sched_setscheduler(0, par->policy, &schedp);

    /* Get current time */
    clock_gettime(par->clock, &now);
    next = now;

    while (!shutdown_sys_timer) {
        int ret;
        next.tv_sec += interval.tv_sec;
        next.tv_nsec += interval.tv_nsec;
        tsnorm(&next);
        ret = clock_nanosleep(par->clock, TIMER_ABSTIME, &next, NULL);
        if (par->callbackfn)
            par->callbackfn();
   }
}

void mytimerfn (void)
{
    count++;  
    if (count == 1)
    {
         clock_gettime(CLOCK_MONOTONIC, (struct timespec *)&pru0_send_time);
         prussdrv_pru_send_event(pruss_drv_handle, ARM_PRU0_EVENT);
#ifdef DEBUG
         printf ("send ARM_PRU0_EVENT at now.sec: %ld now.nsec: %ld\n",
            pru0_send_time.tv_sec, pru0_send_time.tv_nsec);
#endif
    } 
    else
    {
        if (count == 2)
        {
            count = 0;
            clock_gettime(CLOCK_MONOTONIC, (struct timespec *)&pru1_send_time);
            prussdrv_pru_send_event(pruss_drv_handle, ARM_PRU1_EVENT);
#ifdef DEBUG
            printf ("send ARM_PRU1_EVENT at now.sec: %ld now.nsec: %ld\n",
                pru1_send_time.tv_sec, pru1_send_time.tv_nsec);
            printf ("pru0_evt_cnt:%d pru1_evt_cnt:%d\n", pru0_event_count, pru1_event_count);
#endif
        } 
    }   
}

int bsp_systimer_init(int period, bsp_function_handler timerfn)
{
    pthread_attr_t systimer_thread_attr;
    systimer_thread_param.callbackfn = timerfn;
    systimer_thread_param.interval = period;
    systimer_thread_param.prio = sched_get_priority_max(SCHED_FIFO);
    systimer_thread_param.policy = SCHED_FIFO;
    systimer_thread_param.clock = CLOCK_MONOTONIC;
    shutdown_sys_timer = 0;
    pthread_attr_init(&systimer_thread_attr);
    pthread_create(&bsp_systimer_thread, &systimer_thread_attr,
                   timer_thread, &systimer_thread_param);

    pthread_attr_destroy(&systimer_thread_attr);

    return 0;
}

void bsp_systimer_exit(void)
{
    shutdown_sys_timer = 1;
}

void pruss_test_abort (int sig_num)
{
    bsp_systimer_exit();
    sleep(1);
    prussdrv_pru_disable(pruss_drv_handle, 0);
    prussdrv_pru_disable(pruss_drv_handle, 1);
    prussdrv_exit (pruss_drv_handle);
    printf("\npru0_evt_cnt:%d pru1_evt_cnt:%d\n", pru0_event_count, pru1_event_count);
    if (pru0_event_count && pru0_accumulated_time_diff )
        printf("average round trip: pru0 %lu ns \n",
            (unsigned long)(pru0_accumulated_time_diff/pru0_event_count));
    if (pru1_event_count && pru1_accumulated_time_diff)
        printf("average round trip: pru1 %lu ns\n",
            (unsigned long)(pru1_accumulated_time_diff/pru1_event_count));

    exit (0);
}

void *pru0_isr(void *arg)
{
    struct timespec now;

    do {
        prussdrv_pru_wait_event(pruss_drv_handle, PRU_EVTOUT_0);
        clock_gettime(CLOCK_MONOTONIC, &now);
        pru0_accumulated_time_diff += calcdiff_ns(now, pru0_send_time);
#ifdef DEBUG
        printf ("receive ARM_PRU0_EVENT at now.sec: %ld now.nsec: %ld\n", now.tv_sec, now.tv_nsec);
#endif
        pru0_event_count++;
        prussdrv_pru_clear_event(pruss_drv_handle, PRU0_ARM_EVENT);
    } while (pru0_event_count < num_events);
}

void *pru1_isr(void *arg)
{
    struct timespec now;

    do {
        prussdrv_pru_wait_event(pruss_drv_handle, PRU_EVTOUT_1);
        clock_gettime(CLOCK_MONOTONIC, &now);
        pru1_accumulated_time_diff += calcdiff_ns(now, pru1_send_time);
#ifdef DEBUG
        printf ("receive ARM_PRU1_EVENT at now.sec: %ld now.nsec: %ld\n", now.tv_sec, now.tv_nsec);
#endif
        pru1_event_count++;
        prussdrv_pru_clear_event(pruss_drv_handle, PRU1_ARM_EVENT);

    } while (pru1_event_count < num_events);
}

int bsp_init()
{
    int ret_val;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    //Initialize PRUSS UIO userspace
    ret_val = prussdrv_init(&pruss_drv_handle, pruss_device_name);
    if (ret_val < 0) {
        printf("\n prussdrv_init failed %d\n", ret_val);fflush(stdout);
        return ret_val;
    }
    ret_val = prussdrv_open(pruss_drv_handle, PRU_EVTOUT_0);
    if (ret_val < 0) {
        printf("\n prussdrv_open 0 failed %d\n", ret_val);fflush(stdout);
        return ret_val;
    }

    ret_val = prussdrv_open(pruss_drv_handle, PRU_EVTOUT_1);
    if (ret_val < 0) {
        printf("\n prussdrv_open 1 failed %d\n", ret_val);fflush(stdout);
        return ret_val;
    }

    prussdrv_pru_reset(pruss_drv_handle, 0);
    prussdrv_pru_reset(pruss_drv_handle, 1);

    prussdrv_pruintc_init(pruss_drv_handle, &pruss_intc_initdata);

    printf("\n pru download start pru0 \n");

    ret_val = prussdrv_exec_program(pruss_drv_handle, 0, pru0_firmware_file_name);
    if (ret_val < 0 ) {
        printf("\n Error exec program pru 0\n");
        return ret_val;
    }

    printf("\n pru download start pru1 \n");

    ret_val = prussdrv_exec_program(pruss_drv_handle, 1, pru1_firmware_file_name);
    if (ret_val < 0 ) {
        printf("\n Error exec program pru 1\n");
        return ret_val;
    }

    printf("\n pru download end \n");

    pru0_isr_exit_flag = pru1_isr_exit_flag = 0;

    prussdrv_start_irqthread(pruss_drv_handle, PRU_EVTOUT_0,
                             sched_get_priority_max(SCHED_FIFO)-10,
                             pru0_isr);
    prussdrv_start_irqthread(pruss_drv_handle, PRU_EVTOUT_1,
                             sched_get_priority_max(SCHED_FIFO)-11,
                             pru1_isr);
    bsp_systimer_init(test_timer_period, &mytimerfn);

}

void main (int32_t argc, int8_t **argv)
{
    int ret_val;
    
    test_timer_period = DEFAULT_TEST_TIMER_PERIOD;
    num_events = DEFAULT_NUM_EVENTS;

    /* Read parameters from command line */
    if(argc < 4)
    {
        printf("Usage: %s <device_name> <firmware_file_pru0> <firmware_file_pru1>"
            "[<num_events>:default 100]"
            "[<Interval between send events in msec>: default to 50 ms]\n", argv[0]);
        exit(-1);
    }
    sscanf(argv[1], "%s", pruss_device_name);
    sscanf(argv[2], "%s", pru0_firmware_file_name);
    sscanf(argv[3], "%s", pru1_firmware_file_name);
#ifdef DEBUG
    printf("\nnum params %d file names %s %s\n", argc, pru0_firmware_file_name, pru1_firmware_file_name);
#endif
    if(argc > 4)
    {
        sscanf(argv[4], "%u", &num_events);
    }

    if(argc > 5)
    {
        sscanf(argv[5], "%u", &test_timer_period);
        /* convert to usec */
        test_timer_period *= 1000;
    }
    

    ret_val = bsp_init ();
    if (ret_val < 0) {
        printf("\n Error in bsp_init \n");
        exit(-1);
    }

    if(signal (SIGINT, pruss_test_abort) == SIG_ERR)
	printf("\n Cant catch SIGINT \n");

    while (1)
    {
        usleep(test_timer_period);
        if ( (pru0_event_count >= num_events) || (pru1_event_count >= num_events)) {
            pruss_test_abort(0);
        }
    }
}
