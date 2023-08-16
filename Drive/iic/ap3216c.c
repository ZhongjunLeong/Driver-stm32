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
#include "ap3216c.h"

#define AP3216C_CNT 1
#define AP3216C_NAME "ap3216c"

struct ap3216c_dev
{
    struct i2c_client *client;  //i2c设备号
    dev_t devid;    //设备号
    struct cdev cdev;  //cdev
    struct class *class;    //类
    struct device *device;
    struct device_node  *nd;    //设备节点
    unsigned short ir, als, ps; //三个光传感数据
};
//printk(KERN_INFO"start\n");
/*
44 * @description : 从 ap3216c 读取多个寄存器数据
45 * @param – dev : ap3216c 设备
46 * @param – reg : 要读取的寄存器首地址
47 * @param – val : 读取到的数据
48 * @param – len : 要读取的数据长度
49 * @return : 操作结果
50 */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->client;

    /* msg[0]为发送要读取的首地址 */
    msg[0].addr = client->addr; /* ap3216c 地址 */
    msg[0].flags = 0; /* 标记为发送数据 */
    msg[0].buf = &reg; /* 读取的首地址 */
    msg[0].len = 1; /* reg 长度 */

    /* msg[1]读取数据 */
    msg[1].addr = client->addr; /* ap3216c 地址 */
    msg[1].flags = 1; /* 标记为读取数据 */
    msg[1].buf = val; /* 读取数据缓冲区 */
    msg[1].len = len; /* 要读取的数据长度 */

    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret == 2) 
    {
        ret = 0;
    } 
    else 
    {
        printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}

/*
80 * @description: 向 ap3216c 多个寄存器写入数据
81 * @param - dev: ap3216c 设备
82 * @param - reg: 要写入的寄存器首地址
83 * @param - val: 要写入的数据缓冲区
84 * @param - len: 要写入的数据长度
85 * @return : 操作结果
86 */
static s32 ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->client;
    b[0] = reg; /* 寄存器首地址 */
    memcpy(&b[1],buf,len); /* 将要写入的数据拷贝到数组 b 里面 */
    
    msg.addr = client->addr; /* ap3216c 地址 */
    msg.flags = 0; /* 标记为写数据 */

    msg.buf = b; /* 要写入的数据缓冲区 */
    msg.len = len + 1; /* 要写入的数据长度 */

    return i2c_transfer(client->adapter, &msg, 1);

}

/*
106 * @description: 读取 ap3216c 指定寄存器值，读取一个寄存器
107 * @param - dev: ap3216c 设备
108 * @param - reg: 要读取的寄存器
109 * @return : 读取到的寄存器值
110 */
static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
    u8 data = 0;

    ap3216c_read_regs(dev, reg, &data, 1);
    return data;

}

/*
120 * @description: 向 ap3216c 指定寄存器写入指定的值，写一个寄存器
121 * @param - dev: ap3216c 设备
122 * @param - reg: 要写的寄存器
123 * @param - data: 要写入的值
124 * @return : 无
125 */
static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

/*
134 * @description : 读取 AP3216C 的数据，包括 ALS,PS 和 IR, 注意！如果同时
135 * :打开 ALS,IR+PS 两次数据读取的时间间隔要大于 112.5ms
136 * @param – ir : ir 数据
137 * @param - ps : ps 数据
138 * @param - ps : als 数据
139 * @return : 无。
140 */
void ap3216c_readdata(struct ap3216c_dev *dev)
{
    unsigned char i =0;
    unsigned char buf[6];

    /* 循环读取所有传感器数据 */
    for(i = 0; i < 6; i++) 
    {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);  //将IR PS ALS寄存器初始地址高低位分别存buf
    }

    if(buf[0] & 0X80) /* IR_OF 位为 1,则数据无效 */
        dev->ir = 0; 
    else 
    {/* 读取 IR 传感器的数据 */
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03); //？为甚清零
    }
        dev->als = ((unsigned short)buf[3] << 8) | buf[2]; 
    
    if(buf[4] & 0x40) /* IR_OF 位为 1,则数据无效 */
        dev->ps = 0; 
    else /* 读取 PS 传感器的数据 */
        dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] &0X0F);

}

/*
* @description : 打开设备
* @param – inode : 传递给驱动的 inode
* @param - filp : 设备文件，file 结构体有个叫做 private_data 的成员变量
* 一般在 open 的时候将 private_data 指向设备结构体。
* @return : 0 成功;其他 失败
*/
static int ap3216c_open(struct inode *inode, struct file *filp)
{

    /* 从 file 结构体获取 cdev 指针，再根据 cdev 获取 ap3216c_dev 首地址 */
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct ap3216c_dev *ap3216cdev = container_of(cdev,struct ap3216c_dev, cdev);
printk("into open\r\n");
    /* 初始化 AP3216C */
    ap3216c_write_reg(ap3216cdev, AP3216C_SYSTEMCONG, 0x04); //系统配置复位
    mdelay(50); 
    ap3216c_write_reg(ap3216cdev, AP3216C_SYSTEMCONG, 0X03); //ALS PS IR功能开启
    return 0;
}

/*
185 * @description : 从设备读取数据
186 * @param - filp : 要打开的设备文件(文件描述符)
187 * @param - buf : 返回给用户空间的数据缓冲区
188 * @param - cnt : 要读取的数据长度
189 * @param - offt : 相对于文件首地址的偏移
190 * @return : 读取的字节数，如果为负值，表示读取失败
191 */
static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{

    short data[3];
    long err = 0;

    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct ap3216c_dev *dev = container_of(cdev, struct ap3216c_dev,cdev);
    printk("into read\r\n");
    ap3216c_readdata(dev);

    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;

}

/*
210 * @description : 关闭/释放设备
211 * @param - filp : 要关闭的设备文件(文件描述符)
212 * @return : 0 成功;其他 失败
213 */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* AP3216C 操作函数 */
static const struct file_operations ap3216c_ops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .release = ap3216c_release,
};

/*
 * @description : i2c 驱动的 probe 函数，当驱动与设备匹配以后此函数就会执行
 * @param – client : i2c 设备
 * @param - id : i2c 设备 ID
 * @return : 0，成功;其他负值,失败
 */
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
 
    int ret;
    struct ap3216c_dev *ap3216cdev;printk("into prbe1\r\n");
    ap3216cdev = devm_kzalloc(&client->dev, sizeof(*ap3216cdev), GFP_KERNEL);
    if(!ap3216cdev)
        return -ENOMEM;
     
    /*注册字符设备驱动*/
    /*1\创建设备号*/
         printk(KERN_INFO"into prbe2\n");
        ret = alloc_chrdev_region(&ap3216cdev->devid, 0, AP3216C_CNT, AP3216C_NAME);//申请设备号
        if(ret < 0)
        {
                pr_err("%s Couldn't alloc_chrdev_region, ret=%d",AP3216C_NAME, ret);
                return -ENOMEM;
        }

    /*2、初始化cdev*/
    ap3216cdev->cdev.owner = THIS_MODULE;
    cdev_init(&ap3216cdev->cdev, &ap3216c_ops);

    /*3\添加一个cdev*/
    ret = cdev_add(&ap3216cdev->cdev, ap3216cdev->devid, AP3216C_CNT);
    if(ret < 0)
        goto del_unregister;

    /*4\创建类*/
    ap3216cdev->class = class_create(THIS_MODULE, AP3216C_NAME);
    if(IS_ERR(ap3216cdev->class))
    {
        goto del_cdev;
    }
   
   /*5\创建设备*/
   ap3216cdev->device = device_create(ap3216cdev->class, NULL, ap3216cdev->devid, NULL, AP3216C_NAME);
   if(IS_ERR(ap3216cdev->device))
   {
        goto destroy_class;
   }
   
   ap3216cdev->client = client;

   /*保存ap216cdev结构体*/
    i2c_set_clientdata(client,ap3216cdev);
   return 0;
   

    destroy_class:
        class_destroy(ap3216cdev->class);
    del_cdev:
        cdev_del(&ap3216cdev->cdev);
    del_unregister:
        unregister_chrdev_region(ap3216cdev->devid, AP3216C_CNT);
    return -EIO;
}

/*
288 * @description : i2c 驱动的 remove 函数，移除 i2c 驱动的时候此函数会执行
289 * @param - client : i2c 设备
290 * @return : 0，成功;其他负值,失败
291 */
static int ap3216c_remove(struct i2c_client *client)
{
    //printk(KERN_INFO"into remove\n");
    struct ap3216c_dev *ap3216cdev = i2c_get_clientdata(client);
    /* 注销字符设备驱动 */
    /* 1、删除 cdev */
    cdev_del(&ap3216cdev->cdev);
    /* 2、注销设备号 */
    unregister_chrdev_region(ap3216cdev->devid, AP3216C_CNT);
    /* 3、注销设备 */
    device_destroy(ap3216cdev->class, ap3216cdev->devid);
    /* 4、注销类 */
    class_destroy(ap3216cdev->class);
    return 0;
}

/* 传统匹配方式 ID 列表 */
static const struct i2c_device_id ap3216c_id[] = {
    {"alientek, ap3216c", 0}, 
    {}
 };
//MODULE_DEVICE_TABLE(i2c, ap3216c_id);
 /* 设备树匹配列表 */
 static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "alientek, ap3216c" },
    { /* Sentinel */ }
 };
//MODULE_DEVICE_TABLE(of, ap3216c_of_match);
 /* i2c 驱动结构体 */ 
 static struct i2c_driver ap3216c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
    .owner = THIS_MODULE,
    .name = "ap3216c",
    .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id,
 };
 
 /*
332 * @description : 驱动入口函数
333 * @param : 无
334 * @return : 无
335 */
 static int __init ap3216c_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&ap3216c_driver);
    printk(KERN_INFO"添加i2c_driver:%d\n",ret);
    return ret;
    
}

/*
345 * @description : 驱动出口函数
346 * @param : 无
347 * @return : 无
348 */
static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}


module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");