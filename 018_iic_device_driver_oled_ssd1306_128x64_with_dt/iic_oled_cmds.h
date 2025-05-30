#ifndef __IIC_OLED_CMDS_H__
#define __IIC_OLED_CMDS_H__

#include "linux/delay.h"
#include "linux/i2c.h"

#define IOCTL_CMD_OLED_TURN_ON_INIT (_IO(0xAA, 0x01)) // 打开 OLED
#define IOCTL_CMD_OLED_TURN_OFF     (_IO(0xAA, 0x02)) // 关闭 OLED
#define IOCTL_CMD_OLED_SETCURSOR    (_IO(0xAA, 0x03)) // 设置光标位置
#define IOCTL_CMD_OLED_CLEAR        (_IO(0xAA, 0x04)) // 清屏
#define IOCTL_CMD_OLED_WRITE_STR    (_IO(0xAA, 0x05)) // 写字符串

// 不添加新的 C 文件所以在这里定义
static void iic_oled_write_command(struct i2c_client *client, int value) {
    // struct i2c_msg msgs[2] = {
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x00,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)value,
    //     },
    // };
    // i2c_transfer(client->adapter, msgs, 2);
    i2c_smbus_write_byte_data(client, 0x00, value);
}

static void iic_oled_write_data(struct i2c_client *client, int value) {
    struct i2c_msg msgs[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = (u8 *)0x40,
        },
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = (u8 *)value,
        },
    };
    i2c_transfer(client->adapter, msgs, 2);
}

int iic_oled_init(struct i2c_client *client);
int iic_oled_init(struct i2c_client *client) {
    int ret = 0;
    // OLED 上电要等待一会
    mdelay(100);

    // struct i2c_msg msgs[] = {
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xAE, // 关闭显示
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xA8, // 设置多路复用率
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x3F,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xD3, // 设置显示偏移
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x00,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x40, // 设置显示开始行
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xA1, // 设置左右方向，0xA1正常 0xA0左右反置
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xC8, // 设置上下方向，0xC8正常 0xC0上下反置
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xDA, // 设置COM引脚硬件配置
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x12,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x81, // 设置对比度控制
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xCF, // oled_write_command(0x7F); // 复位值默认 0x7F
    //     },
    //     /* {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x7F, // oled_write_command(0x7F); // 复位值默认 0x7F
    //     }, */
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xA4, // 设置整个显示打开/关闭
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xA6, // 设置正常/倒转显示
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xD5, // 设置显示时钟分频比/振荡器频率
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x80,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x8D, // 设置充电泵
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x14,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xD9, // 设置预充电周期
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xF1,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xDB, // 设置VCOMH取消选择级别
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0x30,
    //     },
    //     {
    //         .addr = client->addr,
    //         .flags = 0,
    //         .len = 1,
    //         .buf = (u8 *)0xAF, // 开启显示
    //     },
    // };

    // struct oled_cmd {
    //     unsigned char cmd_reg;
    //     unsigned char data;
    // };
    // unsigned char oled_init_cmds[] = {
    //     0x00, 0xAE, // 关闭显示
    //     0x00, 0xA8, // 设置多路复用率
    //     0x00, 0x3F,
    //     0x00, 0xD3, // 设置显示偏移
    //     0x00, 0x00,
    //     0x00, 0x40, // 设置显示开始行
    //     0x00, 0xA1, // 设置左右方向，0xA1正常 0xA0左右反置
    //     0x00, 0xC8, // 设置上下方向，0xC8正常 0xC0上下反置
    //     0x00, 0xDA, // 设置COM引脚硬件配置
    //     0x00, 0x12,
    //     0x00, 0x81, // 设置对比度控制
    //     0x00, 0xCF,
    //     // 0x00, 0x7F, // oled_write_command(0x7F); // 复位值默认 0x7F
    //     0x00, 0xA4, // 设置整个显示打开/关闭
    //     0x00, 0xA6, // 设置正常/倒转显示
    //     0x00, 0xD5, // 设置显示时钟分频比/振荡器频率
    //     0x00, 0x80,
    //     0x00, 0x8D, // 设置充电泵
    //     0x00, 0x14,
    //     // 0x00, 0xD9, // 设置预充电周期
    //     // 0x00, 0xF1,
    //     // 0x00, 0xDB, // 设置VCOMH取消选择级别
    //     // 0x00, 0x30,
    //     0x00, 0xAF, // 开启显示
    // };

    // use one buf to save all the command
    unsigned char oled_init_cmds[] = {
        0xAE, // 关闭显示
        0xA8, // 设置多路复用率
        0x3F,
        0xD3, // 设置显示偏移
        0x00,
        0x40, // 设置显示开始行
        0xA1, // 设置左右方向，0xA1正常 0xA0左右反置
        0xC8, // 设置上下方向，0xC8正常 0xC0上下反置
        0xDA, // 设置COM引脚硬件配置
        0x12,
        0x81, // 设置对比度控制
        0xCF,
        // 0x7F, // oled_write_command(0x7F); // 复位值默认 0x7F
        0xA4, // 设置整个显示打开/关闭
        0xA6, // 设置正常/倒转显示
        0xD5, // 设置显示时钟分频比/振荡器频率
        0x80,
        0x8D, // 设置充电泵
        0x14,
        // 0xD9, // 设置预充电周期
        // 0xF1,
        // 0xDB, // 设置VCOMH取消选择级别
        // 0x30,
        0xAF, // 开启显示
    };

    int cmd_size = sizeof(oled_init_cmds) / sizeof(oled_init_cmds[0]);
    for (int i = 0; i < cmd_size; i++) {
        // iic_oled_write_command(client, oled_init_cmds[i]);
        // printk("cmd: %02x\n", oled_init_cmds[i]);
        ret = i2c_smbus_write_byte_data(client, 0x00, oled_init_cmds[i]);
    }

    return ret;
}

int iic_oled_close(struct i2c_client *client);
int iic_oled_close(struct i2c_client *client) {
    int ret = 0;
    // 关闭显示
    ret = i2c_smbus_write_byte_data(client, 0x00, 0xAE);
    return ret;
}

void oled_set_cursor(struct i2c_client *client, uint8_t row, uint8_t col);
/**
  * @brief  OLED设置光标位置
  * @param  row 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  col 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void oled_set_cursor(struct i2c_client *client, uint8_t row, uint8_t col) {
	// (0xB0 | row);					//设置row位置
	// (0x10 | ((col & 0xF0) >> 4));	//设置col位置高4位
	// (0x00 | (col & 0x0F));			//设置col位置低4位
    //
    // use i2c client to send command
    i2c_smbus_write_byte_data(client, 0x00, (0xB0 | row));
    i2c_smbus_write_byte_data(client, 0x00, (0x10 | ((col & 0xF0) >> 4)));
    i2c_smbus_write_byte_data(client, 0x00, (col & 0x0F));
}

int iic_oled_clear(struct i2c_client *client);
int iic_oled_clear(struct i2c_client *client)
{
    int ret = 0;
    // 直接写0x00到所有行，清空屏幕
    for (int row = 0; row < 8; ++row) {
        oled_set_cursor(client, row, 0);
        for (int col = 0; col < 128; ++col) {
            // oled_write_data(0x00);
	        // HAL_I2C_Mem_Write(&hi2c1, 0x3c, 0x40, 1, 0x00, 1, 200);
	        // HAL_I2C_Mem_Write(&hi2c1, addr, command, 1, data, 1, 200);
            ret = i2c_smbus_write_byte_data(client, 0x40, 0x00);
        }
    }
    return ret;
}

// uint8_t ssd1306_oled_write_line(struct i2c_client *client, char* ptr)
// {
// }
//
// uint8_t sd1306_oled_write_string(struct i2c_client *client, char* ptr)
// {
// }


// 每行最大字符数 = 128 / 8 = 16
#define OLED_COLS 16
#define OLED_PAGES 8  // 64 / 8
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define FONT_BYTES 16

#include "oled_fonts.h"

// extern const uint8_t OLED_F8x16[]; // 字符字体库，每个字符占 16 字节
extern const unsigned int OLED_F8x16[];

static uint8_t global_x;
static uint8_t global_y;

uint8_t ssd1306_set_cursor(struct i2c_client *client, uint8_t col, uint8_t page);
uint8_t ssd1306_set_cursor(struct i2c_client *client, uint8_t col, uint8_t page) {
    int ret = 0;
    // uint8_t cmds[] = {
    //     0x00,                // 控制字节（命令）
    //     (uint8_t)(0xB0 + page),               // 设置页地址（Y 方向）
    //     (uint8_t)(0x00 + ((col * CHAR_WIDTH) & 0x0F)), // 设置低列地址
    //     (uint8_t)(0x10 + (((col * CHAR_WIDTH) >> 4) & 0x0F)) // 设置高列地址
    // };
    ret = i2c_smbus_write_byte_data(client, 0x00, (0xB0 + page));
    ret = i2c_smbus_write_byte_data(client, 0x00, (0x00 + ((col * CHAR_WIDTH) & 0x0F)));
    ret = i2c_smbus_write_byte_data(client, 0x00, (0x10 + (((col * CHAR_WIDTH) >> 4) & 0x0F)));
    return ret;
}

uint8_t ssd1306_set_cursor_row(struct i2c_client *client, uint8_t row);
uint8_t ssd1306_set_cursor_row(struct i2c_client *client, uint8_t row) {
    int ret = 0;
    ret = ssd1306_set_cursor(client, 0, row);
    if (ret < 0) {
        printk("ssd1306_set_cursor_row failed\n");
        return ret;
    }
    global_x = 0;
    global_y = row;
    return ret;
}

uint8_t ssd1306_write_char_8x16(struct i2c_client *client, char ch);
uint8_t ssd1306_write_char_8x16(struct i2c_client *client, char ch)
{
    if (ch < 0x20 || ch > 0x7E)
        return 1;

    uint8_t buf[CHAR_WIDTH + 1]; // 1 控制字节 + 8 字节数据
    uint8_t i;

    uint16_t font_index = (ch - 0x20) * FONT_BYTES;

    // 显示上半部分（第1页）
    ssd1306_set_cursor(client, global_x, global_y);
    buf[0] = 0x40; // 控制字节，数据
    for (i = 0; i < CHAR_WIDTH; i++) {
        buf[i + 1] = OLED_F8x16[font_index + i];
        i2c_smbus_write_byte_data(client, 0x40, buf[i+1]);
    }
    // _i2c_write(buf, CHAR_WIDTH + 1);

    // 显示下半部分（第2页）
    ssd1306_set_cursor(client, global_x, global_y + 1);
    buf[0] = 0x40;
    for (i = 0; i < CHAR_WIDTH; i++) {
        buf[i + 1] = OLED_F8x16[font_index + i + CHAR_WIDTH];
        i2c_smbus_write_byte_data(client, 0x40, buf[i+1]);
    }
    // _i2c_write(buf, CHAR_WIDTH + 1);

    // 移动到下一列
    global_x++;
    if (global_x >= OLED_COLS) {
        global_x = 0;
        global_y += 2;
        if (global_y >= OLED_PAGES)
            global_y = 0;
    }

    return 0;
}





/**
 * @brief 显示 16x16 的汉字
 * @param row 行 范围 1-
 * @param col 列 范围 1-8
 * @param ch 要显示的汉字，字库中的下标
 * @note 能显示 4 行刚好一行能显示 8 个 16x16 的字符
 * @note 相当于 4 行 8 列
 */
uint8_t ssd1306_write_char_16x16(struct i2c_client *client, uint8_t ch);
uint8_t ssd1306_write_char_16x16(struct i2c_client *client, uint8_t ch)
{
    uint8_t buf[CHAR_WIDTH + 1]; // 1 控制字节 + 8 字节数据
    uint8_t i;
    uint16_t font_index = ch * 32;
    // global_x -> col
    // global_y -> row

    // 显示上半部分的左半边（第1页）
    // oled_set_cursor((row - 1) * 2, (col - 1) * 16);
    ssd1306_set_cursor(client, global_x * 2, global_y * 2);
    buf[0] = 0x40; // 控制字节，数据
    for (i = 0; i < CHAR_WIDTH; i++) {
        buf[i + 1] = hanzi_16x16[font_index + i];
        i2c_smbus_write_byte_data(client, 0x40, buf[i+1]);
    }
    // 显示上半部分的右半边（第1页）
    // 会接着写到下一列，所以不需要设置游标位置
    buf[0] = 0x40;
    for (i = 0; i < CHAR_WIDTH; i++) {
        buf[i + 1] = hanzi_16x16[font_index + i + CHAR_WIDTH];
        i2c_smbus_write_byte_data(client, 0x40, buf[i+1]);
    }

    // 需要重新设置游标位置，因为要移动到下一行，也就是下一个 page
    // 每 8 bit 为一个 page

    // 这里 global_y + 1 实际是移动了一行，并不是按位移动
    ssd1306_set_cursor(client, global_x * 2, global_y * 2 + 1);
    // 显示下半部分的左半边（第2页）
    buf[0] = 0x40;
    for (i = 0; i < CHAR_WIDTH; i++) {
        buf[i + 1] = hanzi_16x16[font_index + i + (CHAR_WIDTH * 2)];
        i2c_smbus_write_byte_data(client, 0x40, buf[i+1]);
    }
    // 显示下半部分的右半边（第2页）
    buf[0] = 0x40;
    for (i = 0; i < CHAR_WIDTH; i++) {
        buf[i + 1] = hanzi_16x16[font_index + i + (CHAR_WIDTH * 3)];
        i2c_smbus_write_byte_data(client, 0x40, buf[i+1]);
    }

    // 移动到下一列
    global_x++;
    if (global_x >= 8) {
        global_x = 0;
        global_y += 1;
        if (global_y >= 4)
            global_y = 0;
    }

    return 0;
}

uint8_t ssd1306_write_string(struct i2c_client *client, const char* str);
uint8_t ssd1306_write_string(struct i2c_client *client, const char* str)
{
    while (*str) {
        if (*str == '\n') {
            global_x = 0;
            global_y += 2;
            if (global_y >= OLED_PAGES)
                global_y = 0;
        } else {
            ssd1306_write_char_8x16(client, *str);
        }
        str++;
    }

    return 0;
}

uint8_t ssd1306_write_hanzi(struct i2c_client *client, const char* str);
uint8_t ssd1306_write_hanzi(struct i2c_client *client, const char* str)
{
    // str to num
    int num = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        num = str[i] - '0';
        ssd1306_write_char_16x16(client, num);
    }
    return 0;
}

#endif /* __IIC_OLED_CMDS_H__ */

