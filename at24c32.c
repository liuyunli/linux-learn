/*
 * already add read write and ioctl function.
 * need write application code for test .
 * 23:37 March 1 2017
*/



#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include "at24c32.h"
//#include <asm/uaccess.h>  //copy_to_user ...

//static int major;
//static int minor;

#define MEM_CLEAR 0x00
#define MEM_CAT 0x01

#define EEPROM_SIZE 32768

//struct cdev *at24c32_dev;    //cdev 数据结构
struct AT24C32_DEV *at24c32_dev;
static dev_t devno;           //设备编号
static struct class * at24c32_class;
static char dev_rw_buff[4096];  //设备内部读写缓冲区


#define DEVICE_NAME "at24c32"

static int at24c32_open(struct inode *inode,struct file *file)
{
  try_module_get(THIS_MODULE);
  printk(KERN_INFO DEVICE_NAME" opened!\n");
  return 0;
}

static int at24c32_release(struct inode *inode,struct file *file)
{
  printk(KERN_INFO DEVICE_NAME" closed!\n");
  module_put(THIS_MODULE);
  return 0;
}

static int at24c32_ioctl(struct inode *inodep, struct file *filep,
unsigned int cmd, unsigned long arg)
{
  size_t i;
  switch (cmd) {
    case MEM_CLEAR:
      for (i = 0; i < sizeof(dev_rw_buff); i++) {
        dev_rw_buff[i]=0;
      }
      break;
    case MEM_CAT:
      for (i = 0; i < sizeof(dev_rw_buff); i++) {
        printk("%c",dev_rw_buff[i]);
      }
      break;
    default:
      return -EINVAL;
  }
  return 0;
}

static ssize_t at24c32_read(struct file *filp, const char __user *buf,
                            size_t size, loff_t *ppos)
{
  unsigned long pos = *ppos;
  unsigned int count = size;
  char *tmp = NULL;
  ssize_t ret = 0,i;

  if (NULL == buf) {
    return EFAULT;
  }
  if (pos >= EEPROM_SIZE) {
    return count ? -ENXIO : 0;
  }
  if (count > EEPROM_SIZE - pos)
    count = EEPROM_SIZE - pos;
  //at24c32_dev = kmalloc(sizeof(struct AT24C32_DEV),GFP_KERNEL);
  tmp = kmalloc(count,/*GFP_KERNEL*/GFP_ATOMIC);
  if (NULL == tmp) {
    return ENOMEM;
  }
  spin_lock(&at24c32_dev->spinlock);
  //i2c_master_rev();
  for (i = 0; i < count; i++) {
    tmp[i] = dev_rw_buff[pos+i];
  }
  spin_lock(&at24c32_dev->spinlock);
  if (copy_to_user(buf, tmp, count)) {
    printk("copy to user err\n");
    kfree(tmp);
    return -EFAULT;
  }
  *ppos += count;
  ret = count;
  printk(KERN_INFO"read %d bytes! from %ld.\n",count,pos);
  kfree(tmp);
  return ret;
}

static ssize_t at24c32_write(struct file *filp, const char __user *buf,
                             size_t size, loff_t *ppos)
{
  unsigned long pos = *ppos;
  unsigned int count = size;
  char *tmp = NULL;
  ssize_t ret = 0,i;

  if (NULL == buf) {
    return EFAULT;
  }
  if (pos >= EEPROM_SIZE) {
    return count ? -ENXIO : 0;
  }
  if (count > EEPROM_SIZE - pos)
    count = EEPROM_SIZE - pos;

  tmp = kmalloc(count,/*GFP_KERNEL*/GFP_ATOMIC);
  if (NULL == tmp) {
    return -ENOMEM;
  }
  if (copy_from_user(tmp, buf, count)) {
    kfree(tmp);
    return -EFAULT;
  }
  spin_lock(&at24c32_dev->spinlock);
  //i2c_master_write();
  for (i = 0; i < count; i++) {
    dev_rw_buff[pos+i] = tmp[i];
  }
  spin_lock(&at24c32_dev->spinlock);

  *ppos += count;
  ret = count;
  printk(KERN_INFO"write %d bytes! from %ld.\n",count,pos);
  kfree(tmp);
  return ret;
}

const struct file_operations at24c32_fops={
  .owner  = THIS_MODULE,
  .read   = at24c32_read,
  .write  = at24c32_write,
  .open   = at24c32_open,
  .release= at24c32_release,
  .ioctl  = at24c32_ioctl,
};

static int __init at24c32_init(void)
{
//  int i;
  int ret;

  printk(KERN_INFO"at24c32 module init start!\n");
  at24c32_dev = kmalloc(sizeof(struct AT24C32_DEV),GFP_KERNEL);
  if (!at24c32_dev) {
    ret = -ENOMEM;
    return ret;
  }

  ret = alloc_chrdev_region(&devno,0,1,DEVICE_NAME);//get devno from system
  if(ret < 0){
    printk(KERN_ERR"cannot region chrdev\n");
    goto FAIL_REGION;
  }
//  major = MAJOR(devno);

  cdev_init(&at24c32_dev->cdev,&at24c32_fops);
  at24c32_dev->cdev.owner = THIS_MODULE;
  ret = cdev_add(&at24c32_dev->cdev,devno,1);
  if (ret < 0) {
    printk(KERN_ERR"add cdev error!\n");
    goto FAIL_ADD;
  }
  //mkdir char_null_udev under /sys/class
  at24c32_class = class_create(THIS_MODULE,DEVICE_NAME);
  if(IS_ERR(at24c32_class)){
    printk(KERN_INFO"create class error\n");
    goto FAIL_CLASS;
  }
  //make /dev/char_null_udev file
  device_create(at24c32_class,NULL,devno,NULL,DEVICE_NAME);
  spin_lock_init(&at24c32_dev->spinlock);
  printk(KERN_INFO"AT24C32 MODULE INIT SUCCESS!\n");
  return 0;
FAIL_CLASS:
  cdev_del(&at24c32_dev->cdev);
FAIL_ADD:
  unregister_chrdev_region(devno,1);
FAIL_REGION:
  kfree(at24c32_dev);
  return ret;
}

static void __exit at24c32_exit(void)
{
  device_destroy(at24c32_class,devno);
  class_destroy(at24c32_class);
  cdev_del(&at24c32_dev->cdev);
  unregister_chrdev_region(devno,1);
  kfree(at24c32_dev);
}

module_init(at24c32_init);
module_exit(at24c32_exit);

MODULE_DESCRIPTION("AT24C32 module for test");
MODULE_VERSION("V1.00");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liuyunli<liu.yunli@zte.com.cn>");
