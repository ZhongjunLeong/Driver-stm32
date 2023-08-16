/**************************************
*文件名：   timer.c
*作者:      lzj
*版本：     v1.0
*描述：     使用gpio子系统驱动led灯，使用内核定时点灯，在定时器周期设置时用自旋锁
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
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define TIMER_CNT 1 //设备号个数 
#define TIMER_NAME "timer" //设备名字 
#define CLOSE_CMD (_IO(0XEF, 0x1)) /* 关闭定时器 */
#define OPEN_CMD (_IO(0XEF, 0x2)) /* 打开定时器 */
#define SETPERIOD_CMD (_IO(0XEF, 0x3)) /* 设置定时器周期命令*/

#define LEDOFF 0 //关灯
#define LEDON 1 //开灯

/*timer设备结构体*/
struct timer_dev
{
    dev_t devid;    //设备号
    struct cdev cdev;   //cdev
    struct class *class; 
    struct device *device;
    int major;
    int minor;
    struct device_node *nd; //设备节点
    int led_gpio;   //led所使用的GPIO编号
    int timeperiod; //定时周期，单位ms
    struct timer_list   timer;  //定义一个定时器
    spinlock_t  lock;   //定义自旋锁
};
struct timer_dev timerdev;
 
 /*
55 * @description : 初始化 LED 灯 IO，open 函数打开驱动的时候
56 * 初始化 LED 灯所使用的 GPIO 引脚。
57 * @param : 无
58 * @return : 无
59 */
 static int led_init(void)
 {
    int ret;
    const char *str;

    /* 设置 LED 所使用的 GPIO */
    /* 1、获取设备节点：timerdev */
    timerdev.nd = of_find_node_by_path("/gpioled");
    if(timerdev.nd == NULL) 
    {
        printk("timerdev node not find!\r\n");
        return -EINVAL;
    }

    /* 2.读取 status 属性 */
    ret = of_property_read_string(timerdev.nd, "status", &str);
    if(ret < 0)
        return -EINVAL;
    if (strcmp(str, "okay"))
        return -EINVAL;
    
    /* 3、获取 compatible 属性值并进行匹配 */
    ret = of_property_read_string(timerdev.nd, "compatible", &str);
    if(ret < 0)
    {
        printk("timerdev: Failed to get compatible property\n");
        return -EINVAL;
    }

    if (strcmp(str, "alientek,led"))
    {
        printk("timerdev: Compatible match failed\n");
        return -EINVAL;
    }

    /* 4、 获取设备树中的 gpio 属性，得到 led-gpio 所使用的 led 编号 */
    timerdev.led_gpio = of_get_named_gpio(timerdev.nd,"led-gpio", 0);
    if(timerdev.led_gpio < 0) 
    {
        printk("can't get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", timerdev.led_gpio);

    /* 5.向 gpio 子系统申请使用 GPIO */
    ret = gpio_request(timerdev.led_gpio, "led");
    if (ret) 
    {
        printk(KERN_ERR "timerdev: Failed to request led-gpio\n");
        return ret;
    }

    /* 6、设置 PI0 为输出，并且输出高电平，默认关闭 LED 灯 */
    ret = gpio_direction_output(timerdev.led_gpio, 1);
    if(ret < 0) 
    {
        printk("can't set gpio!\r\n");
        return ret;
    }
    return 0;
}

/*
*@描述          ：打开设备
*@入参 --inode  ：打开对应设备的字符索引号
*@入参 --filp   ：打开对应的设备文件
*@返回值    ：0成功
*/
static int timer_open(struct inode *inode, struct file *filp)
{   //file结构体中的private_data的成员变量一般在open的时候将该变量指向设备结构体
    int ret;
    filp->private_data = &timerdev;

    timerdev.timeperiod = 1000; //默认周期为1s
    ret = led_init();
    if (ret < 0) 
    {
        return ret;
    }

    return 0;
}



/*
 * @description : ioctl 函数，
 * @param - filp : 要打开的设备文件(文件描述符)
 * @param - cmd : 应用程序发送过来的命令
 * @param - arg : 参数
 * @return : 0 成功;其他 失败
 */
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
 struct timer_dev *dev = (struct timer_dev *)filp->private_data;
 int timerperiod;
 unsigned long flags;
 
 switch (cmd) 
 {
    case CLOSE_CMD: /* 关闭定时器 */
        del_timer_sync(&dev->timer);
        break;
    case OPEN_CMD: /* 打开定时器 */
        spin_lock_irqsave(&dev->lock, flags);
        timerperiod = dev->timeperiod;
        spin_unlock_irqrestore(&dev->lock, flags);
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerperiod));
        break;
    case SETPERIOD_CMD: /* 设置定时器周期 */
        spin_lock_irqsave(&dev->lock, flags);
        dev->timeperiod = arg;
        spin_unlock_irqrestore(&dev->lock, flags);
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
        break;
    default:
        break;
 }
    return 0;
}


 /*
*@描述      ：关闭/释放设备
*@入参--filp ：要关闭的设备文件
*@入参 --inode  ：要关闭的设备的字符索引号
*@返回值    ：0成功
*/
static int timer_release(struct inode *inode, struct file *filp)
{
    struct timer_dev *dev = filp->private_data;
    gpio_set_value(dev->led_gpio, 1); /* APP 结束的时候关闭 LED */
    gpio_free(dev->led_gpio); /* 释放 LED */
    del_timer_sync(&dev->timer); /* 关闭定时器*/
    return 0;
}

 /* 设备操作函数定义 */
static struct file_operations timer_fops = {
    .owner =    THIS_MODULE,
    .open  =    timer_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
    .release =  timer_release,
};

/*
*@描述      ：定时器回调函数
*@入参--arg ：
*@返回值    ：0成功
*/

void timer_function(struct timer_list *arg)
{
    //from_timer是一个宏，可以根据结构体的成员地址获取这个结构体的首地址。第一个参数表示结构体，第二参数表示第一个参数里的成员，第三个参数表示第二个参数的类型，得到第一个参数的首地址
    struct timer_dev *dev = from_timer(dev, arg, timer);
    static int sta = 1;
    int timerperiod;
    unsigned long flags;

    sta = !sta; /* 每次都取反，实现 LED 灯反转 */
    gpio_set_value(dev->led_gpio, sta);

    /*重启定时器*/
    spin_lock_irqsave(&dev->lock, flags);
    timerperiod = dev->timeperiod; //从何获取？
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timeperiod));    //修改内核timer值，

}

/*
*@描述      ：驱动入口函数
*@入参      ：无
*@返回值    ：无
*/
static int __init timer_init(void)
{
    int ret = 0;
   
   /*初始化自旋锁*/
   spin_lock_init(&timerdev.lock);

    /*注册字符设备驱动*/
    /*1\创建设备号*/
    if(timerdev.major)   //定义了设备号
    {
        timerdev.devid = MKDEV(timerdev.major, 0);
        ret = register_chrdev_region(timerdev.devid, TIMER_CNT, TIMER_NAME);
        if(ret < 0)
        {
            pr_err("cannot register %s char driver [ret=%d]\n", TIMER_NAME, TIMER_CNT);
            return -EIO;
        }
     }
    else    //没创建设备号
    {
        ret = alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT, TIMER_NAME);//申请设备号
        if(ret < 0)
        {
                pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n",TIMER_NAME, ret);
               return -EIO;
        }
    }

    timerdev.major = MAJOR(timerdev.devid);   //获取分配好的主设备号
    timerdev.minor = MINOR(timerdev.devid);   //获取分配好的次设备号
    
    printk("timerdev major=%d,minor=%d\r\n",timerdev.major,timerdev.minor); 

    /*2、初始化cdev*/
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev, &timer_fops);

    /*3\添加一个cdev*/
    cdev_add(&timerdev.cdev, timerdev.devid, TIMER_CNT);
    if(ret < 0)
        goto del_unregister;

    /*4\创建类*/
    timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
    if(IS_ERR(timerdev.class))
    {
        goto del_cdev;
    }
   
   /*5\创建设备*/
   timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);
   if(IS_ERR(timerdev.device))
   {
        goto destroy_class;
   }
    /*6\初始化timer，设置定时器处理函数，还未设置周期，所以不会激活定时器*/
    timer_setup(&timerdev.timer, timer_function, 0);
    return 0;
   

    destroy_class:
        class_destroy(timerdev.class);
    del_cdev:
        cdev_del(&timerdev.cdev);
    del_unregister:
        unregister_chrdev_region(timerdev.devid, TIMER_CNT);
    return -EIO;
}



/*
*@描述      ：驱动出口函数
*@入参      ：无
*@返回值    ：无
*/
static void __exit timer_exit(void)
{
    del_timer_sync(&timerdev.timer);
    #if 0
        del_timer(&timerdev.timer);
    #endif
   /* 注销字符设备驱动 */
    cdev_del(&timerdev.cdev); /* 删除 cdev */
    unregister_chrdev_region(timerdev.devid, TIMER_CNT);
    device_destroy(timerdev.class, timerdev.devid);
    class_destroy(timerdev.class); /* 注销类 */
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LZJ");
MODULE_INFO(intree, "Y");
