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



#define RD_04_I2C_ADDR 0X71 //从设备地址
#define AXK_RD04_WR 0X00    //对寄存器写
#define AXK_RD04_RD 0X01    //对寄存器读

/*
44 * @description : 从 rd04 读取多个寄存器数据
45 * @param – dev : rd04 设备
46 * @param – reg : 要读取的寄存器首地址
47 * @param – val : 读取到的数据
48 * @param – len : 要读取的数据长度
49 * @return : 操作结果
50 */
static int rd04_read_regs(struct rd04_dev *dev, u8 reg, void *val, int len)
{
    int ret;
   // u8 reg;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->client;
  //  reg = (u8 *)&Reg;
    /* msg[0]为发送要读取的首地址 */
    msg[0].addr = client->addr; /* 从机rd04 地址 */
    msg[0].flags = 0; /* ACK:主机写 */
    msg[0].buf = &reg; /* 写入的寄存器地址放入缓存 */
    msg[0].len = 1; /* reg 长度 */

    /* msg[1]读取数据 */
    msg[1].addr = client->addr; /* rd04 地址 */
    msg[1].flags = 1; /* ACK:主机读 */
    msg[1].buf = val; /* 读取的数据数据回填 */
    msg[1].len = len; /* 要读取的数据长度 */

    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret == 2) //读取成功。返回0
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
80 * @description: 向 rd04 多个寄存器写入数据
81 * @param - dev: rd04 设备
82 * @param - reg: 要写入的寄存器首地址
83 * @param - val: 要写入的数据缓冲区
84 * @param - len: 要写入的数据长度
85 * @return : 操作结果
86 */
static s32 rd04_write_regs(struct rd04_dev *dev, u8 reg, int *buf, int len)
{
    u8 b[256];
   // u8 reg;
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->client;
    //reg = (u8 *)&Reg;
    b[0] = reg; /* 寄存器地址 */
    memcpy(&b[1],buf,len); /* 将要写入的数据拷贝到数组 b 里面 */
    
    msg.addr = client->addr; /* 从设备树中读取 rd04 地址 */
    msg.flags = 0; /* ACK：主机发送 */

    msg.buf = b; /* 要写入的寄存器地址放入msg的缓冲区 */
    msg.len = len + 1; /* 加上一个字节的寄存器地址 */

    return i2c_transfer(client->adapter, &msg, 1);

}

/*
* @description: 读取 从设备
* @param - dev: rd04 设备
* @param - reg: 要读取的寄存器
* @return : 读取到的寄存器值
*/
static unsigned char rd04_read_reg(struct rd04_dev *dev, int reg)
{
    int data = 0;

    rd04_read_regs(dev, reg, &data, 1);
    return data;

}

/*
* @description: 向 写从设备
* @param - dev: rd04 设备
* @param - reg: 要写的寄存器
* @param - data: 要写入的值
* @return : 无
 */
static int rd04_write_reg(struct rd04_dev *dev, int reg, int data)
{
    int buf = 0;
    int ret;
    buf = data;
    ret = rd04_write_regs(dev, reg, &buf, 1);
    if(ret < 0)
    {
        printk(KERN_ERR "i2c transfer fail : %d\n", ret);
        return -1;
    }

    return 0;
}

/*********************************rd04参数配置******************************************/

void AxkRd04SetIoValOutput(struct rd04_dev *dev, uint8_t OutputStatus)
{
  
    mdelay(10);
    OutputStatus?rd04_write_reg(dev, 0X24, 0X07):rd04_write_reg(dev, 0X24, 0X03);//0x07高 0x03低
  
}
/**
 * @brief AxkRd04SetWayOfWorking
 * Set the working mode of Rd-04
 * @param PSM
*/
void AxkRd04SetWayOfWorking(struct rd04_dev *dev, rd04_psm_t PSM)
{
   
mdelay(10);
    rd04_write_reg(dev, 0X04, PSM);
   
}
/**
 * @brief AxkRd04SetADCSamplingFrequency
 *
 * @param ADC_SF
*/
void AxkRd04SetADCSamplingFrequency(struct rd04_dev *dev, rd04_adc_sf_t ADC_SF)
{
    
mdelay(10);
    switch (ADC_SF)
    {
        case RD04_ADC_SF_1KHz:
            rd04_write_reg(dev, 0X10, 0X20);
            break;
        case RD04_ADC_SF_2KHz:
            rd04_write_reg(dev, 0X10, 0X10);
            break;
        case RD04_ADC_SF_4KHz:
            rd04_write_reg(dev, 0X10, 0X08);
            break;
        case RD04_ADC_SF_16KHz:
            rd04_write_reg(dev, 0X10, 0X02);
            break;
    }
   

}
/**
 * @brief AxkRD04SetTransmittingPower
 *     设置发射功率（Set the transmission power of Rd-04）
 * @param Tpower
*/
void AxkRD04SetTransmittingPower(struct rd04_dev *dev, rd04_tpower_t Tpower)
{
    
mdelay(10);
    rd04_write_reg(dev, 0X03, 0X40+Tpower);
    
}
/**
 * @brief AxkRD04SetInductionThreshold
 *  设置感应门限，当到达门限值之后，控制IO VAL 输出(Set induction threshold，After reaching the threshold value, control the IO VAL output)
 * @param IndTs
*/
void AxkRD04SetInductionThreshold(struct rd04_dev *dev, uint16_t IndTs)
{
   
mdelay(10);
    rd04_write_reg(dev, 0X18, (IndTs&0XFF));
    rd04_write_reg(dev, 0X19, (IndTs>>8)&0XFF);
   
}
/**
 * @brief AxkRD04SetNoiseUpdate
 *
 * @param noiseupdate
*/
void AxkRD04SetNoiseUpdate(struct rd04_dev *dev, uint16_t noiseupdate)
{
    
mdelay(10);
    rd04_write_reg(dev, 0X1A, (noiseupdate&0XFF));
    rd04_write_reg(dev, 0X1B, (noiseupdate>>8)&0XFF);
    
}
/**
 * @brief AxkRD04SetInductionDelay
 *  设置感应延时时间(Set induction delay time)
 * @param _delay_ms
*/
void AxkRD04SetInductionDelayTime(struct rd04_dev *dev, uint32_t _delay_ms)
{
    uint32_t timer_hex = 0;
    timer_hex = _delay_ms*32;
 
mdelay(10);
    rd04_write_reg(dev, 0X1D, (timer_hex&0XFF));
    rd04_write_reg(dev, 0X1E, (timer_hex>>8)&0XFF);
    rd04_write_reg(dev, 0X1F, (timer_hex>>16)&0XFF);

}
/**
 * @brief AxkRD04SetBlockadeTime
 *  设置封锁时间（Set blocking time）
 * @param _delay_ms
*/
void AxkRD04SetBlockadeTime(struct rd04_dev *dev, uint32_t _delay_ms)
{
    uint32_t timer_hex = 0;
    timer_hex = _delay_ms*32;
    
mdelay(10);
    rd04_write_reg(dev, 0X20, (timer_hex&0XFF));
    rd04_write_reg(dev, 0X21, (timer_hex>>8)&0XFF);
    rd04_write_reg(dev, 0X22, (timer_hex>>16)&0XFF);

}



void axk_rd04_default_config(struct rd04_dev *dev)
{
    char buff = 0;
    int i;

mdelay(10);
    for (i = 0; i < 5; i++)
    {
        if (rd04_write_reg(dev, 0x13, 0x9b) >= 0)
        mdelay(10);
        else break;
        

        buff = rd04_read_reg(dev, 0x13);
    mdelay(10);
    }
    rd04_write_reg(dev, 0x24, 0x03);
    rd04_write_reg(dev, 0x04, 0x20);
    rd04_write_reg(dev, 0x10, 0x20);
    rd04_write_reg(dev, 0x03, 0x45);
    rd04_write_reg(dev, 0x1C, 0x21);

    rd04_write_reg(dev, 0x18, 0x5a);
    rd04_write_reg(dev, 0x19, 0x01);

    rd04_write_reg(dev, 0x1A, 0x55);
    rd04_write_reg(dev, 0x1B, 0x01);

    rd04_write_reg(dev, 0x1D, 0x80);
    rd04_write_reg(dev, 0x1E, 0x0C);
    rd04_write_reg(dev, 0x1F, 0x00);

    rd04_write_reg(dev, 0x20, 0x00);
    rd04_write_reg(dev, 0x21, 0x7D);
    rd04_write_reg(dev, 0x22, 0x00);

    rd04_write_reg(dev, 0x23, 0x0C);

mdelay(1000000);
mdelay(1000000);
mdelay(1000000);

}
/*****************************************************************************************/
/*
* @description : 打开设备
* @param – inode : 传递给驱动的 inode
* @param - filp : 设备文件，file 结构体有个叫做 private_data 的成员变量
* 一般在 open 的时候将 private_data 指向设备结构体。
* @return : 0 成功;其他 失败
*/
static int rd04_open(struct inode *inode, struct file *filp)
{

    /* 从 file 结构体获取 cdev 指针，再根据 cdev 获取 rd04_dev 首地址 */
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct rd04_dev *rd04dev = container_of(cdev,struct rd04_dev, cdev);

    /* 打开i2c设备的从设备 rd04 */
    rd04_write_reg(rd04dev, RD_04_I2C_ADDR, AXK_RD04_WR); //系统配置复位
    mdelay(50); 

    axk_rd04_default_config(rd04dev);
    AxkRD04SetTransmittingPower(rd04dev, RD04_TPOWER_5);
    AxkRD04SetInductionDelayTime(rd04dev, 1000);
    AxkRd04SetIoValOutput(rd04dev, 1);
    // AxkRD04SetInductionThreshold();

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
static ssize_t rd04_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{

    char data[5];
    long err = 0;

    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct rd04_dev *rd04dev = container_of(cdev, struct rd04_dev,cdev);

    rd04_write_reg(rd04dev, RD_04_I2C_ADDR, AXK_RD04_RD); //对设备读
    sprintf(data, (0x07==rd04_read_reg(rd04dev, 0x24))? "HIGH": "LOW" );

    err = copy_to_user(buf, data, sizeof(data));
    /*IF ERR*/
    return 0;

}

/*
210 * @description : 关闭/释放设备
211 * @param - filp : 要关闭的设备文件(文件描述符)
212 * @return : 0 成功;其他 失败
213 */
static int rd04_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* rd04 操作函数 */
const struct file_operations rd04_ops = {
    .owner = THIS_MODULE,
    .open = rd04_open,
    .read = rd04_read,
    .release = rd04_release,
};