#include "xparameters.h"
#include "netif/xadapter.h"
#include "platform.h"
#include "platform_config.h"
#include "xil_printf.h"
#include "lwip/tcp.h"
#include "xil_cache.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xaxidma.h"
#include "xgpio.h"
#include "sc035hgs.h"
#include "dma.h"
#include "intr.h"
#include "platform.h"
#include "lwipopts.h"
#include "sleep.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/init.h"
#include "lwip/inet.h"

#define DEBUG                       1
#define DMA_ENABLE                  0
#define ETH_ENABLE                  1

/* 相关外设 ID */
#define DEVICE_DMA			        XPAR_AXIDMA_0_DEVICE_ID                     // DMA
#define DEVICE_IICS                 XPAR_I2CS_DEVICE_ID                         // IIC（AXI GPIO 模拟）
#define DEVICE_BUFW_FS				XPAR_BUFW_FS_DEVICE_ID            		    // AXISBUFW 传输使能（高电平有效）
#define DEVICE_BUFW_RSTN			XPAR_BUFW_RSTN_DEVICE_ID                    // AXISBUFW 复位（低电平有效）
#define DEVICE_CAMERA_PWDN			XPAR_CAMERA_PWDN_DEVICE_ID                  // SC035HGS 休眠控制（正常工作时应拉高）
#define DEVICE_CAMERA_VSYNC			XPAR_CAMERA_VSYNC_DEVICE_ID                 // SC035HGS 输出场同步信号
#define DEVICE_XCLK_LOCKED			XPAR_XCLK_LOCKED_DEVICE_ID                  // SC035HGS 输入时钟锁定监测（高电平代表锁定）
#define DMA_RX_INTR_ID				XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR     // DMA 中断 ID

/* 外设句柄 */
static XAxiDma axiDma;
static XScuGic scuGic;
static XGpio gpio_bufw_fs;
static XGpio gpio_bufw_rstn;
static XGpio gpio_camera_pwdn;
static XGpio gpio_camera_xclk_locked;
static XGpio gpio_camera_vsync;

/* 中断相关标志位 */
extern volatile u8 RxDone;
extern volatile u8 RxError;

/* 帧缓冲区 */
static s32 TxIndex;
static s32 RxIndex;
static s32 RxLastIndex;
static u32 FrameBufferPtr[FRAME_BUFFER_NUMS];
static u8  FrameFrontBuffer[FRAME_FRONT_SIZE] = "image:0,307200,640,480,24\n";

/* 相关地址 */
static const u8 ETH_MAC_ADDRESS[]   = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };
static const u8 ETH_IP_ADDRESS[]    = { 192, 168, 31, 10};
static const u8 ETH_IP_MASK[]       = { 255, 255, 255, 0 };
static const u8 ETH_GATE_ADDRESS[]  = { 192, 168, 31, 1};

/* LWIP 标志位 */
static err_t LwipStatus;

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;

static s32 LwipSendBusy;
static s32 LwipSendBuffBytes;
static s32 LwipPackageBytes;
static s32 LwipTotalSendBytes;

static struct tcp_pcb *client;
static struct netif server_netif;
struct netif *echo_netif = &server_netif;

static s32 Eth_Init();
static s32 Camera_Init();
static s32 System_Init();
static s32 Network_Init();
static s32 Axisbufw_Init();
static s32 tcp_server_init();
static void SC035HGS_Hello_Msg_Print();
static s32 set_axisbufw_transmit(int enable);
static err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static s32 DMA_Init(XScuGic *ScuGicInstancePtr, XAxiDma *AxiDmaInstancePtr, u16 AxiDmaDeviceId, u16 AxiDmaRxIntrId);

int main()
{
	s32 Status;

    /* 欢迎信息 */
    SC035HGS_Hello_Msg_Print();
    
    /* 平台初始化 */
	init_platform();

	/* 系统初始化 */
	Status = System_Init();
	if(Status != XST_SUCCESS)
	{
		xil_printf("[ERROR] System init failed\n");
		return 0;
	}

	/* 网络初始化 */
	Status = Network_Init();
	if(Status != XST_SUCCESS)
	{
		xil_printf("[ERROR] Network init failed\n");
		return 0;
	}

    #if ETH_ENABLE
	#ifdef DEBUG

	/* 初始化待发送的数据 */
	u8 *FrameBuffer = (u8*)FRAME_BUFFER_BASE;
	for(u32 i = 0; i < FRAME_SIZE / 4; i++)
	{
		FrameBuffer[i * 4] 		= (i >> 24);
		FrameBuffer[i * 4 + 1]	= (i >> 16);
		FrameBuffer[i * 4 + 2]	= (i >> 8);
		FrameBuffer[i * 4 + 3]	= (i >> 0);
	}
	Xil_DCacheFlushRange((u32)FrameBuffer, FRAME_SIZE);

    xil_printf("[INFO] ETH last 4 byte should be \"%02x %02x %02x %02x\"\n", FrameBuffer[FRAME_SIZE - 4], FrameBuffer[FRAME_SIZE - 3], FrameBuffer[FRAME_SIZE - 2], FrameBuffer[FRAME_SIZE - 1]);

	#endif
    #endif

    #if DMA_ENABLE
	/* 等待场同步信号 */
	for(u8 frame_ignore_cnt = 0; frame_ignore_cnt < 10; frame_ignore_cnt++)
	{
		while(XGpio_DiscreteRead(&gpio_camera_vsync, 1) == 1);
		while(XGpio_DiscreteRead(&gpio_camera_vsync, 1) == 0);
	}
	set_axisbufw_transmit(1);
    #endif

	while(1) 
	{
        #if DMA_ENABLE
        
        /* 启动首次传输 */
        if(DmaFirstTransfer)
        {
            Status = XAxiDma_SimpleTransfer(&axiDma, (u32)FrameBufferPtr[RxIndex], (u32)FRAME_SIZE, XAXIDMA_DEVICE_TO_DMA);
			if(Status != XST_SUCCESS)
			{
				RxError = 1;
				print("[ERROR] Failed to launch new dma transfer\n"); 
			}
            DmaFirstTransfer = 0;
        }

		/* 复位系统 */
		if(RxError)
		{
			xil_printf("[ERROR] DMA rx error\n");
            RxError = 0;
		}

        /* 成功接收一帧图像 */
		if(RxDone)
		{
			/* 更新缓冲区索引 */
			RxLastIndex = RxIndex;
			do
			{
				RxIndex = (RxIndex + 1) % FRAME_BUFFER_NUMS;
			} while (RxIndex == TxIndex);

			// 非调试情况下应尽量避免打印，会严重影响系统性能
			// 关闭打印情况下 PING 平均延时  1ms, TTL = 255
			// 启动打印情况下 PING 平均延时 38ms, TTL = 255
			#if DEBUG
			xil_printf("[INFO] Rx-RxLast-Tx >> %d-%d-%d\n", RxIndex, RxLastIndex, TxIndex);
			#endif

			/* 启动下一次传输 */
			Status = XAxiDma_SimpleTransfer(&axiDma, (u32)FrameBufferPtr[RxIndex], (u32)FRAME_SIZE, XAXIDMA_DEVICE_TO_DMA);
			if(Status != XST_SUCCESS)
			{
				RxError = 1;
				print("[ERROR] Failed to launch new dma transfer\n"); 
			}

            RxDone = 0;
		}
        #endif

        #if ETH_ENABLE

        /* 定时更新 */
        if(TcpFastTmrFlag) 
        {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if(TcpSlowTmrFlag) 
        {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		xemacif_input(&server_netif);

        if(!client)
		{
			continue;
		}

        #if DEBUG

        /* 判断是否启动传输 */
        if(LwipTotalSendBytes >= FRAME_SIZE)
        {
            continue;
        }

        /* 检查发送缓冲区大小 */
		LwipSendBuffBytes = tcp_sndbuf(client);
        if(LwipSendBuffBytes <= 0)
        {
            continue;
        }

        /* 发送测试数据 */
        LwipPackageBytes = (FRAME_SIZE - LwipTotalSendBytes > LwipSendBuffBytes) ? LwipSendBuffBytes : (FRAME_SIZE - LwipTotalSendBytes);
        LwipStatus = tcp_write(client, (u8*)(FrameBuffer + LwipTotalSendBytes), LwipPackageBytes, 0);
        if(LwipStatus != ERR_OK)
        {
            xil_printf("[ERROR] TCP send package failed\n"); return 0;
        }
        tcp_output(client);

        /* 更新相关计数器 */
        LwipTotalSendBytes += LwipPackageBytes;

		#else
		
		if((TxIndex != RxLastIndex) && !LwipSendBusy)
		{
			TxIndex = RxLastIndex;

			LwipSendBusy = 1;
			LwipTotalSendBytes = 0;

			Xil_DCacheInvalidateRange((u32)FrameBufferPtr[TxIndex], FRAME_SIZE);
		}

		if(LwipSendBusy)
		{
			if((LwipTotalSendBytes > FRAME_SIZE + FRAME_FRONT_SIZE))
			{
				LwipSendBusy = 0;
				continue;
			}

			LwipSendBuffBytes = tcp_sndbuf(client);
			if(LwipSendBuffBytes == 0)
			{
				continue;
			}

			if(LwipTotalSendBytes < FRAME_FRONT_SIZE)
			{
				LwipPackageBytes = (FRAME_FRONT_SIZE - LwipTotalSendBytes > LwipSendBuffBytes) ? LwipSendBuffBytes : (FRAME_FRONT_SIZE - LwipTotalSendBytes);
				LwipStatus = tcp_write(client, (u8*)(FrameFrontBuffer + LwipTotalSendBytes), LwipPackageBytes, 0);
				if(LwipStatus != ERR_OK)
				{
					xil_printf("[ERROR] Lwip send FRAME_FRONT package failed\n"); return 0;
				}
			}
			else if(LwipTotalSendBytes < FRAME_FRONT_SIZE + FRAME_SIZE)
			{
				LwipPackageBytes = (FRAME_SIZE + FRAME_FRONT_SIZE - LwipTotalSendBytes > LwipSendBuffBytes) ? LwipSendBuffBytes : (FRAME_SIZE + FRAME_FRONT_SIZE - LwipTotalSendBytes);
				LwipStatus = tcp_write(client, (u8*)(FrameBufferPtr[TxIndex] + LwipTotalSendBytes - FRAME_FRONT_SIZE), LwipPackageBytes, 0);
				if(LwipStatus != ERR_OK)
				{
					xil_printf("[ERROR] Lwip send FRAME package failed\n"); return 0;
				}
			}
			else
			{
				LwipPackageBytes = 1;
				LwipStatus = tcp_write(client, "\n", LwipPackageBytes, 0);
				if(LwipStatus != ERR_OK)
				{
					xil_printf("[ERROR] Lwip send FRAME_END package failed\n"); return 0;
				}
			}
			tcp_output(client);
            LwipTotalSendBytes += LwipPackageBytes;
		}
		#endif
        #endif
	}

	return 0;
}

/**
 * @brief 初始化系统
 * @return 初始化成功返回 XST_SUCCESS，否则返回 XST_FAILURE
*/
static s32 System_Init()
{
	s32 Status;

	/* 初始化摄像头 */
	Status = Camera_Init();
	if(Status != XST_SUCCESS)
	{
		print("[ERROR] Failed to init camera\n");
		return XST_FAILURE;
	}

	/* 初始化 AXISBUFW 数据流转换模块 */
	Status = Axisbufw_Init();
	if(Status != XST_SUCCESS)
	{
		print("[ERROR] Failed to init AXISBUFW\n");
		return XST_FAILURE;
	}

	/* 初始化 DMA */
	Status = DMA_Init(&scuGic, &axiDma, DEVICE_DMA, DMA_RX_INTR_ID);
	if(Status != XST_SUCCESS)
	{
		print("[ERROR] Failed to init DMA\n");
		return XST_FAILURE;
	}

	return Status;
}

/**
 * @brief 网络初始化
 * @return 初始化成功返回 XST_SUCCESS，否则返回 XST_FAILURE
*/
static s32 Network_Init()
{
	s32 Status;

	// 初始化网口
	Status = Eth_Init();
	if(Status != XST_SUCCESS)
	{
		print("[ERROR] Failed to init eth\n");
		return Status;
	} 

	// 初始化 TCP 服务器
	Status = tcp_server_init();
	if(Status != XST_SUCCESS)
	{
		print("[ERROR] Failed to init tcp server\n");
		return Status;
	}

	return XST_SUCCESS;
}

/**
 * @brief 初始化摄像头
 * @return 
*/
static s32 Camera_Init()
{
    s32 Status;
	camera_t camera;   

    /* 初始化相关引脚 */
    XGpio_Initialize(&gpio_camera_pwdn, DEVICE_CAMERA_PWDN);
    XGpio_Initialize(&gpio_camera_vsync, DEVICE_CAMERA_VSYNC);
    XGpio_Initialize(&gpio_camera_xclk_locked, DEVICE_XCLK_LOCKED);

    /* 等待摄像头输入时钟锁定 */
    while(XGpio_DiscreteRead(&gpio_camera_xclk_locked, 1) == 0);

    /* 拉高 PWDN 引脚以启用摄像头 */
    XGpio_DiscreteWrite(&gpio_camera_pwdn, 1, 0x1);

    // 至少等待 4ms 才能访问寄存器
    usleep(20 * 1000);

    /* 初始化摄像头寄存器 */
    camera.i2c_device_id = DEVICE_IICS;

    Status = sc035hgs_init(&camera);
    if(Status != XST_SUCCESS)
    {
    	print("[ERROR] SC035HGS's regs init failed\n");
    	return XST_FAILURE;
    }
    print("[INFO] Success to init SC035HGS\n");

    // 等待摄像头输出稳定
    u8 camera_ignore_frame_cnt = 0;
    while(camera_ignore_frame_cnt < 10)
    {
        camera_ignore_frame_cnt++;
        while(XGpio_DiscreteRead(&gpio_camera_vsync, 1) == 0);
        while(XGpio_DiscreteRead(&gpio_camera_vsync, 1) == 1);
    }

	return XST_SUCCESS;
}

/**
 * @brief 初始化 AXISBUFW 数据传输模块
 * @return 
*/
static s32 Axisbufw_Init()
{
	XGpio_Initialize(&gpio_bufw_rstn, DEVICE_BUFW_RSTN);
    XGpio_Initialize(&gpio_bufw_fs, DEVICE_BUFW_FS);

	// 结束 BUFW 复位
	XGpio_DiscreteWrite(&gpio_bufw_rstn, 1, 0x1);

	return XST_SUCCESS;
}

/**
 * @brief 初始化 DMA
 * @param ScuGicInstancePtr
 * @param AxiDmaInstancePtr
 * @param AxiDmaDeviceId
 * @param AxiDmaRxIntrId
 * @return *
*/
static s32 DMA_Init(XScuGic *ScuGicInstancePtr, XAxiDma *AxiDmaInstancePtr, u16 AxiDmaDeviceId, u16 AxiDmaRxIntrId)
{
	s32 Status;

    Status = Intr_Init(ScuGicInstancePtr);
    Intr_Exception_Setup(ScuGicInstancePtr);
    XAxiDma_Init(AxiDmaInstancePtr, AxiDmaDeviceId);
    XAxiDma_Intr_Enable(AxiDmaInstancePtr);
	XAxiDma_Intr_Setup(ScuGicInstancePtr, AxiDmaInstancePtr, AxiDmaRxIntrId);

	TxIndex = -1;
	RxIndex = 0;
	RxLastIndex = -1;
	for(u8 i = 0; i < FRAME_BUFFER_NUMS; i++)
	{
		FrameBufferPtr[i] = FRAME_BUFFER_BASE + FRAME_SIZE * i;
	}

	return Status;
}

/**
 * @brief 使能 AXIS 数据传输
 * @param enable AXIS_TRANSMIT_ENABLE/AXIS_TRANSMIT_DISABLE
 * @return *
*/
static s32 set_axisbufw_transmit(int enable)
{
	XGpio_DiscreteWrite(&gpio_bufw_fs, 1, enable ? 0x1 : 0x0);

	return XST_SUCCESS;
}

/**
 * @brief 初始化网口
 * @return 初始化成功返回 XST_SUCCESS，否则返回 XST_FAILURE
*/
static s32 Eth_Init()
{
    lwip_init();

	/* 初始化 IP 地址 */
	ip_addr_t ip, netmask, gw;
	IP4_ADDR(&ip, ETH_IP_ADDRESS[0], ETH_IP_ADDRESS[1], ETH_IP_ADDRESS[2], ETH_IP_ADDRESS[3]);
	IP4_ADDR(&netmask, ETH_IP_MASK[0], ETH_IP_MASK[1], ETH_IP_MASK[2],  ETH_IP_MASK[3]);
	IP4_ADDR(&gw, ETH_GATE_ADDRESS[0], ETH_GATE_ADDRESS[1], ETH_GATE_ADDRESS[2], ETH_GATE_ADDRESS[3]);

	/* 添加网络接口 */
	if(!xemac_add(&server_netif, &ip, &netmask, &gw, ETH_MAC_ADDRESS, XPAR_XEMACPS_0_BASEADDR)) 
	{
		xil_printf("[ERROR] Failed to add mac interface\n");
		return XST_FAILURE;
	}
	netif_set_default(&server_netif);
	netif_set_up(&server_netif);

    // 使能中断
	platform_enable_interrupts();

	return XST_SUCCESS;
}

/**
 * @brief 初始化 TCP 服务器
 * @return 初始化成功返回 XST_SUCCESS，否则返回 XST_FAILURE
*/
static s32 tcp_server_init()
{
	// 创建 TCP PCB 句柄
	struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if(!pcb) 
	{
		xil_printf("[ERROR] Error creating PCB, out of Memory\n");
		return XST_FAILURE;
	}

	// 绑定端口号
	if(tcp_bind(pcb, IP_ANY_TYPE, 7) != ERR_OK) 
	{
		xil_printf("[ERROR] Unable to bind tcp port\n");
		return XST_FAILURE;
	}
	tcp_arg(pcb, NULL);

	// 监听客户端连接
	pcb = tcp_listen(pcb);
	if(!pcb) 
	{
		xil_printf("[ERROR] Out of memory while tcp_listen\n");
		return XST_FAILURE;
	}

	// 设置连接回调函数
	tcp_accept(pcb, accept_callback);

	return XST_SUCCESS;
}

/**
 * @brief TCP 连接回调函数
 * @return *
*/
static err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	if(newpcb == NULL)
	{
		xil_printf("[ERROR] Failed to accept new client connection, input pcb is null\n");
		return ERR_ABRT;
	}

	client = newpcb;

	return ERR_OK;
}

/**
 * @brief 欢迎信息打印
 * @return *
*/
static void SC035HGS_Hello_Msg_Print()
{
	const s8 msg[][100] =
	{
		" .d8888b.   .d8888b.   .d8888b.   .d8888b.  888888888  888    888  .d8888b.   .d8888b.\n",
		"d88P  Y88b d88P  Y88b d88P  Y88b d88P  Y88b 888        888    888 d88P  Y88b d88P  Y88b\n",
		"Y88b.      888    888 888    888      .d88P 888        888    888 888    888 Y88b.\n",
		"\"Y888b.   888        888    888     8888\"  8888888b.  8888888888 888         \"Y888b.\n",
		"	\"Y88b. 888        888    888      \"Y8b.      \"Y88b 888    888 888  88888     \"Y88b.\n",
		"	  \"888 888    888 888    888 888    888        888 888    888 888    888       \"888\n",
		"Y88b  d88P Y88b  d88P Y88b  d88P Y88b  d88P Y88b  d88P 888    888 Y88b  d88P Y88b  d88P\n",
		" \"Y8888P\"   \"Y8888P\"   \"Y8888P\"   \"Y8888P\"   \"Y8888P\"  888    888  \"Y8888P88  \"Y8888P\"\n"
	};

    for(u8 i = 0; i < sizeof(msg) / sizeof(msg[0]); i++)
    {
        xil_printf(msg[i]);
    }
}
