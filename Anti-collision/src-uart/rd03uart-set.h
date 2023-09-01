#ifndef RD03UART_H__
#define RD03UART_H__

static int inter_cmd(int);
static int close_cmd(int);
static int RD_03_Write_cmd(uint8_t parameter, uint8_t data, int);
static int RD_03_systemMode(uint8_t parameter, int fd);

typedef struct RD_03_DATA
{
    uint8_t head[4];
    uint8_t data_size;//数据长度
    uint8_t result; //检测结果 
    uint16_t range; //目标距离
    uint16_t energy[16]; //各距离门能量
    uint8_t tail[4];
     
}RD_03_DATATypedef;

extern RD_03_DATATypedef rd03data;

#endif // !RD03UART_H__
