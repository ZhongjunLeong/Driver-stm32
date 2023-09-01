#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define PLATDRV_MAGIC       0x60     //魔术字
#define SET             _IO (PLATDRV_MAGIC, 0x18)   //没有数据传输的命令
#define OUT              _IO (PLATDRV_MAGIC, 0x19)

#define OPEN 1
#define CLOSE 0
/*命令行输入：./main /dev/out_gpio /dev/gpio_en /dev/RD04*/

/*读取雷达数据*/
static void *go_rd04(void *s)
{
    int fd;
    char *filename;
    char *databuf;
    char out[10];
    int ret = 0;

    filename = s;
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("can't open file %s\r\n", filename);
        return NULL;
    }

    while (1) {
        ret = read(fd, databuf, sizeof(databuf));
        if(ret == 0) 
        { /* 数据读取成功 */
           strncpy(out, databuf, sizeof(out));
           
          printf("接触输出情况：%s\r\n", out);
        }
        usleep(1000000); /*隔1s 读一次 */
    }

    close(fd); /* 关闭文件 */ 
}

int main(int argc, char *argv[])
{
   int fd_en, fd_out, retvalue;
    char *filename_out, *filename_en;
    int databuf;
    pthread_t pid;
    int err;
    if(argc != 4)
    {
        printf("Error Usage:%s /dev/gpio_en /dev/gpio_out /dev/RD04*/\r\n", argv[0]);
        return -1;
    }

    filename_en = argv[1];

    /* 打开 gpio-en 驱动 */
    fd_en = open(filename_en, O_RDWR);
    if(fd_en < 0)
    {
        printf("file %s open failed!\r\n", argv[1]);
        close(fd_en);
        return -1;
    }
    ioctl(fd_en, SET, OPEN);

    /*创建线程：执行rd04驱动文件*/
    err = pthread_create(&pid, NULL, go_rd04, argv[3]);
    if(err < 0)
    {
      fprintf(stderr, "pthread_create fail :%s", strerror(errno));
      return -1;
    }

    /*打开结果输出文件*/
    filename_out = argv[2];
    fd_out = open(filename_out, O_RDWR);
     if(fd_out < 0)
    {
        printf("file %s open failed!\r\n", argv[2]);
        close(fd_out);
        close(fd_en);
        return -1;
    }
    while(1)
    {

        /* 读/dev/out_gpio  */
        retvalue = ioctl(fd_out, OUT, 0);
        if(retvalue < 0)
        {
            printf("LED Control Failed!\r\n");
            close(fd_out);
            close(fd_en);
            return -1;
        }
        if(retvalue == 1)
        {
        printf("有人\n");
        }
        else
        {
        printf("没人\n");
        }

        usleep(1000000);
    }

    close(fd_out); /* 关闭文件 */
    close(fd_en);
   
    return 0;

}