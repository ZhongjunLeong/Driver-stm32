/*1、platform框架，通过与设备树匹配成功，①gpio子系统的初始化获得gpio-key编号。②注册中断号。③执行input_dev驱动注册及初始化//input系统无需注册字符设备
 2、中断处理函数（当按键按下触发中断进入中断处理函数）：定时--消抖，
 3、定时处理函数：上报事件：上报按键状态*/

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

#define KEY_NAME		"key"	/* 名字 		*/

/* key设备结构体 */
struct key_dev{
	struct input_dev *idev; /* 按键对应的 input_dev 指针*/
	int gpio_key;				/* key所使用的GPIO编号		*/
	struct timer_list timer;	/* 消抖定时器		*/
	int irq_key;				/*按键对应的中断号*/

};

static struct key_dev key;          /* 按键设备 */

/*
* @description : 按键中断服务函数
* @param – irq : 触发该中断事件对应的中断号
* @param – arg : arg 参数可以在申请中断的时候进行配置
* @return : 中断执行结果
*/
static irqreturn_t key_interrupt(int irq, void *dev_id) //中断处理函数
{
	if(key.irq_key != irq)
		return IRQ_NONE;

	/* 按键防抖处理，开启定时器延时15ms */
	disable_irq_nosync(irq);	//禁止按键中断
	mod_timer(&key.timer, jiffies + msecs_to_jiffies(15));
    return IRQ_HANDLED;
}

/*
 * @description	: 初始化按键--platform框架下gpio子系统+input子系统
 * @param --nd		: device_none设备指针
 * @return 		: 成功0，
 */
static int gpio_key_init(struct device_node *nd)
{
	int ret;
	unsigned long irq_flags;

	/* 1、 获取设备树中的gpio属性，得到KEY0所使用的KYE编号 */
	key.gpio_key = of_get_named_gpio(nd, "key-gpio", 0);
	if(!gpio_is_valid(key.gpio_key)) {
		printk("can't get key-gpio");
		return -EINVAL;
	}
	/* 2\申请使用 GPIO */
	ret = gpio_request(key.gpio_key, "KEY0");
	if (ret) 
	{
		printk(KERN_ERR "key: Failed to request key-gpio\n");
		return ret;
	} 
	/* 3、将 GPIO 设置为输入模式 */
	gpio_direction_input(key.gpio_key);

    /* 4 、获取GPIO对应的中断号 */
    key.irq_key = irq_of_parse_and_map(nd, 0);  //后面使用这个中断号去申请以及释放中断
    if(!key.irq_key){
        return -EINVAL;
    }
	
	/* 5\获取设备树中指定的中断触发类型 */
	irq_flags = irq_get_trigger_type(key.irq_key);
	if (IRQF_TRIGGER_NONE == irq_flags)
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;
		
	/* 6\申请中断 */
	ret = request_irq(key.irq_key, key_interrupt, irq_flags, "Key0_IRQ", NULL);
	if (ret) {
        gpio_free(key.gpio_key);
        return ret;
    }
	
	return 0;
}
/*
 * @description : 定时器服务函数，用于按键消抖，定时时间到了以后再读取按键值，根据按键的状态上报相应的事件
 * @param – arg : arg 参数就是定时器的结构体
 * @return : 无
 */
static void key_timer_function(struct timer_list *arg)
{
    int val;

    /* 读取按键值并判断按键当前状态 */
    val = gpio_get_value(key.gpio_key);
    input_report_key(key.idev, KEY_0, !val);
	input_sync(key.idev);

	enable_irq(key.irq_key);

}

/*
 * @description : platform 驱动的 probe 函数，当驱动与设备匹配成功以后此函数会被执行
 * @param – pdev : platform 设备指针
 * @return : 0，成功;其他负值,失败
 */
static int atk_key_probe(struct platform_device *pdev)
{
 	int ret;
 
 	/* 初始化 GPIO */
	ret = gpio_key_init(pdev->dev.of_node);
	if(ret < 0)
		return ret;

	/* 初始化定时器 */
	timer_setup(&key.timer, key_timer_function, 0);
	
	/* 申请 input_dev */
	key.idev = input_allocate_device();
	key.idev->name = KEY_NAME;
	
 #if 0
	/* 初始化 input_dev，设置产生哪些事件 */
	__set_bit(EV_KEY, key.idev->evbit); /* 设置产生按键事件 */
	__set_bit(EV_REP, key.idev->evbit); /* 重复事件，比如按下去不放开，就会一直输出信息 */

	/* 初始化 input_dev，设置产生哪些按键 */
	__set_bit(KEY_0, key.idev->keybit);
#endif

#if 0
	key.idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	key.idev->keybit[BIT_WORD(KEY_0)] |= BIT_MASK(KEY_0);
#endif

	key.idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	input_set_capability(key.idev, EV_KEY, KEY_0);

 /* 注册输入设备 */
	ret = input_register_device(key.idev);
	if (ret) 
	{
		printk("register input device failed!\r\n");
		goto free_gpio;
	}
 
	return 0;
	
	free_gpio:
		free_irq(key.irq_key,NULL);
		gpio_free(key.gpio_key);
		del_timer_sync(&key.timer);
	return -EIO;
	
 }

 /*
 * @description : platform 驱动的 remove 函数，当 platform 驱动模块卸载时此函数会被执行
* @param – dev : platform 设备指针
* @return : 0，成功;其他负值,失败
*/
static int atk_key_remove(struct platform_device *pdev)
{
	free_irq(key.irq_key,NULL); /* 释放中断号 */
	gpio_free(key.gpio_key); /* 释放 GPIO */
	del_timer_sync(&key.timer); /* 删除 timer */
	input_unregister_device(key.idev); /* 释放 input_dev */
	
	return 0;
}


/* 匹配列表 */
 static const struct of_device_id key_of_match[] = {
    {.compatible = "alientek,key"}, //与设备树的设备节点compatible对应
    { /*sentinel*/}
 };
   // MODULE_DEVICE_TABLE(of, key_of_match);//声明led_of_match这个匹配表
 /* platform 驱动结构体 */
static struct platform_driver atk_key_driver = {
    .driver = {
        .name = "stm32mp1-key", //驱动名字，会在/sys/bus/platform/drivers/目录下存在一个名为“stm32mp1-key”的文件
        .of_match_table = key_of_match, //设备树匹配表
    },
    .probe = atk_key_probe,
    .remove = atk_key_remove,
};
module_platform_driver(atk_key_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");