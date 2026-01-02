#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "sleep.h"
#include "xaxidma.h"
#include "xiicps.h"

#define CAM_ADDR        0x21
#define WIDTH           640
#define HEIGHT          480
#define FRAME_SIZE      (WIDTH * HEIGHT * 2)

#define FRAME1_ADDR     0x10000000
#define FRAME2_ADDR     0x10200000

#define DMA_BASE        XPAR_AXIDMA_0_BASEADDR
#define S2MM_DMASR      0x34

#define PIR_BASE    XPAR_PIR_ALARM_AXI_V1_0_0_BASEADDR
#define PIR_IN_REG  (PIR_BASE + 0x00)


XIicPs Iic;
XAxiDma Dma;

int I2C_Init()
{
	sleep(20);
    XIicPs_Config *Cfg = XIicPs_LookupConfig(XPAR_XIICPS_0_DEVICE_ID);
    if (!Cfg) return XST_FAILURE;

    int st = XIicPs_CfgInitialize(&Iic, Cfg, Cfg->BaseAddress);
    if (st != XST_SUCCESS) return st;

    XIicPs_SetSClk(&Iic, 100000);
    return XST_SUCCESS;
}

int OV_Read(u8 reg, u8 *val)
{
    if (XIicPs_MasterSendPolled(&Iic, &reg, 1, CAM_ADDR))
        return -1;
    while (XIicPs_BusIsBusy(&Iic));

    if (XIicPs_MasterRecvPolled(&Iic, val, 1, CAM_ADDR))
        return -1;

    while (XIicPs_BusIsBusy(&Iic));
    return 0;
}

int OV_Write(u8 reg, u8 val)
{
    u8 buf[2] = {reg, val};
    if (XIicPs_MasterSendPolled(&Iic, buf, 2, CAM_ADDR))
        return -1;
    while (XIicPs_BusIsBusy(&Iic));
    return 0;
}

typedef struct { u8 reg, val; } regval;

const regval OV7670_reg_simple[] = {
    {0x12, 0x80},
    {0x12, 0x14},
    {0x40, 0xD0},
    {0x11, 0x01},
    {0x0C, 0x00},
    {0x3E, 0x00},
    {0x8C, 0x02},
    {0xFF, 0xFF}
};

void Camera_Init()
{
    xil_printf("Camera reset...\n");
    OV_Write(0x12, 0x80);
    usleep(100000);

    xil_printf("Loading camera regs...\n");
    for (int i=0; OV7670_reg_simple[i].reg != 0xFF; i++) {
        OV_Write(OV7670_reg_simple[i].reg, OV7670_reg_simple[i].val);
        usleep(1000);
    }
}

void DMA_Reset()
{
    XAxiDma_Reset(&Dma);
    while (!XAxiDma_ResetIsDone(&Dma));
}

int DMA_Init()
{
    XAxiDma_Config *Cfg = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
    if (!Cfg) return XST_FAILURE;

    if (XAxiDma_CfgInitialize(&Dma, Cfg) != XST_SUCCESS)
        return XST_FAILURE;

    if (XAxiDma_HasSg(&Dma)) {
        xil_printf("DMA is SG mode (not supported)\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

int Capture_Frame(UINTPTR addr)
{
    xil_printf("DMA start capture to 0x%08X\n", addr);

    Xil_Out32(DMA_BASE + S2MM_DMASR, 0xFFFFFFFF);

    int st = XAxiDma_SimpleTransfer(&Dma, addr, FRAME_SIZE, XAXIDMA_DEVICE_TO_DMA);
    if (st != XST_SUCCESS) {
        xil_printf("DMA transfer start FAILED\n");
        return st;
    }

    int timeout = 20000000;
    while (timeout--)
    {
        u32 sr = Xil_In32(DMA_BASE + S2MM_DMASR);

        if (sr & 0x1000) {
            xil_printf("DMA DONE (DMASR=0x%08X)\n", sr);
            return XST_SUCCESS;
        }

        if (sr & 0x4000) {
            xil_printf("DMA ERROR (DMASR=0x%08X)\n", sr);
            return XST_FAILURE;
        }
    }

    xil_printf("DMA TIMEOUT (no TLAST)\n");
    return XST_FAILURE;
}

void print_head(UINTPTR addr)
{
    u8 *p = (u8*)addr;
    xil_printf("Head: ");
    for (int i=0; i<16; i++)
        xil_printf("%02X ", p[i]);
    xil_printf("\n");
}

void analyze_frame(UINTPTR addr, char *name)
{
    u16 *px = (u16*)addr;

    int diff = 0;
    int repeat = 0;

    for (int i=1; i<5000; i++)
    {
        if (px[i] != px[i-1]) diff++;
        else repeat++;
    }

    xil_printf("=== %s ===\n", name);
    xil_printf("diff   = %d\n", diff);
    xil_printf("repeat = %d\n", repeat);

    if (diff > 2000) xil_printf("Phat hien chuyen dong, phat hien nguoi\n");
    else             xil_printf("TOO STATIC\n");

    xil_printf("===================\n");
}

void compare_frames(UINTPTR f1, UINTPTR f2)
{
    u16 *a = (u16*)f1;
    u16 *b = (u16*)f2;

    int motion = 0;

    for (int i=0; i<50000; i++)
    {
        int da = a[i] & 0xF800;
        int db = b[i] & 0xF800;

        int diff = da - db;
        if (diff < 0) diff = -diff;

        if (diff > 200) motion++;
    }

    xil_printf("=== MOTION RESULT ===\n");
    xil_printf("motion pixels = %d\n", motion);

    if (motion > 2000) xil_printf(">>> MOTION DETECTED <<<\n");
    else               xil_printf("No motion\n");

    xil_printf("======================\n");
}

int main()
{
    sleep(2);
    xil_printf("===== 2 FRAME TEST START =====\n");

    I2C_Init();
    xil_printf("I2C OK\n");

    Camera_Init();
    xil_printf("Camera init OK\n");

    DMA_Init();
    xil_printf("DMA init OK\n");

    xil_printf("\n--- FRAME 1: \n");
    Capture_Frame(FRAME1_ADDR);
    print_head(FRAME1_ADDR);
    analyze_frame(FRAME1_ADDR, "FRAME1");

    xil_printf("\nNow MOVE in front of camera!\n");
    sleep(2);
    if (Xil_In32(PIR_IN_REG) == 1)
    {
        xil_printf("PIR = 1 \r\n");
        xil_printf("\n--- FRAME 2: with motion ---\n");
            Capture_Frame(FRAME2_ADDR);
            print_head(FRAME2_ADDR);
            analyze_frame(FRAME2_ADDR, "FRAME2");

            xil_printf("\n--- FRAME COMPARE ---\n");
            compare_frames(FRAME1_ADDR, FRAME2_ADDR);

            xil_printf("===== END =====\n");
            return 0;
    }

}
