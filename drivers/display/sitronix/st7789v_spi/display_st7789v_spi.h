#ifndef ST7789V_DISPLAY_DRIVER_H_
#define ST7789V_DISPLAY_DRIVER_H_

#define ST7789V_CMD_SW_RESET			0x01

#define ST7789V_CMD_SLEEP_IN			0x10
#define ST7789V_CMD_SLEEP_OUT			0x11
#define ST7789V_CMD_INV_OFF			    0x20
#define ST7789V_CMD_INV_ON			    0x21
#define ST7789V_CMD_GAMSET			    0x26
#define ST7789V_CMD_DISP_OFF			0x28
#define ST7789V_CMD_DISP_ON			    0x29

#define ST7789V_CMD_CASET			    0x2a
#define ST7789V_CMD_RASET			    0x2b
#define ST7789V_CMD_RAMWR			    0x2c

#define ST7789V_CMD_MADCTL			        0x36
#define ST7789V_MADCTL_MY_TOP_TO_BOTTOM		0x00
#define ST7789V_MADCTL_MY_BOTTOM_TO_TOP		0x80
#define ST7789V_MADCTL_MX_LEFT_TO_RIGHT		0x00
#define ST7789V_MADCTL_MX_RIGHT_TO_LEFT		0x40
#define ST7789V_MADCTL_MV_REVERSE_MODE		0x20
#define ST7789V_MADCTL_MV_NORMAL_MODE		0x00
#define ST7789V_MADCTL_ML			        0x10
#define ST7789V_MADCTL_RBG			        0x00
#define ST7789V_MADCTL_BGR			        0x08
#define ST7789V_MADCTL_MH_LEFT_TO_RIGHT		0x00
#define ST7789V_MADCTL_MH_RIGHT_TO_LEFT		0x04

#define ST7789V_CMD_COLMOD			    0x3a
#define ST7789V_COLMOD_RGB_65K			(0x5 << 4)
#define ST7789V_COLMOD_RGB_262K			(0x6 << 4)
#define ST7789V_COLMOD_FMT_12bit		(3)
#define ST7789V_COLMOD_FMT_16bit		(5)
#define ST7789V_COLMOD_FMT_18bit		(6)

#define ST7789V_CMD_RAMCTRL			0xb0
#define ST7789V_CMD_RGBCTRL			0xb1
#define ST7789V_CMD_PORCTRL			0xb2
#define ST7789V_CMD_CMD2EN			0xdf
#define ST7789V_CMD_DGMEN			0xba
#define ST7789V_CMD_GCTRL			0xb7
#define ST7789V_CMD_VCOMS			0xbb

#define ST7789V_CMD_LCMCTRL			0xc0
#define ST7789V_LCMCTRL_XMY			0x40
#define ST7789V_LCMCTRL_XBGR		0x20
#define ST7789V_LCMCTRL_XINV		0x10
#define ST7789V_LCMCTRL_XMX			0x08
#define ST7789V_LCMCTRL_XMH			0x04
#define ST7789V_LCMCTRL_XMV			0x02

#define ST7789V_CMD_VDVVRHEN		0xc2
#define ST7789V_CMD_VRH				0xc3
#define ST7789V_CMD_VDS				0xc4
#define ST7789V_CMD_FRCTRL2			0xc6
#define ST7789V_CMD_PWCTRL1			0xd0

#define ST7789V_CMD_PVGAMCTRL		0xe0
#define ST7789V_CMD_NVGAMCTRL		0xe1

#endif /* ST7789V_DISPLAY_DRIVER_H_ */