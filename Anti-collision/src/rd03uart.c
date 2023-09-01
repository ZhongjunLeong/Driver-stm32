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
#include <termios.h>
#include <stdint.h>

#define roiMin 0X00                     //最小探测距离门
#define roiMax 0X01                     //最远探测距离门

/*串口波特率的设置函数，opt=termios结构体，baudrate=要设置的波特率*/
static void set_baudrate(struct termios *opt, unsigned int baudrate)
{
    cfsetispeed(opt, baudrate);
    cfsetospeed(opt, baudrate);
    printf("Set Baudrate to %d \r\n",baudrate);
}

/*串口数据位的设置函数，opt=termios结构体，databit=要设置的数据位*/
static void set_data_bit(struct termios *opt, unsigned int databit)
{
    opt->c_cflag &= ~CSIZE;

    switch (databit)
    {
    case 8:
        opt->c_cflag |= CS8;
        printf("Set data bit to 8 !\r\n");
        break;
    case 7:
        opt->c_cflag |= CS7;
        printf("Set data bit to 7 !\r\n");
    case 6:
        opt->c_cflag |= CS6;
        printf("Set data bit to 6 !\r\n");
    case 5:
        opt->c_cflag |= CS5;
        printf("Set data bit to 5 !\r\n");    
    default:
        opt->c_cflag |= CS8;
        printf("Default : Set data bit to 8 !\r\n");
        break;
    }
}

/*串口的检验位设置，opt=termios结构体，parity可选 'N' 'E 'O' 分别代表不启用校验或启用奇、偶校验*/
static void set_parity(struct termios *opt, char parity)
{
    switch (parity)
    {
    case 'N'/* constant-expression */:
        /* 不启用奇偶校验*/
        opt->c_cflag &= ~PARENB;
        printf("Dont enable PARITY! \r\n");
        break;

    case 'E':
        /* 启动奇校验*/
        opt->c_cflag |= PARENB;
        opt->c_cflag &= ~PARODD;
        printf("Enable PARITY to Even! \r\n");
        break;

    case 'O':
        /*起动偶校验*/
        opt->c_cflag |= PARENB;
        opt->c_cflag |= ~PARODD;
        printf("Enable PARITY to Odd! \r\n");
        break;

    default:
        /* 不启用奇偶校验*/
        opt->c_cflag &= ~PARENB;
        printf("Default : Dont enable PARITY! \r\n");
        break;
    }
}

/*串口的停止位设置，opt=termios结构体，stopbit可选 '1' '2' 分别代表1位、2位停止位*/
static void set_stopbit(struct termios *opt, const char *stopbit)
{
    if( 0 == strcmp(stopbit, "1"))
    {
        opt->c_cflag &= ~CSTOPB; /*1 stop bit*/
        printf("Set Stop bit is 1 \r\n");
    }else if(0 == strcmp(stopbit,"2"))
    {
        opt->c_cflag |= CSTOPB;/*2 stop bit*/
        printf("Set Stop bit is 1 \r\n");
    }else
    {
        opt->c_cflag &= ~CSTOPB;
        printf("Default : Stop bit is 1 \r\n");
    }
}

/*串口属性设置*/
int set_port_attr(int fd, int baudrate, int databit, const char *stopbit, char parity, int vtime, int vmin)
{
    
    struct termios opt;
    tcgetattr(fd, &opt);
    
    set_baudrate(&opt, baudrate);
    opt.c_cflag |= CLOCAL | CREAD;

    set_data_bit(&opt, databit);
    set_parity(&opt, parity);
    set_stopbit(&opt, stopbit);

    opt.c_oflag = 0;
    opt.c_lflag |= 0;
    opt.c_oflag &= ~OPOST;
    opt.c_cc[VTIME] = vtime;
    opt.c_cc[VMIN] = vmin;
    
    tcflush(fd, TCIFLUSH);
    return (tcsetattr(fd, TCSANOW, &opt));
}


static int inter_cmd(int fd)//进入命令模式
{
    int ret;

    uint8_t cmd[]={0xfd,0xfc,0xfb,0xfa,0x04,0x00,0xff,0x00,0x01,0x00,0x04,0x03,0x02,0x01};

    ret = write(fd, cmd, sizeof(cmd));
    if(ret < 0)
    {
            printf("UART WRITE FAILED:enter_cmd1 \r\n");
            return -1;
    }
   
    usleep(150);
    
    ret = write(fd, cmd, sizeof(cmd));
    if(ret < 0)
    {
            printf("UART WRITE FAILED:enter_cmd2 \r\n");
            return -1;
    }
    
}

static int close_cmd(int fd)//关闭命令模式
{
    int ret;

    uint8_t close_cmd[]={0xfd,0xfc,0xfb,0xfa,0x02,0x00,0xfe,0x00,0x04,0x03,0x02,0x01};
    
    ret = write(fd, close_cmd, sizeof(close_cmd));
    if(ret < 0)
    {
            printf("UART WRITE FAILED:close_cmd \r\n");
            return -1;
    }
}

int static RD_03_Write_cmd(uint8_t parameter, uint8_t data, int fd)//写入命令，参数1参考RD_03.h中的宏定义，四个可调参数可选，参数2位数据，大小为0-15
{
    int ret; 

    ret = inter_cmd(fd);
    if(ret < 0)
    {
        printf("inter fail\n");
    }

    uint8_t Write_cmd[]={0xfd,0xfc,0xfb,0xfa,0x08,0x00,0x07,0x00,parameter,0x00,data,0x00,0x00,0x00,0x04,0x03,0x02,0x01};

    ret = write(fd, Write_cmd, sizeof(Write_cmd));
    if(ret < 0)
    {
            printf("UART WRITE FAILED:close_cmd \r\n");
            return -1;
    }
   
    close_cmd(fd);

    return 0;
}

int main(int argc, char* argv[])
{

    char *filename;
    int fd;
    int ret;
    
    if(argc != 2)
    {
        printf("please uage: ./%s /dev.ttySTM2", argv[0]);
        return -1;
    }
    filename = argv[1];
    /*打开uart驱动/dev/ttySTM2*/
    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
       perror("open()");
       return -2;
    }
    
    set_port_attr(fd, 115200, 8, "1",'N', 0, 0); /*设置串口属性*/

    ret = RD_03_Write_cmd(roiMax, 2, fd);//雷达数据初始化写入，这里设置最大门限距离为2
    if(ret < 0)
    {
        printf("rd data write fail\n");
    }

    /*读uart驱动文件写*/
   while(1)
   {
    /*打开结果输出引脚GPIO文件*/

    //read();
   }
    return 0;
}