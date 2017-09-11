#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#define MAX_NAME_LENGTH		64
#define MAX_FILE_NAME_LENGTH	128
#define MAX_LINE_LENGTH         16
#define IPC_REGISTER_ADDR 0x02620260
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

void main(int32_t argc, int8_t **argv)
{
	int size; 
	char *map ;
	uint32_t *map_addr;
	uint32_t addr;
	uint32_t base, min_length, adjusted_length, offset;
	int ret, i, num_iterations;

	uint32_t value, diff_time, diff_time2, delta_time;
	uint32_t page_size = getpagesize();
	struct timespec tp_start, tp_end, tp_end1, tp_end2;
	int dev_mem_fd;

	/* Read parameters from command line */
   	if(argc < 2)
	{
		printf("Usage: %s <num of iterations> \
			\n",argv[0]);
    		exit(-1);
 	}
	sscanf (argv[1], "%d", &num_iterations);

	dev_mem_fd = open("/dev/mem", (O_RDWR | O_SYNC));
	if (dev_mem_fd < 0) {
		printf("\n Error opening device ");
		return;
	}

	addr = IPC_REGISTER_ADDR;
	size = 0x4;
	base = addr  & (~(page_size-1));
	offset = (addr -base);
	min_length = offset+size;
	adjusted_length = min_length & (~(page_size-1));
	adjusted_length = (adjusted_length == min_length) ? min_length : adjusted_length + page_size;

	map = mmap(NULL, adjusted_length, PROT_READ|PROT_WRITE, MAP_SHARED,
                                            dev_mem_fd, base);

	if (map == MAP_FAILED) {
		printf("\nmmap failed for addr 0x%x size %d (err: %s)",
		addr, size, strerror(errno));
		goto end_close;
		
	}
	map_addr = (uint32_t *)(map + offset);
	
	for(i=0; i< num_iterations; i++) {
		clock_gettime(CLOCK_MONOTONIC, &tp_start);
		*(map_addr) = 0x100001;
		clock_gettime(CLOCK_MONOTONIC, &tp_end);
		printf("\n starttime  %lx %lx  ns ",tp_start.tv_sec, tp_start.tv_nsec);
		printf("\n endtime  %lx %lx   ns ",tp_end.tv_sec, tp_end.tv_nsec);
     	  	diff_time = clock_diff (&tp_start, &tp_end);
		printf("\n diff time  %d ns ",diff_time);
//		sleep(1);
	}
end_close:
        close(dev_mem_fd);
}
