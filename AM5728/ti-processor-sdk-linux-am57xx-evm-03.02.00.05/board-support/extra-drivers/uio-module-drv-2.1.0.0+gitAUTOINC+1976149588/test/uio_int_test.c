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

#if 1
#define MAX_NAME_LENGTH		64
#define MAX_FILE_NAME_LENGTH	128
#define MAX_LINE_LENGTH         16
static int dev_mem_fd;

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
//                name_ptr = remove_prefix_from_device_name(name, MAX_NAME_LENGTH);
//		printf("\n uio_name %s, name_ptr %s,length %d", uio_name , name_ptr,strlen(uio_name)  );
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
#else
struct uio_info_t *uio_info_p;
#endif
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
	int  fd_uio;
	int ret;
	char class_name[MAX_FILE_NAME_LENGTH];
#ifdef UIO_HELPER
	char *dev_name;
#else
	char dev_name[MAX_NAME_LENGTH];
#endif
	fd_set fds;	
	uint32_t value, diff_time, diff_time2, delta_time;
	struct timespec tp_start, tp_end, tp_end1, tp_end2;
	int i, num_iterations, options;

	/* Read parameters from command line */
   	if(argc < 3)
	{
		printf("Usage: %s <uio_device_name> <Number of iterations> [1: pause and step> \
			\n",argv[0]);
    		exit(-1);
 	}
        
#if 1
	ret = uioutil_get_device(argv[1], class_name, 32);
	if ( ret < 0 ) {
		printf("\n Error getting device ");
		return;
	}
	printf("\n Class name %s\n", class_name);
	snprintf(dev_name, MAX_NAME_LENGTH, "/dev/%s", class_name);
#else
	uio_info_p = uio_find_devices_byname("hyperlink0");
	dev_name = uio_info_p->name;
	printf("\n Device name %s",dev_name);
	getchar();
#endif
	sscanf (argv[2], "%d", &num_iterations);
	if (argc > 3) 
		options = atoi(argv[3]);
	else
		options =0;
	fd_uio = open (dev_name, (O_RDWR | O_SYNC));
	if (fd_uio < 0) {
		printf("\n Error opening device ");
		goto end_close;
	}
	FD_ZERO(&fds);
	FD_SET(fd_uio, &fds);

	value=1;
	ret = write(fd_uio, &value, sizeof(uint32_t));
	if (ret != sizeof(uint32_t)) {
		printf("unable to enable interrupt ");
		goto end_close;
	}
	for(i=0; i< num_iterations; i++) {
		if (options != 0)
		{
			printf("\n Press any key to wait for interrupt...");
 			getchar();
		}
		printf("\n waiting for interrupt...");fflush(stdout);
		clock_gettime(CLOCK_MONOTONIC, &tp_end1);
		ret = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
		clock_gettime(CLOCK_MONOTONIC, &tp_end2);
		if (ret == -1) {
			printf("select failed for monitor (error: %s)",
				strerror(errno));
			goto end_close;
		}
		printf("\n endtime1  %lx %lx   ns ",tp_end1.tv_sec, tp_end1.tv_nsec);
		printf("\n endtime2  %lx %lx  ns ",tp_end2.tv_sec, tp_end2.tv_nsec);
      		delta_time = clock_diff (&tp_end1, &tp_end2);
		printf("\n select delta time %d ns ",delta_time);
		if (options != 0)
		{
			printf("\n Press any key to call read...");
 			getchar();
		}
		clock_gettime(CLOCK_MONOTONIC, &tp_end1);
		read( fd_uio, &value, sizeof(uint32_t));
		clock_gettime(CLOCK_MONOTONIC, &tp_end2);
		printf("\n value read : %d", value);
      		delta_time = clock_diff (&tp_end1, &tp_end2);
		printf("\n read delta time %d ns ",delta_time);

	}
	/* Disable interrupt */
	value=0;
	ret = write(fd_uio, &value, sizeof(uint32_t));
	if (ret != sizeof(uint32_t)) {
		printf("unable to disable interrupt ");
		goto end_close;
	}

end_close:
	close(fd_uio);
}
