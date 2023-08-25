#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/*命令行输入：./main /dev/out_gpio /dev/RD04*/

/*读取雷达数据*/
static void *go_rd04(void *s)
{
    int fd;
    char *filename;
    char *databuf;
    char out[10];
    int ret = 0;

    filename = argv[2];
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
   int fd, retvalue;
    char *filename;
    int databuf;
    pthread_t pid;

    if(argc != 3)
    {
        printf("Error Usage:%s /dev/out_gpio!\r\n", argv[0]);
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

    
    /* 读/dev/out_gpio  */
    retvalue = read(fd, &databuf, sizeof(int));
    if(retvalue < 0)
    {
        printf("LED Control Failed!\r\n");
        close(fd);
        return -1;
    }
    if(databuf == 1)
    {
      printf("有人\n");
    }
    else
    {
      printf("没人\n");
    }

    err = pthread_create(&pid, NULL, go_rd04, NULL);
    if(err < 0)
    {
      fprintf(stderr, "pthread_create fail :%s", sterror(errno));
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