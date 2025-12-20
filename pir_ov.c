#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "sleep.h"
#include "xaxidma.h"
#include "xiicps.h"

// =====================================================
// BASIC CONFIG
// =====================================================
#define CAM_ADDR        0x21
#define WIDTH           640
#define HEIGHT          480
#define FRAME_SIZE      (WIDTH * HEIGHT * 2)

#define FRAME1_ADDR     0x10000000
#define FRAME2_ADDR     0x10200000

#define DMA_BASE        XPAR_AXIDMA_0_BASEADDR
#define S2MM_DMASR      0x34

// =====================================================
// PIR + LED + BUZZER
// =====================================================
#define PIR_BASE    XPAR_PIR_ALARM_AXI_V1_0_BASEADDR
#define PIR_IN_REG  (PIR_BASE + 0x00)
#define LED_REG     (PIR_BASE + 0x04)
#define BUZZER_REG  (PIR_BASE + 0x08)

XIicPs Iic;
XAxiDma Dma;

// =====================================================
// I2C INIT
// =====================================================
int I2C_Init()
{
    sleep(2);
    XIicPs_Config *Cfg = XIicPs_LookupConfig(XPAR_XIICPS_0_DEVICE_ID);
    if (!Cfg) return -1;

    if (XIicPs_CfgInitialize(&Iic, Cfg, Cfg->BaseAddress) != XST_SUCCESS)
        return -1;

    XIicPs_SetSClk(&Iic, 100000);
    xil_printf("I2C OK\r\n");
    return 0;
}

// =====================================================
// OV7670 REGISTER WRITE
// =====================================================
int OV_Write(u8 reg, u8 val)
{
    u8 buf[2] = {reg, val};
    if (XIicPs_MasterSendPolled(&Iic, buf, 2, CAM_ADDR))
        return -1;
    while (XIicPs_BusIsBusy(&Iic));
    return 0;
}

// =====================================================
// CAMERA INIT
// =====================================================
void Camera_Init()
{
    xil_printf("Camera reset...\r\n");
    OV_Write(0x12, 0x80);
    usleep(100000);

    xil_printf("Loading basic regs...\r\n");
    OV_Write(0x12, 0x14); // RGB VGA
    OV_Write(0x40, 0xD0); // RGB565
    OV_Write(0x11, 0x01);

    xil_printf("Camera init OK\r\n");
}

// =====================================================
// DMA INIT
// =====================================================
int DMA_Init()
{
    XAxiDma_Config *Cfg = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
    if (!Cfg) return -1;
    if (XAxiDma_CfgInitialize(&Dma, Cfg) != XST_SUCCESS)
        return -1;

    xil_printf("DMA init OK\r\n");
    return 0;
}

// =====================================================
// DMA FRAME CAPTURE
// =====================================================
int Capture_Frame(UINTPTR addr)
{
    xil_printf("DMA capture → 0x%08X\r\n", addr);
    Xil_Out32(DMA_BASE + S2MM_DMASR, 0xFFFFFFFF);

    int st = XAxiDma_SimpleTransfer(&Dma, addr, FRAME_SIZE, XAXIDMA_DEVICE_TO_DMA);
    if (st != XST_SUCCESS) {
        xil_printf("DMA START ERROR\r\n");
        return st;
    }

    int timeout = 20000000;
    while (timeout--)
    {
        u32 sr = Xil_In32(DMA_BASE + S2MM_DMASR);

        if (sr & 0x1000) {
            xil_printf("DONE (DMASR=%08X)\r\n", sr);
            return 0;
        }
        if (sr & 0x4000) {
            xil_printf("ERROR (DMASR=%08X)\r\n", sr);
            return -1;
        }
    }

    xil_printf("TIMEOUT → buffer may still be valid\r\n");
    return 0;
}

// =====================================================
// PRINT 16 BYTES
// =====================================================
void print_head(UINTPTR addr)
{
    u8 *p = (u8*)addr;
    xil_printf("Head: ");
    for (int i=0; i<16; i++) xil_printf("%02X ", p[i]);
    xil_printf("\r\n");
}

// =====================================================
// SIMPLE HUMAN DETECTION
// =====================================================
int detect_human(UINTPTR f1, UINTPTR f2)
{
    u16 *A = (u16*)f1;
    u16 *B = (u16*)f2;

    int motion = 0;
    int minx=WIDTH, miny=HEIGHT, maxx=0, maxy=0;

    for (int y=0; y<HEIGHT; y++)
    for (int x=0; x<WIDTH; x++)
    {
        int idx = y*WIDTH + x;

        int a = A[idx] & 0xF800;
        int b = B[idx] & 0xF800;

        int diff = a>b ? a-b : b-a;

        if (diff > 200)
        {
            motion++;
            if (x < minx) minx = x;
            if (x > maxx) maxx = x;
            if (y < miny) miny = y;
            if (y > maxy) maxy = y;
        }
    }

    xil_printf("Motion = %d\r\n", motion);
    if (motion < 2000) return 0;

    int w = maxx - minx + 1;
    int h = maxy - miny + 1;
    int area = w * h;

    xil_printf("BBox: w=%d h=%d area=%d\r\n", w,h,area);
    if (area < 60*60) return 0;

    int ratio10 = (10*h)/w;
    xil_printf("ratio*10 = %d\r\n", ratio10);

    if (ratio10 < 12 || ratio10 > 42) return 0;

    return 1;
}

// =====================================================
// MAIN PROGRAM
// =====================================================
int main()
{
    xil_printf("===== HUMAN DETECTION START =====\r\n");

    I2C_Init();
    Camera_Init();
    DMA_Init();

    xil_printf("Waiting for PIR...\r\n");

    while (1)
    {
        if (Xil_In32(PIR_IN_REG) == 1)
        {
            xil_printf("\r\nPIR = 1 → CAPTURE START\r\n");

            // ===============================
            // RESET DMA TRƯỚC FRAME 1
            // ===============================
            xil_printf("Reset DMA before FRAME1...\r\n");
            XAxiDma_Reset(&Dma);
            while (!XAxiDma_ResetIsDone(&Dma));

            //--------------------------------
            // FRAME 1
            //--------------------------------
            Capture_Frame(FRAME1_ADDR);
            print_head(FRAME1_ADDR);

            // Nếu đầu ảnh vẫn 02 02 02 02 → chụp lại
            u8 *h1 = (u8*)FRAME1_ADDR;
            if (h1[0]==0x02 && h1[1]==0x02 && h1[2]==0x02)
            {
                xil_printf("FRAME1 INVALID → RECAPTURE\r\n");
                Capture_Frame(FRAME1_ADDR);
                print_head(FRAME1_ADDR);
            }

            // ===============================
            // RESET DMA TRƯỚC FRAME 2
            // ===============================
            xil_printf("Reset DMA before FRAME2...\r\n");
            XAxiDma_Reset(&Dma);
            while (!XAxiDma_ResetIsDone(&Dma));

            sleep(1);

            //--------------------------------
            // FRAME 2
            //--------------------------------
            Capture_Frame(FRAME2_ADDR);
            print_head(FRAME2_ADDR);

            xil_printf("Analyzing human...\r\n");

            if (detect_human(FRAME1_ADDR, FRAME2_ADDR))
            {
                xil_printf("\r\n>>> HUMAN DETECTED <<<\r\n");
                Xil_Out32(LED_REG, 1);
                Xil_Out32(BUZZER_REG, 1);
            }
            else
            {
                xil_printf("NO HUMAN\r\n");
                Xil_Out32(LED_REG, 0);
                Xil_Out32(BUZZER_REG, 0);
            }
        }
        else
        {
            Xil_Out32(LED_REG, 0);
            Xil_Out32(BUZZER_REG, 0);
        }

        usleep(100000);
    }

    return 0;
}
