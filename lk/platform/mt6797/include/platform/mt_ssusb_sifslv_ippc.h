/*
 * Copyright(c) 2013 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 *(the "Software"), to deal in the Software without restriction,
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

#ifndef _MT_SSUSB_SIFSLV_IPPC_H_
#define _MT_SSUSB_SIFSLV_IPPC_H_

/* SSUSB_SIFSLV_IPPC REGISTER DEFINITION */

/* Newer address for phy clock and ref clock setting -- begin */
/* USB3_SIF_BASE */
#define U3D_SPLLC_XTALCTL3          (SSUSB_SIFSLV_SPLLC_BASE+0x0018)    /* SIF+0x018 */
#define U3D_PHYD_CDR1               (SSUSB_SIFSLV_U3PHYD_BASE+0x005c)   /* SIF+0x95c */

/* USB3_SIF2_BASE */
#define U3D_USB30_PHYA_REG0         (SSUSB_USB30_PHYA_SIV_B_BASE+0x0000)    /* SIF2+0xB00 */
#define U3D_USB30_PHYA_REG6         (SSUSB_USB30_PHYA_SIV_B_BASE+0x0018)    /* SIF2+0xB18 */
#define U3D_U3PHYA_DA_REG0          (SSUSB_SIFSLV_U3PHYA_DA_BASE+0x0000)    /* SIF2+0xC00 */

/* U3D_USB30_PHYA_REG0 */
#define RG_SSUSB_VUSB10_ON          (1<<5)
#define RG_SSUSB_VUSB10_ON_OFST     (5)
/* Newer address for phy clock and ref clock setting -- end */

/* referenecd from ssusb_USB20_PHY_regmap_com_T28.xls */
#define U3D_USBPHYACR0              (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0000) /* bit 2~bit 30 */
#define U3D_USBPHYACR1              (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0004)
#define U3D_USBPHYACR2              (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0008) /* bit 0~ bit15 */
#define U3D_USBPHYACR4              (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0010)
#define U3D_USBPHYACR5              (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0014)
#define U3D_USBPHYACR6              (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0018)
#define U3D_U2PHYACR3               (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x001c)
#define U3D_U2PHYACR4_0             (SSUSB_SIFSLV_U2PHY_COM_SIV_B_BASE+0x0020) /* bit0~ bit5 */

#define U3D_U2PHYACR4               (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0020) /* bit8~ bit18 */
#define U3D_U2PHYAMON0              (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0024)
#define U3D_U2PHYDCR0               (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0060)
#define U3D_U2PHYDCR1               (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0064)
#define U3D_U2PHYDTM0               (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0068)
#define U3D_U2PHYDTM1               (SSUSB_SIFSLV_U2PHY_COM_BASE+0x006C)
#define U3D_U2PHYDMON0              (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0070)
#define U3D_U2PHYDMON1              (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0074)
#define U3D_U2PHYDMON2              (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0078)
#define U3D_U2PHYDMON3              (SSUSB_SIFSLV_U2PHY_COM_BASE+0x007C)
#define U3D_U2PHYBC12C              (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0080)
#define U3D_U2PHYBC12C1             (SSUSB_SIFSLV_U2PHY_COM_BASE+0x0084)
#define U3D_U2PHYREGFPPC            (SSUSB_SIFSLV_U2PHY_COM_BASE+0x00e0)
#define U3D_U2PHYVERSIONC           (SSUSB_SIFSLV_U2PHY_COM_BASE+0x00f0)
#define U3D_U2PHYREGFCOM            (SSUSB_SIFSLV_U2PHY_COM_BASE+0x00fc)

#define U3D_SSUSB_IP_PW_CTRL0           (SSUSB_SIFSLV_IPPC_BASE+0x0000)
#define U3D_SSUSB_IP_PW_CTRL1           (SSUSB_SIFSLV_IPPC_BASE+0x0004)
#define U3D_SSUSB_IP_PW_CTRL2           (SSUSB_SIFSLV_IPPC_BASE+0x0008)
#define U3D_SSUSB_IP_PW_CTRL3           (SSUSB_SIFSLV_IPPC_BASE+0x000C)
#define U3D_SSUSB_IP_PW_STS1            (SSUSB_SIFSLV_IPPC_BASE+0x0010)
#define U3D_SSUSB_IP_PW_STS2            (SSUSB_SIFSLV_IPPC_BASE+0x0014)
#define U3D_SSUSB_IP_MAC_CAP            (SSUSB_SIFSLV_IPPC_BASE+0x0020)
#define U3D_SSUSB_IP_XHCI_CAP           (SSUSB_SIFSLV_IPPC_BASE+0x0024)
#define U3D_SSUSB_IP_DEV_CAP            (SSUSB_SIFSLV_IPPC_BASE+0x0028)
#define U3D_SSUSB_U3_CTRL_0P            (SSUSB_SIFSLV_IPPC_BASE+0x0030)
#define U3D_SSUSB_U3_CTRL_1P            (SSUSB_SIFSLV_IPPC_BASE+0x0038)
#define U3D_SSUSB_U3_CTRL_2P            (SSUSB_SIFSLV_IPPC_BASE+0x0040)
#define U3D_SSUSB_U3_CTRL_3P            (SSUSB_SIFSLV_IPPC_BASE+0x0048)
#define U3D_SSUSB_U2_CTRL_0P            (SSUSB_SIFSLV_IPPC_BASE+0x0050)
#define U3D_SSUSB_U2_CTRL_1P            (SSUSB_SIFSLV_IPPC_BASE+0x0058)
#define U3D_SSUSB_U2_CTRL_2P            (SSUSB_SIFSLV_IPPC_BASE+0x0060)
#define U3D_SSUSB_U2_CTRL_3P            (SSUSB_SIFSLV_IPPC_BASE+0x0068)
#define U3D_SSUSB_U2_CTRL_4P            (SSUSB_SIFSLV_IPPC_BASE+0x0070)
#define U3D_SSUSB_U2_CTRL_5P            (SSUSB_SIFSLV_IPPC_BASE+0x0078)
#define U3D_SSUSB_U2_PHY_PLL            (SSUSB_SIFSLV_IPPC_BASE+0x007C)
#define U3D_SSUSB_DMA_CTRL          (SSUSB_SIFSLV_IPPC_BASE+0x0080)
#define U3D_SSUSB_MAC_CK_CTRL           (SSUSB_SIFSLV_IPPC_BASE+0x0084)
#define U3D_SSUSB_CSR_CK_CTRL           (SSUSB_SIFSLV_IPPC_BASE+0x0088)
#define U3D_SSUSB_REF_CK_CTRL           (SSUSB_SIFSLV_IPPC_BASE+0x008C)
#define U3D_SSUSB_XHCI_CK_CTRL          (SSUSB_SIFSLV_IPPC_BASE+0x0090)
#define U3D_SSUSB_XHCI_RST_CTRL         (SSUSB_SIFSLV_IPPC_BASE+0x0094)
#define U3D_SSUSB_DEV_RST_CTRL          (SSUSB_SIFSLV_IPPC_BASE+0x0098)
#define U3D_SSUSB_SYS_CK_CTRL           (SSUSB_SIFSLV_IPPC_BASE+0x009C)
#define U3D_SSUSB_HW_ID             (SSUSB_SIFSLV_IPPC_BASE+0x00A0)
#define U3D_SSUSB_HW_SUB_ID         (SSUSB_SIFSLV_IPPC_BASE+0x00A4)
#define U3D_SSUSB_PRB_CTRL0         (SSUSB_SIFSLV_IPPC_BASE+0x00B0)
#define U3D_SSUSB_PRB_CTRL1         (SSUSB_SIFSLV_IPPC_BASE+0x00B4)
#define U3D_SSUSB_PRB_CTRL2         (SSUSB_SIFSLV_IPPC_BASE+0x00B8)
#define U3D_SSUSB_PRB_CTRL3         (SSUSB_SIFSLV_IPPC_BASE+0x00BC)
#define U3D_SSUSB_PRB_CTRL4         (SSUSB_SIFSLV_IPPC_BASE+0x00C0)
#define U3D_SSUSB_PRB_CTRL5         (SSUSB_SIFSLV_IPPC_BASE+0x00C4)
#define U3D_SSUSB_IP_SPARE0         (SSUSB_SIFSLV_IPPC_BASE+0x00C8)
#define U3D_SSUSB_IP_SPARE1         (SSUSB_SIFSLV_IPPC_BASE+0x00CC)
#define U3D_SSUSB_FPGA_I2C_OUT_0P       (SSUSB_SIFSLV_IPPC_BASE+0x00D0)
#define U3D_SSUSB_FPGA_I2C_IN_0P        (SSUSB_SIFSLV_IPPC_BASE+0x00D4)
#define U3D_SSUSB_FPGA_I2C_OUT_1P       (SSUSB_SIFSLV_IPPC_BASE+0x00D8)
#define U3D_SSUSB_FPGA_I2C_IN_1P        (SSUSB_SIFSLV_IPPC_BASE+0x00DC)
#define U3D_SSUSB_FPGA_I2C_OUT_2P       (SSUSB_SIFSLV_IPPC_BASE+0x00E0)
#define U3D_SSUSB_FPGA_I2C_IN_2P        (SSUSB_SIFSLV_IPPC_BASE+0x00E4)
#define U3D_SSUSB_FPGA_I2C_OUT_3P       (SSUSB_SIFSLV_IPPC_BASE+0x00E8)
#define U3D_SSUSB_FPGA_I2C_IN_3P        (SSUSB_SIFSLV_IPPC_BASE+0x00EC)
#define U3D_SSUSB_FPGA_I2C_OUT_4P       (SSUSB_SIFSLV_IPPC_BASE+0x00F0)
#define U3D_SSUSB_FPGA_I2C_IN_4P        (SSUSB_SIFSLV_IPPC_BASE+0x00F4)
#define U3D_SSUSB_IP_SLV_TMOUT          (SSUSB_SIFSLV_IPPC_BASE+0x00F8)

/* SSUSB_SIFSLV_IPPC FIELD DEFINITION */

/* U3D_SSUSB_IP_PW_CTRL0 */
#define SSUSB_IP_U2_ENTER_SLEEP_CNT     (0xff << 8) /* 15:8 */
#define SSUSB_IP_SW_RST             (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_PW_CTRL1 */
#define SSUSB_IP_HOST_PDN           (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_PW_CTRL2 */
#define SSUSB_IP_DEV_PDN            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_PW_CTRL3 */
#define SSUSB_IP_PCIE_PDN           (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_PW_STS1 */
#define SSUSB_U2_MAC_RST_B_STS_5P       (0x1 << 29) /* 29:29 */
#define SSUSB_U2_MAC_RST_B_STS_4P       (0x1 << 28) /* 28:28 */
#define SSUSB_U2_MAC_RST_B_STS_3P       (0x1 << 27) /* 27:27 */
#define SSUSB_U2_MAC_RST_B_STS_2P       (0x1 << 26) /* 26:26 */
#define SSUSB_U2_MAC_RST_B_STS_1P       (0x1 << 25) /* 25:25 */
#define SSUSB_U2_MAC_RST_B_STS          (0x1 << 24) /* 24:24 */
#define SSUSB_U3_MAC_RST_B_STS_3P       (0x1 << 19) /* 19:19 */
#define SSUSB_U3_MAC_RST_B_STS_2P       (0x1 << 18) /* 18:18 */
#define SSUSB_U3_MAC_RST_B_STS_1P       (0x1 << 17) /* 17:17 */
#define SSUSB_U3_MAC_RST_B_STS          (0x1 << 16) /* 16:16 */
#define SSUSB_DEV_DRAM_RST_B_STS        (0x1 << 13) /* 13:13 */
#define SSUSB_XHCI_DRAM_RST_B_STS       (0x1 << 12) /* 12:12 */
#define SSUSB_XHCI_RST_B_STS            (0x1 << 11) /* 11:11 */
#define SSUSB_SYS125_RST_B_STS          (0x1 << 10) /* 10:10 */
#define SSUSB_SYS60_RST_B_STS           (0x1 << 9) /* 9:9 */
#define SSUSB_REF_RST_B_STS         (0x1 << 8) /* 8:8 */
#define SSUSB_DEV_RST_B_STS         (0x1 << 3) /* 3:3 */
#define SSUSB_DEV_BMU_RST_B_STS         (0x1 << 2) /* 2:2 */
#define SSUSB_DEV_QMU_RST_B_STS         (0x1 << 1) /* 1:1 */
#define SSUSB_SYSPLL_STABLE         (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_PW_STS2 */
#define SSUSB_U2_MAC_SYS_RST_B_STS_5P       (0x1 << 5) /* 5:5 */
#define SSUSB_U2_MAC_SYS_RST_B_STS_4P       (0x1 << 4) /* 4:4 */
#define SSUSB_U2_MAC_SYS_RST_B_STS_3P       (0x1 << 3) /* 3:3 */
#define SSUSB_U2_MAC_SYS_RST_B_STS_2P       (0x1 << 2) /* 2:2 */
#define SSUSB_U2_MAC_SYS_RST_B_STS_1P       (0x1 << 1) /* 1:1 */
#define SSUSB_U2_MAC_SYS_RST_B_STS      (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_MAC_CAP */
#define SSUSB_IP_MAC_U2_PORT_NO         (0xff << 8) /* 15:8 */
#define SSUSB_IP_MAC_U3_PORT_NO         (0xff << 0) /* 7:0 */

/* U3D_SSUSB_IP_XHCI_CAP */
#define SSUSB_IP_XHCI_U2_PORT_NO        (0xff << 8) /* 15:8 */
#define SSUSB_IP_XHCI_U3_PORT_NO        (0xff << 0) /* 7:0 */

/* U3D_SSUSB_IP_DEV_CAP */
#define SSUSB_IP_DEV_U2_PORT_NO         (0xff << 8) /* 15:8 */
#define SSUSB_IP_DEV_U3_PORT_NO         (0xff << 0) /* 7:0 */

/* U3D_SSUSB_U3_CTRL_0P */
#define SSUSB_U3_PORT_PHYD_RST          (0x1 << 5) /* 5:5 */
#define SSUSB_U3_PORT_MAC_RST           (0x1 << 4) /* 4:4 */
#define SSUSB_U3_PORT_U2_CG_EN          (0x1 << 3) /* 3:3 */
#define SSUSB_U3_PORT_HOST_SEL          (0x1 << 2) /* 2:2 */
#define SSUSB_U3_PORT_PDN           (0x1 << 1) /* 1:1 */
#define SSUSB_U3_PORT_DIS           (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U3_CTRL_1P */
#define SSUSB_U3_PORT_PHYD_RST_1P       (0x1 << 5) /* 5:5 */
#define SSUSB_U3_PORT_MAC_RST_1P        (0x1 << 4) /* 4:4 */
#define SSUSB_U3_PORT_U2_CG_EN_1P       (0x1 << 3) /* 3:3 */
#define SSUSB_U3_PORT_HOST_SEL_1P       (0x1 << 2) /* 2:2 */
#define SSUSB_U3_PORT_PDN_1P            (0x1 << 1) /* 1:1 */
#define SSUSB_U3_PORT_DIS_1P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U3_CTRL_2P */
#define SSUSB_U3_PORT_PHYD_RST_2P       (0x1 << 5) /* 5:5 */
#define SSUSB_U3_PORT_MAC_RST_2P        (0x1 << 4) /* 4:4 */
#define SSUSB_U3_PORT_U2_CG_EN_2P       (0x1 << 3) /* 3:3 */
#define SSUSB_U3_PORT_HOST_SEL_2P       (0x1 << 2) /* 2:2 */
#define SSUSB_U3_PORT_PDN_2P            (0x1 << 1) /* 1:1 */
#define SSUSB_U3_PORT_DIS_2P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U3_CTRL_3P */
#define SSUSB_U3_PORT_PHYD_RST_3P       (0x1 << 5) /* 5:5 */
#define SSUSB_U3_PORT_MAC_RST_3P        (0x1 << 4) /* 4:4 */
#define SSUSB_U3_PORT_U2_CG_EN_3P       (0x1 << 3) /* 3:3 */
#define SSUSB_U3_PORT_HOST_SEL_3P       (0x1 << 2) /* 2:2 */
#define SSUSB_U3_PORT_PDN_3P            (0x1 << 1) /* 1:1 */
#define SSUSB_U3_PORT_DIS_3P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_CTRL_0P */
#define SSUSB_U2_PORT_SYS_CK_SEL        (0x1 << 6) /* 6:6 */
#define SSUSB_U2_PORT_PHYD_RST          (0x1 << 5) /* 5:5 */
#define SSUSB_U2_PORT_MAC_RST           (0x1 << 4) /* 4:4 */
#define SSUSB_U2_PORT_U2_CG_EN          (0x1 << 3) /* 3:3 */
#define SSUSB_U2_PORT_HOST_SEL          (0x1 << 2) /* 2:2 */
#define SSUSB_U2_PORT_PDN           (0x1 << 1) /* 1:1 */
#define SSUSB_U2_PORT_DIS           (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_CTRL_1P */
#define SSUSB_U2_PORT_SYS_CK_SEL_1P     (0x1 << 6) /* 6:6 */
#define SSUSB_U2_PORT_PHYD_RST_1P       (0x1 << 5) /* 5:5 */
#define SSUSB_U2_PORT_MAC_RST_1P        (0x1 << 4) /* 4:4 */
#define SSUSB_U2_PORT_U2_CG_EN_1P       (0x1 << 3) /* 3:3 */
#define SSUSB_U2_PORT_HOST_SEL_1P       (0x1 << 2) /* 2:2 */
#define SSUSB_U2_PORT_PDN_1P            (0x1 << 1) /* 1:1 */
#define SSUSB_U2_PORT_DIS_1P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_CTRL_2P */
#define SSUSB_U2_PORT_SYS_CK_SEL_2P     (0x1 << 6) /* 6:6 */
#define SSUSB_U2_PORT_PHYD_RST_2P       (0x1 << 5) /* 5:5 */
#define SSUSB_U2_PORT_MAC_RST_2P        (0x1 << 4) /* 4:4 */
#define SSUSB_U2_PORT_U2_CG_EN_2P       (0x1 << 3) /* 3:3 */
#define SSUSB_U2_PORT_HOST_SEL_2P       (0x1 << 2) /* 2:2 */
#define SSUSB_U2_PORT_PDN_2P            (0x1 << 1) /* 1:1 */
#define SSUSB_U2_PORT_DIS_2P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_CTRL_3P */
#define SSUSB_U2_PORT_SYS_CK_SEL_3P     (0x1 << 6) /* 6:6 */
#define SSUSB_U2_PORT_PHYD_RST_3P       (0x1 << 5) /* 5:5 */
#define SSUSB_U2_PORT_MAC_RST_3P        (0x1 << 4) /* 4:4 */
#define SSUSB_U2_PORT_U2_CG_EN_3P       (0x1 << 3) /* 3:3 */
#define SSUSB_U2_PORT_HOST_SEL_3P       (0x1 << 2) /* 2:2 */
#define SSUSB_U2_PORT_PDN_3P            (0x1 << 1) /* 1:1 */
#define SSUSB_U2_PORT_DIS_3P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_CTRL_4P */
#define SSUSB_U2_PORT_SYS_CK_SEL_4P     (0x1 << 6) /* 6:6 */
#define SSUSB_U2_PORT_PHYD_RST_4P       (0x1 << 5) /* 5:5 */
#define SSUSB_U2_PORT_MAC_RST_4P        (0x1 << 4) /* 4:4 */
#define SSUSB_U2_PORT_U2_CG_EN_4P       (0x1 << 3) /* 3:3 */
#define SSUSB_U2_PORT_HOST_SEL_4P       (0x1 << 2) /* 2:2 */
#define SSUSB_U2_PORT_PDN_4P            (0x1 << 1) /* 1:1 */
#define SSUSB_U2_PORT_DIS_4P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_CTRL_5P */
#define SSUSB_U2_PORT_SYS_CK_SEL_5P     (0x1 << 6) /* 6:6 */
#define SSUSB_U2_PORT_PHYD_RST_5P       (0x1 << 5) /* 5:5 */
#define SSUSB_U2_PORT_MAC_RST_5P        (0x1 << 4) /* 4:4 */
#define SSUSB_U2_PORT_U2_CG_EN_5P       (0x1 << 3) /* 3:3 */
#define SSUSB_U2_PORT_HOST_SEL_5P       (0x1 << 2) /* 2:2 */
#define SSUSB_U2_PORT_PDN_5P            (0x1 << 1) /* 1:1 */
#define SSUSB_U2_PORT_DIS_5P            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_U2_PHY_PLL */
#define SSUSB_U2_PORT_PLL_STABLE_TIMER      (0xff << 8) /* 15:8 */
#define SSUSB_U2_PORT_1US_TIMER         (0xff << 0) /* 7:0 */

/* U3D_SSUSB_DMA_CTRL */
#define SSUSB_IP_DMA_BUS_CK_GATE_DIS        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_MAC_CK_CTRL */
#define SSUSB_MAC3_SYS_CK_GATE_MASK_TIME    (0xff << 16) /* 23:16 */
#define SSUSB_MAC2_SYS_CK_GATE_MASK_TIME    (0xff << 8) /* 15:8 */
#define SSUSB_MAC3_SYS_CK_GATE_MODE     (0x3 << 2) /* 3:2 */
#define SSUSB_MAC2_SYS_CK_GATE_MODE     (0x3 << 0) /* 1:0 */

/* U3D_SSUSB_CSR_CK_CTRL */
#define SSUSB_CSR_MCU_BUS_CK_GATE_EN        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_REF_CK_CTRL */
#define SSUSB_REF_MAC3_CK_GATE_EN       (0x1 << 3) /* 3:3 */
#define SSUSB_REF_CK_GATE_EN            (0x1 << 2) /* 2:2 */
#define SSUSB_REF_PHY_CK_GATE_EN        (0x1 << 1) /* 1:1 */
#define SSUSB_REF_MAC_CK_GATE_EN        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_XHCI_CK_CTRL */
#define SSUSB_XACT3_XHCI_CK_GATE_MASK_TIME  (0xff << 8) /* 15:8 */
#define SSUSB_XACT3_XHCI_CK_GATE_MODE       (0x3 << 4) /* 5:4 */
#define SSUSB_XHCI_CK_DIV2_EN           (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_XHCI_RST_CTRL */
#define SSUSB_XHCI_SW_DRAM_RST          (0x1 << 4) /* 4:4 */
#define SSUSB_XHCI_SW_SYS60_RST         (0x1 << 3) /* 3:3 */
#define SSUSB_XHCI_SW_SYS125_RST        (0x1 << 2) /* 2:2 */
#define SSUSB_XHCI_SW_XHCI_RST          (0x1 << 1) /* 1:1 */
#define SSUSB_XHCI_SW_RST           (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_DEV_RST_CTRL */
#define SSUSB_DEV_SW_DRAM_RST           (0x1 << 3) /* 3:3 */
#define SSUSB_DEV_SW_QMU_RST            (0x1 << 2) /* 2:2 */
#define SSUSB_DEV_SW_BMU_RST            (0x1 << 1) /* 1:1 */
#define SSUSB_DEV_SW_RST            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_SYS_CK_CTRL */
#define SSUSB_SYS_CK_DIV2_EN            (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_HW_ID */
#define SSUSB_HW_ID             (0xffffffff << 0) /* 31:0 */

/* U3D_SSUSB_HW_SUB_ID */
#define SSUSB_HW_SUB_ID             (0xffffffff << 0) /* 31:0 */

/* U3D_SSUSB_PRB_CTRL0 */
#define PRB_BYTE3_EN                (0x1 << 3) /* 3:3 */
#define PRB_BYTE2_EN                (0x1 << 2) /* 2:2 */
#define PRB_BYTE1_EN                (0x1 << 1) /* 1:1 */
#define PRB_BYTE0_EN                (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_PRB_CTRL1 */
#define PRB_BYTE1_SEL               (0xffff << 16) /* 31:16 */
#define PRB_BYTE0_SEL               (0xffff << 0) /* 15:0 */

/* U3D_SSUSB_PRB_CTRL2 */
#define PRB_BYTE3_SEL               (0xffff << 16) /* 31:16 */
#define PRB_BYTE2_SEL               (0xffff << 0) /* 15:0 */

/* U3D_SSUSB_PRB_CTRL3 */
#define PRB_BYTE3_MODULE_SEL            (0xff << 24) /* 31:24 */
#define PRB_BYTE2_MODULE_SEL            (0xff << 16) /* 23:16 */
#define PRB_BYTE1_MODULE_SEL            (0xff << 8) /* 15:8 */
#define PRB_BYTE0_MODULE_SEL            (0xff << 0) /* 7:0 */

/* U3D_SSUSB_PRB_CTRL4 */
#define SW_PRB_OUT              (0xffffffff << 0) /* 31:0 */

/* U3D_SSUSB_PRB_CTRL5 */
#define PRB_RD_DATA             (0xffffffff << 0) /* 31:0 */

/* U3D_SSUSB_IP_SPARE0 */
#define SSUSB_IP_SPARE0             (0xffffffff << 0) /* 31:0 */

/* U3D_SSUSB_IP_SPARE1 */
#define SSUSB_IP_SPARE1             (0xffffffff << 0) /* 31:0 */

/* U3D_SSUSB_FPGA_I2C_OUT_0P */
#define SSUSB_FPGA_I2C_SCL_OEN_0P       (0x1 << 3) /* 3:3 */
#define SSUSB_FPGA_I2C_SCL_OUT_0P       (0x1 << 2) /* 2:2 */
#define SSUSB_FPGA_I2C_SDA_OEN_0P       (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_OUT_0P       (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_IN_0P */
#define SSUSB_FPGA_I2C_SCL_IN_0P        (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_IN_0P        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_OUT_1P */
#define SSUSB_FPGA_I2C_SCL_OEN_1P       (0x1 << 3) /* 3:3 */
#define SSUSB_FPGA_I2C_SCL_OUT_1P       (0x1 << 2) /* 2:2 */
#define SSUSB_FPGA_I2C_SDA_OEN_1P       (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_OUT_1P       (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_IN_1P */
#define SSUSB_FPGA_I2C_SCL_IN_1P        (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_IN_1P        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_OUT_2P */
#define SSUSB_FPGA_I2C_SCL_OEN_2P       (0x1 << 3) /* 3:3 */
#define SSUSB_FPGA_I2C_SCL_OUT_2P       (0x1 << 2) /* 2:2 */
#define SSUSB_FPGA_I2C_SDA_OEN_2P       (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_OUT_2P       (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_IN_2P */
#define SSUSB_FPGA_I2C_SCL_IN_2P        (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_IN_2P        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_OUT_3P */
#define SSUSB_FPGA_I2C_SCL_OEN_3P       (0x1 << 3) /* 3:3 */
#define SSUSB_FPGA_I2C_SCL_OUT_3P       (0x1 << 2) /* 2:2 */
#define SSUSB_FPGA_I2C_SDA_OEN_3P       (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_OUT_3P       (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_IN_3P */
#define SSUSB_FPGA_I2C_SCL_IN_3P        (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_IN_3P        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_OUT_4P */
#define SSUSB_FPGA_I2C_SCL_OEN_4P       (0x1 << 3) /* 3:3 */
#define SSUSB_FPGA_I2C_SCL_OUT_4P       (0x1 << 2) /* 2:2 */
#define SSUSB_FPGA_I2C_SDA_OEN_4P       (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_OUT_4P       (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_FPGA_I2C_IN_4P */
#define SSUSB_FPGA_I2C_SCL_IN_4P        (0x1 << 1) /* 1:1 */
#define SSUSB_FPGA_I2C_SDA_IN_4P        (0x1 << 0) /* 0:0 */

/* U3D_SSUSB_IP_SLV_TMOUT */
#define SSUSB_IP_SLV_TMOUT          (0xffffffff << 0) /* 31:0 */

/* SSUSB_SIFSLV_IPPC FIELD OFFSET DEFINITION */
/* U3D_SSUSB_IP_PW_CTRL0 */
#define SSUSB_IP_U2_ENTER_SLEEP_CNT_OFST    (8)
#define SSUSB_IP_SW_RST_OFST            (0)

/* U3D_SSUSB_IP_PW_CTRL1 */
#define SSUSB_IP_HOST_PDN_OFST          (0)

/* U3D_SSUSB_IP_PW_CTRL2 */
#define SSUSB_IP_DEV_PDN_OFST           (0)

/* U3D_SSUSB_IP_PW_CTRL3 */
#define SSUSB_IP_PCIE_PDN_OFST          (0)

/* U3D_SSUSB_IP_PW_STS1 */
#define SSUSB_U2_MAC_RST_B_STS_5P_OFST      (29)
#define SSUSB_U2_MAC_RST_B_STS_4P_OFST      (28)
#define SSUSB_U2_MAC_RST_B_STS_3P_OFST      (27)
#define SSUSB_U2_MAC_RST_B_STS_2P_OFST      (26)
#define SSUSB_U2_MAC_RST_B_STS_1P_OFST      (25)
#define SSUSB_U2_MAC_RST_B_STS_OFST     (24)
#define SSUSB_U3_MAC_RST_B_STS_3P_OFST      (19)
#define SSUSB_U3_MAC_RST_B_STS_2P_OFST      (18)
#define SSUSB_U3_MAC_RST_B_STS_1P_OFST      (17)
#define SSUSB_U3_MAC_RST_B_STS_OFST     (16)
#define SSUSB_DEV_DRAM_RST_B_STS_OFST       (13)
#define SSUSB_XHCI_DRAM_RST_B_STS_OFST      (12)
#define SSUSB_XHCI_RST_B_STS_OFST       (11)
#define SSUSB_SYS125_RST_B_STS_OFST     (10)
#define SSUSB_SYS60_RST_B_STS_OFST      (9)
#define SSUSB_REF_RST_B_STS_OFST        (8)
#define SSUSB_DEV_RST_B_STS_OFST        (3)
#define SSUSB_DEV_BMU_RST_B_STS_OFST        (2)
#define SSUSB_DEV_QMU_RST_B_STS_OFST        (1)
#define SSUSB_SYSPLL_STABLE_OFST        (0)

/* U3D_SSUSB_IP_PW_STS2 */
#define SSUSB_U2_MAC_SYS_RST_B_STS_5P_OFST  (5)
#define SSUSB_U2_MAC_SYS_RST_B_STS_4P_OFST  (4)
#define SSUSB_U2_MAC_SYS_RST_B_STS_3P_OFST  (3)
#define SSUSB_U2_MAC_SYS_RST_B_STS_2P_OFST  (2)
#define SSUSB_U2_MAC_SYS_RST_B_STS_1P_OFST  (1)
#define SSUSB_U2_MAC_SYS_RST_B_STS_OFST     (0)

/* U3D_SSUSB_IP_MAC_CAP */
#define SSUSB_IP_MAC_U2_PORT_NO_OFST        (8)
#define SSUSB_IP_MAC_U3_PORT_NO_OFST        (0)

/* U3D_SSUSB_IP_XHCI_CAP */
#define SSUSB_IP_XHCI_U2_PORT_NO_OFST       (8)
#define SSUSB_IP_XHCI_U3_PORT_NO_OFST       (0)

/* U3D_SSUSB_IP_DEV_CAP */
#define SSUSB_IP_DEV_U2_PORT_NO_OFST        (8)
#define SSUSB_IP_DEV_U3_PORT_NO_OFST        (0)

/* U3D_SSUSB_U3_CTRL_0P */
#define SSUSB_U3_PORT_PHYD_RST_OFST     (5)
#define SSUSB_U3_PORT_MAC_RST_OFST      (4)
#define SSUSB_U3_PORT_U2_CG_EN_OFST     (3)
#define SSUSB_U3_PORT_HOST_SEL_OFST     (2)
#define SSUSB_U3_PORT_PDN_OFST          (1)
#define SSUSB_U3_PORT_DIS_OFST          (0)

/* U3D_SSUSB_U3_CTRL_1P */
#define SSUSB_U3_PORT_PHYD_RST_1P_OFST      (5)
#define SSUSB_U3_PORT_MAC_RST_1P_OFST       (4)
#define SSUSB_U3_PORT_U2_CG_EN_1P_OFST      (3)
#define SSUSB_U3_PORT_HOST_SEL_1P_OFST      (2)
#define SSUSB_U3_PORT_PDN_1P_OFST       (1)
#define SSUSB_U3_PORT_DIS_1P_OFST       (0)

/* U3D_SSUSB_U3_CTRL_2P */
#define SSUSB_U3_PORT_PHYD_RST_2P_OFST      (5)
#define SSUSB_U3_PORT_MAC_RST_2P_OFST       (4)
#define SSUSB_U3_PORT_U2_CG_EN_2P_OFST      (3)
#define SSUSB_U3_PORT_HOST_SEL_2P_OFST      (2)
#define SSUSB_U3_PORT_PDN_2P_OFST       (1)
#define SSUSB_U3_PORT_DIS_2P_OFST       (0)

/* U3D_SSUSB_U3_CTRL_3P */
#define SSUSB_U3_PORT_PHYD_RST_3P_OFST      (5)
#define SSUSB_U3_PORT_MAC_RST_3P_OFST       (4)
#define SSUSB_U3_PORT_U2_CG_EN_3P_OFST      (3)
#define SSUSB_U3_PORT_HOST_SEL_3P_OFST      (2)
#define SSUSB_U3_PORT_PDN_3P_OFST       (1)
#define SSUSB_U3_PORT_DIS_3P_OFST       (0)

/* U3D_SSUSB_U2_CTRL_0P */
#define SSUSB_U2_PORT_SYS_CK_SEL_OFST       (6)
#define SSUSB_U2_PORT_PHYD_RST_OFST     (5)
#define SSUSB_U2_PORT_MAC_RST_OFST      (4)
#define SSUSB_U2_PORT_U2_CG_EN_OFST     (3)
#define SSUSB_U2_PORT_HOST_SEL_OFST     (2)
#define SSUSB_U2_PORT_PDN_OFST          (1)
#define SSUSB_U2_PORT_DIS_OFST          (0)

/* U3D_SSUSB_U2_CTRL_1P */
#define SSUSB_U2_PORT_SYS_CK_SEL_1P_OFST    (6)
#define SSUSB_U2_PORT_PHYD_RST_1P_OFST      (5)
#define SSUSB_U2_PORT_MAC_RST_1P_OFST       (4)
#define SSUSB_U2_PORT_U2_CG_EN_1P_OFST      (3)
#define SSUSB_U2_PORT_HOST_SEL_1P_OFST      (2)
#define SSUSB_U2_PORT_PDN_1P_OFST       (1)
#define SSUSB_U2_PORT_DIS_1P_OFST       (0)

/* U3D_SSUSB_U2_CTRL_2P */
#define SSUSB_U2_PORT_SYS_CK_SEL_2P_OFST    (6)
#define SSUSB_U2_PORT_PHYD_RST_2P_OFST      (5)
#define SSUSB_U2_PORT_MAC_RST_2P_OFST       (4)
#define SSUSB_U2_PORT_U2_CG_EN_2P_OFST      (3)
#define SSUSB_U2_PORT_HOST_SEL_2P_OFST      (2)
#define SSUSB_U2_PORT_PDN_2P_OFST       (1)
#define SSUSB_U2_PORT_DIS_2P_OFST       (0)

/* U3D_SSUSB_U2_CTRL_3P */
#define SSUSB_U2_PORT_SYS_CK_SEL_3P_OFST    (6)
#define SSUSB_U2_PORT_PHYD_RST_3P_OFST      (5)
#define SSUSB_U2_PORT_MAC_RST_3P_OFST       (4)
#define SSUSB_U2_PORT_U2_CG_EN_3P_OFST      (3)
#define SSUSB_U2_PORT_HOST_SEL_3P_OFST      (2)
#define SSUSB_U2_PORT_PDN_3P_OFST       (1)
#define SSUSB_U2_PORT_DIS_3P_OFST       (0)

/* U3D_SSUSB_U2_CTRL_4P */
#define SSUSB_U2_PORT_SYS_CK_SEL_4P_OFST    (6)
#define SSUSB_U2_PORT_PHYD_RST_4P_OFST      (5)
#define SSUSB_U2_PORT_MAC_RST_4P_OFST       (4)
#define SSUSB_U2_PORT_U2_CG_EN_4P_OFST      (3)
#define SSUSB_U2_PORT_HOST_SEL_4P_OFST      (2)
#define SSUSB_U2_PORT_PDN_4P_OFST       (1)
#define SSUSB_U2_PORT_DIS_4P_OFST       (0)

/* U3D_SSUSB_U2_CTRL_5P */
#define SSUSB_U2_PORT_SYS_CK_SEL_5P_OFST    (6)
#define SSUSB_U2_PORT_PHYD_RST_5P_OFST      (5)
#define SSUSB_U2_PORT_MAC_RST_5P_OFST       (4)
#define SSUSB_U2_PORT_U2_CG_EN_5P_OFST      (3)
#define SSUSB_U2_PORT_HOST_SEL_5P_OFST      (2)
#define SSUSB_U2_PORT_PDN_5P_OFST       (1)
#define SSUSB_U2_PORT_DIS_5P_OFST       (0)

/* U3D_SSUSB_U2_PHY_PLL */
#define SSUSB_U2_PORT_PLL_STABLE_TIMER_OFST (8)
#define SSUSB_U2_PORT_1US_TIMER_OFST        (0)

/* U3D_SSUSB_DMA_CTRL */
#define SSUSB_IP_DMA_BUS_CK_GATE_DIS_OFST   (0)

/* U3D_SSUSB_MAC_CK_CTRL */
#define SSUSB_MAC3_SYS_CK_GATE_MASK_TIME_OFST   (16)
#define SSUSB_MAC2_SYS_CK_GATE_MASK_TIME_OFST   (8)
#define SSUSB_MAC3_SYS_CK_GATE_MODE_OFST    (2)
#define SSUSB_MAC2_SYS_CK_GATE_MODE_OFST    (0)

/* U3D_SSUSB_CSR_CK_CTRL */
#define SSUSB_CSR_MCU_BUS_CK_GATE_EN_OFST   (0)

/* U3D_SSUSB_REF_CK_CTRL */
#define SSUSB_REF_MAC3_CK_GATE_EN_OFST      (3)
#define SSUSB_REF_CK_GATE_EN_OFST       (2)
#define SSUSB_REF_PHY_CK_GATE_EN_OFST       (1)
#define SSUSB_REF_MAC_CK_GATE_EN_OFST       (0)

/* U3D_SSUSB_XHCI_CK_CTRL */
#define SSUSB_XACT3_XHCI_CK_GATE_MASK_TIME_OFST (8)
#define SSUSB_XACT3_XHCI_CK_GATE_MODE_OFST  (4)
#define SSUSB_XHCI_CK_DIV2_EN_OFST      (0)

/* U3D_SSUSB_XHCI_RST_CTRL */
#define SSUSB_XHCI_SW_DRAM_RST_OFST     (4)
#define SSUSB_XHCI_SW_SYS60_RST_OFST        (3)
#define SSUSB_XHCI_SW_SYS125_RST_OFST       (2)
#define SSUSB_XHCI_SW_XHCI_RST_OFST     (1)
#define SSUSB_XHCI_SW_RST_OFST          (0)

/* U3D_SSUSB_DEV_RST_CTRL */
#define SSUSB_DEV_SW_DRAM_RST_OFST      (3)
#define SSUSB_DEV_SW_QMU_RST_OFST       (2)
#define SSUSB_DEV_SW_BMU_RST_OFST       (1)
#define SSUSB_DEV_SW_RST_OFST           (0)

/* U3D_SSUSB_SYS_CK_CTRL */
#define SSUSB_SYS_CK_DIV2_EN_OFST       (0)

/* U3D_SSUSB_HW_ID */
#define SSUSB_HW_ID_OFST            (0)

/* U3D_SSUSB_HW_SUB_ID */
#define SSUSB_HW_SUB_ID_OFST            (0)

/* U3D_SSUSB_PRB_CTRL0 */
#define PRB_BYTE3_EN_OFST           (3)
#define PRB_BYTE2_EN_OFST           (2)
#define PRB_BYTE1_EN_OFST           (1)
#define PRB_BYTE0_EN_OFST           (0)

/* U3D_SSUSB_PRB_CTRL1 */
#define PRB_BYTE1_SEL_OFST          (16)
#define PRB_BYTE0_SEL_OFST          (0)

/* U3D_SSUSB_PRB_CTRL2 */
#define PRB_BYTE3_SEL_OFST          (16)
#define PRB_BYTE2_SEL_OFST          (0)

/* U3D_SSUSB_PRB_CTRL3 */
#define PRB_BYTE3_MODULE_SEL_OFST       (24)
#define PRB_BYTE2_MODULE_SEL_OFST       (16)
#define PRB_BYTE1_MODULE_SEL_OFST       (8)
#define PRB_BYTE0_MODULE_SEL_OFST       (0)

/* U3D_SSUSB_PRB_CTRL4 */
#define SW_PRB_OUT_OFST             (0)

/* U3D_SSUSB_PRB_CTRL5 */
#define PRB_RD_DATA_OFST            (0)

/* U3D_SSUSB_IP_SPARE0 */
#define SSUSB_IP_SPARE0_OFST            (0)

/* U3D_SSUSB_IP_SPARE1 */
#define SSUSB_IP_SPARE1_OFST            (0)

/* U3D_SSUSB_FPGA_I2C_OUT_0P */
#define SSUSB_FPGA_I2C_SCL_OEN_0P_OFST      (3)
#define SSUSB_FPGA_I2C_SCL_OUT_0P_OFST      (2)
#define SSUSB_FPGA_I2C_SDA_OEN_0P_OFST      (1)
#define SSUSB_FPGA_I2C_SDA_OUT_0P_OFST      (0)

/* U3D_SSUSB_FPGA_I2C_IN_0P */
#define SSUSB_FPGA_I2C_SCL_IN_0P_OFST       (1)
#define SSUSB_FPGA_I2C_SDA_IN_0P_OFST       (0)

/* U3D_SSUSB_FPGA_I2C_OUT_1P */
#define SSUSB_FPGA_I2C_SCL_OEN_1P_OFST      (3)
#define SSUSB_FPGA_I2C_SCL_OUT_1P_OFST      (2)
#define SSUSB_FPGA_I2C_SDA_OEN_1P_OFST      (1)
#define SSUSB_FPGA_I2C_SDA_OUT_1P_OFST      (0)

/* U3D_SSUSB_FPGA_I2C_IN_1P */
#define SSUSB_FPGA_I2C_SCL_IN_1P_OFST       (1)
#define SSUSB_FPGA_I2C_SDA_IN_1P_OFST       (0)

/* U3D_SSUSB_FPGA_I2C_OUT_2P */
#define SSUSB_FPGA_I2C_SCL_OEN_2P_OFST      (3)
#define SSUSB_FPGA_I2C_SCL_OUT_2P_OFST      (2)
#define SSUSB_FPGA_I2C_SDA_OEN_2P_OFST      (1)
#define SSUSB_FPGA_I2C_SDA_OUT_2P_OFST      (0)

/* U3D_SSUSB_FPGA_I2C_IN_2P */
#define SSUSB_FPGA_I2C_SCL_IN_2P_OFST       (1)
#define SSUSB_FPGA_I2C_SDA_IN_2P_OFST       (0)

/* U3D_SSUSB_FPGA_I2C_OUT_3P */
#define SSUSB_FPGA_I2C_SCL_OEN_3P_OFST      (3)
#define SSUSB_FPGA_I2C_SCL_OUT_3P_OFST      (2)
#define SSUSB_FPGA_I2C_SDA_OEN_3P_OFST      (1)
#define SSUSB_FPGA_I2C_SDA_OUT_3P_OFST      (0)

/* U3D_SSUSB_FPGA_I2C_IN_3P */
#define SSUSB_FPGA_I2C_SCL_IN_3P_OFST       (1)
#define SSUSB_FPGA_I2C_SDA_IN_3P_OFST       (0)

/* U3D_SSUSB_FPGA_I2C_OUT_4P */
#define SSUSB_FPGA_I2C_SCL_OEN_4P_OFST      (3)
#define SSUSB_FPGA_I2C_SCL_OUT_4P_OFST      (2)
#define SSUSB_FPGA_I2C_SDA_OEN_4P_OFST      (1)
#define SSUSB_FPGA_I2C_SDA_OUT_4P_OFST      (0)

/* U3D_SSUSB_FPGA_I2C_IN_4P */
#define SSUSB_FPGA_I2C_SCL_IN_4P_OFST       (1)
#define SSUSB_FPGA_I2C_SDA_IN_4P_OFST       (0)

/* U3D_SSUSB_IP_SLV_TMOUT */
#define SSUSB_IP_SLV_TMOUT_OFST         (0)

#endif
