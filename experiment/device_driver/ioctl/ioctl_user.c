#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <linux/unistd.h>
#include <string.h>

/** For IOCTLS **/
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#define AV_VID_DRIVER_TYPE               'V'
typedef struct tag_VID_OBJ_CMD_MSG VID_OBJ_CMD_MSG;

typedef enum Cmdmsg
{
	DeviceStart,
	DeviceStop

}Cmdmsg;

struct tag_VID_OBJ_CMD_MSG
{
	unsigned int ui32StillBufferSize;
	char         msg[128];
	char         result[128];
	Cmdmsg       commandMsg;

};

/** The combination is made keeping in mind the unique IOC CMD otherwise other device may do something **/
#define AV_VID_IOC_CMD       _IOW( AV_VID_DRIVER_TYPE, 1, VID_OBJ_CMD_MSG )

#define VIDDRVNAME  "VidIoc"

#define VIDEODRV   "/dev/" VIDDRVNAME

int main()
{
	int ret = 0;
	int fd = -1;
	VID_OBJ_CMD_MSG vidMsg;
	memset(vidMsg.msg,0,sizeof(vidMsg.msg));
	strncpy(vidMsg.msg,"DeviceStart",strlen("DeviceStart"));
	vidMsg.ui32StillBufferSize = strlen(vidMsg.msg);
	vidMsg.commandMsg = DeviceStart;

	if( (fd = open(VIDEODRV,O_RDONLY)) <=0 )
	{
		printf("Device Open failed!!\r\n");
		return -1;
	}

	printf("Calling IOCTL ..DeviceStart\r\n");
	ret = ioctl(fd,AV_VID_IOC_CMD,&vidMsg);
	if( ret != 0)
	{
		printf("Error in IOCTL ..DeviceStart command\r\n");

	}

	printf("The result is the following == %s ==\r\n",vidMsg.result);

	memset(vidMsg.msg,0,sizeof(vidMsg.msg));
	strncpy(vidMsg.msg,"DeviceStop",strlen("DeviceStop"));
	vidMsg.ui32StillBufferSize = strlen(vidMsg.msg);
	vidMsg.commandMsg = DeviceStop;

	printf("Calling IOCTL ..DeviceStop\r\n");
	ret = ioctl(fd,AV_VID_IOC_CMD,&vidMsg);
	if( ret != 0)
	{
		printf("Error in IOCTL ..DeviceStop..command\r\n");

	}

	printf("The result is the following == %s ==\r\n",vidMsg.result);

	while(1)
		pause();
	return 0;
}
