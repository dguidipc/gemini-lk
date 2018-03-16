/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <reg.h>
#include <platform/bitops.h>
#include <platform/mt_irq.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_usb.h>
#include <platform/mt_typedefs.h>
#include <platform/timer.h>
#include <kernel/thread.h>

#include <dev/udc.h>

#include <platform/mt_usb.h>
#include <platform/mt_ssusb_qmu.h>
#include <platform/mt_mu3d_hal_qmu_drv.h>

/* USB DEBUG */
#ifdef DBG_USB_QMU
#define DBG_C(x...) dprintf(CRITICAL, "[USB][QMU] " x)
#define DBG_I(x...) dprintf(INFO, "[USB][QMU] " x)
#define DBG_S(x...) dprintf(SPEW, "[USB][QMU] " x)
#else
#define DBG_C(x...) do {} while (0)
#define DBG_I(x...) do {} while (0)
#define DBG_S(x...) do {} while (0)
#endif

#ifdef SUPPORT_QMU
/*
 * get_bd - get a null bd
 * @args - arg1: dir, arg2: ep number
 */
struct tbd *get_bd(u8 dir, u32 num)
{
	struct tbd *ptr;

	if (dir == USB_DIR_OUT) {
		ptr = rx_bd_list[num].pnext;
		DBG_I("(rx_bd_list[%d].pnext: %p)\n", num, rx_bd_list[num].pnext);

		if ((rx_bd_list[num].pnext + 1) < rx_bd_list[num].pend)
			rx_bd_list[num].pnext++;
		else
			rx_bd_list[num].pnext = rx_bd_list[num].pstart;
	} else {
		ptr = tx_bd_list[num].pnext;
		DBG_I("(tx_gpd_list[%d].pnext: %p)\n", num, (void *)(tx_bd_list[num].pnext));
		tx_bd_list[num].pnext++;
		tx_bd_list[num].pnext = (struct tbd *)((u8*)(tx_bd_list[num].pnext) + AT_BD_EXT_LEN);

		if (tx_bd_list[num].pnext >= tx_bd_list[num].pend) {

			tx_bd_list[num].pnext = tx_bd_list[num].pstart;
		}
	}
	return ptr;
}

/*
 * get_bd - get a null gpd
 * @args - arg1: dir, arg2: ep number
 */
struct tgpd *get_gpd(u8 dir, u32 num)
{
	struct tgpd *ptr;

	if (dir == USB_DIR_OUT) {
		ptr = (struct tgpd *)rx_gpd_list[num].pnext;
		DBG_I("(rx_gpd_list[%d].pnext: %p)\n", num, (void *)(rx_gpd_list[num].pnext));
		if ((rx_gpd_list[num].pnext +1) < rx_gpd_list[num].pend)
			rx_gpd_list[num].pnext++;
		else
			rx_gpd_list[num].pnext = rx_gpd_list[num].pstart;
	} else {
		ptr = (struct tgpd *)tx_gpd_list[num].pnext;
		DBG_I("(tx_gpd_list[%d].pnext: %p)\n", num, (void *)(tx_gpd_list[num].pnext));
		tx_gpd_list[num].pnext++;
#ifdef USB_QMU_GPDEXT
		tx_gpd_list[num].pnext = (struct tbd *)((u8 *)tx_gpd_list[num].pnext + AT_GPD_EXT_LEN);
#endif

		if (tx_gpd_list[num].pnext >= tx_gpd_list[num].pend) {
			tx_gpd_list[num].pnext = tx_gpd_list[num].pstart;
		}
	}
	return ptr;
}

/*
 * get_bd - align gpd ptr to target ptr
 * @args - arg1: dir, arg2: ep number, arg3: target ptr
 */
void gpd_ptr_align(u8 dir, u32 num, struct tgpd *ptr)
{
	u32 run_next;
	run_next = true;
	while (run_next) {
		if (ptr == get_gpd(dir, num)) {
			run_next = false;
		}
	}
}

/*
 * bd_virt_to_phys - map bd virtual address to physical address
 * @args - arg1: virtual address, arg2: dir, arg3: ep number
 * @return - physical address
 */
void *bd_virt_to_phys(void *vaddr, u8 dir, u32 num)
{
	void *ptr;

	if (dir == USB_DIR_OUT) {
		ptr = (vaddr - rx_bd_offset[num]);
	} else {
		ptr = (vaddr - tx_bd_offset[num]);
	}

	return ptr;
}

/*
 * bd_phys_to_virt - map bd physical address to virtual address
 * @args - arg1: physical address, arg2: dir, arg3: ep number
 * @return - virtual address
 */
void *bd_phys_to_virt(void *paddr, u8 dir, u32 num)
{
	void *ptr;

	DBG_I("paddr: %p\n", paddr);
	DBG_I("num: %d\n", num);

	if (dir == USB_DIR_OUT) {
		DBG_I("rx_bd_offset[%d]: %p\n", num, (void *)rx_bd_offset[num]);
		ptr = (paddr + rx_bd_offset[num]);
	} else {
		DBG_I("tx_bd_offset[%d]: %p\n", num, (void *)tx_bd_offset[num]);
		ptr = (paddr + tx_bd_offset[num]);
	}
	DBG_I("ptr: %p\n", ptr);

	return ptr;
}

/*
 * mu3d_hal_gpd_virt_to_phys - map gpd virtual address to physical address
 * @args - arg1: virtual address, arg2: dir, arg3: ep number
 * @return - physical address
 */
void *mu3d_hal_gpd_virt_to_phys(void *vaddr, u8 dir, u32 num)
{
	void *ptr;

	if (dir == USB_DIR_OUT) {
		ptr = (vaddr - rx_gpd_offset[num]);
	} else {
		ptr = (vaddr - tx_gpd_offset[num]);
	}
	return ptr;
}

/*
 * gpd_phys_to_virt - map gpd physical address to virtual address
 * @args - arg1: physical address, arg2: dir, arg3: ep number
 * @return - virtual address
 */
void *gpd_phys_to_virt(void *paddr, u8 dir, u32 num)
{
	void *ptr;

	DBG_I("paddr: %p\n", paddr);
	DBG_I("num:   %d\n", num);

	if (dir == USB_DIR_OUT) {
		DBG_I("rx_gpd_offset[%d]: %p\n", num, (void *)rx_gpd_offset[num]);
		ptr = (paddr + rx_gpd_offset[num]);
	} else {
		DBG_I("tx_gpd_offset[%d]: %p\n", num, (void *)tx_gpd_offset[num]);
		ptr = (paddr + tx_gpd_offset[num]);
	}
	DBG_I("ptr: %p\n", ptr);
	return ptr;
}

/*
 * init_bd_list - initialize bd management list
 * @args - arg1: dir, arg2: ep number, arg3: bd virtual addr, arg4: bd ioremap addr, arg5: bd number
 */
void init_bd_list(u8 dir, int num, struct tbd * ptr, struct tbd *io_ptr, u32 size)
{

	if (dir == USB_DIR_OUT) {
		rx_bd_list[num].pstart = io_ptr;
		rx_bd_list[num].pend = (struct tbd *)(io_ptr + size);
#if defined(SUPPORT_VA)
		rx_bd_offset[num] = (u32)io_ptr - (u32)os_virt_to_phys(ptr);
#else
		rx_bd_offset[num] = (u32)io_ptr - (u32)ptr;
#endif
		io_ptr++;
		rx_bd_list[num].pnext = io_ptr;

		DBG_I("rx_bd_list[%d].pstart: %p\n", num, rx_bd_list[num].pstart);
		DBG_I("rx_bd_list[%d].pnext:  %p\n", num, rx_bd_list[num].pnext);
		DBG_I("rx_bd_list[%d].pend :  %p\n", num, rx_bd_list[num].pend);
		DBG_I("rx_bd_offset[%d]: %p\n", num, (void *)rx_bd_offset[num]);

#if defined(SUPPORT_VA)
		DBG_I("phy: %p\n", os_virt_to_phys(ptr));
#else
		DBG_I("phy: %p\n", ptr);
#endif
		DBG_I("io_ptr: %p\n", io_ptr);
		DBG_I("io_ptr end:%p\n", io_ptr+size);
	} else {
		tx_bd_list[num].pstart = io_ptr;
		tx_bd_list[num].pend = (struct tbd *)((u8*)(io_ptr+size) + AT_BD_EXT_LEN * size);
#if defined(SUPPORT_VA)
		tx_bd_offset[num] = (u32)io_ptr-(u32)os_virt_to_phys(ptr);
#else
		tx_bd_offset[num] = (u32)io_ptr-(u32)ptr;
#endif
		io_ptr++;
		tx_bd_list[num].pnext = (struct tbd *)((u8*)io_ptr + AT_BD_EXT_LEN);
		DBG_I("tx_bd_list[%d].pstart: %p\n", num, tx_bd_list[num].pstart);
		DBG_I("tx_bd_list[%d].pnext : %p\n", num, tx_bd_list[num].pnext);
		DBG_I("tx_bd_list[%d].pend  : %p\n", num, tx_bd_list[num].pend);
		DBG_I("tx_bd_offset[%d]: %p\n", num, (void *)tx_bd_offset[num]);
#if defined(SUPPORT_VA)
		DBG_I("phy: %p\n", os_virt_to_phys(ptr));
#else
		DBG_I("phy: %p\n", ptr);
#endif
		DBG_I("io_ptr: %p\n", io_ptr);
		DBG_I("io_ptr end:%p\n", io_ptr+size);
	}
}

/*
 * init_gpd_list - initialize gpd management list
 * @args - arg1: dir, arg2: ep number, arg3: gpd virtual addr, arg4: gpd ioremap addr, arg5: gpd number
 */
void init_gpd_list(u8 dir, int num, struct tgpd *ptr, struct tgpd *io_ptr, u32 size)
{
	if (dir == USB_DIR_OUT) {
		rx_gpd_list[num].pstart = (struct tbd *)io_ptr;
		rx_gpd_list[num].pend = (struct tbd *)(io_ptr + size);
#if defined(SUPPORT_VA)
		rx_gpd_offset[num] = (u32)io_ptr - (u32)os_virt_to_phys(ptr);
#else
		rx_gpd_offset[num] = (u32)io_ptr - (u32)ptr;
#endif
		io_ptr++;
		rx_gpd_list[num].pnext = (struct tbd *)io_ptr;
		DBG_I("rx_gpd_list[%d].pstart: %p\n", num, rx_gpd_list[num].pstart);
		DBG_I("rx_gpd_list[%d].pnext:  %p\n", num, rx_gpd_list[num].pnext);
		DBG_I("rx_gpd_list[%d].pend:   %p\n", num, rx_gpd_list[num].pend);
		DBG_I("rx_gpd_offset[%d]: %p\n", num, (void *)rx_gpd_offset[num]);
#if defined(SUPPORT_VA)
		DBG_I("phy: %p\n", os_virt_to_phys(ptr));
		DBG_I("phy end: %p\n", os_virt_to_phys(ptr) + size);
#else
		DBG_I("phy: %p\n", ptr);
		DBG_I("phy end: %p\n", ptr + size);
#endif
		DBG_I("io_ptr: %p\n", io_ptr);
		DBG_I("io_ptr end: %p\n", io_ptr + size);
	} else {
		tx_gpd_list[num].pstart = (struct tbd *)io_ptr;
#ifdef USB_QMU_GPDEXT
		tx_gpd_list[num].pend = (struct tbd *)((u8 *)(io_ptr + size) + AT_GPD_EXT_LEN * size);
#else
		tx_gpd_list[num].pend = (struct tbd *)((u8 *)(io_ptr + size));
#endif
#if defined(SUPPORT_VA)
		tx_gpd_offset[num] = (u32)io_ptr - (u32)os_virt_to_phys(ptr);
#else
		tx_gpd_offset[num] = (u32)io_ptr - (u32)ptr;
#endif
		io_ptr++;
#ifdef USB_QMU_GPDEXT
		tx_gpd_list[num].pnext = (struct tbd *)((u8 *)io_ptr + AT_GPD_EXT_LEN);
#else
		tx_gpd_list[num].pnext = (struct tbd *)((u8 *)io_ptr);
#endif
		DBG_I("tx_gpd_list[%d].pstart: %p\n", num, tx_gpd_list[num].pstart);
		DBG_I("tx_gpd_list[%d].pnext:  %p\n", num, tx_gpd_list[num].pnext);
		DBG_I("tx_gpd_list[%d].pend:   %p\n", num, tx_gpd_list[num].pend);
		DBG_I("tx_gpd_offset[%d]: %p\n", num, (void *)tx_gpd_offset[num]);
#if defined(SUPPORT_VA)
		DBG_I("phy: %p\n", os_virt_to_phys(ptr));
		DBG_I("phy end: %p\n", os_virt_to_phys(ptr)); /* no need to add size? */
#else
		DBG_I("phy: %p\n", ptr);
		DBG_I("phy end: %p\n", ptr); /* no need to add size? */
#endif
		DBG_I("io_ptr: %p\n", io_ptr);
		DBG_I("io_ptr end: %p\n", io_ptr+size);
	}
}

/*
 * free_gpd - free gpd management list
 * @args - arg1: dir, arg2: ep number
 */
void free_gpd(u8 dir, int num)
{

	if (dir == USB_DIR_OUT) {
		memset(rx_gpd_list[num].pstart, 0, MAX_GPD_NUM * sizeof(struct tgpd));
	} else {
		memset(tx_gpd_list[num].pstart, 0, MAX_GPD_NUM * sizeof(struct tgpd));
	}
}

/*
 * mu3d_hal_alloc_qmu_mem - allocate gpd and bd memory for all ep
 *
 */
void mu3d_hal_alloc_qmu_mem(void)
{
	u32 i, size;
	struct tgpd *ptr, *io_ptr;
	struct tbd *bptr, *io_bptr;

	for (i = 1; i <= MAX_QMU_EP; i++) {

		/* alloc RX */
		size = sizeof(struct tgpd);
		size *= MAX_GPD_NUM;
		//ptr = (struct tgpd*)malloc(size);
		ptr = (struct tgpd*)memalign(32, size);
		memset(ptr, 0 , size);
#if defined(SUPPORT_VA)
		io_ptr = (struct tgpd*)os_ioremap(os_virt_to_phys(ptr), size);
#else
		//io_ptr = (struct tgpd*)os_ioremap(ptr, size);
		io_ptr = (struct tgpd*)ptr;
#endif

		init_gpd_list(USB_DIR_OUT, i, ptr, io_ptr, MAX_GPD_NUM);
		rx_gpd_end[i] = io_ptr;
		DBG_I("ALLOC RX GPD End [%d] Virtual Mem@%p\n", i, (void *)rx_gpd_end[i]);
		memset(rx_gpd_end[i], 0 , sizeof(struct tgpd));
		TGPD_CLR_FLAGS_HWO(rx_gpd_end[i]);
		rx_gpd_head[i] = rx_gpd_last[i] = rx_gpd_end[i];

#if defined(SUPPORT_VA)
		DBG_I("RQSAR[%d]: %p\n", i, mu3d_hal_gpd_virt_to_phys(rx_gpd_end[i], USB_DIR_OUT, i));
#else
		DBG_I("RQSAR[%d]: %p\n", i, rx_gpd_end[i]);
#endif
		/* alloc TX */
		size = sizeof(struct tgpd);
#ifdef USB_QMU_GPDEXT
		size += AT_GPD_EXT_LEN;
#endif
		size *= MAX_GPD_NUM;
		ptr = (struct tgpd*)memalign(32, size);
		memset(ptr, 0, size);
#if defined(SUPPORT_VA)
		io_ptr = (struct tgpd*)os_ioremap(os_virt_to_phys(ptr), size);
#else
		io_ptr = (struct tgpd*)ptr;
#endif
		init_gpd_list(USB_DIR_IN, i, ptr, io_ptr, MAX_GPD_NUM);
		tx_gpd_end[i] = io_ptr;
		DBG_I("ALLOC TX GPD End [%d] Virtual Mem@%p\n", i, (void *)tx_gpd_end[i]);
#ifdef USB_QMU_GPDEXT
		memset(tx_gpd_end[i], 0 , sizeof(struct tgpd) + AT_GPD_EXT_LEN);
#else
		memset(tx_gpd_end[i], 0 , sizeof(struct tgpd));
#endif
		TGPD_CLR_FLAGS_HWO(tx_gpd_end[i]);
		tx_gpd_head[i] = tx_gpd_last[i] = tx_gpd_end[i];
#if defined(SUPPORT_VA)
		DBG_I("TQSAR[%d]: %p\n", i, mu3d_hal_gpd_virt_to_phys(tx_gpd_end[i], USB_DIR_IN, i));
#else
		DBG_I("TQSAR[%d]: %p\n", i, tx_gpd_end[i]);
#endif
		size = (sizeof(struct tbd));
		size *= MAX_BD_NUM;
		bptr = (struct tbd*)memalign(32, size);
		memset(bptr, 0, size);
#if defined(SUPPORT_VA)
		io_bptr = (struct tbd*)os_ioremap(os_virt_to_phys(bptr), size);
#else
		io_bptr = (struct tbd*)bptr;
#endif
		init_bd_list(USB_DIR_OUT, i, bptr, io_bptr ,MAX_BD_NUM);
		size = (sizeof(struct tbd));
		size += AT_BD_EXT_LEN;
		size *= MAX_BD_NUM;
		bptr = (struct tbd*)memalign(32, size);
		memset(bptr, 0, size);
#if defined(SUPPORT_VA)
		io_bptr = (struct tbd*)os_ioremap(os_virt_to_phys(bptr),size);
#else
		io_bptr = (struct tbd*)bptr;
#endif
		init_bd_list(USB_DIR_IN, i, bptr, io_bptr ,MAX_BD_NUM);
	}
}

/*
 * mu3d_hal_init_qmu - initialize qmu
 *
 */
void mu3d_hal_init_qmu(void)
{
	DBG_I("%s\n", __func__);
	u32 i;
	u32 qcr = 0;
	/* Initialize QMU Tx/Rx start address. */
	for (i = 1; i <= MAX_QMU_EP; i++) {
		qcr |= QMU_RX_EN(i);
		qcr |= QMU_TX_EN(i);

#if defined(SUPPORT_VA)
		writel(mu3d_hal_gpd_virt_to_phys(rx_gpd_head[i], USB_DIR_OUT, i), U3D_RXQSAR(i));
		writel(mu3d_hal_gpd_virt_to_phys(tx_gpd_head[i], USB_DIR_IN, i), U3D_TXQSAR(i));
#else
		writel((u32)rx_gpd_head[i], U3D_RXQSAR(i));
		writel((u32)tx_gpd_head[i], U3D_TXQSAR(i));
		DBG_I("U3D_RXQSAR%d: %p, val: %x\n", i, (void *)U3D_RXQSAR(i), readl(U3D_RXQSAR(i)));
		DBG_I("U3D_TXQSAR%d: %p, val: %x\n", i, (void *)U3D_TXQSAR(i), readl(U3D_TXQSAR(i)));
#endif
		arch_clean_invalidate_cache_range((addr_t) tx_gpd_head[i], MAX_GPD_NUM * sizeof(struct tgpd));
		arch_clean_invalidate_cache_range((addr_t) rx_gpd_head[i], MAX_GPD_NUM * sizeof(struct tgpd));

		tx_gpd_end[i] = tx_gpd_last[i] = tx_gpd_head[i];
		rx_gpd_end[i] = rx_gpd_last[i] = rx_gpd_head[i];

		gpd_ptr_align(USB_DIR_OUT, i, rx_gpd_end[i]);
		gpd_ptr_align(USB_DIR_IN, i, tx_gpd_end[i]);
	}

	/* Enable QMU Tx/Rx. */
	writel(qcr, U3D_QGCSR);
	writel(qcr, U3D_QIESR0);
	/* Enable QMU interrupt. */
	writel((TXQ_EMPTY_IESR | TXQ_CSERR_IESR | TXQ_LENERR_IESR | RXQ_EMPTY_IESR | RXQ_CSERR_IESR | RXQ_LENERR_IESR | RXQ_ZLPERR_IESR), U3D_QIESR1);
	writel(EP0ISR, U3D_EPIESR);
}

/*
 * mu3d_hal_cal_checksum - calculate check sum
 * @args - arg1: data buffer, arg2: data length
 */
u8 mu3d_hal_cal_checksum(u8 *data, int len)
{
	u8 *pdata, cksum;
	int i;

	*(data + 1) = 0x0;
	pdata = data;
	cksum = 0;
	for (i = 0; i < len; i++) {
		cksum += *(pdata + i);
	}
	return 0xFF - cksum;
}

/*
 * mu3d_hal_resume_qmu - resume qmu function
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_resume_qmu(int q_ep_num, u8 dir)
{

	DBG_I("%s\n", __func__);
#if defined(USB_RISC_CACHE_ENABLED)
	os_flushinvalidateDcache();
#endif

	if (dir == USB_DIR_IN) {
		writel(QMU_Q_RESUME, U3D_TXQCSR(q_ep_num));
		if (!readl(U3D_TXQCSR(q_ep_num))) {  //judge if Queue is still inactive
			DBG_I("%s: TXQCSR1 val: %x\n", __func__, readl(U3D_TXQCSR(q_ep_num)));
			writel(QMU_Q_RESUME, U3D_TXQCSR(q_ep_num));
		}
	} else if (dir == USB_DIR_OUT) {
		writel(QMU_Q_RESUME, U3D_RXQCSR(q_ep_num));
		if (!readl(U3D_RXQCSR(q_ep_num))) {
			DBG_I("%s: RXQCSR1 val: %x\n", __func__, readl(U3D_RXQCSR(q_ep_num)));
			writel(QMU_Q_RESUME, U3D_RXQCSR(q_ep_num));
		}
	}
}

/*
 * mu3d_hal_prepare_tx_gpd - prepare tx gpd/bd
 * @args - arg1: gpd address, arg2: data buffer address, arg3: data length, arg4: ep number, arg5: with bd or not, arg6: write hwo bit or not,  arg7: write ioc bit or not
 */
struct tgpd* mu3d_hal_prepare_tx_gpd(struct tgpd *gpd, u8 *pbuf, u32 data_len, u8 ep_num, u8 _is_bdp, u8 ishwo, u8 ioc, u8 bps, u8 zlp)
{
	u32     offset;
	int     i, bd_num;

	struct tbd  *bd_next;
	struct tbd  *bd_head, *bd;
	u32     length;
	u8      *pbuffer;
	u8      *_tmp;

#if defined(SUPPORT_VA)
	u8      *vbuffer;
#endif

	DBG_I("%s: ep_num: %d, pbuf: %x, data_len: %x, zlp: %x\n", __func__, (int)ep_num, (u32)pbuf, data_len, (u32)zlp);

	if (data_len <= gpd_extension) {
		_is_bdp = 0;
	}

	arch_clean_invalidate_cache_range((addr_t) gpd, sizeof(struct tgpd));

	if (!_is_bdp) {
		TGPD_SET_DATA(gpd, pbuf + gpd_extension);
		TGPD_CLR_FORMAT_BDP(gpd);
	} else {
		bd_head = (struct tbd*)get_bd(USB_DIR_IN, ep_num);
		DBG_I("Malloc Tx 01 (BD): 0x%x\n", (u32)bd_head);
		bd = bd_head;
		memset(bd, 0, sizeof(struct tbd) + bd_extension);
		length = data_len - gpd_extension;
		pbuffer = (u8*)(pbuf + gpd_extension);
		offset = bd_buf_size + bd_extension;
		bd_num = (!(length%offset)) ? (length/offset) : ((length/offset) + 1);

		if (offset > length) {
			offset = length;
		}

		for (i = 0; i < bd_num; i++) {
			DBG_I("bd[%d]: %p\n", i, bd);
			if (i == (bd_num - 1)) {
				if (length < bd_extension) {
					TBD_SET_EXT_LEN(bd, length);
					TBD_SET_BUF_LEN(bd, 0);
					TBD_SET_DATA(bd, pbuffer + bd_extension);
				} else {
					TBD_SET_EXT_LEN(bd, bd_extension);
					TBD_SET_BUF_LEN(bd, length - bd_extension);
					TBD_SET_DATA(bd, pbuffer + bd_extension);
				}

				TBD_SET_FLAGS_EOL(bd);
				TBD_SET_NEXT(bd, 0);
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);

				if (bd_extension) {
#if defined(SUPPORT_VA)
					vbuffer = os_phys_to_virt(pbuffer);
#endif
					arch_clean_invalidate_cache_range((addr_t) pbuf, g_dma_buffer_size);
					_tmp = TBD_GET_EXT(bd);
#if defined(SUPPORT_VA)
					memcpy(_tmp, vbuffer, bd_extension);
#else
					memcpy(_tmp, pbuffer, bd_extension);
#endif
					arch_clean_invalidate_cache_range((addr_t) pbuf, g_dma_buffer_size);
				}
				DBG_I("BD number %d\n", i + 1);
				data_len = length + gpd_extension;
				length = 0;

				break;
			} else { /* if (i != (bd_num - 1)) */
				TBD_SET_EXT_LEN(bd, bd_extension);
				TBD_SET_BUF_LEN(bd, offset - bd_extension);
				TBD_SET_DATA(bd, pbuffer + bd_extension);
				TBD_CLR_FLAGS_EOL(bd);
				bd_next = (struct tbd*)get_bd(USB_DIR_IN, ep_num);
				memset(bd_next, 0, sizeof(struct tbd) + bd_extension);
#if defined(SUPPORT_VA)
				TBD_SET_NEXT(bd, bd_virt_to_phys(bd_next, USB_DIR_IN, ep_num));
#else
				TBD_SET_NEXT(bd, bd_next);
#endif
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);

				if (bd_extension) {

#if defined(SUPPORT_VA)
					vbuffer = os_phys_to_virt(pbuffer);
#endif
					arch_clean_invalidate_cache_range((addr_t) pbuf, g_dma_buffer_size);
					_tmp = TBD_GET_EXT(bd);
#if defined(SUPPORT_VA)
					memcpy(_tmp, vbuffer, bd_extension);
#else
					memcpy(_tmp, pbuffer, bd_extension);
#endif
					arch_clean_invalidate_cache_range((addr_t) pbuf, g_dma_buffer_size);
				}

				length -= offset;
				pbuffer += offset;
				bd = bd_next;
			}
		}

#if defined(SUPPORT_VA)
		TGPD_SET_DATA(gpd, bd_virt_to_phys(bd_head, USB_DIR_IN, ep_num));
#else
		TGPD_SET_DATA(gpd, bd_head);
#endif
		TGPD_SET_FORMAT_BDP(gpd);
	}

	DBG_I("GPD data_len %d\n", (data_len - gpd_extension));

	if (data_len < gpd_extension) {
		TGPD_SET_BUF_LEN(gpd, 0);
		TGPD_SET_EXT_LEN(gpd, data_len);
	} else {
		TGPD_SET_BUF_LEN(gpd, data_len - gpd_extension);
		TGPD_SET_EXT_LEN(gpd, gpd_extension);
	}

	if (gpd_extension) {
#if defined(SUPPORT_VA)
		vbuffer = os_phys_to_virt(pbuf);
#endif
		arch_clean_invalidate_cache_range((addr_t) pbuf, g_dma_buffer_size);
		_tmp = TGPD_GET_EXT(gpd);
#if defined(SUPPORT_VA)
		memcpy(_tmp, vbuffer, gpd_extension);
#else
		memcpy(_tmp, pbuf, gpd_extension);
#endif
		arch_clean_invalidate_cache_range((addr_t) pbuf, g_dma_buffer_size);
	}

	if (zlp) {
		TGPD_SET_FORMAT_ZLP(gpd);
	} else {
		TGPD_CLR_FORMAT_ZLP(gpd);
	}

	if (bps) {
		TGPD_SET_FORMAT_BPS(gpd);
	} else {
		TGPD_CLR_FORMAT_BPS(gpd);
	}

	if (ioc) {
		TGPD_SET_FORMAT_IOC(gpd);
	} else {
		TGPD_CLR_FORMAT_IOC(gpd);
	}

	/* Create next GPD */
	tx_gpd_end[ep_num] = get_gpd(USB_DIR_IN, ep_num);
	DBG_I("Malloc Tx 01 (GPD+EXT) (tx_gpd_end): 0x%x\n", (u32)tx_gpd_end[ep_num]);

	arch_clean_invalidate_cache_range((addr_t) tx_gpd_end[ep_num], sizeof(struct tgpd));
	memset(tx_gpd_end[ep_num], 0 , sizeof(struct tgpd) + gpd_extension);
	TGPD_CLR_FLAGS_HWO(tx_gpd_end[ep_num]);
#if defined(SUPPORT_VA)
	TGPD_SET_NEXT(gpd, mu3d_hal_gpd_virt_to_phys(tx_gpd_end[ep_num],USB_DIR_IN, ep_num));
#else
	TGPD_SET_NEXT(gpd, tx_gpd_end[ep_num]);
#endif

	if (ishwo) {
		TGPD_SET_CHKSUM(gpd, CHECKSUM_LENGTH);
		TGPD_SET_FLAGS_HWO(gpd);
	} else {
		TGPD_CLR_FLAGS_HWO(gpd);
		TGPD_SET_CHKSUM_HWO(gpd, CHECKSUM_LENGTH);
	}

#if defined(USB_RISC_CACHE_ENABLED)
	os_flushinvalidateDcache();
#endif

	/* gpd end */
	arch_clean_invalidate_cache_range((addr_t) gpd, sizeof(struct tgpd));

	return gpd;
}

/*
 * mu3d_hal_prepare_rx_gpd - prepare rx gpd/bd
 * @args - arg1: gpd address, arg2: data buffer address, arg3: data length, arg4: ep number, arg5: with bd or not, arg6: write hwo bit or not,  arg7: write ioc bit or not
 */
struct tgpd* mu3d_hal_prepare_rx_gpd(struct tgpd *gpd, u8 *pbuf, u32 data_len, u8 ep_num, u8 _is_bdp, u8 ishwo, u8 ioc, u8 bps, u32 max_pkt_size)
{
	u32     offset;
	int     i, bd_num;
	unsigned int    length;

	struct tbd  *bd_next;
	struct tbd  *bd_head, *bd;
	u8      *pbuffer;

	DBG_I("%s: GPD: %p, ep_num: %d, pbuf: %x, data_len: %x, _is_bdp: %d, ishwo: %d, ioc: %d, bps: %d\n", __func__, gpd, (int)ep_num, (u32)pbuf, data_len, _is_bdp, ishwo, ioc, bps);

	length = data_len;

	arch_clean_invalidate_cache_range((addr_t) gpd, sizeof(struct tgpd));

	if (!_is_bdp) {
		TGPD_SET_DATA(gpd, pbuf);
		TGPD_CLR_FORMAT_BDP(gpd);
	} else {
		bd_head = (struct tbd*)get_bd(USB_DIR_OUT, ep_num);
		bd = bd_head;
		memset(bd, 0, sizeof(struct tbd));
		offset = bd_buf_size;
		pbuffer = (u8*)(pbuf);
		bd_num = (!(length%offset)) ? (length/offset) : ((length/offset)+1);

		for (i = 0; i < bd_num; i++) {

			DBG_I("bd[%d]: %p\n", i, bd);
			TBD_SET_BUF_LEN(bd, 0);
			TBD_SET_DATA(bd, pbuffer);
			if (i == (bd_num - 1)) {
				length = (!(length%max_pkt_size)) ? (length) : ((length/max_pkt_size)+1)*max_pkt_size;
				TBD_SET_DATABUF_LEN(bd, length); //The last one's data buffer lengnth must be precise, or the GPD will never done unless ZLP or short packet.
				TBD_SET_FLAGS_EOL(bd);
				TBD_SET_NEXT(bd, 0);
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);
				DBG_I("BD number %d\n", i + 1);
				break;
			} else {
				TBD_SET_DATABUF_LEN(bd, offset);
				TBD_CLR_FLAGS_EOL(bd);
				bd_next = (struct tbd*)get_bd(USB_DIR_OUT, ep_num);
				memset(bd_next, 0, sizeof(struct tbd));

#if defined(SUPPORT_VA)
				TBD_SET_NEXT(bd, bd_virt_to_phys(bd_next,USB_DIR_OUT, ep_num));
#else
				TBD_SET_NEXT(bd, bd_next);
#endif
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);
				pbuffer += offset;
				length -= offset;
				bd = bd_next;
			}
		}

#if defined(SUPPORT_VA)
		TGPD_SET_DATA(gpd, bd_virt_to_phys(bd_head, USB_DIR_OUT, ep_num));
#else
		TGPD_SET_DATA(gpd, bd_head);
#endif
		TGPD_SET_FORMAT_BDP(gpd);
	}

	if (data_len < gpd_buf_size)
		TGPD_SET_DATABUF_LEN(gpd, data_len); /* or length?? */
	else
		TGPD_SET_DATABUF_LEN(gpd, gpd_buf_size);

	TGPD_SET_BUF_LEN(gpd, 0);

	if (bps) {
		TGPD_SET_FORMAT_BPS(gpd);
	} else {
		TGPD_CLR_FORMAT_BPS(gpd);
	}

	if (ioc) {
		TGPD_SET_FORMAT_IOC(gpd);
	} else {
		TGPD_CLR_FORMAT_IOC(gpd);
	}

	rx_gpd_end[ep_num] = get_gpd(USB_DIR_OUT, ep_num);
	memset(rx_gpd_end[ep_num], 0, sizeof(struct tgpd));
	DBG_I("Rx Next GPD 0x%x\n", (u32)rx_gpd_end[ep_num]);
	TGPD_CLR_FLAGS_HWO(rx_gpd_end[ep_num]);

#if defined(SUPPORT_VA)
	TGPD_SET_NEXT(gpd, mu3d_hal_gpd_virt_to_phys(rx_gpd_end[ep_num], USB_DIR_OUT, ep_num));
#else
	TGPD_SET_NEXT(gpd, rx_gpd_end[ep_num]);
#endif

	if (ishwo) {
		TGPD_SET_CHKSUM(gpd, CHECKSUM_LENGTH);
		TGPD_SET_FLAGS_HWO(gpd);
	} else {
		TGPD_CLR_FLAGS_HWO(gpd);
		TGPD_SET_CHKSUM_HWO(gpd, CHECKSUM_LENGTH);
	}

	DBG_I("Rx gpd info { HWO %d, Next_GPD %x ,databuf_length %d, DataBuffer %x, Recived Len %d, Endpoint %d, TGL %d, ZLP %d}\n",
	      (u32)TGPD_GET_FLAG(gpd), (u32)TGPD_GET_NEXT(gpd),
	      (u32)TGPD_GET_DATABUF_LEN(gpd), (u32)TGPD_GET_DATA(gpd),
	      (u32)TGPD_GET_BUF_LEN(gpd), (u32)TGPD_GET_EPaddr(gpd),
	      (u32)TGPD_GET_TGL(gpd), (u32)TGPD_GET_ZLP(gpd));

	arch_clean_invalidate_cache_range((addr_t) gpd, sizeof(struct tgpd));
	return gpd;
}

/*
 * mu3d_hal_insert_transfer_gpd - insert new gpd/bd
 * @args - arg1: ep number, arg2: dir, arg3: data buffer, arg4: data length,  arg5: write hwo bit or not,  arg6: write ioc bit or not
 */
void mu3d_hal_insert_transfer_gpd(int ep_num, u8 dir, u8* buf, u32 count, u8 ishwo, u8 ioc, u8 bps, u8 zlp, u32 max_pkt_size)
{
	struct tgpd* gpd;

	DBG_I("%s: ep_num: %d, dir: %d, buf: %x, count: %x, ishwo: %d, ioc: :%d, bps: %d, zlp: %x, maxp: %x\n", __func__, (int)ep_num, dir, (u32)buf, count, ishwo, ioc, bps, (u32)zlp, (u32)max_pkt_size);

	if (dir == USB_DIR_IN) {
		gpd = tx_gpd_end[ep_num];
		DBG_I("TX gpd: %x\n", (unsigned int)gpd);
		mu3d_hal_prepare_tx_gpd(gpd, buf, count, ep_num, is_bdp, ishwo, ioc, bps, zlp);
	} else if (dir == USB_DIR_OUT) {
		gpd = rx_gpd_end[ep_num];
		DBG_I("RX gpd: %x\n", (unsigned int)gpd);
		mu3d_hal_prepare_rx_gpd(gpd, buf, count, ep_num, is_bdp, ishwo, ioc, bps, max_pkt_size);
	}
}

/*
 * mu3d_hal_start_qmu - start qmu function (QMU flow : mu3d_hal_init_qmu ->mu3d_hal_start_qmu -> mu3d_hal_insert_transfer_gpd -> mu3d_hal_resume_qmu)
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_start_qmu(int q_ep_num, u8 dir)
{

	u32 qcr;
	u32 txcsr;

	DBG_I("%s: dir: %x\n", __func__, dir);

	if (dir == USB_DIR_IN) {

		txcsr = USB_READCSR32(U3D_TX1CSR0, q_ep_num) & 0xFFFEFFFF;
		USB_WRITECSR32(U3D_TX1CSR0, q_ep_num, txcsr | TX_DMAREQEN);
		qcr = readl(U3D_QCR0);
		writel(qcr | QMU_TX_CS_EN(q_ep_num), U3D_QCR0);
#if (TXZLP == HW_MODE)
		qcr = readl(U3D_QCR1);
		writel(qcr &~ QMU_TX_ZLP(q_ep_num), U3D_QCR1);
		qcr = readl(U3D_QCR2);
		writel(qcr | QMU_TX_ZLP(q_ep_num), U3D_QCR2);
#elif (TXZLP == GPD_MODE)
		qcr = readl(U3D_QCR1);
		writel(qcr | QMU_TX_ZLP(q_ep_num), U3D_QCR1);
#endif
		writel(readl(U3D_QEMIESR) | QMU_TX_EMPTY(q_ep_num), U3D_QEMIESR);
		writel(QMU_TX_LEN_ERR(q_ep_num) | QMU_TX_CS_ERR(q_ep_num), U3D_TQERRIESR0);

		if (readl(U3D_TXQCSR(q_ep_num)) & QMU_Q_ACTIVE) {
			DBG_I("Tx %d Active Now!\n", q_ep_num);
			return;
		}
#if defined(USB_RISC_CACHE_ENABLED)
		os_flushinvalidateDcache();
#endif
		writel(QMU_Q_START, U3D_TXQCSR(q_ep_num));
	} else if (dir == USB_DIR_OUT) {
		USB_WRITECSR32(U3D_RX1CSR0, q_ep_num, USB_READCSR32(U3D_RX1CSR0, q_ep_num) |(RX_DMAREQEN));
		qcr = readl(U3D_QCR0);
		writel(qcr | QMU_RX_CS_EN(q_ep_num), U3D_QCR0);

#ifdef CFG_RX_ZLP_EN
		qcr = readl(U3D_QCR3);
		writel(qcr | QMU_RX_ZLP(q_ep_num), U3D_QCR3);
#else
		qcr = readl(U3D_QCR3);
		writel(qcr & ~(QMU_RX_ZLP(q_ep_num)), U3D_QCR3);
#endif

#ifdef CFG_RX_COZ_EN
		qcr = readl(U3D_QCR3);
		writel(qcr | QMU_RX_COZ(q_ep_num), U3D_QCR3);
#else
		qcr = readl(U3D_QCR3);
		writel(qcr & ~(QMU_RX_COZ(q_ep_num)), U3D_QCR3);
#endif

		writel(readl(U3D_QEMIESR) | QMU_RX_EMPTY(q_ep_num), U3D_QEMIESR);
		writel(QMU_RX_LEN_ERR(q_ep_num) | QMU_RX_CS_ERR(q_ep_num), U3D_RQERRIESR0);
		writel(QMU_RX_EP_ERR(q_ep_num) | QMU_RX_ZLP_ERR(q_ep_num), U3D_RQERRIESR1);

		if (readl(U3D_RXQCSR(q_ep_num)) & QMU_Q_ACTIVE) {
			DBG_I("Rx %d Active Now!\n", q_ep_num);
			return;
		}

#if defined(USB_RISC_CACHE_ENABLED)
		os_flushinvalidateDcache();
#endif
		writel(QMU_Q_START, U3D_RXQCSR(q_ep_num));
	}

#if (CHECKSUM_TYPE == CS_16B)
	writel(readl(U3D_QCR0) | CS16B_EN, U3D_QCR0);
#else
	writel(readl(U3D_QCR0) &~ CS16B_EN, U3D_QCR0);
#endif
}

/*
 * mu3d_hal_stop_qmu - stop qmu function (after qmu stop, fifo should be flushed)
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_stop_qmu(int q_ep_num, u8 dir)
{
	DBG_I("%s\n", __func__);
	if (dir == USB_DIR_IN) {
		if (!(readl(U3D_TXQCSR(q_ep_num)) & (QMU_Q_ACTIVE))) {
			DBG_I("Tx %d inActive Now!\n", q_ep_num);
			return;
		}
		writel(QMU_Q_STOP, U3D_TXQCSR(q_ep_num));
		while ((readl(U3D_TXQCSR(q_ep_num)) & (QMU_Q_ACTIVE)));
		DBG_I("Tx %d stop Now!\n", q_ep_num);
	} else if (dir == USB_DIR_OUT) {
		if (!(readl(U3D_RXQCSR(q_ep_num)) & QMU_Q_ACTIVE)) {
			DBG_I("Rx %d inActive Now!\n", q_ep_num);
			return;
		}
		writel(QMU_Q_STOP, U3D_RXQCSR(q_ep_num));
		while ((readl(U3D_RXQCSR(q_ep_num)) & (QMU_Q_ACTIVE)));
		DBG_I("Rx %d stop Now!\n", q_ep_num);
	}
}

/*
 * mu3d_hal_send_stall - send stall
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_send_stall(int q_ep_num,  u8 dir)
{
	if (dir == USB_DIR_IN) {
		USB_WRITECSR32(U3D_TX1CSR0, q_ep_num, USB_READCSR32(U3D_TX1CSR0, q_ep_num) | TX_SENDSTALL);
		while (!(USB_READCSR32(U3D_TX1CSR0, q_ep_num) & TX_SENTSTALL));
		USB_WRITECSR32(U3D_TX1CSR0, q_ep_num, USB_READCSR32(U3D_TX1CSR0, q_ep_num) | TX_SENTSTALL);
		USB_WRITECSR32(U3D_TX1CSR0, q_ep_num, USB_READCSR32(U3D_TX1CSR0, q_ep_num) &~ TX_SENDSTALL);
		DBG_I("EP[%d] SENTSTALL!\n",q_ep_num);
	} else if (dir == USB_DIR_OUT) {
		USB_WRITECSR32(U3D_RX1CSR0, q_ep_num, USB_READCSR32(U3D_RX1CSR0, q_ep_num) | RX_SENDSTALL);
		while (!(USB_READCSR32(U3D_RX1CSR0, q_ep_num) & RX_SENTSTALL));
		USB_WRITECSR32(U3D_RX1CSR0, q_ep_num, USB_READCSR32(U3D_RX1CSR0, q_ep_num) | RX_SENTSTALL);
		USB_WRITECSR32(U3D_RX1CSR0, q_ep_num, USB_READCSR32(U3D_RX1CSR0, q_ep_num) &~ RX_SENDSTALL);
		DBG_I("EP[%d] SENTSTALL!\n",q_ep_num);
	}
}

/*
 * mu3d_hal_reset_qmu_ep - clear toggle(or sequence) number
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_reset_qmu_ep(int q_ep_num,  u8 dir)
{
	u32 ep_rst;

	DBG_I("%s\n", __func__);

	if (dir == USB_DIR_IN) {
		ep_rst = BIT16 << q_ep_num;
		writel(ep_rst, U3D_EP_RST);
		mdelay(1);
		writel(0, U3D_EP_RST);
	} else {
		ep_rst = 1 << q_ep_num;
		writel(ep_rst, U3D_EP_RST);
		mdelay(1);
		writel(0, U3D_EP_RST);
	}
}

/*
 * mu3d_hal_restart_qmu - clear toggle(or sequence) number and start qmu
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_restart_qmu_ep(int q_ep_num,  u8 dir)
{
	mu3d_hal_reset_qmu_ep(q_ep_num, dir);
	mu3d_hal_start_qmu(q_ep_num, dir);
}

/*
 * flush_qmu - stop qmu and align qmu start ptr t0 current ptr
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_flush_qmu(int q_ep_num,  u8 dir)
{
	struct tgpd *gpd_current;
	struct udc_endpoint *ept;
	struct urb *urb;
	//struct USB_REQ *req = mu3d_hal_get_req(q_ep_num, dir);

	ept = mt_find_ep(q_ep_num, dir);

	DBG_I("%s\n", __func__);

	if (dir == USB_DIR_IN) {
		DBG_I("flush_qmu USB_DIR_IN\n");
		urb = ept->tx_urb;
		mu3d_hal_stop_qmu(q_ep_num, USB_DIR_IN);
		gpd_current = (struct tgpd*)(readl(U3D_TXQCPR(q_ep_num)));

		if (!gpd_current) {
			gpd_current = (struct tgpd*)(readl(U3D_TXQSAR(q_ep_num)));
		}

#if defined(SUPPORT_VA)
		gpd_current = gpd_phys_to_virt(gpd_current, USB_DIR_IN, q_ep_num);
#endif
		tx_gpd_end[q_ep_num] = tx_gpd_last[q_ep_num] = tx_gpd_head[q_ep_num];
		DBG_I("gpd_current:%x, tx to HEAD :%x\n", gpd_current, tx_gpd_head[q_ep_num]);

		gpd_ptr_align(dir, q_ep_num, tx_gpd_end[q_ep_num]);
		free_gpd(dir, q_ep_num);

		arch_clean_invalidate_cache_range((addr_t) tx_gpd_list[q_ep_num].pstart, MAX_GPD_NUM * sizeof(struct tgpd));
#if defined(SUPPORT_VA)
		writel(mu3d_hal_gpd_virt_to_phys(tx_gpd_last[q_ep_num], USB_DIR_IN, q_ep_num), U3D_TXQSAR(q_ep_num));
#else
		writel((u32)tx_gpd_last[q_ep_num], U3D_TXQSAR(q_ep_num));
#endif
		urb->qmu_complete = true;
		DBG_I("TxQ %d Flush Now!\n", q_ep_num);
	} else if (dir == USB_DIR_OUT) {
		urb = ept->rcv_urb;
		mu3d_hal_stop_qmu(q_ep_num, USB_DIR_OUT);
		gpd_current = (struct tgpd *)(readl(U3D_RXQCPR(q_ep_num)));

		if (!gpd_current) {
			gpd_current = (struct tgpd *)(readl(U3D_RXQSAR(q_ep_num)));
		}

#if defined(SUPPORT_VA)
		gpd_current = gpd_phys_to_virt(gpd_current, USB_DIR_OUT, q_ep_num);
#endif
		rx_gpd_end[q_ep_num] = rx_gpd_last[q_ep_num] = rx_gpd_head[q_ep_num];
		DBG_I("gpd_current:%x, rx to HEAD :%x\n", gpd_current, rx_gpd_head[q_ep_num]);

		gpd_ptr_align(dir, q_ep_num, rx_gpd_end[q_ep_num]);
		free_gpd(dir, q_ep_num);

		arch_clean_invalidate_cache_range((addr_t) rx_gpd_list[q_ep_num].pstart, MAX_GPD_NUM * sizeof(struct tgpd));
#if defined(SUPPORT_VA)
		writel(mu3d_hal_gpd_virt_to_phys(rx_gpd_end[q_ep_num], USB_DIR_OUT, q_ep_num), U3D_RXQSAR(q_ep_num));
#else
		writel((u32)rx_gpd_end[q_ep_num], U3D_RXQSAR(q_ep_num));
#endif
		urb->qmu_complete = true;
		DBG_I("RxQ %d Flush Now!\n", q_ep_num);
	}
}
#endif /* #ifdef SUPPORT_QMU */
