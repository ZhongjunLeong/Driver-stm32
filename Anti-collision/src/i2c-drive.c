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
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "rd04.h"
/*
 * @description : i2c 驱动的 probe 函数，当驱动与设备匹配以后此函数就会执行
 * @param – client : i2c 设备
 * @param - id : i2c 设备 ID
 * @return : 0，成功;其他负值,失败
 */
static int rd04_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
 
    int ret;
    struct rd04_dev *rd04dev;

    rd04dev = devm_kzalloc(&client->dev, sizeof(*rd04dev), GFP_KERNEL);
    if(!rd04dev)
        return -ENOMEM;
     
    /*注册字符设备驱动*/
    /*1\创建设备号*/
         printk(KERN_INFO"into prbe2\n");
        ret = alloc_chrdev_region(&rd04dev->devid, 0, rd04_CNT, rd04_NAME);//申请设备号
        if(ret < 0)
        {
                pr_err("%s Couldn't alloc_chrdev_region, ret=%d",rd04_NAME, ret);
                return -ENOMEM;
        }

    /*2、初始化cdev*/
    rd04dev->cdev.owner = THIS_MODULE;
    cdev_init(&rd04dev->cdev, &rd04_ops);

    /*3\添加一个cdev*/
    ret = cdev_add(&rd04dev->cdev, rd04dev->devid, rd04_CNT);
    if(ret < 0)
        goto del_unregister;

    /*4\创建类*/
    rd04dev->class = class_create(THIS_MODULE, rd04_NAME);
    if(IS_ERR(rd04dev->class))
    {
        goto del_cdev;
    }
   
   /*5\创建设备*/
   rd04dev->device = device_create(rd04dev->class, NULL, rd04dev->devid, NULL, rd04_NAME);
   if(IS_ERR(rd04dev->device))
   {
        goto destroy_class;
   }
   
   rd04dev->client = client;

   /*保存rd04dev结构体*/
    i2c_set_clientdata(client,rd04dev);
   return 0;
   

    destroy_class:
        class_destroy(rd04dev->class);
    del_cdev:
        cdev_del(&rd04dev->cdev);
    del_unregister:
        unregister_chrdev_region(rd04dev->devid, rd04_CNT);
    return -EIO;
}

/*
288 * @description : i2c 驱动的 remove 函数，移除 i2c 驱动的时候此函数会执行
289 * @param - client : i2c 设备
290 * @return : 0，成功;其他负值,失败
291 */
static int rd04_remove(struct i2c_client *client)
{
    //printk(KERN_INFO"into remove\n");
    struct rd04_dev *rd04dev = i2c_get_clientdata(client);
    /* 注销字符设备驱动 */
    /* 1、删除 cdev 设备号*/
    cdev_del(&rd04dev->cdev);
    /* 2、注销字符设备 */
    unregister_chrdev_region(rd04dev->devid, rd04_CNT);
    /* 3、注销设备 */
    device_destroy(rd04dev->class, rd04dev->devid);
    /* 4、注销类 */
    class_destroy(rd04dev->class);
    return 0;
}

/* 传统匹配方式 ID 列表 */
static const struct i2c_device_id rd04_id[] = {
    {"alientek,rd04", 0}, 
    {}
 };
//MODULE_DEVICE_TABLE(i2c, rd04_id);
 /* 设备树匹配列表 */
 static const struct of_device_id rd04_of_match[] = {
    { .compatible = "alientek,rd04" },
    { /* Sentinel */ }
 };
//MODULE_DEVICE_TABLE(of, rd04_of_match);
 /* i2c 驱动结构体 */ 
 static struct i2c_driver rd04_driver = {
    .probe = rd04_probe,
    .remove = rd04_remove,
    .driver = {
    .owner = THIS_MODULE,
    .name = "rd04",
    .of_match_table = rd04_of_match,
    },
    .id_table = rd04_id,
 };
 
 /*
332 * @description : 驱动入口函数
333 * @param : 无
334 * @return : 无
335 */
 static int __init rd04_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&rd04_driver);
    printk(KERN_INFO"添加i2c_driver:%d\n", ret);
    return ret;
    
}

/*
345 * @description : 驱动出口函数
346 * @param : 无
347 * @return : 无
348 */
static void __exit rd04_exit(void)
{
    i2c_del_driver(&rd04_driver);
}


module_init(rd04_init);
module_exit(rd04_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");