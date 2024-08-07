#ifndef __SC035HGS_H__
#define __SC035HGS_H__

#include "math.h"
#include "i2cs.h"
#include "sleep.h"

#define ENABLE                                  1
#define DISABLE                                 0

#define MAX_SET_REG_NUMS                        10
#define FRAME_MAX_WIDTH                         640
#define FRAME_MAX_HEIGHT                        480

/* 摄像头信息 */
#define REG_CHIP_ID_HIGH_ADDR                   0x3107      // CHIP ID REG ADDR
#define REG_CHIP_ID_LOW_ADDR                    0x3108

/* 软复位 & 休眠 */
#define REG_SOFTWARE_RESET_ADDR                 0x0103      // 软复位（写 1 复位）
#define REG_SLEEP_MODE_CTRL_ADDR                0x0100      // 休眠模式（写 0 休眠）

#define REG_SOFTWARE_RESET_ENABLE               0x01        // 复位
#define REG_SOFTWARE_RESET_DISABLE              0x00        // 解复位
#define REG_SLEEP_MODE_ENABLE                   0x00        // 休眠（此时可读取寄存器）
#define REG_SLEEP_MODE_DISABLE                  0x01        // 工作

/* 输出窗口 */
#define REG_FRAME_WIDTH_HIGH_ADDR               0x3208      // 帧宽度（以像素为单位）
#define REG_FRAME_WIDTH_LOW_ADDR                0x3209
#define REG_FRAME_HEIGHT_HIGH_ADDR              0x320a      // 帧高度（以行为单位）
#define REG_FRAME_HEIGHT_LOW_ADDR               0x320b
#define REG_WINDOW_WIDTH_HIGH_ADDR              0x320c      // 行长（帧宽度 + 水平消隐区）
#define REG_WINDOW_WIDTH_LOW_ADDR               0x320d
#define REG_WINDOW_HEIGHT_HIGH_ADDR             0x320e      // 帧长（帧高度 + 垂直消隐区）
#define REG_WINDOW_HEIGHT_LOW_ADDR              0x320f
#define REG_FRAME_COL_START_HIGH_ADDR           0x3210      // 输出窗口列起始位置
#define REG_FRAME_COL_START_LOW_ADDR            0x3211
#define REG_FRAME_ROW_START_HIGH_ADDR           0x3212      // 输出窗口行起始位置
#define REG_FRAME_ROW_START_LOW_ADDR            0x3213

/* 外触发全局曝光 */
#define REG_EXT_EXPOSURE_BLANK_ROWS_ADDR        0x3218      // 曝光后读出芯片图像数据之后的消隐时间（以行为单位）
#define REG_EXT_EXPOSURE_TRIG_DELAY_ADDR        0x3226      // 曝光延迟（以行为单位）
#define REG_EXT_EXPOSURE_EXP_ROWS_HIGH_ADDR     0x3e01      // 曝光时间（以行为单位）
#define REG_EXT_EXPOSURE_EXP_ROWS_LOW_ADDR      0x3e02

/* HDR */
#define REG_HDR_ENABLE_CTRL_ADDR                0x3220      // HDR 使能控制

/* BLC */
#define REG_BLC_ENABLE_ADDR                     0x3900      // BLC 使能
#define REG_BLC_MODE_CTRL_ADDR                  0x3902      // BLC 模式控制（自动/手动）
#define REG_BLC_CHANNEL_HIGH_ADDR               0x3928      // BLC 通道选择
#define REG_BLC_CHANNEL_LOW_ADDR                0x3905
#define REG_BLC_TARGET_VALUE_HIGH_ADDR          0x3907      // BLC 目标值（自动模式无需设置）
#define REG_BLC_TARGET_VALUE_LOW_ADDR           0x3908

/* DVP */
#define REG_DVP_POL_CTRL_ADDR                   0x3d08      // DVP 信号极性

/* AGC */
#define REG_AGC_MODE_CTRL_ADDR                  0x3e03      // AGC 模式控制
#define REG_AGC_GAIN_MAPPING_HIGH_ADDR          0x3e08      // Gain Mapping 模式下的增益
#define REG_AGC_GAIN_MAPPING_LOW_ADDR           0x3e09
#define REG_DIGITAL_COARSE_GAIN_ADDR            0x3e06      // 数字粗调增益
#define REG_DIGITAL_FINE_GAIN_ADDR              0x3e07      // 数字细调增益
#define REG_ANALOG_COARSE_GAIN_ADDR             0x3e08      // 模拟增益粗调
#define REG_ANALOG_FINE_GAIN_ADDR               0x3e09      // 模拟增益细调

/* AEC */
#define REG_AEC_TOTAL_EXPOSURE_TIME_HIGH_ADDR   0x3e01      // 总曝光时间（以 1/16 行为单位）
#define REG_AEC_TOTAL_EXPOSURE_TIME_LOW_ADDR    0x3e02
#define REG_HDR_EXPOSURE_TIME_HIGH_ADDR         0x3e31      // HDR 曝光时间（以 1/16 行为单位）
#define REG_HDR_EXPOSURE_TIME_LOW_ADDR          0x3e32

/* 图像处理 */
#define REG_MIRROR_ENABLE_ADDR                  0x3221      // 水平镜像使能（Bit[2:1]）
#define REG_FLIP_ENABLE_ADDR                    0x3221      // 垂直翻转使能（Bit[6:5]）

/* 测试模式 */
#define REG_INCREMENT_PATTERN_ENABLE_ADDR       0x4501      // 灰度渐变模式使能

/* AGC 增益控制 */
// 模拟粗调
typedef enum
{
    ANALOG_COARSE_GAIN_X1               =   0x0,
    ANALOG_COARSE_GAIN_X2               =   0x1,
    ANALOG_COARSE_GAIN_X4               =   0x3,
    ANALOG_COARSE_GAIN_X8               =   0x7,
} analog_coarse_gain;

// 模拟精调
typedef enum
{
    ANALOG_FINE_GAIN_X1                 =   0x10,
    ANALOG_FINE_GAIN_1X0625             =   0x11,
    ANALOG_FINE_GAIN_1X125              =   0x12,
    ANALOG_FINE_GAIN_1X1875             =   0x13,
    ANALOG_FINE_GAIN_1X25               =   0x14,
    ANALOG_FINE_GAIN_1X3125             =   0x15,
    ANALOG_FINE_GAIN_1X375              =   0x16,
    ANALOG_FINE_GAIN_1X4375             =   0x17,
    ANALOG_FINE_GAIN_1X5                =   0x18,
    ANALOG_FINE_GAIN_1X5625             =   0x19,
    ANALOG_FINE_GAIN_1X625              =   0x1a,
    ANALOG_FINE_GAIN_1X6875             =   0x1b,
    ANALOG_FINE_GAIN_1X75               =   0x1c,
    ANALOG_FINE_GAIN_1X8125             =   0x1d,
    ANALOG_FINE_GAIN_1X875              =   0x1e,
    ANALOG_FINE_GAIN_1X9375             =   0x1f
} analog_fine_gain;

// 数字粗调
typedef enum
{
    DIGITAL_COARSE_GAIN_X1              =   0x0,
    DIGITAL_COARSE_GAIN_X2              =   0x1,
    DIGITAL_COARSE_GAIN_X4              =   0x3
} digital_coarse_gain;

// 数字精调
typedef enum
{
    DIGITAL_FINE_GAIN_X1                =   0x80,
    DIGITAL_FINE_GAIN_1X0625            =   0x88,
    DIGITAL_FINE_GAIN_1X125             =   0x90,
    DIGITAL_FINE_GAIN_1X1875            =   0x98,
    DIGITAL_FINE_GAIN_1X25              =   0xa0,
    DIGITAL_FINE_GAIN_1X3125            =   0xa8,
    DIGITAL_FINE_GAIN_1X375             =   0xb0,
    DIGITAL_FINE_GAIN_1X4375            =   0xb8,
    DIGITAL_FINE_GAIN_1X5               =   0xc0,
    DIGITAL_FINE_GAIN_1X5625            =   0xc8,
    DIGITAL_FINE_GAIN_1X625             =   0xd0,
    DIGITAL_FINE_GAIN_1X6875            =   0xd8,
    DIGITAL_FINE_GAIN_1X75              =   0xe0,
    DIGITAL_FINE_GAIN_1X8125            =   0xe8,
    DIGITAL_FINE_GAIN_1X875             =   0xf0,
    DIGITAL_FINE_GAIN_1X9375            =   0xf8
} digital_fine_gain;

/* BLC 配置 */
// BLC 通道选择
typedef enum
{
    BLC_CHANNEL_OFFSET_8                =   0,
    BLC_CHANNEL_OFFSET_4,
    BLC_CHANNEL_ENABLE_1_OFFSET_8_OR_4,
    BLC_CHANNEL_ENABLE_1
} BLC_Channel;

typedef enum
{
    BLC_MODE_MANUAL =   0x0,
    BLC_MODE_AUTO   =   0x1
} BLC_Mode;

// BLC 配置参数
typedef struct
{
    u8 Enable;              // BLC 使能
    BLC_Mode Mode;          // 自动模式使能
    BLC_Channel Channel;    // 通道选择
    u16 TargetValue;        // 目标值
} BLC_Args;

/**
 * 曝光模式
 */
typedef enum
{
    EXPOSURE_NORMAL_MODE                =   0,
    EXPOSURE_HDR_MODE
} Exposure_Mode;

/**
 * 复位寄存器列表
 */
static const RegValuePair REGS_SOFTWARE_RESET[] = 
{
	{REG_SOFTWARE_RESET_ADDR, REG_SOFTWARE_RESET_ENABLE},
    {REG_NULL_ADDR, 0x00}
};

/**
 * 640*480
 * xclk=24M, pclk=85MHz
 * fps=60
 */
static const RegValuePair REGS_INIT_640_480_60FPS_24M_XCLK[] = 
{
    {REG_SOFTWARE_RESET_ADDR, REG_SOFTWARE_RESET_ENABLE},
	{REG_SLEEP_MODE_CTRL_ADDR, REG_SLEEP_MODE_ENABLE},
	{0x36e9, 0x80},
	{0x36f9, 0x80},
    {REG_DELAY_ADDR, 0x80},
    {REG_DELAY_ADDR, 0x80},
	{0x300f, 0x0f},
	{0x3018, 0x1f},
	{0x3019, 0xff},
	{0x301c, 0xb4},
	{0x301f, 0x89},
	{0x3028, 0x82},
    {REG_FRAME_WIDTH_HIGH_ADDR, 0x02},                  // 帧宽（0x0280 = 640）
	{REG_FRAME_WIDTH_LOW_ADDR, 0x80},
	{REG_FRAME_HEIGHT_HIGH_ADDR, 0x01},                 // 帧高（0x01e0 = 480）
	{REG_FRAME_HEIGHT_LOW_ADDR, 0xe0},
    {REG_WINDOW_WIDTH_HIGH_ADDR, 0x05},                 // 行长（0x0554 = 1364）
	{REG_WINDOW_WIDTH_LOW_ADDR, 0x54},
	{REG_WINDOW_HEIGHT_HIGH_ADDR, 0x04},                // 帧长（0x040e = 1038, 0x0210=528）
	{REG_WINDOW_HEIGHT_LOW_ADDR, 0x0e},
    {REG_FRAME_COL_START_HIGH_ADDR, 0x00},              // 输出列起始位置
	{REG_FRAME_COL_START_LOW_ADDR, 0x08},
	{REG_FRAME_ROW_START_HIGH_ADDR, 0x00},              // 输出行起始位置
	{REG_FRAME_ROW_START_LOW_ADDR, 0x08},
	{0x3217, 0x00},
    {REG_EXT_EXPOSURE_BLANK_ROWS_ADDR, 0x00},
    {REG_HDR_ENABLE_CTRL_ADDR, 0x10},                   // 关闭 HDR 模式
	{0x3223, 0x48},
    {REG_EXT_EXPOSURE_TRIG_DELAY_ADDR, 0x74},
    {0x3227, 0x07},
    {0x323b, 0x00},
    {0x3250, 0xf0},
    {0x3251, 0x02},
    {0x3252, 0x02},
    {0x3253, 0x08},
    {0x3254, 0x02},
    {0x3255, 0x07},
    {0x3304, 0x48},
    {0x3305, 0x00},
    {0x3306, 0x98},
    {0x3309, 0x50},
    {0x330a, 0x01},
    {0x330b, 0x18},
    {0x330c, 0x18},
    {0x330f, 0x40},
    {0x3310, 0x10},
    {0x3314, 0x1e},
    {0x3315, 0x30},
    {0x3316, 0x68},
    {0x3317, 0x1b},
    {0x3329, 0x5c},
    {0x332d, 0x5c},
    {0x332f, 0x60},
    {0x3335, 0x64},
    {0x3344, 0x64},
    {0x335b, 0x80},
    {0x335f, 0x80},
    {0x3366, 0x06},
    {0x3385, 0x31},
    {0x3387, 0x39},
    {0x3389, 0x01},
    {0x33b1, 0x03},
    {0x33b2, 0x06},
    {0x33bd, 0xe0},
    {0x33bf, 0x10},
    {0x3621, 0xa4},
    {0x3622, 0x05},
    {0x3624, 0x47},
    {0x3630, 0x4a},
    {0x3631, 0x58},
    {0x3633, 0x52},
    {0x3635, 0x03},
    {0x3636, 0x25},
    {0x3637, 0x8a},
    {0x3638, 0x0f},
    {0x3639, 0x08},
    {0x363a, 0x00},
    {0x363b, 0x48},
    {0x363c, 0x86},
    {0x363d, 0x10},
    {0x363e, 0xf8},
    {0x3640, 0x00},
    {0x3641, 0x01},
    {0x36ea, 0x2f},
    {0x36eb, 0x0d},
    {0x36ec, 0x0d},
    {0x36ed, 0x20},
    {0x36fa, 0x2f},
    {0x36fb, 0x10},
    {0x36fc, 0x02},
    {0x36fd, 0x00},
    {REG_BLC_TARGET_VALUE_LOW_ADDR, 0x91},
    {0x391b, 0x81},
    {REG_DVP_POL_CTRL_ADDR, 0x01},
    {REG_EXT_EXPOSURE_EXP_ROWS_HIGH_ADDR, 0x18},
    {REG_EXT_EXPOSURE_EXP_ROWS_LOW_ADDR, 0xf0},
    {REG_AGC_MODE_CTRL_ADDR, 0x2b},
    {0x3e06, 0x0c},
    {0x3f04, 0x03},
    {0x3f05, 0x80},
    {0x4500, 0x59},
    {REG_INCREMENT_PATTERN_ENABLE_ADDR, 0xc4},
    {0x4800, 0x64},
    {0x4809, 0x01},
    {0x4810, 0x00},
    {0x4811, 0x01},
    {0x4837, 0x18},
    {0x5011, 0x00},
    {0x5988, 0x02},
    {0x598e, 0x05},
    {0x598f, 0x17},
    {0x36e9, 0x40},
    {0x36f9, 0x62},
    {REG_SLEEP_MODE_CTRL_ADDR, REG_SLEEP_MODE_DISABLE},
	{REG_DELAY_ADDR, 0x0a},                             // 0x4418 和 0x4419 两个寄存器需要在 0x0100 写 0x01
    {0x4418, 0x0a},                                     // 且至少间隔 (2^17/mclk + 1) ms 之后再写入
    {0x4419, 0x80},
	{REG_NULL_ADDR, 0x00}
};

/* Camera instance */
typedef struct _camera Camera_t;
typedef struct _camera
{
    u8 SlaveAddr;
    u16 ChipId;
    u16 I2Cs_DeviceId;             

    u8 Fps;
    uint32_t XclkFreq;

    u16 FrameWidth;
    u16 FrameHeight;

    Exposure_Mode ExposureMode;

    BLC_Args BLC_Args;
} Camera_t;

u32 SC035HGS_Init(Camera_t *InstancePtr);
u32 SC035HGS_Reset(Camera_t *InstancePtr);
u32 SC035HGS_Get_PclkFreq(Camera_t *InstancePtr);
u16 SC035HGS_Get_ChipId(Camera_t *InstancePtr);
u16 SC035HGS_Get_WindowWidth(Camera_t *InsatncePtr);
u16 SC035HGS_Get_WindowHeight(Camera_t *InsatncePtr);
u32 SC035HGS_Set_WindowWidth(Camera_t *InstancePtr, u16 Width);
u32 SC035HGS_Set_WindowHeight(Camera_t *InstancePtr, u16 Height);
u32 SC035HGS_Set_WindowSize(Camera_t *InstancePtr, u16 Width, u16 Height);
u32 SC035HGS_Set_Fps(Camera_t *InstancePtr, u8 Fps);
u32 SC035HGS_Set_Agc(Camera_t *InstancePtr, u8 Gain);
u32 SC035HGS_Set_Vflip(Camera_t *InstancePtr, u8 Enable);
u32 SC035HGS_Set_Hmirror(Camera_t *InstancePtr, u8 Enable);
u32 SC035HGS_Set_Sleep_Mode(Camera_t *InstancePtr, u8 Enable);
u32 SC035HGC_Set_Blc(Camera_t *InstancePtr, BLC_Args *Args);
u32 SC035HGS_Set_Increment_Pattern(Camera_t *InstancePtr, u8 Enable);
u32 SC035HGS_Set_FrameSize(Camera_t *InstancePtr, u16 FrameWidth, u16 FrameHeight);
u32 SC035HGS_Set_ExposureMode(Camera_t *InsatncePtr, Exposure_Mode Mode);
u32 SC035HGS_Set_Aec(Camera_t *InstancePtr, u16 TotalExposureTime, u16 HDRExposureTime);

#endif
