#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

//#include <linux/dma-mapping.h>
#include "mt_typedefs.h"
#include "mt_gpio.h"
#include "sync_write.h"
//#include "disp_drv_log.h"

#ifdef OUTREG32
#undef OUTREG32
#define OUTREG32(x, y) mt65xx_reg_sync_writel(y, x)
#endif

#ifndef OUTREGBIT
#define OUTREGBIT(TYPE,REG,bit,value)  \
                    do {    \
                        TYPE r = *((TYPE*)&INREG32(&REG));    \
                        r.bit = value;    \
                        OUTREG32(&REG, AS_UINT32(&r));    \
                    } while (0)
#endif

///LCD HW feature options for MT6575
#define MTK_LCD_HW_SIF_VERSION      2       ///for MT6575, we naming it is V2 because MT6516/73 is V1...
#define MTKFB_NO_M4U
#define MT65XX_NEW_DISP
//#define MTK_LCD_HW_3D_SUPPORT
#define ALIGN_TO(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))
#define MTK_FB_ALIGNMENT 32  //Hardware 3D
//#define MTK_FB_ALIGNMENT 1 //SW 3D
#define MTK_FB_START_DSI_ISR

#define DFO_USE_NEW_API
#define MTKFB_FPGA_ONLY
#endif //__DISP_DRV_PLATFORM_H__
