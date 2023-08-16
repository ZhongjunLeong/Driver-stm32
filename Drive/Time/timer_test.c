/**************************************
*文件名：   timer_test.c
*作者:      lzj
*版本：     v1.0
*描述：     led打开关闭测试app
*使用方法 ：    ./timertest /dev/timer
**************************************/
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <sys/ioctl.h>
#define LEDOFF 0
#define LEDON 1

/* 命令值 */
 #define CLOSE_CMD (_IO(0XEF, 0x1)) /* 关闭定时器 */
 #define OPEN_CMD (_IO(0XEF, 0x2)) /* 打开定时器 */
 #define SETPERIOD_CMD (_IO(0XEF, 0x3)) /* 设置定时器周期命令 */

/*
* @description : main 主程序
* @param - argc : argv 数组元素个数
* @param - argv : 具体参数
* @return : 0 成功;其他 失败
*/
int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];

    
    if(argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    /* 打开 led 驱动 */
    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("file %s open failed!\r\n", argv[1]);
        return -1;
    }

    while (1) 
    {
        int ret;
        printf("Input CMD:");
        ret = scanf("%d", &cmd);
        if (ret != 1) 
        { /* 参数输入错误 */
            fgets(str, sizeof(str), stdin); /* 防止卡死 */
        }
        if(4 == cmd) /* 退出 APP */
            goto out;
        if(cmd == 1) /* 关闭 LED 灯 */
            cmd = CLOSE_CMD;
        else if(cmd == 2) /* 打开 LED 灯 */
            cmd = OPEN_CMD;
        else if(cmd == 3) 
        {
            cmd = SETPERIOD_CMD; /* 设置周期值 */
            printf("Input Timer Period:");
            ret = scanf("%d", &arg);
            if (ret != 1) 
            { /* 参数输入错误 */
                fgets(str, sizeof(str), stdin); /* 防止卡死 */
            }
        }
        ioctl(fd, cmd, arg); /* 控制定时器的打开和关闭 */
    }

 out:
 close(fd);
 }

