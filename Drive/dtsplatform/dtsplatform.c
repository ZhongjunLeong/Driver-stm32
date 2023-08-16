/**************************************
*文件名：   dtsplatform.c
*作者:      lzj
*版本：     v1.0
*描述：     使用设备树的platform框架，驱动led/PI0
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
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define LEDDEV_CNT 1 //设备号个数 
#define LEDDEV_NAME "dtsplatled" //设备名字 /dev/dtsplatled

#define LEDOFF 0 //关灯
#define LEDON 1 //开灯

/*gpioled设备结构体*/
struct leddev_dev
{
    dev_t devid;    //设备号
    struct cdev cdev;   //cdev
    struct class *class; 
    struct device *device;
    struct device_node *nd; //led设备节点
    int gpio_led;   //led所使用的GPIO标号
};
struct leddev_dev leddev;
 

/*
 * @description : LED 打开/关闭
* @param - sta : LEDON(0) 打开 LED，LEDOFF(1) 关闭 LED
 * @return : 无
 */
void led_switch(u8 sta)
{
    if (sta == LEDON )
    gpio_set_value(leddev.gpio_led, 0);
    else if (sta == LEDOFF)
    gpio_set_value(leddev.gpio_led, 1);
 }



static int led_gpio_init(struct device_node *nd)
{
    int ret = 0;
    
    /*1\获取设备树中gpio属性，得到led所使用的led编号*/
    leddev.gpio_led = of_get_named_gpio(nd,  "led-gpio", 0); //设备编号有platform给
   if(!gpio_is_valid(leddev.gpio_led))
    {
        printk(KERN_ERR "leddev: Failed to get led-gpio\n");
        return -EINVAL;
    }
    /*2\申请使用GPIO*/
    ret = gpio_request(leddev.gpio_led, "LED0");
    if (ret) 
    {
        printk(KERN_ERR "leddev: Failed to request led-gpio\n");
        return ret;
    }

    /* 3、设置 PI0 为输出，并且输出高电平，默认关闭 LED 灯 */
    ret = gpio_direction_output(leddev.gpio_led, 1);
    if(ret < 0)
    {
        printk("can't set gpio!\r\n");
    }
    return 0;
}

/*
*@描述          ：打开设备
*@入参 --inode  ：打开对应设备的字符索引号
*@入参 --filp   ：打开对应的设备文件
*@返回值    ：0成功
*/
static int led_open(struct inode *inode, struct file *filp)
{   //file结构体中的private_data的成员变量一般在open的时候将该变量指向设备结构体
    
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
    
    retvalue = copy_from_user(databuf, buf, cnt);   //将要写的数据从用户空间的buf中写入到内核态的databuf，从用户到内核态写
    if(retvalue < 0) 
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];   //获取状态值

    if(ledstat == LEDON) 
    { 
        led_switch(LEDON); /* 打开 LED 灯 */
    } 
    else if(ledstat == LEDOFF)
    {
        led_switch(LEDOFF); /* 关闭 LED 灯 */
    }
    return 0;
 }


 /* 设备操作函数定义 */
static struct file_operations led_fops = {
    .owner =    THIS_MODULE,
    .open  =    led_open,
    .write =    led_write,
    
};

/*
 * @description : flatform 驱动的 probe 函数，当驱动与设备匹配以后此函数
 * 就会执行
* @param - dev : platform 设备
 * @return : 0，成功;其他负值,失败
 */
 static int led_probe(struct platform_device *pdev)
{
    int ret;
    
    printk("led driver and device was matched!\r\n");
    
    /* 初始化 LED */
    ret = led_gpio_init(pdev->dev.of_node);
    if(ret < 0)
        return ret;


    /*注册字符设备驱动*/
    /*1\创建设备号*/
   
        ret = alloc_chrdev_region(&leddev.devid, 0, LEDDEV_CNT, LEDDEV_NAME);//申请设备号
        if(ret < 0)
        {
                pr_err("%s Couldn't alloc_chrdev_region, ret=%d",LEDDEV_NAME, ret);
                goto free_gpio;
        }

    /*2、初始化cdev*/
    leddev.cdev.owner = THIS_MODULE;
    cdev_init(&leddev.cdev, &led_fops);

    /*3\添加一个cdev*/
    ret = cdev_add(&leddev.cdev, leddev.devid, LEDDEV_CNT);
    if(ret < 0)
        goto del_unregister;

    /*4\创建类*/
    leddev.class = class_create(THIS_MODULE, LEDDEV_NAME);
    if(IS_ERR(leddev.class))
    {
        goto del_cdev;
    }
   
   /*5\创建设备*/
   leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, LEDDEV_NAME);
   if(IS_ERR(leddev.device))
   {
        goto destroy_class;
   }
   return 0;
   

    destroy_class:
        class_destroy(leddev.class);
    del_cdev:
        cdev_del(&leddev.cdev);
    del_unregister:
        unregister_chrdev_region(leddev.devid, LEDDEV_CNT);
    free_gpio:
        gpio_free(leddev.gpio_led);
    return -EIO;
}


 /*
 * @description : platform 驱动的 remove 函数
 * @param - dev : platform 设备
 * @return : 0，成功;其他负值,失败
 */
static int led_remove(struct platform_device *dev)
{
    gpio_set_value(leddev.gpio_led, 1); /* 卸载驱动的时候关闭 LED */
    gpio_free(leddev.gpio_led); /* 注销 GPIO */
    cdev_del(&leddev.cdev); /* 删除 cdev */
    unregister_chrdev_region(leddev.devid, LEDDEV_CNT);
    device_destroy(leddev.class, leddev.devid); /* 注销设备 */
    class_destroy(leddev.class); /* 注销类 */
    return 0;

}

 /* 匹配列表 */
 static const struct of_device_id led_of_match[] = {
    {.compatible = "alientek,led"}, //与设备树的设备节点compatible对应
    { /*sentinel*/}
 };
    MODULE_DEVICE_TABLE(of, led_of_match);//声明led_of_match这个匹配表
 /* platform 驱动结构体 */
static struct platform_driver led_driver = {
    .driver = {
        .name = "stm32mp1-led", //驱动名字，会在/sys/bus/platform/drivers/目录下存在一个名为“stm32mp1-led”的文件
        .of_match_table = led_of_match, //设备树匹配表
    },
    .probe = led_probe,
    .remove = led_remove,
};

/*
*@描述      ：驱动入口函数
*@入参      ：无
*@返回值    ：无
*/
static int __init leddriver_init(void)
{
   return platform_driver_register(&led_driver);
}



/*
*@描述      ：驱动出口函数
*@入参      ：无
*@返回值    ：无
*/
static void __exit leddriver_exit(void)
{
   platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LZJ");
MODULE_INFO(intree, "Y");
