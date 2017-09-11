#include <linux/init.h>
#include <linux/module.h> /** needed by all modules **/
#include <linux/kernel.h>  /** This is for KERN_ALERT **/
#include <linux/fs.h> /** for file operations **/
#include <linux/cdev.h>  /** character device **/
#include <linux/device.h>  /** for sys device registration in /dev/ and /sys/class **/
/** for copy_to_user **/
#include <asm/uaccess.h>

/** For class registration to work, you need GPL license **/
MODULE_LICENSE("GPL");

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

#define VIDDRVNAME  "VidIoc"

#define VIDEODRV   "/dev/" VIDDRVNAME

#define AV_VID_DRIVER_TYPE               'V'

/** The combination is made keeping in mind the unique IOC CMD otherwise other device may do something **/
#define AV_VID_IOC_CMD       _IOW( AV_VID_DRIVER_TYPE, 1, VID_OBJ_CMD_MSG )

static struct cdev basicCdev;
static struct class *basicDriverClass;
static int  basicMajorNumber = 0;

#define NUMBER_OF_MINOR_DEVICE (0)

/** Prototype for read, this will be invoked when the read function is done on to the driver **/
/** The declaration type is file operations based function pointer - read **/

/** File Operations function pointer table **/
/** There are plenty of file operations  **/

static long av_vid_ioctl(struct file *filp, unsigned int uiVidCmd, unsigned long arg);

/** Below is the old way, in the linux  kernel 2.6.x , the inode is removed now **/
//static int av_vid_ioctl( struct inode  *inode, struct file   *filp, unsigned int  uiVidCmd, unsigned long arg );

/**
 * Explanation: When ioctl was executed, it took the Big Kernel Lock (BKL), so nothing
 * else could execute at the same time. This is very bad on a multiprocessor machine,
 * so there was a big effort to get rid of the BKL. First, unlocked_ioctl was introduced.
 * It lets each driver writer choose what lock to use instead. This can be difficult,
 * so there was a period of transition during which old drivers still worked (using ioctl)
 * but new drivers could use the improved interface (unlocked_ioctl). Eventually all
 * drivers were converted and ioctl could be removed.
 * compat_ioctl is actually unrelated, even though it was added at the same time.
 * Its purpose is to allow 32-bit userland programs to make ioctl calls on a 64-bit kernel.
 * The meaning of the last argument to ioctl depends on the driver, so there is no way to
 * do a driver-independent conversion.
 */

static struct file_operations fops = {
	/** In newer kernel ioctl has been changed **/
	//.ioctl = av_vid_ioctl,
	.unlocked_ioctl = av_vid_ioctl,
};

int av_vid_CmdAction(VID_OBJ_CMD_MSG *pstVidObjCmdMsg)
{
	printk("Command Action Called\r\n");
	if(!strcmp(pstVidObjCmdMsg->msg,"DeviceStart"))
	{
		printk("Start Device now\r\n");
		strcpy(pstVidObjCmdMsg->result,"DeviceStartedNow");
		return 0;

	}

	else if(!strcmp(pstVidObjCmdMsg->msg,"DeviceStop"))
	{
		printk("Start Device now\r\n");
		strcpy(pstVidObjCmdMsg->result,"DeviceStoppedNow");
		return 0;

	}

	return 1;
}

static long av_vid_ioctl(struct file *filp, unsigned int uiVidCmd, unsigned long arg)
{
	int         rc    = 0;
	void __user *argp = (void __user *)arg;
	VID_OBJ_CMD_MSG stVidObjCmdMsg;

	printk("Ioctl is called\r\n");

	switch( uiVidCmd )
	{
		case AV_VID_IOC_CMD:

			if( copy_from_user( &stVidObjCmdMsg,
						argp,
						sizeof( stVidObjCmdMsg ) ) )
			{
				printk("The copy from user failed\r\n");
				rc = -EFAULT;
			}
			else if( av_vid_CmdAction( &stVidObjCmdMsg ) != 0 )
			{
				printk("av_vid_CmdAction failed\r\n");
				rc = -EFAULT;
			}
			else if( copy_to_user( argp,
						&stVidObjCmdMsg,
						sizeof( stVidObjCmdMsg ) ) )
			{
				printk("copy_to_user failed\r\n");
				rc = -EFAULT;
			}
			break;
	}

	printk("return from Ioctl\r\n");
	return rc;
}

static void setup_cdev(struct cdev *dev, int minor, struct file_operations *fops)
{
	int err = -1;
	/** MKDEV call creates a device number i.e. combination of major and minor number **/
	int devno = MKDEV(basicMajorNumber, minor);
	/** Initiliaze character dev with fops **/
	cdev_init(dev, fops);
	/**owner and operations initialized **/
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	/** add the character device to the system**/
	/** Here 1 means only 1 minor number, you can give 2 for 2 minor device, the last param is the count of minor number enrolled **/
	err = cdev_add (dev, devno, 1);

	if (err)
	{
		printk (KERN_NOTICE "Couldn't add cdev");
	}
}

static int chrDriverInit(void)
{

	int result;
	dev_t dev;

	printk("Welcome!! Device Init now..\r\n");

	/** int alloc_chrdev_region(dev_t *dev, unsigned int firstminor,unsigned int count, char *name);  **/
	/** dev -> The dev_t variable type,which will get the major number that the kernel allocates.  **/
	/**The same name will appear in /proc/devices.  **/

	/** it is registering the character device **/
	/** a major number will be dynamically allocated here **/
	/**  alloc_chrdev_region(&dev_num, FIRST_MINOR, COUNT, DEVICE_NAME); **/
	result = alloc_chrdev_region(&dev, 0, NUMBER_OF_MINOR_DEVICE, VIDDRVNAME);

	if( result < 0 )
	{
		printk("Error in allocating device");
		return -1;
	}

	/** From these two if's we are avoiding the manual mknod command to create the /dev/<driver> **/
	/**  creating class, and then device created removes the dependency of calling mknod  **/
	/** A good method - the mknod way is depreciated **/
	/** mknod way is -  mknod /dev/<driver_name> c <majorNumber> <minorNumber> */

	/** add the driver to /-sys/class/chardrv */
	if ((basicDriverClass = class_create(THIS_MODULE, "chardrv")) == NULL)    //$ls /sys/class
	{
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	/** add the driver to /dev/pSeudoDrv -- here **/
	if (device_create(basicDriverClass, NULL, dev, NULL, VIDDRVNAME) == NULL) //$ls /dev/
	{
		class_destroy(basicDriverClass);
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	/** let's see what major number was assigned by the Kernel **/
	basicMajorNumber = MAJOR(dev);
	printk("Kernel assigned major number is %d ..\r\n",basicMajorNumber );

	/** Now setup the cdev **/
	setup_cdev(&basicCdev,NUMBER_OF_MINOR_DEVICE, &fops);

	return 0;

}

static void chrDriverExit(void)
{
	/** stop the running thread **/

	//kthread_stop(basicCharThread);
	/** A reverse - destroy mechansim -- the way it was created **/
	printk("Releasing Simple Devs -- %s\r\n",  __FUNCTION__);
	/** delete the character driver added **/
	cdev_del(&basicCdev);
	/** destroy the device created **/
	device_destroy(basicDriverClass, MKDEV(basicMajorNumber, 0));
	/** destroy the class created **/
	class_destroy(basicDriverClass);
	/** unregister the chr dev **/
	unregister_chrdev(basicMajorNumber, NUMBER_OF_MINOR_DEVICE);

}

module_init(chrDriverInit);
module_exit(chrDriverExit);
