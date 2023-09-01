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
//#define PLATDRV_MAGIC       0x60     //魔术字
//#define SET             _IO (PLATDRV_MAGIC, 0x18)   //没有数据传输的命令
//#define OUT              _IO (PLATDRV_MAGIC, 0x19)


#define LEDDEV_CNT 2 //设备号个数 
#define LEDDEV_NAME "rd04gpio" //设备名字 /dev/out_gpio


dev_t parent_devid; //设备号
struct class *class;    //类

/*gpioled设备结构体*/
struct gpio_dev
{
    dev_t devid;    //设备号
    struct cdev cdev;   //cdev
    struct device *device;
    struct device_node *nd; //led设备节点
    int gpio_en;//led所使用的GPIO标号
   const char *name;
   
};
struct gpio_dev gpiodev[LEDDEV_CNT];
 

#if 0
static long rd04_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gpio_dev *dev = filp->private_data;
	int ret;

	switch (cmd)
	{
	case SET:
		gpiod_set_value((struct gpio_desc *)dev->gpio_en, arg);
		printk(KERN_INFO "rd04 is en.\n");
		break;
		
	case OUT:
        ret = gpiod_get_value((struct gpio_desc *)dev->gpio_out);
        if(ret == 0)
        {
            printk(KERN_INFO "people in!\n");
        }
        else
        {
            printk(KERN_INFO "prople out!\n");
        }
		return ret;
	
	default:
		return -EINVAL;
	}
	
	return 0;

}

#endif

/*
*@描述          ：打开设备
*@入参 --inode  ：打开对应设备的字符索引号
*@入参 --filp   ：打开对应的设备文件
*@返回值    ：0成功
*/
static int rd04_open(struct inode *inode, struct file *filp)
{   
/*在打开多个设备时，需要用container_of函数获取对应的设备描述文件并保持在私有变量中*/
    struct gpio_dev *dev = container_of(inode->i_cdev, struct gpio_dev, cdev);
    filp->private_data = dev;

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
 static ssize_t rd04_read(struct file *filp, char __user *buf,size_t cnt, loff_t *offt)
{
    int ret;
    int current_val;

    current_val = gpio_get_value(gpiodev[1].gpio_en);
    /* 将按键状态信息发送给应用程序 */
    ret = copy_to_user(buf, &current_val, sizeof(int));

    return ret;
 }


 /* 设备操作函数定义 */
static struct file_operations rd_fops = {
    .owner =    THIS_MODULE,
    .open  =    rd04_open,
    .read =    rd04_read,
    //.unlocked_ioctl = rd04_ioctl,
};

/*gpio初始化，需要分别获取各个子节点
*dev：各个子节点的设备描述
*return ：成功0失败-1
*/
static int gpio_init(struct gpio_dev *dev)
{
    int ret;
    const char *str;
    dev->gpio_en = of_get_named_gpio(dev->nd,  "gpios", 0); //获取设备树属性gpios名称对应的第一个设备编号
    printk(KERN_ERR "gpiodev-en: %d\n", dev->gpio_en);
   if(!gpio_is_valid(dev->gpio_en))
    {
        printk(KERN_ERR "gpiodev-en: Failed to get gpios:0\n");
        return -EINVAL;
    }
    /*从设备树的子节点中获取各个label*/
	ret = of_property_read_string(dev->nd, "label", &str);
    if (ret < 0)
	{
 
		printk("of_property_read_string label failed! \r\n");
 
	}
 
	dev->name = str;
 
	printk("of_property_read_string label is %s \r\n", dev->name);
    /*申请GPIO*/
    ret = gpio_request(dev->gpio_en, str);	
    if (ret) 
    {
        printk(KERN_ERR "gpiodev-en: Failed to request \n");
        return ret;
    }
 
    if(!strcmp(dev->name, "gpio-set" ))
    {

        /*默认使能端为高*/
        ret = gpio_direction_output(dev->gpio_en, 1);
        if(ret < 0)
        {
            printk("can't set gpio!\r\n");
        }
       
    }
   
    if(!strcmp(dev->name, "gpio-out" ))
    {
        ret = gpio_direction_input(dev->gpio_en);
        if(ret < 0)
        {
            printk("can't out gpio!\r\n");
        }
    }

  return 0;
}
/*
 * @description : flatform 驱动的 probe 函数，当驱动与设备匹配以后此函数
 * 就会执行
* @param - dev : platform 设备
 * @return : 0，成功;其他负值,失败
 */
 static int rd_probe(struct platform_device *pdev)
{
    int ret, i;
    int major_devid;
	struct device_node *parent_nd;
    char node_path[32] = { 0 };


    parent_nd = pdev->dev.of_node;

 
   
   /*获取子设备节点*/
   for(i = 0; i < LEDDEV_CNT; i++)
   {
        sprintf(node_path, "/gpioled/led%d", i);
        gpiodev[i].nd = of_find_node_by_path(node_path);

        ret = gpio_init(&gpiodev[i]);
        if(ret < 0)
            return ret;
   }

    /*注册字符设备驱动*/
    /*1\创建父设备号*/
    ret = alloc_chrdev_region(&parent_devid, 0, LEDDEV_CNT, LEDDEV_NAME);//申请设备号
   if(ret < 0)
    {
        pr_err("%s Couldn't alloc_chrdev_region, ret=%d",LEDDEV_NAME, ret);
        goto free_gpio;
    }
   
   major_devid = MAJOR(parent_devid);
   printk("request major_devid is %d \r\n", major_devid);

   for(i = 0; i < LEDDEV_CNT; i++ )
   {
        gpiodev[i].devid = parent_devid+i;  //有别于直接使用MKDEV创建，因为parent_devid已经是dev_t类型，包含了主次设备号了
        /*2、初始化cdev*/
        gpiodev[i].cdev.owner = THIS_MODULE;
        cdev_init(&gpiodev[i].cdev, &rd_fops);

        /*3\添加一个cdev*/
        ret = cdev_add(&gpiodev[i].cdev, gpiodev[i].devid, LEDDEV_CNT);
        if(ret < 0)
            goto del_unregister;
   }
    /*4\创建类*/
    class = class_create(THIS_MODULE, LEDDEV_NAME);
    if(IS_ERR(class))
    {
        goto del_cdev;
    }
   
   for(i = 0; i < LEDDEV_CNT; i++)
   {
     
        /*5\创建次设备*/
        gpiodev[i].device = device_create(class, NULL, gpiodev[i].devid, NULL, "%s%d",LEDDEV_NAME, i);
        if(IS_ERR(gpiodev[i].device))
        {
            ret = PTR_ERR(gpiodev[i].device);
                goto destroy_class;
        }
  
    }


 
    return 0;
  
    destroy_class:
        class_destroy(class);
    del_cdev:
        for(i = 0; i < LEDDEV_CNT; i++)
        {
            cdev_del(&gpiodev[i].cdev);
        }
        
    del_unregister:
        unregister_chrdev_region(parent_devid, LEDDEV_CNT);
    free_gpio:
        gpio_free(gpiodev[0].gpio_en);
        gpio_free(gpiodev[1].gpio_en);
    return -EIO;
}


 /*
 * @description : platform 驱动的 remove 函数
 * @param - dev : platform 设备
 * @return : 0，成功;其他负值,失败
 */
static int rd_remove(struct platform_device *dev)
{
     int i;
  

    gpio_set_value(gpiodev[0].gpio_en, 0); /* 卸载驱动的时候关闭 rd04 */
    gpio_free(gpiodev[0].gpio_en); /* 注销 GPIO */
    gpio_free(gpiodev[1].gpio_en); /* 注销 GPIO */
   
    for(i = 0; i < LEDDEV_CNT; i++)
   {

        device_destroy(class, gpiodev[i].devid);/* 注销设备 */
   }
    class_destroy(class); /* 注销类 */
    cdev_del(&gpiodev[0].cdev); /* 删除 cdev */
    cdev_del(&gpiodev[1].cdev); /* 删除 cdev */
    unregister_chrdev_region(parent_devid, LEDDEV_CNT);
    return 0;

}

 /* 匹配列表 */
 static const struct of_device_id rd_of_match[] = {
    {.compatible = "alientek,led"}, //与设备树的设备节点compatible对应
    { /*sentinel*/}
 };
    MODULE_DEVICE_TABLE(of, rd_of_match);//声明led_of_match这个匹配表
 /* platform 驱动结构体 */
static struct platform_driver rd_driver = {
    .driver = {
        .name = "stm32mp1-rd04", //驱动名字，会在/sys/bus/platform/drivers/目录下存在一个名为“stm32mp1-rd04”的文件
        .of_match_table = rd_of_match, //设备树匹配表
    },
    .probe = rd_probe,
    .remove = rd_remove,
	
};

/*
*@描述      ：驱动入口函数
*@入参      ：无
*@返回值    ：无
*/
static int __init gpiodriver_init(void)
{
    int ret;
    ret = platform_driver_register(&rd_driver);
     printk(KERN_ERR "gpiodriver init scuess\n");
   return ret;
}



/*
*@描述      ：驱动出口函数
*@入参      ：无
*@返回值    ：无
*/
static void __exit gpiodriver_exit(void)
{
   platform_driver_unregister(&rd_driver);
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LZJ");
MODULE_INFO(intree, "Y");
