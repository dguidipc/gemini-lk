/*
 * Copyright (c) 2012 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _MT6797_USB_H_
#define _MT6797_USB_H_

#include <dev/udc.h>
#include <platform/mt_typedefs.h>

/* USB registers */
/* APB Module ssusb_top - USB3_BASE */
#define SSUSB_DEV_BASE              (USB3_BASE + 0x1000)
#define SSUSB_EPCTL_CSR_BASE            (USB3_BASE + 0x1800)
#define SSUSB_USB3_MAC_CSR_BASE         (USB3_BASE + 0x2400)
#define SSUSB_USB3_SYS_CSR_BASE         (USB3_BASE + 0x2400)
#define SSUSB_USB2_CSR_BASE         (USB3_BASE + 0x3400)

/* APB Module ssusb_top_sif - USB3_SIF_BASE */
#define SSUSB_SIFSLV_SPLLC_BASE         (USB3_SIF2_BASE + 0x000)
#define SSUSB_SIFSLV_IPPC_BASE          (USB3_SIF_BASE + 0x700)
#define SSUSB_SIFSLV_U2PHY_COM_BASE     (USB3_SIF2_BASE + 0x800)
#define SSUSB_SIFSLV_U3PHYD_BASE        (USB3_SIF2_BASE + 0x900)

/* APB Module ssusb_top_sif2 - USB3_SIF2_BASE */
#define SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE   (USB3_SIF2_BASE + 0x800)
#define SSUSB_USB30_PHYA_SIV_B_BASE     (USB3_SIF2_BASE + 0xB00)
#define SSUSB_SIFSLV_U3PHYA_DA_BASE     (USB3_SIF2_BASE + 0xC00)

/* USB3 support */
#define SUPPORT_U3

#define SUPPORT_QMU
//#define DBG_USB_QMU
//#define DBG_USB_PHY

//#define SUPPORT_DMA
#define POWER_SAVING_MODE
#define U2_U3_SWITCH
#define EXT_VBUS_DET

#define MT_EP_NUM 4
#define MT_CHAN_NUM 4
#define MT_EP0_FIFOSIZE 64

#define MAX_EP_NUM 4
#ifdef SUPPORT_U3
/* EP0, TX, RX has separate SRAMs */
#define USB_TX_FIFO_START_ADDRESS   0
#define USB_RX_FIFO_START_ADDRESS   0
#else
/* EP0, TX, RX share one SRAM. 0-63 bytes are reserved for EP0 */
#define USB_TX_FIFO_START_ADDRESS   (64)
#define USB_RX_FIFO_START_ADDRESS   (64+512*MAX_EP_NUM)
#endif

#define EP0_MAX_PACKET_SIZE 64
#define EP0_MAX_PACKET_SIZE_U3 512

#define MT_BULK_MAXP    512
#define MT_INT_MAXP 1024

#ifdef MACH_FPGA
#define CFG_FPGA_PLATFORM   (1)
#else
#define DBG_PHY_CALIBRATION 1
#endif

/* choose the USB3 here when doing FPGA porting */
#if CFG_FPGA_PLATFORM
#define CONFIG_A60810_SUPPORT
//#define CONFIG_D60802_SUPPORT
//#define CONFIG_E60802_SUPPORT
#endif

/* Higher level functions for abstracting away from specific device */

void mt_udc_irq (
    unsigned int ltssm,
    unsigned int intrusb,
    unsigned int dmaintr,
    unsigned short intrtx,
    unsigned short intrrx,
    unsigned int intrqmu,
    unsigned int intrqmudone,
    unsigned int linkint);

/* SSUSB_DEV REGISTER DEFINITION */
#define U3D_LV1ISR          (SSUSB_DEV_BASE+0x0000)
#define U3D_LV1IER          (SSUSB_DEV_BASE+0x0004)
#define U3D_LV1IESR         (SSUSB_DEV_BASE+0x0008)
#define U3D_LV1IECR         (SSUSB_DEV_BASE+0x000C)
#define U3D_MAC_U1_EN_CTRL      (SSUSB_DEV_BASE+0x0030)
#define U3D_MAC_U2_EN_CTRL      (SSUSB_DEV_BASE+0x0034)
#define U3D_SRAM_DBG_CTRL       (SSUSB_DEV_BASE+0x0040)
#define U3D_SRAM_DBG_CTRL_1     (SSUSB_DEV_BASE+0x0044)
#define U3D_RISC_SIZE           (SSUSB_DEV_BASE+0x0050)
#define U3D_WRBUF_ERR_STS       (SSUSB_DEV_BASE+0x0070)
#define U3D_BUF_ERR_EN          (SSUSB_DEV_BASE+0x0074)
#define U3D_EPISR           (SSUSB_DEV_BASE+0x0080)
#define U3D_EPIER           (SSUSB_DEV_BASE+0x0084)
#define U3D_EPIESR          (SSUSB_DEV_BASE+0x0088)
#define U3D_EPIECR          (SSUSB_DEV_BASE+0x008C)
#define U3D_DMAISR          (SSUSB_DEV_BASE+0x0090)
#define U3D_DMAIER          (SSUSB_DEV_BASE+0x0094)
#define U3D_DMAIESR         (SSUSB_DEV_BASE+0x0098)
#define U3D_DMAIECR         (SSUSB_DEV_BASE+0x009C)
#define U3D_EP0DMACTRL          (SSUSB_DEV_BASE+0x00C0)
#define U3D_EP0DMASTRADDR       (SSUSB_DEV_BASE+0x00C4)
#define U3D_EP0DMATFRCOUNT      (SSUSB_DEV_BASE+0x00C8)
#define U3D_EP0DMARLCOUNT       (SSUSB_DEV_BASE+0x00CC)
#define U3D_TXDMACTRL           (SSUSB_DEV_BASE+0x00D0)
#define U3D_TXDMASTRADDR        (SSUSB_DEV_BASE+0x00D4)
#define U3D_TXDMATRDCNT         (SSUSB_DEV_BASE+0x00D8)
#define U3D_TXDMARLCOUNT        (SSUSB_DEV_BASE+0x00DC)
#define U3D_RXDMACTRL           (SSUSB_DEV_BASE+0x00E0)
#define U3D_RXDMASTRADDR        (SSUSB_DEV_BASE+0x00E4)
#define U3D_RXDMATRDCNT         (SSUSB_DEV_BASE+0x00E8)
#define U3D_RXDMARLCOUNT        (SSUSB_DEV_BASE+0x00EC)
#define U3D_EP0CSR          (SSUSB_DEV_BASE+0x0100)
#define U3D_RXCOUNT0            (SSUSB_DEV_BASE+0x0108)
#define U3D_RESERVED            (SSUSB_DEV_BASE+0x010C)

/* U3D_TX(ep)CSR(x): ep(1-15), offset: 0x110-0x1f8 */
#define U3D_TXCSR_OFFSET        (SSUSB_DEV_BASE+0x0100)
/*
#define U3D_TX(ep)CSR_OFFSET        (U3D_TXCSR_OFFSET + ((ep) * (0x10)))
#define U3D_TX(ep)CSR(x)        ((U3D_TX(ep)CSR_OFFSET) + ((x) * 0x4))
*/

#define U3D_TX1CSR0         (SSUSB_DEV_BASE+0x0110)
#define U3D_TX1CSR1         (SSUSB_DEV_BASE+0x0114)
#define U3D_TX1CSR2         (SSUSB_DEV_BASE+0x0118)
#define U3D_TX2CSR0         (SSUSB_DEV_BASE+0x0120)
#define U3D_TX2CSR1         (SSUSB_DEV_BASE+0x0124)
#define U3D_TX2CSR2         (SSUSB_DEV_BASE+0x0128)
#define U3D_TX3CSR0         (SSUSB_DEV_BASE+0x0130)
#define U3D_TX3CSR1         (SSUSB_DEV_BASE+0x0134)
#define U3D_TX3CSR2         (SSUSB_DEV_BASE+0x0138)
#define U3D_TX4CSR0         (SSUSB_DEV_BASE+0x0140)
#define U3D_TX4CSR1         (SSUSB_DEV_BASE+0x0144)
#define U3D_TX4CSR2         (SSUSB_DEV_BASE+0x0148)
#define U3D_TX5CSR0         (SSUSB_DEV_BASE+0x0150)
#define U3D_TX5CSR1         (SSUSB_DEV_BASE+0x0154)
#define U3D_TX5CSR2         (SSUSB_DEV_BASE+0x0158)
#define U3D_TX6CSR0         (SSUSB_DEV_BASE+0x0160)
#define U3D_TX6CSR1         (SSUSB_DEV_BASE+0x0164)
#define U3D_TX6CSR2         (SSUSB_DEV_BASE+0x0168)
#define U3D_TX7CSR0         (SSUSB_DEV_BASE+0x0170)
#define U3D_TX7CSR1         (SSUSB_DEV_BASE+0x0174)
#define U3D_TX7CSR2         (SSUSB_DEV_BASE+0x0178)
#define U3D_TX8CSR0         (SSUSB_DEV_BASE+0x0180)
#define U3D_TX8CSR1         (SSUSB_DEV_BASE+0x0184)
#define U3D_TX8CSR2         (SSUSB_DEV_BASE+0x0188)
#define U3D_TX9CSR0         (SSUSB_DEV_BASE+0x0190)
#define U3D_TX9CSR1         (SSUSB_DEV_BASE+0x0194)
#define U3D_TX9CSR2         (SSUSB_DEV_BASE+0x0198)
#define U3D_TX10CSR0            (SSUSB_DEV_BASE+0x01A0)
#define U3D_TX10CSR1            (SSUSB_DEV_BASE+0x01A4)
#define U3D_TX10CSR2            (SSUSB_DEV_BASE+0x01A8)
#define U3D_TX11CSR0            (SSUSB_DEV_BASE+0x01B0)
#define U3D_TX11CSR1            (SSUSB_DEV_BASE+0x01B4)
#define U3D_TX11CSR2            (SSUSB_DEV_BASE+0x01B8)
#define U3D_TX12CSR0            (SSUSB_DEV_BASE+0x01C0)
#define U3D_TX12CSR1            (SSUSB_DEV_BASE+0x01C4)
#define U3D_TX12CSR2            (SSUSB_DEV_BASE+0x01C8)
#define U3D_TX13CSR0            (SSUSB_DEV_BASE+0x01D0)
#define U3D_TX13CSR1            (SSUSB_DEV_BASE+0x01D4)
#define U3D_TX13CSR2            (SSUSB_DEV_BASE+0x01D8)
#define U3D_TX14CSR0            (SSUSB_DEV_BASE+0x01E0)
#define U3D_TX14CSR1            (SSUSB_DEV_BASE+0x01E4)
#define U3D_TX14CSR2            (SSUSB_DEV_BASE+0x01E8)
#define U3D_TX15CSR0            (SSUSB_DEV_BASE+0x01F0)
#define U3D_TX15CSR1            (SSUSB_DEV_BASE+0x01F4)
#define U3D_TX15CSR2            (SSUSB_DEV_BASE+0x01F8)

/* U3D_RX(ep)CSR(x): ep(1-15), offset: 0x210-0x2f8 */
#define U3D_RXCSR_OFFSET        (SSUSB_DEV_BASE+0x0200)
/*
#define U3D_RX(ep)CSR_OFFSET        (U3D_RXCSR_OFFSET + ((ep) * (0x10)))
#define U3D_RX(ep)CSR(x)        ((U3D_RX(ep)CSR_OFFSET) + ((x) * 0x4))
*/

#define U3D_RX1CSR0         (SSUSB_DEV_BASE+0x0210)
#define U3D_RX1CSR1         (SSUSB_DEV_BASE+0x0214)
#define U3D_RX1CSR2         (SSUSB_DEV_BASE+0x0218)
#define U3D_RX1CSR3         (SSUSB_DEV_BASE+0x021C)
#define U3D_RX2CSR0         (SSUSB_DEV_BASE+0x0220)
#define U3D_RX2CSR1         (SSUSB_DEV_BASE+0x0224)
#define U3D_RX2CSR2         (SSUSB_DEV_BASE+0x0228)
#define U3D_RX2CSR3         (SSUSB_DEV_BASE+0x022C)
#define U3D_RX3CSR0         (SSUSB_DEV_BASE+0x0230)
#define U3D_RX3CSR1         (SSUSB_DEV_BASE+0x0234)
#define U3D_RX3CSR2         (SSUSB_DEV_BASE+0x0238)
#define U3D_RX3CSR3         (SSUSB_DEV_BASE+0x023C)
#define U3D_RX4CSR0         (SSUSB_DEV_BASE+0x0240)
#define U3D_RX4CSR1         (SSUSB_DEV_BASE+0x0244)
#define U3D_RX4CSR2         (SSUSB_DEV_BASE+0x0248)
#define U3D_RX4CSR3         (SSUSB_DEV_BASE+0x024C)
#define U3D_RX5CSR0         (SSUSB_DEV_BASE+0x0250)
#define U3D_RX5CSR1         (SSUSB_DEV_BASE+0x0254)
#define U3D_RX5CSR2         (SSUSB_DEV_BASE+0x0258)
#define U3D_RX5CSR3         (SSUSB_DEV_BASE+0x025C)
#define U3D_RX6CSR0         (SSUSB_DEV_BASE+0x0260)
#define U3D_RX6CSR1         (SSUSB_DEV_BASE+0x0264)
#define U3D_RX6CSR2         (SSUSB_DEV_BASE+0x0268)
#define U3D_RX6CSR3         (SSUSB_DEV_BASE+0x026C)
#define U3D_RX7CSR0         (SSUSB_DEV_BASE+0x0270)
#define U3D_RX7CSR1         (SSUSB_DEV_BASE+0x0274)
#define U3D_RX7CSR2         (SSUSB_DEV_BASE+0x0278)
#define U3D_RX7CSR3         (SSUSB_DEV_BASE+0x027C)
#define U3D_RX8CSR0         (SSUSB_DEV_BASE+0x0280)
#define U3D_RX8CSR1         (SSUSB_DEV_BASE+0x0284)
#define U3D_RX8CSR2         (SSUSB_DEV_BASE+0x0288)
#define U3D_RX8CSR3         (SSUSB_DEV_BASE+0x028C)
#define U3D_RX9CSR0         (SSUSB_DEV_BASE+0x0290)
#define U3D_RX9CSR1         (SSUSB_DEV_BASE+0x0294)
#define U3D_RX9CSR2         (SSUSB_DEV_BASE+0x0298)
#define U3D_RX9CSR3         (SSUSB_DEV_BASE+0x029C)
#define U3D_RX10CSR0            (SSUSB_DEV_BASE+0x02A0)
#define U3D_RX10CSR1            (SSUSB_DEV_BASE+0x02A4)
#define U3D_RX10CSR2            (SSUSB_DEV_BASE+0x02A8)
#define U3D_RX10CSR3            (SSUSB_DEV_BASE+0x02AC)
#define U3D_RX11CSR0            (SSUSB_DEV_BASE+0x02B0)
#define U3D_RX11CSR1            (SSUSB_DEV_BASE+0x02B4)
#define U3D_RX11CSR2            (SSUSB_DEV_BASE+0x02B8)
#define U3D_RX11CSR3            (SSUSB_DEV_BASE+0x02BC)
#define U3D_RX12CSR0            (SSUSB_DEV_BASE+0x02C0)
#define U3D_RX12CSR1            (SSUSB_DEV_BASE+0x02C4)
#define U3D_RX12CSR2            (SSUSB_DEV_BASE+0x02C8)
#define U3D_RX12CSR3            (SSUSB_DEV_BASE+0x02CC)
#define U3D_RX13CSR0            (SSUSB_DEV_BASE+0x02D0)
#define U3D_RX13CSR1            (SSUSB_DEV_BASE+0x02D4)
#define U3D_RX13CSR2            (SSUSB_DEV_BASE+0x02D8)
#define U3D_RX13CSR3            (SSUSB_DEV_BASE+0x02DC)
#define U3D_RX14CSR0            (SSUSB_DEV_BASE+0x02E0)
#define U3D_RX14CSR1            (SSUSB_DEV_BASE+0x02E4)
#define U3D_RX14CSR2            (SSUSB_DEV_BASE+0x02E8)
#define U3D_RX14CSR3            (SSUSB_DEV_BASE+0x02EC)
#define U3D_RX15CSR0            (SSUSB_DEV_BASE+0x02F0)
#define U3D_RX15CSR1            (SSUSB_DEV_BASE+0x02F4)
#define U3D_RX15CSR2            (SSUSB_DEV_BASE+0x02F8)
#define U3D_RX15CSR3            (SSUSB_DEV_BASE+0x02FC)

/* U3D_FIFO: ep(0-15), offset: 0x300-0x3f0 */
#define U3D_FIFO_OFFSET         (SSUSB_DEV_BASE + 0x0300)
#define U3D_FIFO(x)         (U3D_FIFO_OFFSET + ((x) * 0x10))
#define USB_FIFO(x)         (U3D_FIFO(x))

/* U3D_QCR: (0-3), offset: 0x400-0x40c */
#if 0
#define U3D_QCR_OFFSET          (SSUSB_DEV_BASE+0x0400)
#define U3D_QCR(x)          (U3D_QCR_OFFSET + ((x) * 0x4))
#endif
#define U3D_QCR0            (SSUSB_DEV_BASE+0x0400)
#define U3D_QCR1            (SSUSB_DEV_BASE+0x0404)
#define U3D_QCR2            (SSUSB_DEV_BASE+0x0408)
#define U3D_QCR3            (SSUSB_DEV_BASE+0x040C)

/* U3D_QGCSR: offset: 0x410 */
#define U3D_QGCSR           (SSUSB_DEV_BASE+0x0410)

/* U3D_TXQCSR: ep(1-15), offset: 0x510-0x5f8 */
#define U3D_TXQCSR_OFFSET       (SSUSB_DEV_BASE + 0x0500)
#define U3D_TXQCSR(x)           (U3D_TXQCSR_OFFSET + ((x) * 0x10))
#define U3D_TXQSAR(x)           (U3D_TXQCSR(x) + 0x4)
#define U3D_TXQCPR(x)           (U3D_TXQCSR(x) + 0x8)

/* U3D_RXQCSR: ep(1-15), offset: 0x610-0x5f8 */
#define U3D_RXQCSR_OFFSET       (SSUSB_DEV_BASE + 0x0600)
#define U3D_RXQCSR(x)           (U3D_RXQCSR_OFFSET + ((x) * 0x10))
#define U3D_RXQSAR(x)           (U3D_RXQCSR(x) + 0x4)
#define U3D_RXQCPR(x)           (U3D_RXQCSR(x) + 0x8)
#define U3D_RXQLDPR(x)          (U3D_RXQCSR(x) + 0xC)

#define U3D_QISAR0          (SSUSB_DEV_BASE+0x0700)
#define U3D_QIER0           (SSUSB_DEV_BASE+0x0704)
#define U3D_QIESR0          (SSUSB_DEV_BASE+0x0708)
#define U3D_QIECR0          (SSUSB_DEV_BASE+0x070C)
#define U3D_QISAR1          (SSUSB_DEV_BASE+0x0710)
#define U3D_QIER1           (SSUSB_DEV_BASE+0x0714)
#define U3D_QIESR1          (SSUSB_DEV_BASE+0x0718)
#define U3D_QIECR1          (SSUSB_DEV_BASE+0x071C)
#define U3D_QEMIR           (SSUSB_DEV_BASE+0x0740)
#define U3D_QEMIER          (SSUSB_DEV_BASE+0x0744)
#define U3D_QEMIESR         (SSUSB_DEV_BASE+0x0748)
#define U3D_QEMIECR         (SSUSB_DEV_BASE+0x074C)
#define U3D_TQERRIR0            (SSUSB_DEV_BASE+0x0780)
#define U3D_TQERRIER0           (SSUSB_DEV_BASE+0x0784)
#define U3D_TQERRIESR0          (SSUSB_DEV_BASE+0x0788)
#define U3D_TQERRIECR0          (SSUSB_DEV_BASE+0x078C)
#define U3D_RQERRIR0            (SSUSB_DEV_BASE+0x07C0)
#define U3D_RQERRIER0           (SSUSB_DEV_BASE+0x07C4)
#define U3D_RQERRIESR0          (SSUSB_DEV_BASE+0x07C8)
#define U3D_RQERRIECR0          (SSUSB_DEV_BASE+0x07CC)
#define U3D_RQERRIR1            (SSUSB_DEV_BASE+0x07D0)
#define U3D_RQERRIER1           (SSUSB_DEV_BASE+0x07D4)
#define U3D_RQERRIESR1          (SSUSB_DEV_BASE+0x07D8)
#define U3D_RQERRIECR1          (SSUSB_DEV_BASE+0x07DC)
#define U3D_CAP_EP0FFSZ         (SSUSB_DEV_BASE+0x0C04)
#define U3D_CAP_EPNTXFFSZ       (SSUSB_DEV_BASE+0x0C08)
#define U3D_CAP_EPNRXFFSZ       (SSUSB_DEV_BASE+0x0C0C)
#define U3D_CAP_EPINFO          (SSUSB_DEV_BASE+0x0C10)
#define U3D_CAP_TX_SLOT1        (SSUSB_DEV_BASE+0x0C20)
#define U3D_CAP_TX_SLOT2        (SSUSB_DEV_BASE+0x0C24)
#define U3D_CAP_TX_SLOT3        (SSUSB_DEV_BASE+0x0C28)
#define U3D_CAP_TX_SLOT4        (SSUSB_DEV_BASE+0x0C2C)
#define U3D_CAP_RX_SLOT1        (SSUSB_DEV_BASE+0x0C30)
#define U3D_CAP_RX_SLOT2        (SSUSB_DEV_BASE+0x0C34)
#define U3D_CAP_RX_SLOT3        (SSUSB_DEV_BASE+0x0C38)
#define U3D_CAP_RX_SLOT4        (SSUSB_DEV_BASE+0x0C3C)
#define U3D_MISC_CTRL           (SSUSB_DEV_BASE+0x0C84)

/* U3D_EPISR */
#define EPRISR              (0x7fff << 17) /* 31:17 */
#define SETUPENDISR         (0x1 << 16) /* 16:16 */
#define EPTISR              (0x7fff << 1) /* 15:1 */
#define EP0ISR              (0x1 << 0) /* 0:0 */

/* U3D_EP0CSR */
#define EP0_EP_RESET            (0x1 << 31) /* 31:31 */
#define EP0_AUTOCLEAR           (0x1 << 30) /* 30:30 */
#define EP0_AUTOSET         (0x1 << 29) /* 29:29 */
#define EP0_DMAREQEN            (0x1 << 28) /* 28:28 */
#define EP0_SENDSTALL           (0x1 << 25) /* 25:25 */
#define EP0_FIFOFULL            (0x1 << 23) /* 23:23 */
#define EP0_SENTSTALL           (0x1 << 22) /* 22:22 */
#define EP0_DPHTX           (0x1 << 20) /* 20:20 */
#define EP0_DATAEND         (0x1 << 19) /* 19:19 */
#define EP0_TXPKTRDY            (0x1 << 18) /* 18:18 */
#define EP0_SETUPPKTRDY         (0x1 << 17) /* 17:17 */
#define EP0_RXPKTRDY            (0x1 << 16) /* 16:16 */
#define EP0_MAXPKTSZ0           (0x3ff << 0) /* 9:0 */
#define EP0_W1C_BITS            (~(EP0_RXPKTRDY | EP0_SETUPPKTRDY | EP0_SENTSTALL))

/* U3D_LV1ISR */
#define EP_CTRL_INTR            (0x1 << 5) /* 5:5 */
#define MAC2_INTR           (0x1 << 4) /* 4:4 */
#define DMA_INTR            (0x1 << 3) /* 3:3 */
#define MAC3_INTR           (0x1 << 2) /* 2:2 */
#define QMU_INTR            (0x1 << 1) /* 1:1 */
#define BMU_INTR            (0x1 << 0) /* 0:0 */

/* U3D_DMAIESR */
#define RXDMAIESR           (0x1 << 2) /* 2:2 */
#define TXDMAIESR           (0x1 << 1) /* 1:1 */
#define EP0DMAIESR          (0x1 << 0) /* 0:0 */

/* U3D_TX1CSR0(TX1~TX15 are identical) */
#define TX_EP_RESET         (0x1 << 31) /* 31:31 */
#define TX_AUTOSET          (0x1 << 30) /* 30:30 */
#define TX_DMAREQEN         (0x1 << 29) /* 29:29 */
#define TX_FIFOFULL         (0x1 << 25) /* 25:25 */
#define TX_FIFOEMPTY            (0x1 << 24) /* 24:24 */
#define TX_SENTSTALL            (0x1 << 22) /* 22:22 */
#define TX_SENDSTALL            (0x1 << 21) /* 21:21 */
#define TX_TXPKTRDY         (0x1 << 16) /* 16:16 */
#define TX_TXMAXPKTSZ           (0x7ff << 0) /* 10:0 */
#define TX_W1C_BITS             (~(TX_SENTSTALL))

/* U3D_TX1CSR1(TX1~TX15 are identical) */
#define TX_MULT             (0x3 << 22) /* 23:22 */
#define TX_MAX_PKT          (0x3f << 16) /* 21:16 */
#define TX_SLOT             (0x3f << 8) /* 13:8 */
#define TXTYPE              (0x3 << 4) /* 5:4 */
#define SS_TX_BURST         (0xf << 0) /* 3:0 */
#define TX_MULT_OFST            (22)
#define TX_MAX_PKT_OFST         (16)
#define TX_SLOT_OFST            (8)
#define TXTYPE_OFST         (4)
#define SS_TX_BURST_OFST        (0)

/* U3D_TX1CSR2(TX1~TX15 are identical) */
#define TXBINTERVAL         (0xff << 24) /* 31:24 */
#define TXFIFOSEGSIZE           (0xf << 16) /* 19:16 */
#define TXFIFOADDR          (0x1fff << 0) /* 12:0 */
#define TXBINTERVAL_OFST        (24)
#define TXFIFOSEGSIZE_OFST      (16)
#define TXFIFOADDR_OFST         (0)

/* U3D_RX1CSR0(RX1~RX15 are identical) */
#define RX_EP_RESET         (0x1 << 31) /* 31:31 */
#define RX_AUTOCLEAR            (0x1 << 30) /* 30:30 */
#define RX_DMAREQEN         (0x1 << 29) /* 29:29 */
#define RX_SENTSTALL            (0x1 << 22) /* 22:22 */
#define RX_SENDSTALL            (0x1 << 21) /* 21:21 */
#define RX_FIFOFULL         (0x1 << 18) /* 18:18 */
#define RX_FIFOEMPTY            (0x1 << 17) /* 17:17 */
#define RX_RXPKTRDY         (0x1 << 16) /* 16:16 */
#define RX_RXMAXPKTSZ           (0x7ff << 0) /* 10:0 */
#define RX_W1C_BITS             (~(RX_SENTSTALL|RX_RXPKTRDY))

/* U3D_RX1CSR1(RX1~RX15 are identical) */
#define RX_MULT             (0x3 << 22) /* 23:22 */
#define RX_MAX_PKT          (0x3f << 16) /* 21:16 */
#define RX_SLOT             (0x3f << 8) /* 13:8 */
#define RX_TYPE             (0x3 << 4) /* 5:4 */
#define SS_RX_BURST         (0xf << 0) /* 3:0 */
#define RX_MULT_OFST            (22)
#define RX_MAX_PKT_OFST         (16)
#define RX_SLOT_OFST            (8)
#define RX_TYPE_OFST            (4)
#define SS_RX_BURST_OFST        (0)

/* U3D_RX1CSR2(RX1~RX15 are identical) */
#define RXBINTERVAL         (0xff << 24) /* 31:24 */
#define RXFIFOSEGSIZE           (0xf << 16) /* 19:16 */
#define RXFIFOADDR          (0x1fff << 0) /* 12:0 */
#define RXBINTERVAL_OFST        (24)
#define RXFIFOSEGSIZE_OFST      (16)
#define RXFIFOADDR_OFST         (0)

/* U3D_RX1CSR3(RX1~RX15 are identical) */
#define EP_RX_COUNT         (0x7ff << 16) /* 26:16 */
#define EP_RX_COUNT_OFST        (16)

/* SSUSB_USB2_CSR REGISTER DEFINITION */
#define U3D_XHCI_PORT_CTRL      (SSUSB_USB2_CSR_BASE+0x0000)
#define U3D_POWER_MANAGEMENT        (SSUSB_USB2_CSR_BASE+0x0004)
#define U3D_TIMING_TEST_MODE        (SSUSB_USB2_CSR_BASE+0x0008)
#define U3D_DEVICE_CONTROL      (SSUSB_USB2_CSR_BASE+0x000C)
#define U3D_POWER_UP_COUNTER        (SSUSB_USB2_CSR_BASE+0x0010)
#define U3D_USB2_TEST_MODE      (SSUSB_USB2_CSR_BASE+0x0014)
#define U3D_COMMON_USB_INTR_ENABLE  (SSUSB_USB2_CSR_BASE+0x0018)
#define U3D_COMMON_USB_INTR     (SSUSB_USB2_CSR_BASE+0x001C)
#define U3D_USB_BUS_PERFORMANCE     (SSUSB_USB2_CSR_BASE+0x0020)
#define U3D_LINK_RESET_INFO     (SSUSB_USB2_CSR_BASE+0x0024)
#define U3D_RESET_RESUME_TIME_VALUE (SSUSB_USB2_CSR_BASE+0x0034)
#define U3D_UTMI_SIGNAL_SEL     (SSUSB_USB2_CSR_BASE+0x0038)
#define U3D_USB20_FRAME_NUM     (SSUSB_USB2_CSR_BASE+0x003C)
#define U3D_USB20_TIMING_PARAM      (SSUSB_USB2_CSR_BASE+0x0040)
#define U3D_USB20_LPM_PARAM     (SSUSB_USB2_CSR_BASE+0x0044)
#define U3D_USB20_LPM_ENTRY_COUNT   (SSUSB_USB2_CSR_BASE+0x0048)
#define U3D_USB20_MISC_CONTROL      (SSUSB_USB2_CSR_BASE+0x004C)
#define U3D_USB20_LPM_TIMING_PARAM  (SSUSB_USB2_CSR_BASE+0x0050)

/* U3D_USB20_MISC_CONTROL */
#define LPM_U3_ACK_EN           (0x1 << 0) /* 0:0 */

/* U3D_COMMON_USB_INTR_ENABLE */
#define LPM_RESUME_INTR_EN      (0x1 << 9) /* 9:9 */
#define LPM_INTR_EN         (0x1 << 8) /* 8:8 */
#define VBUSERR_INTR_EN         (0x1 << 7) /* 7:7 */
#define SESSION_REQ_INTR_EN     (0x1 << 6) /* 6:6 */
#define DISCONN_INTR_EN         (0x1 << 5) /* 5:5 */
#define CONN_INTR_EN            (0x1 << 4) /* 4:4 */
#define SOF_INTR_EN         (0x1 << 3) /* 3:3 */
#define RESET_INTR_EN           (0x1 << 2) /* 2:2 */
#define RESUME_INTR_EN          (0x1 << 1) /* 1:1 */
#define SUSPEND_INTR_EN         (0x1 << 0) /* 0:0 */

/* U3D_LINK_RESET_INFO */
#define WRFSSE0             (0xf << 20) /* 23:20 */
#define WTCHRP              (0xf << 16) /* 19:16 */
#define VPLEN               (0xff << 8) /* 15:8 */
#define WTCON               (0xf << 4) /* 7:4 */
#define WTID                (0xf << 0) /* 3:0 */
#define WRFSSE0_OFST            (20)
#define WTCHRP_OFST         (16)
#define VPLEN_OFST          (8)
#define WTCON_OFST          (4)
#define WTID_OFST           (0)

/* U3D_USB20_TIMING_PARAM */
#define CHOPPER_DELAY_TIME      (0xff << 16) /* 23:16 */
#define SOFTCON_DELAY_TIME      (0xff << 8) /* 15:8 */
#define TIME_VALUE_1US          (0xff << 0) /* 7:0 */

/* U3D_POWER_MANAGEMENT */
#define LPM_BESL_STALL          (0x1 << 14) /* 14:14 */
#define LPM_BESLD_STALL         (0x1 << 13) /* 13:13 */
#define BC12_EN             (0x1 << 12) /* 12:12 */
#define LPM_RWP             (0x1 << 11) /* 11:11 */
#define LPM_HRWE            (0x1 << 10) /* 10:10 */
#define LPM_MODE            (0x3 << 8) /* 9:8 */
#define ISO_UPDATE          (0x1 << 7) /* 7:7 */
#define SOFT_CONN           (0x1 << 6) /* 6:6 */
#define HS_ENABLE           (0x1 << 5) /* 5:5 */
#define USB_HS_MODE         (0x1 << 4) /* 4:4 */
#define BUS_RESET           (0x1 << 3) /* 3:3 */
#define RESUME              (0x1 << 2) /* 2:2 */
#define SUSPEND             (0x1 << 1) /* 1:1 */
#define SUSPENDM_ENABLE         (0x1 << 0) /* 0:0 */

/* LPM_MODE */
#define LPM_ALWAYS_REJECT_REQ       (0x01)

/* U3D_COMMON_USB_INTR */
#define LPM_RESUME_INTR         (0x1 << 9) /* 9:9 */
#define LPM_INTR            (0x1 << 8) /* 8:8 */
#define VBUSERR_INTR            (0x1 << 7) /* 7:7 */
#define SESSION_REQ_INTR        (0x1 << 6) /* 6:6 */
#define DISCONN_INTR            (0x1 << 5) /* 5:5 */
#define CONN_INTR           (0x1 << 4) /* 4:4 */
#define SOF_INTR            (0x1 << 3) /* 3:3 */
#define RESET_INTR          (0x1 << 2) /* 2:2 */
#define RESUME_INTR         (0x1 << 1) /* 1:1 */
#define SUSPEND_INTR            (0x1 << 0) /* 0:0 */

/* SSUSB_EPCTL_CSR REGISTER DEFINITION */
#define U3D_DEVICE_CONF         (SSUSB_EPCTL_CSR_BASE+0x0000)
#define U3D_EP_RST          (SSUSB_EPCTL_CSR_BASE+0x0004)
#define U3D_USB3_ERDY_TIMING_PARAM  (SSUSB_EPCTL_CSR_BASE+0x0008)
#define U3D_USB3_EPCTRL_CAP     (SSUSB_EPCTL_CSR_BASE+0x000C)
#define U3D_USB2_ISOINEP_INCOMP_INTR    (SSUSB_EPCTL_CSR_BASE+0x0010)
#define U3D_USB2_ISOOUTEP_INCOMP_ERR    (SSUSB_EPCTL_CSR_BASE+0x0014)
#define U3D_ISO_UNDERRUN_INTR       (SSUSB_EPCTL_CSR_BASE+0x0018)
#define U3D_ISO_OVERRUN_INTR        (SSUSB_EPCTL_CSR_BASE+0x001C)
#define U3D_USB2_RX_EP_DATAERR_INTR (SSUSB_EPCTL_CSR_BASE+0x0020)
#define U3D_USB2_EPCTRL_CAP     (SSUSB_EPCTL_CSR_BASE+0x0024)
#define U3D_USB2_EPCTL_LPM      (SSUSB_EPCTL_CSR_BASE+0x0028)
#define U3D_USB3_SW_ERDY        (SSUSB_EPCTL_CSR_BASE+0x0030)
#define U3D_EP_FLOW_CTRL        (SSUSB_EPCTL_CSR_BASE+0x0040)
#define U3D_USB3_EP_ACT         (SSUSB_EPCTL_CSR_BASE+0x0044)
#define U3D_USB3_EP_PACKET_PENDING  (SSUSB_EPCTL_CSR_BASE+0x0048)
#define U3D_DEV_LINK_INTR_ENABLE    (SSUSB_EPCTL_CSR_BASE+0x0050)
#define U3D_DEV_LINK_INTR       (SSUSB_EPCTL_CSR_BASE+0x0054)
#define U3D_USB2_EPCTL_LPM_FC_CHK   (SSUSB_EPCTL_CSR_BASE+0x0060)
#define U3D_DEVICE_MONITOR      (SSUSB_EPCTL_CSR_BASE+0x0064)

/* U3D_EP_RST */
#define EP_IN_RST           (0x7fff << 17) /* 31:17 */
#define EP_OUT_RST          (0x7fff << 1) /* 15:1 */
#define EP0_RST             (0x1 << 0) /* 0:0 */

/* U3D_DEV_LINK_INTR */
#define SSUSB_DEV_SPEED_CHG_INTR    (0x1 << 0) /* 0:0 */

/* U3D_DEV_LINK_INTR_ENABLE */
#define SSUSB_DEV_SPEED_CHG_INTR_EN (0x1 << 0) /* 0:0 */

/* U3D_DEVICE_CONF */
#define DEV_ADDR            (0x7f << 24) /* 30:24 */
#define HW_USB2_3_SEL           (0x1 << 18) /* 18:18 */
#define SW_USB2_3_SEL_EN        (0x1 << 17) /* 17:17 */
#define SW_USB2_3_SEL           (0x1 << 16) /* 16:16 */
#define SSUSB_DEV_SPEED         (0x7 << 0) /* 2:0 */
#define DEV_ADDR_OFST           (24)
#define HW_USB2_3_SEL_OFST      (18)
#define SW_USB2_3_SEL_EN_OFST       (17)
#define SW_USB2_3_SEL_OFST      (16)
#define SSUSB_DEV_SPEED_OFST        (0)

#define RISC_SIZE_1B 0x0
#define RISC_SIZE_2B 0x1
#define RISC_SIZE_4B 0x2

/* SSUSB_USB3_SYS_CSR REGISTER DEFINITION */
#define U3D_LINK_HP_TIMER       (SSUSB_USB3_SYS_CSR_BASE+0x0200)
#define U3D_LINK_CMD_TIMER      (SSUSB_USB3_SYS_CSR_BASE+0x0204)
#define U3D_LINK_PM_TIMER       (SSUSB_USB3_SYS_CSR_BASE+0x0208)
#define U3D_LINK_UX_INACT_TIMER     (SSUSB_USB3_SYS_CSR_BASE+0x020C)
#define U3D_LINK_POWER_CONTROL      (SSUSB_USB3_SYS_CSR_BASE+0x0210)
#define U3D_LINK_ERR_COUNT      (SSUSB_USB3_SYS_CSR_BASE+0x0214)
#define U3D_LTSSM_TRANSITION        (SSUSB_USB3_SYS_CSR_BASE+0x0218)
#define U3D_LINK_RETRY_CTRL     (SSUSB_USB3_SYS_CSR_BASE+0x0220)
#define U3D_SYS_FAST_SIMULATIION    (SSUSB_USB3_SYS_CSR_BASE+0x0224)
#define U3D_LINK_CAPABILITY_CTRL    (SSUSB_USB3_SYS_CSR_BASE+0x0228)
#define U3D_LINK_DEBUG_INFO     (SSUSB_USB3_SYS_CSR_BASE+0x022C)
#define U3D_USB3_U1_REJECT      (SSUSB_USB3_SYS_CSR_BASE+0x0240)
#define U3D_USB3_U2_REJECT      (SSUSB_USB3_SYS_CSR_BASE+0x0244)
#define U3D_DEV_NOTIF_0         (SSUSB_USB3_SYS_CSR_BASE+0x0290)
#define U3D_DEV_NOTIF_1         (SSUSB_USB3_SYS_CSR_BASE+0x0294)
#define U3D_VENDOR_DEV_TEST     (SSUSB_USB3_SYS_CSR_BASE+0x0298)
#define U3D_VENDOR_DEF_DATA_LOW     (SSUSB_USB3_SYS_CSR_BASE+0x029C)
#define U3D_VENDOR_DEF_DATA_HIGH    (SSUSB_USB3_SYS_CSR_BASE+0x02A0)
#define U3D_HOST_SET_PORT_CTRL      (SSUSB_USB3_SYS_CSR_BASE+0x02A4)
#define U3D_LINK_CAP_CONTROL        (SSUSB_USB3_SYS_CSR_BASE+0x02AC)
#define U3D_PORT_CONF_TIMEOUT       (SSUSB_USB3_SYS_CSR_BASE+0x02B0)
#define U3D_TIMING_PULSE_CTRL       (SSUSB_USB3_SYS_CSR_BASE+0x02B4)
#define U3D_ISO_TIMESTAMP       (SSUSB_USB3_SYS_CSR_BASE+0x02B8)
#define U3D_RECEIVE_PKT_INTR_EN     (SSUSB_USB3_SYS_CSR_BASE+0x02C0)
#define U3D_RECEIVE_PKT_INTR        (SSUSB_USB3_SYS_CSR_BASE+0x02C4)
#define U3D_CRC_ERR_INTR_EN     (SSUSB_USB3_SYS_CSR_BASE+0x02C8)
#define U3D_CRC_ERR_INTR        (SSUSB_USB3_SYS_CSR_BASE+0x02CC)
#define U3D_PORT_STATUS_INTR_EN     (SSUSB_USB3_SYS_CSR_BASE+0x02D0)
#define U3D_PORT_STATUS_INTR        (SSUSB_USB3_SYS_CSR_BASE+0x02D4)
#define U3D_RECOVERY_COUNT      (SSUSB_USB3_SYS_CSR_BASE+0x02D8)
#define U3D_T2R_LOOPBACK_TEST       (SSUSB_USB3_SYS_CSR_BASE+0x02DC)

/* U3D_LINK_POWER_CONTROL */
#define SW_U2_ACCEPT_ENABLE     (0x1 << 9) /* 9:9 */
#define SW_U1_ACCEPT_ENABLE     (0x1 << 8) /* 8:8 */
#define UX_EXIT             (0x1 << 5) /* 5:5 */
#define LGO_U3              (0x1 << 4) /* 4:4 */
#define LGO_U2              (0x1 << 3) /* 3:3 */
#define LGO_U1              (0x1 << 2) /* 2:2 */
#define SW_U2_REQUEST_ENABLE        (0x1 << 1) /* 1:1 */
#define SW_U1_REQUEST_ENABLE        (0x1 << 0) /* 0:0 */

/* U3D_LINK_CAPABILITY_CTRL */
#define INSERT_CRC32_ERR_DP_NUM     (0xff << 16) /* 23:16 */
#define INSERT_CRC32_ERR_EN     (0x1 << 8) /* 8:8 */
#define ZLP_CRC32_CHK_DIS       (0x1 << 0) /* 0:0 */

/* U3D_TIMING_PULSE_CTRL */
#define CNT_1MS_VALUE           (0xf << 28) /* 31:28 */
#define CNT_100US_VALUE         (0xf << 24) /* 27:24 */
#define CNT_10US_VALUE          (0xf << 20) /* 23:20 */
#define CNT_1US_VALUE           (0xff << 0) /* 7:0 */


#define USB_END_OFFSET(_bEnd, _bOffset) ((0x10*(_bEnd-1)) + _bOffset)
#define USB_WRITECSR32(_bOffset, _bEnd, _bData) \
    writel(_bData, USB_END_OFFSET(_bEnd, _bOffset))
#define USB_READCSR32(_bOffset, _bEnd) \
    readl(USB_END_OFFSET(_bEnd, _bOffset))

void explain_csr0(u32 csr0);

/* USB SET/CLEAR MASK MACRO */
#define setbits(addr, mask) writel(readl(addr) | mask, addr)
#define clrbits(addr, mask) writel(readl(addr) &~mask, addr)

#if defined(DBG_USB_EP0CSR)
#define ep0csr_setbits(mask) \
    u32 _temp = readl(U3D_EP0CSR); \
    explain_csr0(_temp); \
    writel((_temp & EP0_W1C_BITS) | (mask), U3D_EP0CSR); \
    explain_csr0(readl(U3D_EP0CSR));

#else
#define ep0csr_setbits(mask) \
    u32 _temp = readl(U3D_EP0CSR); \
    writel((_temp & EP0_W1C_BITS) | (mask), U3D_EP0CSR);

#endif /* if defined(DBG_USB_EP0CSR) */

#ifdef SUPPORT_U3
#define U3D_DFT_SPEED SSUSB_SPEED_SUPER
#else
#define U3D_DFT_SPEED SSUSB_SPEED_HIGH
#endif

#define TX_FIFO_NUM 5   /* including ep0 */
#define RX_FIFO_NUM 5   /* including ep0 */

#define USB_FIFOSZ_SIZE_8           (0x03)
#define USB_FIFOSZ_SIZE_16          (0x04)
#define USB_FIFOSZ_SIZE_32          (0x05)
#define USB_FIFOSZ_SIZE_64          (0x06)
#define USB_FIFOSZ_SIZE_128         (0x07)
#define USB_FIFOSZ_SIZE_256         (0x08)
#define USB_FIFOSZ_SIZE_512         (0x09)
#define USB_FIFOSZ_SIZE_1024            (0x0A)
#define USB_FIFOSZ_SIZE_2048            (0x0B)
#define USB_FIFOSZ_SIZE_4096            (0x0C)
#define USB_FIFOSZ_SIZE_8192            (0x0D)
#define USB_FIFOSZ_SIZE_16384           (0x0E)
#define USB_FIFOSZ_SIZE_32768           (0x0F)

#define BIT0                    (1 << 0)
#define BIT16                   (1 << 16)

/* RXxCSR1 or TXxCSR1 */
#define TYPE_BULK               (0x00)
#define TYPE_INT                (0x10)
#define TYPE_ISO                (0x20)
#define TYPE_MASK               (0x30)

#ifdef SUPPORT_QMU
/* from mu3d_hal_hw.h */
/* QMU macros */
#if 0
#define U3D_TXQCSR1             (SSUSB_DEV_BASE+0x0510)
#define U3D_TXQSAR1             (SSUSB_DEV_BASE+0x0514)
#define U3D_TXQCPR1             (SSUSB_DEV_BASE+0x0518)
#define U3D_RXQCSR1             (SSUSB_DEV_BASE+0x0610)
#define U3D_RXQSAR1             (SSUSB_DEV_BASE+0x0614)
#define U3D_RXQCPR1             (SSUSB_DEV_BASE+0x0618)
#define U3D_RXQLDPR1                (SSUSB_DEV_BASE+0x061C)

#define USB_QMU_RXQCSR(n)           (U3D_RXQCSR1 +  0x0010 * ((n)-1))
#define USB_QMU_RXQSAR(n)           (U3D_RXQSAR1 +  0x0010 * ((n)-1))
#define USB_QMU_RXQCPR(n)           (U3D_RXQCPR1 +  0x0010 * ((n)-1))
#define USB_QMU_RXQLDPR(n)          (U3D_RXQLDPR1 + 0x0010 * ((n)-1))
#define USB_QMU_TXQCSR(n)           (U3D_TXQCSR1 +  0x0010 * ((n)-1))
#define USB_QMU_TXQSAR(n)           (U3D_TXQSAR1 +  0x0010 * ((n)-1))
#define USB_QMU_TXQCPR(n)           (U3D_TXQCPR1 +  0x0010 * ((n)-1))

/* ?? */
#define USB_QMU_RQCSR(n)            (U3D_RXQCSR1 +  0x0010 * ((n)-1))
#define USB_QMU_RQSAR(n)            (U3D_RXQSAR1 +  0x0010 * ((n)-1))
#define USB_QMU_RQCPR(n)            (U3D_RXQCPR1 +  0x0010 * ((n)-1))
#define USB_QMU_RQLDPR(n)           (U3D_RXQLDPR1 + 0x0010 * ((n)-1))
#define USB_QMU_TQCSR(n)            (U3D_TXQCSR1 +  0x0010 * ((n)-1))
#define USB_QMU_TQSAR(n)            (U3D_TXQSAR1 +  0x0010 * ((n)-1))
#define USB_QMU_TQCPR(n)            (U3D_TXQCPR1 +  0x0010 * ((n)-1))
#endif

#define QMU_Q_START             (0x00000001)
#define QMU_Q_RESUME                (0x00000002)
#define QMU_Q_STOP              (0x00000004)
#define QMU_Q_ACTIVE                (0x00008000)

#define QMU_TX_EN(n)                (BIT0 << (n))
#define QMU_RX_EN(n)                (BIT16 << (n))
#define QMU_TX_CS_EN(n)             (BIT0 << (n))
#define QMU_RX_CS_EN(n)             (BIT16 << (n))
#define QMU_TX_ZLP(n)               (BIT0 << (n))
#define QMU_RX_MULTIPLE(n)          (BIT16 << ((n)-1))
#define QMU_RX_ZLP(n)               (BIT0 << (n))
#define QMU_RX_COZ(n)               (BIT16 << (n))

#define QMU_RX_EMPTY(n)             (BIT16 << (n))
#define QMU_TX_EMPTY(n)             (BIT0 << (n))
#define QMU_RX_DONE(n)              (BIT16 << (n))
#define QMU_TX_DONE(n)              (BIT0 << (n))

#define QMU_RX_ZLP_ERR(n)           (BIT16 << (n))
#define QMU_RX_EP_ERR(n)            (BIT0 << (n))
#define QMU_RX_LEN_ERR(n)           (BIT16 << (n))
#define QMU_RX_CS_ERR(n)            (BIT0 << (n))

#define QMU_TX_LEN_ERR(n)           (BIT16 << (n))
#define QMU_TX_CS_ERR(n)            (BIT0 << (n))

#define CS_12B 1
#define CS_16B 2
#define CHECKSUM_TYPE CS_16B
#define U3D_COMMAND_TIMER 10

#if (CHECKSUM_TYPE==CS_16B)
#define CHECKSUM_LENGTH 16
#else
#define CHECKSUM_LENGTH 12
#endif

/* from ssusb_dev_c_header.h */
/* U3D_QCR0 */
#define RXQ_CS_EN               (0x7fff << 17) /* 31:17 */
#define TXQ_CS_EN               (0x7fff << 1) /* 15:1 */
#define CS16B_EN                (0x1 << 0) /* 0:0 */

/* U3D_QCR1 */
#define CFG_TX_ZLP_GPD              (0x7fff << 1) /* 15:1 */

/* U3D_QCR2 */
#define CFG_TX_ZLP              (0x7fff << 1) /* 15:1 */

/* U3D_QCR3 */
#define CFG_RX_COZ              (0x7fff << 17) /* 31:17 */
#define CFG_RX_ZLP              (0x7fff << 1) /* 15:1 */

/* U3D_QGCSR */
#define RXQ_EN                  (0x7fff << 17) /* 31:17 */
#define TXQ_EN                  (0x7fff << 1) /* 15:1 */

/* U3D_TXQCSR1 */
#define TXQ_DMGR_DMSM_CS            (0xf << 16) /* 19:16 */
#define TXQ_ACTIVE              (0x1 << 15) /* 15:15 */
#define TXQ_EPQ_STATE               (0xf << 8) /* 11:8 */
#define TXQ_STOP                (0x1 << 2) /* 2:2 */
#define TXQ_RESUME              (0x1 << 1) /* 1:1 */
#define TXQ_START               (0x1 << 0) /* 0:0 */

/* U3D_TXQSAR1 */
#define TXQ_START_ADDR              (0x3fffffff << 2) /* 31:2 */

/* U3D_TXQCPR1 */
#define TXQ_CUR_GPD_ADDR            (0x3fffffff << 2) /* 31:2 */

/* U3D_RXQCSR1 */
#define RXQ_DMGR_DMSM_CS            (0xf << 16) /* 19:16 */
#define RXQ_ACTIVE              (0x1 << 15) /* 15:15 */
#define RXQ_EPQ_STATE               (0x1f << 8) /* 12:8 */
#define RXQ_STOP                (0x1 << 2) /* 2:2 */
#define RXQ_RESUME              (0x1 << 1) /* 1:1 */
#define RXQ_START               (0x1 << 0) /* 0:0 */

/* U3D_RXQSAR1 */
#define RXQ_START_ADDR              (0x3fffffff << 2) /* 31:2 */

/* U3D_RXQCPR1 */
#define RXQ_CUR_GPD_ADDR            (0x3fffffff << 2) /* 31:2 */

/* U3D_RXQLDPR1 */
#define RXQ_LAST_DONE_PTR           (0x3fffffff << 2) /* 31:2 */

/* U3D_QISAR0 */
#define RXQ_DONE_INT                (0x7fff << 17) /* 31:17 */
#define TXQ_DONE_INT                (0x7fff << 1) /* 15:1 */

/* U3D_QIER0 */
#define RXQ_DONE_IER                (0x7fff << 17) /* 31:17 */
#define TXQ_DONE_IER                (0x7fff << 1) /* 15:1 */

/* U3D_QIESR0 */
#define RXQ_DONE_IESR               (0x7fff << 17) /* 31:17 */
#define TXQ_DONE_IESR               (0x7fff << 1) /* 15:1 */

/* U3D_QIECR0 */
#define RXQ_DONE_IECR               (0x7fff << 17) /* 31:17 */
#define TXQ_DONE_IECR               (0x7fff << 1) /* 15:1 */

/* U3D_QISAR1 */
#define RXQ_ZLPERR_INT              (0x1 << 20) /* 20:20 */
#define RXQ_LENERR_INT              (0x1 << 18) /* 18:18 */
#define RXQ_CSERR_INT               (0x1 << 17) /* 17:17 */
#define RXQ_EMPTY_INT               (0x1 << 16) /* 16:16 */
#define TXQ_LENERR_INT              (0x1 << 2) /* 2:2 */
#define TXQ_CSERR_INT               (0x1 << 1) /* 1:1 */
#define TXQ_EMPTY_INT               (0x1 << 0) /* 0:0 */

/* U3D_QIER1 */
#define RXQ_ZLPERR_IER              (0x1 << 20) /* 20:20 */
#define RXQ_LENERR_IER              (0x1 << 18) /* 18:18 */
#define RXQ_CSERR_IER               (0x1 << 17) /* 17:17 */
#define RXQ_EMPTY_IER               (0x1 << 16) /* 16:16 */
#define TXQ_LENERR_IER              (0x1 << 2) /* 2:2 */
#define TXQ_CSERR_IER               (0x1 << 1) /* 1:1 */
#define TXQ_EMPTY_IER               (0x1 << 0) /* 0:0 */

/* U3D_QIESR1 */
#define RXQ_ZLPERR_IESR             (0x1 << 20) /* 20:20 */
#define RXQ_LENERR_IESR             (0x1 << 18) /* 18:18 */
#define RXQ_CSERR_IESR              (0x1 << 17) /* 17:17 */
#define RXQ_EMPTY_IESR              (0x1 << 16) /* 16:16 */
#define TXQ_LENERR_IESR             (0x1 << 2) /* 2:2 */
#define TXQ_CSERR_IESR              (0x1 << 1) /* 1:1 */
#define TXQ_EMPTY_IESR              (0x1 << 0) /* 0:0 */

/* U3D_QIECR1 */
#define RXQ_ZLPERR_IECR             (0x1 << 20) /* 20:20 */
#define RXQ_LENERR_IECR             (0x1 << 18) /* 18:18 */
#define RXQ_CSERR_IECR              (0x1 << 17) /* 17:17 */
#define RXQ_EMPTY_IECR              (0x1 << 16) /* 16:16 */
#define TXQ_LENERR_IECR             (0x1 << 2) /* 2:2 */
#define TXQ_CSERR_IECR              (0x1 << 1) /* 1:1 */
#define TXQ_EMPTY_IECR              (0x1 << 0) /* 0:0 */

/* U3D_QEMIR */
#define RXQ_EMPTY_MASK              (0x7fff << 17) /* 31:17 */
#define TXQ_EMPTY_MASK              (0x7fff << 1) /* 15:1 */

/* U3D_QEMIER */
#define RXQ_EMPTY_IER_MASK          (0x7fff << 17) /* 31:17 */
#define TXQ_EMPTY_IER_MASK          (0x7fff << 1) /* 15:1 */

/* U3D_QEMIESR */
#define RXQ_EMPTY_IESR_MASK         (0x7fff << 17) /* 31:17 */
#define TXQ_EMPTY_IESR_MASK         (0x7fff << 1) /* 15:1 */

/* U3D_QEMIECR */
#define RXQ_EMPTY_IECR_MASK         (0x7fff << 17) /* 31:17 */
#define TXQ_EMPTY_IECR_MASK         (0x7fff << 1) /* 15:1 */

/* U3D_TQERRIR0 */
#define TXQ_LENERR_MASK             (0x7fff << 17) /* 31:17 */
#define TXQ_CSERR_MASK              (0x7fff << 1) /* 15:1 */

/* U3D_TQERRIER0 */
#define TXQ_LENERR_IER_MASK         (0x7fff << 17) /* 31:17 */
#define TXQ_CSERR_IER_MASK          (0x7fff << 1) /* 15:1 */

/* U3D_TQERRIESR0 */
#define TXQ_LENERR_IESR_MASK            (0x7fff << 17) /* 31:17 */
#define TXQ_CSERR_IESR_MASK         (0x7fff << 1) /* 15:1 */

/* U3D_TQERRIECR0 */
#define TXQ_LENERR_IECR_MASK            (0x7fff << 17) /* 31:17 */
#define TXQ_CSERR_IECR_MASK         (0x7fff << 1) /* 15:1 */

/* U3D_RQERRIR0 */
#define RXQ_LENERR_MASK             (0x7fff << 17) /* 31:17 */
#define RXQ_CSERR_MASK              (0x7fff << 1) /* 15:1 */

/* U3D_RQERRIER0 */
#define RXQ_LENERR_IER_MASK         (0x7fff << 17) /* 31:17 */
#define RXQ_CSERR_IER_MASK          (0x7fff << 1) /* 15:1 */

/* U3D_RQERRIESR0 */
#define RXQ_LENERR_IESR_MASK            (0x7fff << 17) /* 31:17 */
#define RXQ_CSERR_IESR_MASK         (0x7fff << 1) /* 15:1 */

/* U3D_RQERRIECR0 */
#define RXQ_LENERR_IECR_MASK            (0x7fff << 17) /* 31:17 */
#define RXQ_CSERR_IECR_MASK         (0x7fff << 1) /* 15:1 */

/* U3D_RQERRIR1 */
#define RXQ_ZLPERR_MASK             (0x7fff << 17) /* 31:17 */

/* U3D_RQERRIER1 */
#define RXQ_ZLPERR_IER_MASK         (0x7fff << 17) /* 31:17 */

/* U3D_RQERRIESR1 */
#define RXQ_ZLPERR_IESR_MASK            (0x7fff << 17) /* 31:17 */

/* U3D_RQERRIECR1 */
#define RXQ_ZLPERR_IECR_MASK            (0x7fff << 17) /* 31:17 */

#endif /* ifdef SUPPORT_QMU */

#ifdef EXT_VBUS_DET
#define FPGA_REG 0xf0008098
#define VBUS_RISE_BIT   (1 << 11) /* W1C */
#define VBUS_FALL_BIT   (1 << 12) /* W1C */
#define VBUS_MSK    (VBUS_RISE_BIT | VBUS_FALL_BIT)
#define VBUS_RISE_IRQ 13
#define VBUS_FALL_IRQ 14
#endif

/* mt_usb defines */
typedef enum {
	EP0_IDLE = 0,
	EP0_RX,
	EP0_TX,
} EP0_STATE;

typedef enum {
	RET_SUCCESS = 0,
	RET_FAIL,
} USB_RESULT;

/* MTK USB3 ssusb defines */
typedef enum {
	SSUSB_SPEED_UNKNOWN = 0,
	SSUSB_SPEED_FULL = 1,
	SSUSB_SPEED_HIGH = 2,
	SSUSB_SPEED_SUPER = 3,
} USB_SPEED;

/* from preloader */
#define USB_EP_XFER_CTRL    0
#define USB_EP_XFER_ISO     1
#define USB_EP_XFER_BULK    2
#define USB_EP_XFER_INT     3

/* some macros */
#define EPMASK(X)   (1 << X)
#define CHANMASK(X) (1 << X)

void mt_usb_phy_poweron(void);
void mt_usb_phy_savecurrent(void);

/* mt_usbtty.c */
void enable_highspeed(void);
void enable_superspeed(void);

/* mt_usb.c */
struct udc_endpoint *mt_find_ep(int ep_num, u8 dir);
void mt_setup_ep(unsigned int ep_num, struct udc_endpoint *endpoint);
void mt_usb_connect_internal(void);
void mt_usb_disconnect_internal(void);

/* USB transfer directions */
#define USB_DIR_IN  DEVICE_WRITE    /* val: 0x80 */
#define USB_DIR_OUT DEVICE_READ /* val: 0x00 */

struct urb {
	struct udc_endpoint *ept;
	struct udc_device *device;
	struct setup_packet device_request;

	u8 *buf;
	unsigned int actual_length;

//#ifdef SUPPORT_QMU
	u32 qmu_complete;
//#endif
};

/* endpoint data - mt_ep */
struct udc_endpoint {
	/* rx side */
	struct urb *rcv_urb;    /* active urb */

	/* tx side */
	struct urb *tx_urb; /* active urb */

	/* info from hsusb */
	struct udc_request *req;
	unsigned int bit;   /* EPT_TX/EPT_RX */
	unsigned char num;
	unsigned char in;

	unsigned int maxpkt;
	int status;     /* status for error handling */

	unsigned int sent;  /* data already sent */
	unsigned int last;  /* data sent in last packet XXX do we need this */
	unsigned char mode; /* double buffer */
	unsigned char type; /* Transfer type */

	unsigned char slot;
	unsigned char burst;
	unsigned char mult;
	unsigned char binterval;
};

/* from mt_usbtty.h */
#define NUM_ENDPOINTS   3

/* origin endpoint_array */
struct udc_endpoint ep_list[NUM_ENDPOINTS + 1]; /* one extra for control endpoint */

#endif
