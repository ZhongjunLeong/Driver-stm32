/**************************************
*文件名：   led_chr.c
*作者:      lzj
*版本：     v1.0
*描述：     字符设备：LED驱动文件，包括gpio配置
**************************************/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*gpio相关寄存器物理地址*/
#define PERIPH_BASE (0x40000000)    //外设基地址
#define MPU_AHB4_PERIPH_BASE (PERIPH_BASE + 0x10000000) //供gpioi使用的高速时钟线基地址
#define RCC_BASE (MPU_AHB4_PERIPH_BASE + 0x0000)    
#define RCC_MP_AHB4ENSETR (RCC_BASE + 0XA28)
#define GPIOI_BASE  (MPU_AHB4_PERIPH_BASE + 0xA000)    //gpioi基地址
#define GPIOI_MODER (GPIOI_BASE + 0x000)    //gpioi模式配置
#define GPIOI_OTYPER (GPIOI_BASE + 0x004)
#define GPIOI_OSPEEDR (GPIOI_BASE + 0x008)
#define GPIOI_PUPDR (GPIOI_BASE + 0x000c)
#define GPIOI_BSRR  (GPIOI_BASE + 0x0018)   //gpioi置位/复位寄存器

#define LED_MAJOR 200 //主设备号
#define LED_NAME "led" //设备名字 

#define LEDOFF 0 //关灯
#define LEDON 1 //开灯

/*映射后寄存器虚拟地址指针*/
static void __iomem *MPU_AHB4_PERIPH_RCC_PI;
static void __iomem *GPIOI_MODER_PI;
static void __iomem *GPIOI_OTYPER_PI;
static void __iomem *GPIOI_OSPEEDR_PI;
static void __iomem *GPIOI_PUPDR_PI;
static void __iomem *GPIOI_BSRR_PI;
 
/*
*@描述      ：led打开/关闭
*@入参      ：sta ==LEDON:打开 sta==LEDOFF：关闭
*@返回值    ：无
*/
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON)
    {
        val = readl(GPIOI_BSRR_PI);
        val |= (1 << 16);   //高位的第一位置1，reset：拉低---打开灯
        writel(val, GPIOI_BSRR_PI);
    }
    else if (sta == LEDOFF)
    {
        val = readl(GPIOI_BSRR_PI);
        val |= (1 << 0);   //低位的第一位置1 ，set：拉高---关灯
        writel(val, GPIOI_BSRR_PI);
    }
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

/*
*@描述      ：取消映射
*@入参      ：无
*@返回值    ：无
*/
void led_unmap(void)
{
    /* 取消映射 */
    iounmap(MPU_AHB4_PERIPH_RCC_PI);
    iounmap(GPIOI_MODER_PI);
    iounmap(GPIOI_OTYPER_PI);
    iounmap(GPIOI_OSPEEDR_PI);
    iounmap(GPIOI_PUPDR_PI);
    iounmap(GPIOI_BSRR_PI);
}
 /* 设备操作函数定义 */
static struct file_operations led_fops = {
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
    int retvalue = 0;
    u32 val = 0;
    /*初始化led*/
    /*寄存器地址映射*/
    MPU_AHB4_PERIPH_RCC_PI = ioremap(RCC_MP_AHB4ENSETR, 4);
    GPIOI_MODER_PI = ioremap(GPIOI_MODER, 4);
    GPIOI_OTYPER_PI = ioremap(GPIOI_OTYPER, 4);
    GPIOI_OSPEEDR_PI = ioremap(GPIOI_OSPEEDR, 4);
    GPIOI_PUPDR_PI = ioremap(GPIOI_PUPDR, 4);
    GPIOI_BSRR_PI = ioremap(GPIOI_BSRR, 4);

    /*使能PI时钟*/
    val = readl(MPU_AHB4_PERIPH_RCC_PI);   //读取AHB4时钟地址内容
    val &= ~(0x1 << 8); //清楚之前的设置，清0
    val |= (0x1 << 8); //设置新值
    writel(val, MPU_AHB4_PERIPH_RCC_PI);

    /*设置PI0通用输出模式*/
    val = readl(GPIOI_MODER_PI);
    val &= ~(0x3 << 0); //bit0:1清零
    val |= (0X1 << 0); /* bit0:1 设置 01 */
    writel(val, GPIOI_MODER_PI);

    /*设置 PI0 为推挽模式*/
    val = readl(GPIOI_OTYPER_PI);
    val &= ~(0x1 << 0); //bit0清零,设置上拉
    writel(val, GPIOI_OTYPER_PI);

    /*设置 PI0 为高速*/
    val = readl(GPIOI_OSPEEDR_PI);
    val &= ~(0x3 << 0); //bit0：1清零
    val |= (0X1 << 1); //bit0：1 设置 10 高速
    writel(val, GPIOI_OSPEEDR_PI);

    /*设置 PI0 为上拉*/
    val = readl(GPIOI_PUPDR_PI);
    val &= ~(0x3 << 0); //bit0：1清零
    val |= (0X1 << 0); //bit0：1 设置 01 上拉
    writel(val, GPIOI_PUPDR_PI);

    /*默认关闭 LED*/
    val = readl(GPIOI_BSRR_PI);
    val |= (0x1 << 0); //bit0置1，复位
    writel(val, GPIOI_BSRR_PI);

    /*注册字符设备驱动*/
    retvalue = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    if(retvalue < 0)
    {
        printk("register chrdev faile\r\n");
        goto fail_map;
    }

    return 0;

fail_map:
    led_unmap();
    return -EIO;
}


/*
*@描述      ：驱动出口函数
*@入参      ：无
*@返回值    ：无
*/
static void __exit led_exit(void)
{
    /*取消映射*/
    led_unmap();

    /*注销字符设备驱动*/
    unregister_chrdev(LED_MAJOR, LED_NAME);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LZJ");
MODULE_INFO(intree, "Y");
