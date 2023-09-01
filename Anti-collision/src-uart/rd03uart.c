/*************************
**uart驱动rd03雷达对人体存在的检测,有人时，向M4发送信号，接收M4发来的电机stop信号
*使用方法：
1、加载gpio.ko模块
2、进入/lib/firmware/中执行./test.sh start RPMsg_PWM_CM4.elf，启动M4
3、执行./rd03uart /dev/ttySTM1 /dev/rd04gpio1
******************************/
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

    switch (databit)    //设置几位的数据位
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
    tcgetattr(fd, &opt);    //获取终端当前的配置参数
    
    set_baudrate(&opt, baudrate);   //设置波特率
    opt.c_cflag |= CLOCAL | CREAD;  //控制模式、接收使能

    set_data_bit(&opt, databit);
    set_parity(&opt, parity);
    set_stopbit(&opt, stopbit);

    opt.c_oflag = 0;
    opt.c_lflag |= 0;
    opt.c_oflag &= ~OPOST;
    opt.c_cc[VTIME] = vtime;    //设置非规范模式，read调用非阻塞方式，即时有效
    opt.c_cc[VMIN] = vmin;
    
    tcflush(fd, TCIFLUSH);  //缓冲区处理，清空输入输出缓冲区数据
    return (tcsetattr(fd, TCSANOW, &opt));//写入配置，使生效
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

int static RD_03_Write_cmd(uint8_t parameter, uint8_t data, int fd)//写入命令，参数1设置探测距离门，参数2位数据，大小为0-15设置
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
    int fd, fd_gpio, fd_msg;
    int ret;
    int buff;
    char result[1024] = {0};
    int flags;

    if(argc != 3)
    {
        printf("please uage: ./%s /dev/ttySTM2 /dev/rd04gpio1", argv[0]);
        return -1;
    }
    filename = argv[1];
    /*打开uart驱动/dev/ttySTM1*/
    fd = open(filename, O_RDWR |O_NONBLOCK);
    if(fd < 0)
    {
       perror("open()");
       close(fd);
       return -2;
    }
    
    set_port_attr(fd, 115200, 8, "1",'N', 0, 0); /*设置串口属性*/

    ret = RD_03_Write_cmd(roiMax, 2, fd);//雷达数据初始化写入，这里设置最远探测距离门2
    if(ret < 0)
    {
        printf("rd data write fail\n");
    }

    printf("open rd03 success\n");
    /*打开结果输出引脚GPIO文件/dev/rd04gpio1*/
    fd_gpio = open(argv[2], O_RDWR);
    if(fd_gpio < 0)
    {
       perror("gpio:open()");
       close(fd_gpio);
       return -3;
    }
   /*打开异核通信文件/dev/ttyRPMSG0*/
    fd_msg = open("/dev/ttyRPMSG0", O_RDWR|O_NONBLOCK);
    if(fd_msg < 0)
    {
       perror("RPMSG:open()");
       close(fd_msg);
       return -3;
    }
#if 0
    result = malloc(sizeof(result));
    if(result == NULL)
    {
        perror("malloc fail");
    }

    /*将读文件的文件描述符设置为非阻塞的方式*/
    // 获取当前文件描述符的标志位
    flags = fcntl(fd_gpio, F_GETFL, 0);

    // 设置非阻塞标志位
    flags |= O_NONBLOCK;

    // 将修改后的标志位写回文件描述符
    if (fcntl(fd_gpio, F_SETFL, flags) == -1) 
    {
        perror("fcntl_gpio");
        return -1;
    }
  // 获取当前文件描述符的标志位
    flags = fcntl(fd_msg, F_GETFL, 0);

    // 设置非阻塞标志位
    flags |= O_NONBLOCK;

    // 将修改后的标志位写回文件描述符
    if (fcntl(fd_msg, F_SETFL, flags) == -1) 
    {
        perror("fcntl_msg");
        return -1;
    }
  #endif 
 /*读rd03输出结果*/
   while(1)
   {
    
        //read();
        ret = read(fd_gpio, &buff, sizeof(buff));
        if(ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                printf("No data available for non-blocking read:gpio.\n");
            }
            else 
            {
                perror("read");
                close(fd_gpio); 
                return -4;
            }
        
        } 
        else if(ret == 0)
        {
            //printf("No data available for non-blocking read:gpio.\n");
        }
       
        
            printf("gpioRead %zd bytes: %d\n", ret, buff);
        

    /*读取结果输出值*/
        if(buff == 1)
        {
            printf("有人\n");
            //memcpy(rebuf, "1", 2);
            ret = write(fd_msg, "1", sizeof("1"));
           if(ret < 0)
            {
                printf("ret: %d\n", ret);
                perror("RPMSG:wirte()");
                close(fd_msg);
                return -5;
            }

          
            ret = read(fd_msg, result, sizeof(result));
            if(ret < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) 
                {
                    printf("No data available for non-blocking read:msg.\n");
                }
                else 
                {
                    perror("read:msg");
                    close(fd_msg); 
                    return -4;
                }
            
            } 
            else if(ret == 0)
            {
               // printf("No data available for non-blocking read:msg.\n");
            }
          
            
                printf("msgRead %zd bytes: %s\n", ret, result);
            
         
        
            usleep(1000000);
        }
        else if(buff == 0)
        {
            printf("没人\n");
       
            ret = write(fd_msg, "0", sizeof("0"));
            if(ret < 0)
            {
                printf("ret: %d\n", ret);
                perror("RPMSG:wirte()");
                close(fd_msg);
                return -5;
            }
            ret = read(fd_msg, result, sizeof(result));
            if(ret < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) 
                {
                    printf("No data available for non-blocking read:msg.\n");
                }
                else 
                {
                    perror("read:msg");
                    close(fd_msg); 
                    return -4;
                }
            
            } 
            else if(ret == 0)
            {
               // printf("No data available for non-blocking read:msg.\n");
            }
            else
            {
                printf("msgRead %zd bytes: %s\n", ret, result);
            }

            usleep(1000000);
        }
        else
        {
            printf("都不是\n");
        }

    }
    free(result);
    close(fd_gpio);
    close(fd);
    close(fd_msg);
        return 0;
}