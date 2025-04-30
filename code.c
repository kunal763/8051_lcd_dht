#include <at89x51.h>
#include <string.h>

sbit DHT11 = P2^0;
#define LCD_Port P1
sbit rs = P2^5;
sbit rw = P2^6;
sbit en = P2^7;

#define VERSION "1.4"

void timer_delay20ms16mhz(void);
void timer_delay30us16mhz(void);
void delay(unsigned int count);
void LCD_Command(char cmnd);
void LCD_Char(char char_data);
void LCD_String(char *str);
void LCD_String_xy(char row, char pos, char *str);
void LCD_Init(void);
void Request(void);
void Response(void);
unsigned char Receive_data(void);
void intToStr(char *buf, int value);

void timer_delay20ms16mhz(void)
{
    TMOD = 0x01;
    TH0 = 0x97;
    TL0 = 0xF4;
    TR0 = 1;
    while (TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

void timer_delay30us16mhz(void)
{
    TMOD = 0x01;
    TH0 = 0xFF;
    TL0 = 0xDB;
    TR0 = 1;
    while (TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

void delay(unsigned int count)
{
    int i, j;
    for (i = 0; i < count; i++)
        for (j = 0; j < 112; j++);
}

void LCD_Command(char cmnd)
{
    LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);
    rs = 0;
    rw = 0;
    en = 1;
    delay(1);
    en = 0;
    delay(2);
    LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);
    en = 1;
    delay(1);
    en = 0;
    delay(5);
}

void LCD_Char(char char_data)
{
    LCD_Port = (LCD_Port & 0x0F) | (char_data & 0xF0);
    rs = 1;
    rw = 0;
    en = 1;
    delay(1);
    en = 0;
    delay(2);
    LCD_Port = (LCD_Port & 0x0F) | (char_data << 4);
    en = 1;
    delay(1);
    en = 0;
    delay(5);
}

void LCD_String(char *str)
{
    int i;
    for (i = 0; str[i] != 0; i++)
        LCD_Char(str[i]);
}

void LCD_String_xy(char row, char pos, char *str)
{
    if (row == 0)
        LCD_Command((pos & 0x0F) | 0x80);
    else if (row == 1)
        LCD_Command((pos & 0x0F) | 0xC0);
    LCD_String(str);
}

void LCD_Init(void)
{
    delay(20);
    LCD_Command(0x02);
    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
    LCD_Command(0x80);
}

void Request(void)
{
    DHT11 = 0;
    timer_delay20ms16mhz();
    DHT11 = 1;
}

void Response(void)
{
    unsigned char try_count = 0;
    while (DHT11 == 1)
        if (++try_count == 0) return;
    try_count = 0;
    while (DHT11 == 0)
        if (++try_count == 0) return;
    try_count = 0;
    while (DHT11 == 1)
        if (++try_count == 0) return;
}

unsigned char Receive_data(void)
{
    unsigned char i, try_count, hdata;
    try_count = 0;
    hdata = 0;
    for (i = 0; i < 8; i++)
    {
        TL0 = 0;
        while (DHT11 == 0)
        {
            if (++try_count == 0)
                return 0;
        }
        TR0 = 1;
        try_count = 0;
        while (DHT11 == 1)
        {
            if (++try_count == 0)
                return 0;
        }
        TR0 = 0;
        if (TL0 >= 50)
            hdata = (hdata << 1) | 0x01;
        else
            hdata = (hdata << 1);
    }
    return hdata;
}

void intToStr(char *buf, int value)
{
    int i = 0;
    int len, j;
    char temp;
    if (value == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        while (value > 0)
        {
            buf[i++] = '0' + (value % 10);
            value /= 10;
        }
    }
    buf[i] = '\0';
    len = i;
    for (j = 0; j < len / 2; j++)
    {
        temp = buf[j];
        buf[j] = buf[len - j - 1];
        buf[len - j - 1] = temp;
    }
}

void main(void)
{
    unsigned char dat[20];
    char intPart[4], decPart[4];
    int I_RH, D_RH, I_Temp, D_Temp, CheckSum;

    LCD_Init();
    LCD_String_xy(0, 0, "LCD Test");

    strcpy(dat, "Ver=");
    strcat(dat, VERSION);
    LCD_String_xy(1, 0, dat);

    delay(2000);

    while (1)
    {
        memset(dat, 0, 20);

        TMOD = 0x01;
        TH0 = 0;
        TL0 = 0;

        Request();
        Response();

        I_RH = Receive_data();
        D_RH = Receive_data();
        I_Temp = Receive_data();
        D_Temp = Receive_data();
        CheckSum = Receive_data();

        intToStr(intPart, I_RH);
        intToStr(decPart, D_RH);
        strcpy(dat, "Hum=");
        strcat(dat, intPart);
        strcat(dat, ".");
        strcat(dat, decPart);
        LCD_String_xy(0, 0, dat);

        intToStr(intPart, I_Temp);
        intToStr(decPart, D_Temp);
        strcpy(dat, "Tem=");
        strcat(dat, intPart);
        strcat(dat, ".");
        strcat(dat, decPart);
        LCD_String_xy(1, 0, dat);

        intToStr(dat, CheckSum);
        if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum)
            strcat(dat, "!");

        LCD_String_xy(1, 12, dat);

        DHT11 = 1;
        delay(15000);
        LCD_Command(0x01);
    }
}