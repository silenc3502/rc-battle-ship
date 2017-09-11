#include <linux/init.h>
#include <linux/module.h> /** needed by all modules **/
#include <linux/kernel.h>  /** This is for KERN_ALERT **/
#include <linux/fs.h> /** for file operations **/
#include <linux/cdev.h>  /** character device **/
#include <linux/device.h>  /** for sys device registration in /dev/ and /sys/class **/
/** for copy_to_user **/
#include <asm/uaccess.h>
/** for netFilter - nf_hook_ops **/
#include <linux/netfilter.h>
/** Priority enum **/
#include <linux/netfilter_ipv4.h>
/** to access ip header here **/
#include <linux/ip.h>
/** to access the interface name - netdevice **/
#include <linux/inetdevice.h>
/** for string functions - strcpy here **/
#include <linux/string.h>
/** for ntohs **/
#include <linux/socket.h>
#include <linux/tcp.h>

/** For class registration to work, you need GPL license **/
MODULE_LICENSE("GPL");

static int  basicMajorNumber = 0;
static struct cdev basicCdev;

static long nfFilterIoctl(struct file *filp, unsigned int nfFilterCmd, unsigned long arg);

static struct file_operations fops = {
	/** In newer kernel ioctl has been changed **/
	//.ioctl = nfFilterIoctl,
	.unlocked_ioctl = nfFilterIoctl,
};

/** netfilter hook file operations for input packets**/
struct nf_hook_ops inputFilter;

unsigned int Input_khook_handler ( unsigned int hooknum, struct sk_buff *skb, const struct net_device *in,
		const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *ip = NULL;
	struct tcphdr *tcp = NULL;

	char src_addr[16], dst_addr[16], iface[16];
	unsigned int src_port = 0, dst_port = 0;

	ip  = (struct iphdr *)skb_network_header(skb);
	/** when the packet is coming in - the packet may have not traversed to the l4 **/
	/** Thus, we should skip 20 bytes to access the transport layer **/
	tcp = (struct tcphdr *)(skb_transport_header(skb)+20);
	// Get src & dst port addresses as received from packets
	src_port = (unsigned int)ntohs(tcp->source);
	dst_port = (unsigned int)ntohs(tcp->dest);
	/** Get interface name **/
	strcpy(iface, in->name);
	// Get src & dst ip addresses as received from packets
	snprintf(src_addr, 16, "%pI4", &ip->saddr);
	snprintf(dst_addr, 16, "%pI4", &ip->daddr);

	printk("The value of ip src is %s\r\n",src_addr );
	printk("The value of ip des is %s\r\n",dst_addr );
	printk("The value of src port is %d\r\n",src_port );
	printk("The value of dest port is %d\r\n",dst_port );
	printk("The value of interface is %s\r\n",iface );
	/** for tcp - the protocol number is 6 **/
	/** for udp - the protocol number is 17 **/
	/** for IGMP - the protocol number is 2 **/
	printk("The value of protocol is %d\r\n",ip->protocol );
	/** Drop UDP packets **/
	if( ip->protocol == 17 )
	{
		return NF_DROP;
	}

	return NF_ACCEPT;
}

static long nfFilterIoctl(struct file *filp, unsigned int nfFilterCmd, unsigned long arg)
{
	int ret = 0;

	return ret;
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

static int nhookFilter_init(void)
{
	int result;
	dev_t dev;
	printk("Hello netfilter !!! Welcome!!\r\n");

	result = alloc_chrdev_region(&dev, 0, 0, "BasicNetFilter");

	if( result < 0 )
	{
		printk("Error in allocating device");
		return -1;
	}

	/** let's see what major number was assigned by the Kernel **/
	basicMajorNumber = MAJOR(dev);
	printk("Kernel assigned major number is %d ..\r\n",basicMajorNumber );

	/** Now setup the cdev **/
	setup_cdev(&basicCdev,0, &fops);

	/** setup Input net filter **/
	inputFilter.list.next = NULL;
	inputFilter.list.prev = NULL;
	inputFilter.hook = Input_khook_handler;
	inputFilter.pf = PF_INET; /* IPv4 */
	inputFilter.hooknum = NF_INET_LOCAL_IN;
	inputFilter.priority = NF_IP_PRI_FIRST;

	/* hooks registration - Finally*/
	result = nf_register_hook(&inputFilter);

	if( result != 0 )
	{
		printk("Hook registration failed\r\n");
	}

	return 0;

}

static void nhookFilter_exit(void)
{
	/** A reverse - destroy mechansim -- the way it was created **/
	printk("Releasing Simple Devs -- %s\r\n",  __FUNCTION__);
	/** delete the character driver added **/
	cdev_del(&basicCdev);
	/** unregister the chr dev **/
	unregister_chrdev(basicMajorNumber, 0);
	printk("unregister the Input hook now");
	nf_unregister_hook(&inputFilter);
}

module_init(nhookFilter_init);
module_exit(nhookFilter_exit);
