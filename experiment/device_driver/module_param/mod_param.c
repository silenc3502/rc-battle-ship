#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Device Driver");
MODULE_DESCRIPTION("Module Parameter Test");

static short int myshort = 1;
static int myint = 420;
static long int mylong = 9999;
static char *mystring = "mykernel";
static int myintarr[2] = {3, 3};
static int arr_argc = 0;

static unsigned int internal = 333;
static unsigned int external;
module_param_named(external, internal, int, 0);

module_param(myshort, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myshort, "A short integer");

module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myint, "An integer");

module_param(mylong, long, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(mylong, "A long integer");

module_param(mystring, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(mystring, "A character string");

module_param_array(myintarr, int, &arr_argc, 0);
MODULE_PARM_DESC(myintarr, "An array of integers");

static int __init hello_param_init(void)
{
	int i;
	printk("Hello, world \n");
	printk("myshort short integer: %hd\n", myshort);
	printk("myint is an integer: %d\n", myint);
	printk("mylong long integer: %ld\n", mylong);
	printk("myshort string is a string: %s\n", mystring);
	printk("external: %d\n", external);
	printk("internal: %d\n", internal);

	for(i = 0; i < sizeof(myintarr) / sizeof(int); i++)
		printk("myintarr[%d] = %d\n", i, myintarr[i]);

	printk("got %d arguments for myintarr\n", arr_argc);
	return 0;
}

static void __exit hello_param_exit(void)
{
	printk("GoodBye, world \n");
}

module_init(hello_param_init);
module_exit(hello_param_exit);
