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
//#include "uio_helper.h"
#include "uio_module_drv.h"

#define MAX_NAME_LENGTH		64
#define MAX_FILE_NAME_LENGTH	128
#define MAX_LINE_LENGTH         16

#define HEXA_DECIMAL_BASE 16
static int dev_mem_fd;

#define DEBUG

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

int uio_get_mem_section_by_id(char *class_name, int section_id, int mmap_fd,
	unsigned int *phys_base_p, unsigned int *size_p)
{
	int fd;
	char hexstring[MAX_NAME_LENGTH];
	char filename[MAX_NAME_LENGTH];
	int ret=0;
	uint32_t page_size = getpagesize();

	sprintf(filename, "/sys/class/misc/%s/maps/map%d/addr", class_name, section_id);

	fd = open(filename, O_RDONLY);
	if (fd >= 0) {
		ret = read(fd, hexstring, MAX_NAME_LENGTH);
		if ( ret == 0) {
			/* Read failed */
			ret = -1;
			goto err_exit;
		}
		*phys_base_p = strtoul(hexstring, NULL, HEXA_DECIMAL_BASE);
	} else
		return -1;
	close(fd);

	sprintf(filename, "/sys/class/misc/%s/maps/map%d/size", class_name, section_id);
	fd = open(filename, O_RDONLY);
	if (fd >= 0) {
		ret = read(fd, hexstring, MAX_NAME_LENGTH);
		if ( ret == 0) {
			/* Read failed */
			ret = -1;
			goto err_exit;
		}

		*size_p = strtoul(hexstring, NULL, HEXA_DECIMAL_BASE);
	} else
		return -1;
err_exit:
	close(fd);
	return ret;
}

int uio_get_mem_section_by_name(char *class_name, char *map_name,  int mmap_fd,
	unsigned int *index, unsigned int *phys_base_p, unsigned int *size_p)
{
	int fd;
	char hexstring[MAX_NAME_LENGTH];
	char filename[MAX_NAME_LENGTH];
	int ret=0;

	*index = 0;
	do {
		sprintf(filename, "/sys/class/misc/%s/maps/map%d/name", class_name, *index);

		fd = open(filename, O_RDONLY);
		if (fd >= 0) {
		ret = read(fd, hexstring, MAX_NAME_LENGTH);
		if ( ret == 0) {
			ret = -1;
			/* Read failed */
			goto err_exit;
		}
		hexstring[ret-1] = '\0';
	} else {
		return -1;
	}

	close(fd);
	(*index)++;
	} while(strcmp(map_name, hexstring));
	(*index)--;

	ret =  uio_get_mem_section_by_id(class_name, *index, mmap_fd, phys_base_p, size_p);
	if (ret < 0) {
		ret = -1;
		goto err_exit1;
	}
#ifdef DEBUG
	printf("\nDEBUG: Got Memory section %s:%s physbase 0x%x,size 0x%x\n",
		class_name, map_name, *phys_base_p, *size_p);
#endif
	return 0;
err_exit:
	close(fd);
err_exit1:
	return ret;
}

void main(int32_t argc, int8_t **argv)
{
	int  fd_map;
	int ret;
	char class_name[MAX_NAME_LENGTH];
	char dev_name[MAX_NAME_LENGTH];
	fd_set fds;	
	uint32_t value, diff_time, diff_time2, delta_time;
	struct timespec tp_start, tp_end, tp_end1, tp_end2;
	int i, num_iterations, options;
	char temp[16];
	int command;
	uint32_t *virt_p;
	void *virt_page_p;
	char mem_name[16];
	uint32_t offset, size, write_value;
	uint32_t pg_offset, phys_base_address, base_correction;
	uint32_t mmap_length, mem_size;
	int ret_value;
	int page_size = getpagesize();
	int index;
	
	/* Read parameters from command line */
	if(argc < 6)
	{
		printf("Usage: %s <uio_device_name> <read|write> <mem-name> <offset> <size> <write_value>\n",
			argv[0]);
		exit(-1);
	}

	sscanf (argv[1], "%s", class_name);
	sscanf (argv[2], "%s", temp);
	sscanf (argv[3], "%s", mem_name);
	if(!strcmp(temp, "read")) {
		command = 1;
	} else if (!strcmp(temp,"write")) {
		command = 2;
	} else {
		printf("\n invalid operation \n");
		return;	
	}
	sscanf (argv[4], "%x", &offset);
	sscanf (argv[5], "%x", &size);

	if (command == 2) {
 		if( argc < 7) { 
			printf("\n Write value not specified \n");
			return;
		}
		sscanf (argv[6], "%x", &write_value);
	}

	snprintf(dev_name, MAX_NAME_LENGTH, "/dev/%s", class_name);
#ifdef DEBUG
	printf("\nDEBUG: Parameters: class_name: %s, command: %s, mem_name: %s\n",
		class_name, temp, mem_name);fflush(stdout);

	printf("DEBUG:              offset 0x%x, size 0x%x, write_value 0x%x\n",
		offset, size, write_value);fflush(stdout);
	printf("DEBUG: dev_name: %s \n", dev_name);

#endif
	fd_map = open (dev_name, (O_RDWR | O_SYNC));
	if (fd_map < 0) {
		printf("\n Error opening device \n");
		return;
	}

	ret_value = uio_get_mem_section_by_name(class_name, mem_name, fd_map, &index,
		&phys_base_address, &mem_size);
	if(ret_value < 0) {
		printf("\n Error : Unable to get index from name %s\n", mem_name);
		goto end_close;
	}
	base_correction = (phys_base_address & (page_size - 1));
	pg_offset = offset & (~((page_size<< UIO_MODULE_DRV_MAP_OFFSET_SHIFT) - 1));
	mmap_length = size + offset - pg_offset + base_correction;

	virt_page_p = mmap(0, mmap_length, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd_map, (index * page_size + pg_offset));
	virt_p = virt_page_p + (offset - pg_offset) + base_correction;


	if ((uint32_t)virt_p == (uint32_t)-1)
	{
		printf("mmap failed!\n");
		goto end_close;
	}

	/* Do read or write operation */
	switch(command) {
	case 1:
		for ( i =0 ; i < size/4; i++ ) {
			printf("Read value offset %d, 0x%x\n",i*4, virt_p[i]);
		}
		break;
	case 2:
		for ( i =0 ; i < size/4; i++ ) {
			virt_p[i] = write_value;
			printf("Read value after write offset %d, 0x%x\n",i*4, virt_p[i]);
		}
		break;

	default:
		printf("\n Unexpected operation \n");
		break;
	}

	munmap(virt_page_p, mmap_length);	
	
end_close:
	close(fd_map);
}
