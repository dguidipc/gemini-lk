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

#ifndef _MT_USBPHY_H_
#define _MT_USBPHY_H_

#include <platform/mt_typedefs.h>

#if CFG_FPGA_PLATFORM
/* i2c */
u32 u3_phy_read_reg32(u32 addr);
u32 u3_phy_write_reg32(u32 addr, u32 data);
void u3_phy_write_field32(int addr, int offset, int mask, int value);
u32 u3_phy_write_reg8(u32 addr, u8 data);
char u3_phy_read_reg8(u32 addr);

/* general phy API */
void mt_usb11_phy_savecurrent(void);
void mt_usb_phy_recover(void);
void mt_usb_phy_savecurrent(void);
void mt_usb_phy_poweron(void);
//int mu3d_hal_phy_scan(struct u3phy_info *u3phy, int latch_val);
#else
void mt_usb_phy_recover(void);
void mt_usb_phy_savecurrent(void);
void mt_usb_phy_poweron(void);
void switch_2_usb();
#endif

#endif
