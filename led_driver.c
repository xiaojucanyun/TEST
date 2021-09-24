#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/sizes.h>
#include <asm/uaccess.h>

#include <asm/arch/S3C2410.h>
#include <asm/arch/irqs.h>
#include "led.h"

#define NRDEV 1
#define DEVNAME "myleds"

#define GPBCON 0x56000010
#define GPBDAT 0x56000014
#define rGPBCON *(volatile unsigned int *)gpbcon
#define rGPBDAT *(volatile unsigned int *)gpbdat

//设备结构，存放设备相关的信息
struct leddev{
	struct cdev cdev;
};

int *gpbcon,*gpbdat;
int major,minor;
dev_t devno;
struct leddev *leddev;
struct class *class;
char *devName[NRDEV] = {"myleds"};

int leddev_open(struct inode *inode,struct file *filp)
{
	struct leddev *leddev;
	leddev = container_of(inode->i_cdev,struct leddev,cdev);
	filp->private_data = leddev;

	/**
	 * reg init
	 */
	request_mem_region(GPBCON,4,DEVNAME);
	request_mem_region(GPBDAT,4,DEVNAME);
	gpbcon = ioremap(GPBCON,4);
	gpbdat = ioremap(GPBDAT,4);

	printk(KERN_INFO"ledev_open is invoked.\n");
	return 0;
}
int leddev_release(struct inode *inode,struct file *filp)
{
	iounmap(gpbcon);
	iounmap(gpbdat);
	release_mem_region(GPBCON,4);
	release_mem_region(GPBDAT,4);

	printk(KERN_INFO"leddev_release is invoked.\n");
	return 0;
}

static int getled(int ledno)
{
	return LEDON;	
}

ssize_t leddev_read(struct file *filp,char __user *buf,size_t len,loff_t *offset)
{
	//printk(KERN_INFO"leddev_read is invoked.\n");
	struct leddev *leddev = filp->private_data;

	struct ledinfo ledinfo;
	int stat;
	if(copy_from_user(&ledinfo,buf,sizeof(struct ledinfo))){
		printk(KERN_INFO"copy_from_user failed.\n");
		return -ENOMEM;
	}

	stat = getled(ledinfo.ledno);
	ledinfo.ledstat = stat;

	if(copy_to_user(buf,&ledinfo,sizeof(struct ledinfo))){
		printk(KERN_INFO"copy_to_user failed.\n");
		return -ENOMEM;
	}
	return 0;
}

static int setled(int ledno,int ledstat)
{
	//set gpbcon5-8 as output
	//rGPBCON &= ~(0xFF << 10);
	//rGPBCON |= (0x55<<10);
	iowrite32((ioread32(gpbcon) & ~(0xFF<<10)) | (0x55<<10),gpbcon);
	//set gpbdat
	switch(ledno)
	{
	case LED0:
		switch(ledstat)
		{
		case LEDON:
			//rGPBDAT &= ~(0x1<<5);
			iowrite32(ioread32(gpbdat) & ~(0x1<<5),gpbdat);
			break;
		case LEDOFF:
			//rGPBDAT |= (0x1<<5);
			iowrite32(ioread32(gpbdat) | (0x1<<5),gpbdat);
			break;
		default:
			return -EINVAL;
		}
		break;
	case LED1:
		switch(ledstat)
		{
		case LEDON:
			//rGPBDAT &= ~(0x1<<6);
			iowrite32(ioread32(gpbdat) & ~(0x1<<6),gpbdat);
			break;
		case LEDOFF:
			//rGPBDAT |= (0x1<<6);
			iowrite32(ioread32(gpbdat) | (0x1<<6),gpbdat);
			break;
		default:
			return -EINVAL;
		}
		break;
	case LED2:
		switch(ledstat)
		{
		case LEDON:
			//rGPBDAT &= ~(0x1<<7);
			iowrite32(ioread32(gpbdat) & ~(0x1<<7),gpbdat);
			break;
		case LEDOFF:
			//rGPBDAT |= (0x1<<7);
			iowrite32(ioread32(gpbdat) | (0x1<<7),gpbdat);
			break;
		default:
			return -EINVAL;
		}
		break;
	case LED3:
		switch(ledstat)
		{
		case LEDON:
			//rGPBDAT &= ~(0x1<<8);
			iowrite32(ioread32(gpbdat) & ~(0x1<<8),gpbdat);
			break;
		case LEDOFF:
			//rGPBDAT |= (0x1<<8);
			iowrite32(ioread32(gpbdat) | (0x1<<8),gpbdat);
			break;
		default:
			return -EINVAL;
		}
		break;
	case LEDALL:
		switch(ledstat)
		{
		case LEDON:
			//rGPBDAT &= ~(0xF<<5);
			iowrite32(ioread32(gpbdat) & ~(0xF<<5),gpbdat);
			break;
		case LEDOFF:
			//rGPBDAT |= (0xF<<5);
			iowrite32(ioread32(gpbdat) | (0xF<<5),gpbdat);
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
ssize_t leddev_write(struct file *filp,const char __user *buf,size_t len,loff_t *offset)
{
	//printk(KERN_INFO"leddev_write is invoked.\n");
	struct leddev *leddev = filp->private_data;
	struct ledinfo ledinfo;
	if(copy_from_user(&ledinfo,buf,sizeof(struct ledinfo))){
		printk(KERN_INFO"copy_from_user failed.\n");
		return -ENOMEM;
	}

	return setled(ledinfo.ledno,ledinfo.ledstat);
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = leddev_open,
	.read = leddev_read,
	.write = leddev_write,
	.release = leddev_release,
};

static int __init leddev_init(void)
{
	int result,i;
	//1.register devno
	if(major){
		devno = MKDEV(major,minor);
		result = register_chrdev_region(devno,NRDEV,DEVNAME);	
	}else{
		result = alloc_chrdev_region(&devno,minor,NRDEV,DEVNAME);
		major = MAJOR(devno);
	}
	if(result<0){
		printk(KERN_INFO"can't get major: %d\n",major);
		return result;
	}
	//2.create cdev
	leddev = kmalloc(sizeof(struct leddev) * NRDEV,GFP_KERNEL);
	if(leddev==NULL){
		printk(KERN_INFO"kmalloc failed.\n");
		result = -ENOMEM;
		goto err1;
	}
	for(i=0;i<NRDEV;i++){
		cdev_init(&leddev[i].cdev,&fops);
		leddev[i].cdev.owner = THIS_MODULE;
		leddev[i].cdev.ops = &fops;
	}
	//3.将cdev和devno关联，并且添加到内核的驱动列表
	for(i=0;i<NRDEV;i++){
		result = cdev_add(&leddev[i].cdev,MKDEV(major,minor+i),1);
		if(result){
			printk(KERN_INFO"cdev_add failed");
			goto err2;
		}
	}
	//4.自动创建设备文件
	class = class_create(THIS_MODULE,DEVNAME);
	for(i=0;i<NRDEV;i++)
		device_create(class,NULL,MKDEV(major,minor+i),NULL,devName[i]);

	printk(KERN_INFO"module init success.\n");
	return 0;
err2:
	kfree(leddev);
err1:
	unregister_chrdev_region(devno,NRDEV);
	return result;
}

static void __exit leddev_exit(void)
{
	int i;
	//1.将cdev从内核驱动列表移除
	for(i=0;i<NRDEV;i++){
		cdev_del(&leddev[i].cdev);
	}
	//2.释放设备结构
	kfree(leddev);
	//3.注销设备号
	unregister_chrdev_region(devno,NRDEV);
	//4.自动删除设备文件
	for(i=0;i<NRDEV;i++)
		device_destroy(class,MKDEV(major,minor+i));
	class_destroy(class);

	printk(KERN_INFO"module exit success.\n");
}

module_init(leddev_init);
module_exit(leddev_exit);

MODULE_AUTHOR("SRAM");
MODULE_DESCRIPTION("LED Char Device.");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
