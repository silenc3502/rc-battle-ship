#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>

//#define PROFILE_ENABLED
#define EMUCNT_PROFILE
#define MAX_NAME_LENGTH		64
#define MAX_FILE_NAME_LENGTH	128
#define MAX_LINE_LENGTH         16

#define MAX_NUM_THREADS 24
#define MAX_NUM_INTERRUPTS 100

#define EMUCNT0_ADDR 0x23101f0

#define REF_EMUCNT0_FROM_INT_SOURCE 0x10800000

/* List of device to receive interrupts from */
char uio_dev_list[MAX_NUM_THREADS][MAX_NAME_LENGTH] = {
	"cic2_out32", "cic2_out33", "cic2_out34", "cic2_out35", "cic2_out36", "cic2_out37", "cic2_out38", "cic2_out39",
	"cic2_out40", "cic2_out41", "cic2_out42", "cic2_out43", "cic2_out44", "cic2_out45", "cic2_out46", "cic2_out47",
	"cic2_out18", "cic2_out19", "cic2_out22", "cic2_out23", "cic2_out50", "cic2_out51", "cic2_out66", "cic2_out67"
};
typedef struct thread_arg_s {
	int fd_uio;
	int index;
	int interrupt_occurred_count;
} thread_arg_t;
pthread_t tid[MAX_NUM_THREADS];
#ifdef EMUCNT_PROFILE
int fd_mem;
uint32_t *emucnt_ptr;
uint32_t emucnt0Log[MAX_NUM_THREADS][MAX_NUM_INTERRUPTS], emucnt1Log[MAX_NUM_THREADS][MAX_NUM_INTERRUPTS];

void *map_dev_mem(uint32_t addr, int size)
{
	uint32_t base, offset, min_length, adjusted_length;
	uint32_t page_size = getpagesize();
	char *map1;

	base = addr  & (~(page_size-1));
	offset = (addr -base);
	min_length = offset+size;
	adjusted_length = min_length & (~(page_size-1));
	adjusted_length = (adjusted_length == min_length) ? min_length : adjusted_length + page_size;

	map1 = mmap(NULL, adjusted_length, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd_mem, base);
	if (map1 == MAP_FAILED) {
		printf("\nmmap failed for addr 0x%x size %d (err: %s)",
		addr, size, strerror(errno));
		return (void *)(-1);
	}
	return((void *)(map1 + offset));
}
int kbhit(void)
{
	struct timeval tv;
	fd_set read_fd;

	tv.tv_sec=0;
	tv.tv_usec=0;
	FD_ZERO(&read_fd);
	FD_SET(STDIN_FILENO,&read_fd);

	if(select(STDIN_FILENO+1, &read_fd, NULL, NULL, &tv) == -1)
		return 0;

	if(FD_ISSET(STDIN_FILENO,&read_fd))
		return 1;

	return 0;
}

#endif

int uioutil_get_string (char *filename, char *str, int str_len)
{
	FILE *fpr = 0;
	int ret_val = -1;

	if (!filename || !str || !str_len) {
		goto close_n_exit;
	}

	fpr = fopen(filename, "r");
	if (!fpr) {
		printf("Error opening file %s (%s)",
		       filename, strerror(errno));
		goto close_n_exit;
	}
	if (!fgets(str, str_len, fpr)) {
		printf("Error reading file %s (%s)",
		       filename, strerror(errno));
		goto close_n_exit;			
	}

	ret_val = 0;

close_n_exit:
	if (fpr) fclose(fpr);
	return ret_val;
}

int uioutil_get_int (char *filename, int *val)
{
	FILE *fpr = 0;
	int ret_val = -1;
	char line[MAX_LINE_LENGTH];

	if (!filename || !val)
		return -1;

	if (uioutil_get_string(filename, line, MAX_LINE_LENGTH) < 0)
		return -1;

	*val = strtoul(line, NULL, 0);
	return 0;
}
char  *remove_prefix_from_device_name(char *name, int max_length)
{ 
	int index;

	index=0;

	/* strip anything before . in the name */
	while(name[index] != '\0')
	{
		if(name[index] == '.')
		{ 
			index++;
			return(&name[index]);
		}
		if(index > max_length)
			break;
		index++;
	}
	return(name);
}
char  *remove_postfix_from_device_name(char *name, int max_length)
{ 
	int index;

	index=0;

	/* strip anything after . in the name */
	while(name[index] != '\0')
	{
		if(name[index] == '.')
		{
			name[index] = '\0';
			index++;
			return(&name[0]);
		}
		if(index > max_length)
			break;
		index++;
	}
	return(name);
}

int uioutil_get_device (char *uio_name, char *class_name, int class_name_length)
{
	struct dirent *entry = 0;
	char filename[MAX_FILE_NAME_LENGTH];
	char name[MAX_NAME_LENGTH];
	int ret_val = -1;
	DIR *dir = 0;
	char *name_ptr;

	if (!uio_name || !class_name || !strlen(uio_name)) {
		printf("Error in call to function");
		goto close_n_exit;
	}

	dir = opendir("/sys/class/uio");
	if (!dir) {
		printf("readdir /sys/class/uio");
		goto close_n_exit;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strstr (entry->d_name, "uio") == NULL) {
			continue;
		}

		snprintf(filename, MAX_FILE_NAME_LENGTH, "/sys/class/uio/%s/name", entry->d_name);

		if (uioutil_get_string(filename, name, MAX_NAME_LENGTH) < 0)
			goto close_n_exit;
		name_ptr = remove_postfix_from_device_name(name, MAX_NAME_LENGTH);
//		name_ptr = remove_prefix_from_device_name(name, MAX_NAME_LENGTH);
#ifdef DEBUG
		printf("\n uio_name %s, name_ptr %s,length %d", uio_name , name_ptr,strlen(uio_name)  );
#endif
		if (!strncmp (uio_name, name_ptr, strlen(uio_name))) {
			ret_val = 0;
			strncpy (class_name, entry->d_name, class_name_length);
			goto close_n_exit;
		}
	}

close_n_exit:
	if (dir) closedir(dir);

	return ret_val;
}
#ifdef PROFILE_ENABLED

/**
 *  @brief Function clock_diff returns difference between two timeval
 *  @param[in]     start            start time
 *  @param[in]     end              end time
 *  @retval        deltaTime
 */
static int64_t clock_diff (struct timespec *start, struct timespec *end)
{
	return (end->tv_sec - start->tv_sec) * 1000000000 
		+ end->tv_nsec - start->tv_nsec;
}
#endif

void *wait_inthread_for_interrupt(void *arg) 
{
	int ret;
	fd_set fds;
	thread_arg_t *thread_argP=((thread_arg_t *)arg);
	int fd_uio = thread_argP->fd_uio;
	int index = thread_argP->index;
	uint32_t value, diff_time, diff_time2, delta_time;

	struct timespec tp_start, tp_end, tp_end1, tp_end2;

	FD_ZERO(&fds);
	FD_SET(fd_uio, &fds);

	value=1;
	ret = write(fd_uio, &value, sizeof(uint32_t));
	if (ret != sizeof(uint32_t)) {
		printf("unable to enable interrupt %d", fd_uio);
		return;
	}
	while (1)
	{
#ifdef PROFILE_ENABLED
		clock_gettime(CLOCK_MONOTONIC, &tp_start);
#endif

		ret = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
#ifdef EMUCNT_PROFILE
		if (thread_argP->interrupt_occurred_count < MAX_NUM_INTERRUPTS ) {
			emucnt0Log[index][thread_argP->interrupt_occurred_count] = emucnt_ptr[0];
		}
#endif
#ifdef PROFILE_ENABLED
		clock_gettime(CLOCK_MONOTONIC, &tp_end);
#endif
		if (ret == -1) {
			printf("select failed for monitor (error: %s)",
				strerror(errno));
			return;
		}
#ifdef PROFILE_ENABLED
		printf("\n %d: start time  %lx %lx   ns ", fd_uio, tp_start.tv_sec, tp_end.tv_nsec);
		printf("\n %d: end time  %lx %lx  ns ", fd_uio,tp_start.tv_sec, tp_end.tv_nsec);
		delta_time = clock_diff (&tp_start, &tp_end);
		printf("\n %d: select delta time %d ns ", fd_uio,delta_time);
		clock_gettime(CLOCK_MONOTONIC, &tp_end1);
#endif		
		read( fd_uio, &value, sizeof(uint32_t));
#ifdef PROFILE_ENABLED	
		clock_gettime(CLOCK_MONOTONIC, &tp_end2);
		printf("\n %d: value read : %d", fd_uio, value);
		delta_time = clock_diff (&tp_end1, &tp_end2);
		printf("\n %d: read delta time %d ns ", fd_uio,delta_time);fflush(stdout);
#endif
		thread_argP->interrupt_occurred_count += 1;
	}
}

void main(int32_t argc, int8_t **argv)
{
	thread_arg_t thread_arg[MAX_NUM_THREADS];
	int ret, err;
	char class_name[MAX_FILE_NAME_LENGTH];
	char dev_name[MAX_NAME_LENGTH];
	int i, j, num_iterations, options;
	int result;
	int interrupts_received;
	int print_stats_delay=0;
	char c = '0';

#ifdef EMUCNT_PROFILE
	unsigned long long accumulated_delay=0;
	unsigned delay, average_delay, min_delay=0xffffffff, max_delay=0;

	uint32_t *ref_emucnt0_ptr, *ref_emucnt1_ptr;
#endif

#ifdef EMUCNT_PROFILE
	fd_mem = open("/dev/mem", (O_RDWR | O_SYNC));
	if (fd_mem < 0) {
		printf("\n Error opening device ");
		return;
	}
	emucnt_ptr  = (uint32_t *)map_dev_mem(EMUCNT0_ADDR, 8);
	if (emucnt_ptr == NULL)
	{
		printf("\n Error with mmaping emucnt ");
		return;
	}
	/* Write emucnt0 to start counter */
	*(emucnt_ptr) = 1;
	ref_emucnt0_ptr = (uint32_t *)map_dev_mem(REF_EMUCNT0_FROM_INT_SOURCE, 4*MAX_NUM_INTERRUPTS);
	if (ref_emucnt0_ptr == NULL)
	{
		printf("\n Error with mmaping ref emucnt0 address");
		return;
	}

#endif
	if (argc > 1)
		sscanf(argv[1], "%d", &print_stats_delay);


	for (i =0; i < MAX_NUM_THREADS; i++)
	{
		ret = uioutil_get_device(uio_dev_list[i], class_name, 32);
		if ( ret < 0 ) {
			printf("\n Error getting device ");
			return;
		}
		printf("\n Class name %s\n", class_name);

		snprintf(dev_name, MAX_NAME_LENGTH, "/dev/%s", class_name);
		thread_arg[i].fd_uio = open (dev_name, (O_RDWR | O_SYNC));
		if (thread_arg[i].fd_uio < 0) {
			printf("\n Error opening device ");
			goto end_close;
		}
		thread_arg[i].interrupt_occurred_count = 0;
		thread_arg[i].index = i;
	}
	for (i =0; i < MAX_NUM_THREADS; i++)
	{
		/* Create Multiple threads to handle interrupts */
		err = pthread_create(&tid[i], NULL, &wait_inthread_for_interrupt, &thread_arg[i]);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
		else
			printf("\n Thread %d created successfully\n", i);fflush(stdout);
	}

	do
	{
		if(print_stats_delay == 0) {
			printf("When interrupts from DSP are all generated, press any key to continue for interrupt Rx count...\n");
			c = getchar();
		} else {
			sleep(print_stats_delay);
			if(kbhit()) {
				c = getchar();
				printf("\n got char %c", c);
			}
		}
		interrupts_received = 0;
		printf("Interrupt received count per interrupt source:\n");fflush(stdout);
		for(i=0;i<MAX_NUM_THREADS; i++) {
			interrupts_received += thread_arg[i].interrupt_occurred_count;
			printf("%d ",thread_arg[i].interrupt_occurred_count);
		}
		printf("\nTotal Interrupts_received :  %d\n", interrupts_received);fflush(stdout);
#ifdef EMUCNT_PROFILE
		if ( c == 'q' ) {
#ifdef DEBUG
			for(i=0; i< MAX_NUM_THREADS; i++) {
				for(j=0; j< MAX_NUM_INTERRUPTS; j++) {
					printf("0x%x 0x%x\n", emucnt1Log[i][j], emucnt0Log[i][j]);
				}
			}
#endif
			for(j=0; j< MAX_NUM_INTERRUPTS; j++) {
				delay = (emucnt0Log[0][j] - ref_emucnt0_ptr[j]) * 4;
				printf("Ref: 0x%x Int: 0x%x delay %d cycles\n",
					ref_emucnt0_ptr[j], emucnt0Log[0][j],
					delay);
				accumulated_delay += delay;
				min_delay = (delay < min_delay) ? delay : min_delay;
				max_delay = (delay > max_delay) ? delay : max_delay;
			}
			average_delay = accumulated_delay/MAX_NUM_INTERRUPTS;
			printf("\n Cycles: Average delay = %d Min delay = %d Max Delay %d\n",
				average_delay, min_delay, max_delay);
			break;
		}
#endif
	} while(1);
end_close:
#ifdef EMUCNT_PROFILE
	close(fd_mem);
#endif
	for (i =0; i < MAX_NUM_THREADS; i++)
		close(thread_arg[i].fd_uio);
}
