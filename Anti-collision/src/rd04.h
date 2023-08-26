/**
 * @file rd_04_driver.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-05-31
 *
 * @copyright Copyright (c) 2023
 *
*/

#ifndef RD04_H__
#define RD04_H__

#define rd04_CNT    1
#define rd04_NAME   "RD04"

typedef enum {
    RD04_PSM_DEF = 0X20,//Pulse gap operation
    RD04_PSM_CONTINUOUS = 0XA0, //Continuous operation
}rd04_psm_t;

typedef enum {
    RD04_ADC_SF_1KHz = 0,
    RD04_ADC_SF_2KHz,
    RD04_ADC_SF_4KHz,
    RD04_ADC_SF_16KHz,
}rd04_adc_sf_t;

typedef enum {
    RD04_TPOWER_0 = 0,  //发射功率(0) -- 124.1uA@6V 靠近感应反应正常  --- 发射功率最大 (Transmission power (0)-- 124.1uA@6V Proximity induction response normal - maximum emission power)
    RD04_TPOWER_1,      //发射功率(1) -- 123.2uA@6V 靠近感应反应正常(Transmission power (1)-- 123.2uA@6V Proximity induction response is normal)
    RD04_TPOWER_2,      //发射功率(2) -- 121.1uA@6V 靠近感应反应正常(Transmission power (2)-- 121.1uA@6V Proximity induction response is normal)
    RD04_TPOWER_3,      //发射功率(3) -- 119.7uA@6V 靠近感应反应正常(Transmission power (3)-- 119.7uA@6V Proximity induction response is normal)
    RD04_TPOWER_4,      //发射功率(4) -- 117.9uA@6V 靠近感应反应正常(Transmission power (4)-- 117.9uA@6V Proximity induction response is normal)
    RD04_TPOWER_5,      //发射功率(5) -- 115.2uA@6V 靠近感应反应正常(Transmission power (5)-- 115.2uA@6V Proximity induction response is normal)
    RD04_TPOWER_6,      //发射功率(6) -- 114.5uA@6V 靠近感应反应正常（很近）(Transmission power (6)-- 114.5uA@6V Proximity induction response normal (very close))
    RD04_TPOWER_7       //发射功率(7) -- 108.5uA@6V 靠近感应无反应(Transmission power (7)-- 108.5uA@6V Proximity induction without response)
}rd04_tpower_t;


extern const struct file_operations rd04_ops;
/*i2c从设备结构体*/
struct rd04_dev
{
    struct i2c_client *client;  //i2c设备号
    dev_t devid;    //设备号
    struct cdev cdev;  //cdev
    struct class *class;    //类
    struct device *device;
    struct device_node  *nd;    //设备节点
};

//uint8_t axk_rd04_i2c_init(void);    //i2c初始化
void axk_rd04_default_config(struct rd04_dev* ); //默认配置
//void axk_rd04_display_config(void);     //显示配置
void AxkRd04SetIoValOutput(struct rd04_dev* , uint8_t OutputStatus);   //设置结果输出参数
void AxkRd04SetADCSamplingFrequency(struct rd04_dev*, rd04_adc_sf_t ADC_SF);  //设置频率
void AxkRD04SetTransmittingPower(struct rd04_dev*, rd04_tpower_t Tpower); //设置传送电源
void AxkRD04SetInductionThreshold(struct rd04_dev *, uint16_t IndTs);  //设置感应阈值
void AxkRD04SetNoiseUpdate(struct rd04_dev *, uint16_t noiseupdate);   //设置噪声上传
void AxkRD04SetInductionDelayTime(struct rd04_dev *, uint32_t _delay_ms);  //设置感应延迟时间
void AxkRD04SetBlockadeTime(struct rd04_dev *, uint32_t _delay_ms);    //设置阻塞时间
#endif
