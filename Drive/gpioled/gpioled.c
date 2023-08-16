/**************************************
*文件名：   gpioled.c
*作者:      lzj
*版本：     v1.0
*描述：     使用gpio子系统驱动led灯
**************************************/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define GPIOLED_CNT 1 //设备号个数 
#define GPIOLED_NAME "gpioled" //设备名字 

#define LEDOFF 0 //关灯
#define LEDON 1 //开灯

/*gpioled设备结构体*/
struct gpioled_dev
{
    dev_t devid;    //设备号
    struct cdev cdev;   //cdev
    struct class *class; 
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;   //led所使用的GPIO编号
};
struct gpioled_dev gpioled;
 


/*
*@描述          ：打开设备
*@入参 --inode  ：打开对应设备的字符索引号
*@入参 --filp   ：打开对应的设备文件
*@返回值    ：0成功
*/
static int led_open(struct inode *inode, struct file *filp)
{   //file结构体中的private_data的成员变量一般在open的时候将该变量指向设备结构体
    filp->private_data = &gpioled;
    return 0;
}

/*
*@描述          ：从设备在读取数据
*@入参 --filp   ：要打开的设备文件（文件描述符）
*@入参 --buf    ：存放读取到的数据
*@入参--cnt     ：要读取的数据长度
*@入参 --offt   ：相当于文件首地址的偏移
*@返回值    ：读取的字节数，为负表示读取失败
*/
static ssize_t led_read(struct file *filp, char __user *buf,size_t cnt, loff_t *offt)
{
     return 0;
}

/*
*@描述          ：向设备写数据
*@入参 --filp   ：要打开的设备文件（文件描述符）
*@入参 --buf    ：存放要写的数据
*@入参--cnt     ：要写的数据长度
*@入参 --offt   ：相当于文件首地址的偏移
*@返回值    ：写入的字节数，为负表示读取失败
*/
 static ssize_t led_write(struct file *filp, const char __user *buf,size_t cnt, loff_t *offt)
 {
    u32 retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;
    struct gpioled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);   //将要写的数据从用户空间的buf中写入到内核态的databuf，从用户到内核态写
    if(retvalue < 0) 
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];   //获取状态值

    if(ledstat == LEDON) 
    { 
        gpio_set_value(dev->led_gpio, 0); /* 打开 LED 灯 */
    } 
    else if(ledstat == LEDOFF)
    {
        gpio_set_value(dev->led_gpio, 1); /* 关闭 LED 灯 */
    }
    return 0;
 }

 /*
*@描述      ：关闭/释放设备
*@入参--filp ：要关闭的设备文件
*@入参 --inode  ：要关闭的设备的字符索引号
*@返回值    ：0成功
*/
static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

 /* 设备操作函数定义 */
static struct file_operations gpioled_fops = {
    .owner =    THIS_MODULE,
    .open  =    led_open,
    .read  =    led_read,
    .write =    led_write,
    .release =  led_release,
};

/*
*@描述      ：驱动入口函数
*@入参      ：无
*@返回值    ：无
*/
static int __init led_init(void)
{
    int ret = 0;
    const char *str;

    /*设置led所使用的gpio*/
    /*1、获取设备节点：gpioled*/
    gpioled.nd = of_find_node_by_path("/gpioled");  //获取/proc/device-tree中创建得到的设备节点
    if(gpioled.nd == NULL)
    {
        printk("gpioled node not find\r\n");
        return -EINVAL;
    }
    /* \读取节点内容2.读取 status 属性 */
    ret = of_property_read_string(gpioled.nd, "status", &str);
    if(ret < 0)
        return -EINVAL;

    if (strcmp(str, "okay"))
        return -EINVAL;

    /*3\获取compatible属性值并进行匹配*/
    ret = of_property_read_string(gpioled.nd, "compatible", &str);
    if(ret < 0)
    {
        printk("gpioled: Failed to get compatible property\n");
        return -EINVAL;

    }

    if(strcmp(str, "alientek,led"))
    {
        printk("gpioled: Compatible match failed\n");
        return -EINVAL;
    }
    /*4\获取设备树中gpio属性，得到led所使用的led编号*/
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd,  "led-gpio", 0);
    if(gpioled.led_gpio < 0) 
    {
        printk("can't get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", gpioled.led_gpio);

    /*5\向gpio子系统申请使用GPIO*/
    ret = gpio_request(gpioled.led_gpio, "LED-GPIO");
    if (ret) 
    {
        printk(KERN_ERR "gpioled: Failed to request led-gpio\n");
        return ret;
    }

    /* 6、设置 PI0 为输出，并且输出高电平，默认关闭 LED 灯 */
    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if(ret < 0)
    {
        printk("can't set gpio!\r\n");
    }

    /*注册字符设备驱动*/
    /*1\创建设备号*/
    if(gpioled.major)   //定义了设备号
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
        if(ret < 0)
        {
            pr_err("cannot register %s char driver [ret=%d]\n", GPIOLED_NAME, GPIOLED_CNT);
            goto free_gpio;
        }
     }
    else    //没创建设备号
    {
        ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);//申请设备号
        if(ret < 0)
        {
                pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n",GPIOLED_NAME, ret);
                goto free_gpio;
        }
    }

    gpioled.major = MAJOR(gpioled.devid);   //获取分配好的主设备号
    gpioled.minor = MINOR(gpioled.devid);   //获取分配好的次设备号
    
    printk("gpioled major=%d,minor=%d\r\n",gpioled.major,gpioled.minor); 

    /*2、初始化cdev*/
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);

    /*3\添加一个cdev*/
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
    if(ret < 0)
        goto del_unregister;

    /*4\创建类*/
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.class))
    {
        goto del_cdev;
    }
   
   /*5\创建设备*/
   gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
   if(IS_ERR(gpioled.device))
   {
        goto destroy_class;
   }
   return 0;
   

    destroy_class:
        class_destroy(gpioled.class);
    del_cdev:
        cdev_del(&gpioled.cdev);
    del_unregister:
        unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
    free_gpio:
        gpio_free(gpioled.led_gpio);
    return -EIO;
}



/*
*@描述      ：驱动出口函数
*@入参      ：无
*@返回值    ：无
*/
static void __exit led_exit(void)
{
   /* 注销字符设备驱动 */
    cdev_del(&gpioled.cdev); /* 删除 cdev */
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class); /* 注销类 */
    gpio_free(gpioled.led_gpio); /* 释放 GPIO */
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LZJ");
MODULE_INFO(intree, "Y");
