/**************************************
*文件名：   test_APP.c
*作者:      lzj
*版本：     v1.0
*描述：     led打开关闭测试app
*使用方法 ：    ./led_App /dev/led 0 //关闭LED
                ./led_App /dev/led 1 //打开LED
**************************************/
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
 
#define LEDOFF 0
#define LEDON 1

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
    unsigned char databuf[1];
    
    if(argc != 3)
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

    databuf[0] = atoi(argv[2]); /* 要执行的操作：打开或关闭 */
    /* 向/dev/led 文件写入数据 */
    retvalue = write(fd, databuf, sizeof(databuf));
    if(retvalue < 0)
    {
        printf("LED Control Failed!\r\n");
        close(fd);
        return -1;
    }

    retvalue = close(fd); /* 关闭文件 */
    if(retvalue < 0)
    {
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}
