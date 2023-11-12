/**
 * hello_module.c - Linux kernel module example
 */

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int __init hello_init(void)
{
    printk(KERN_ALERT "Hello world\n");
    return 0;    
}


static void __exit hello_exit(void)
{
    printk(KERN_ALERT "Bye!\n");
}

module_init(hello_init);
module_exit(hello_exit);
