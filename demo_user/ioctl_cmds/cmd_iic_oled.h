#ifndef __CMD_IIC_OLED_H__
#define __CMD_IIC_OLED_H__

#define IOCTL_CMD_OLED_TURN_ON_INIT (_IO(0xAA, 0x01)) // 打开 OLED
#define IOCTL_CMD_OLED_TURN_OFF     (_IO(0xAA, 0x02)) // 关闭 OLED
#define IOCTL_CMD_OLED_SETCURSOR    (_IO(0xAA, 0x03)) // 设置光标位置
#define IOCTL_CMD_OLED_CLEAR        (_IO(0xAA, 0x04)) // 清屏
#define IOCTL_CMD_OLED_WRITE_STR    (_IO(0xAA, 0x05)) // 写字符串

#endif /* __CMD_IIC_OLED_H__ */