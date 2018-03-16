/*
 * Copyright (c) 2013 MediaTek Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in
 *  the documentation and/or other materials provided with the
 *  distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
#include <kernel/thread.h>

#include <dev/udc.h>

#include <platform/mt_usb.h>
#include <platform/mt_ssusb_qmu.h>
#include <platform/mt_mu3d_hal_qmu_drv.h>

//#define MTK_QMU_DRV_EXT /* only need to define this when QMU vars are extern */

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

/* TODO: replace these 2 static to extern */
extern struct udc_device *the_device;
extern struct udc_gadget *the_gadget;

u8 is_bdp;

#ifdef SUPPORT_QMU
/*
 * 1. Configure the HW register (starting address) with gpd heads (starting address) for each Queue
 * 2. Enable Queue with each EP and interrupts
 */
void usb_initialize_qmu()
{
	mu3d_hal_init_qmu();
	is_bdp = 0; /* We need to use BD because for mass storage, the largest data > GPD limitation */
	//gpd_buf_size = GPD_BUF_SIZE; //max allowable data buffer length. Don't care when linking with BD.
	gpd_buf_size = GPD_BUF_SIZE_ALIGN; //max allowable data buffer length. Don't care when linking with BD.
	bd_buf_size = BD_BUF_SIZE;
	bd_extension = 0;
	gpd_extension = 0;
	g_dma_buffer_size = STRESS_DATA_LENGTH * MAX_GPD_NUM;
}

/*
 * qmu_handler - handle qmu error events
 * @args - arg1: ep number
 */
void qmu_handler(u32 qmu_val)
{
	int i, err;
	i = (int)qmu_val;

	DBG_I("%s: qmu_val: %x\n", __func__, qmu_val);


	if (qmu_val & RXQ_CSERR_INT) {
		DBG_I("Rx %d checksum error!\n", i);
	}

	if (qmu_val & RXQ_LENERR_INT) {
		DBG_I("Rx %d length error!\n", i);
		//g_rx_len_err_cnt++;
	}

	if (qmu_val & TXQ_CSERR_INT) {
		DBG_I("Tx %d checksum error!\n", i);
	}

	if (qmu_val & TXQ_LENERR_INT) {
		DBG_I("Tx %d length error!\n", i);
	}

	if ((qmu_val & RXQ_CSERR_INT) || (qmu_val & RXQ_LENERR_INT)) {
		err = readl(U3D_RQERRIR0);
		DBG_I("U3D_RQERRIR0 : [0x%x]\n", err);
		for (i = 1; i <= MAX_QMU_EP; i++) {
			if (err & QMU_RX_CS_ERR(i)) {
				DBG_I("Rx %d length error!\n", i);
			}

			if (err &QMU_RX_LEN_ERR(i)) {
				DBG_I("Rx %d recieve length error!\n", i);
			}
		}
		writel(err, U3D_RQERRIR0);
	}

	if (qmu_val & RXQ_ZLPERR_INT) {
		err = readl(U3D_RQERRIR1);
		DBG_I("U3D_RQERRIR1: [0x%x]\n", err);

		for (i = 1; i <= MAX_QMU_EP; i++) {
			if (err & QMU_RX_ZLP_ERR(i)) {
				DBG_I("Rx %d recieve an zlp packet!\n", i);
			}
		}
		writel(err, U3D_RQERRIR1);
	}

	if ((qmu_val & TXQ_CSERR_INT) || (qmu_val & TXQ_LENERR_INT)) {

		err = readl(U3D_TQERRIR0);
		DBG_I("Tx Queue error in QMU mode![0x%x]\n", err);

		for (i = 1; i <= MAX_QMU_EP; i++) {
			if (err & QMU_TX_CS_ERR(i)) {
				DBG_I("Tx %d checksum error!\n", i);
			}

			if (err & QMU_TX_LEN_ERR(i)) {
				DBG_I("Tx %d buffer length error!\n", i);
			}
		}
		writel(err, U3D_TQERRIR0);
	}

	if ((qmu_val & RXQ_EMPTY_INT) || (qmu_val & TXQ_EMPTY_INT)) {
		u32 empty = readl(U3D_QEMIR);
		DBG_I("Rx/Tx Empty in QMU mode![0x%x]\n", empty);
		writel(empty, U3D_QEMIR);
	}
}

/*
 * put usb_request buffer into GPD/BD data structures, and ask QMU to start TX.
 *
 * caller: musb_g_tx
 */
void txstate_qmu(struct urb *req)
{
	u8 ep_num;
	struct udc_endpoint *ept;
	//struct usb_request    *request;
	struct urb      *current_urb;
	u32 txcsr0 = 0;

	ep_num = req->ept->num;

	//ept = &musb->endpoints[ep_num].ep_in;
	//struct udc_endpoint *endpoint = &mt_usb_ep[TX_ENDPOINT];
	//TODO: fix
	//ept = &musb->p_ep_array[ep_num];
	ept = mt_find_ep(ep_num, USB_DIR_IN);

	/* do not use "urb->ept->tx_pktsz ??*/
	//request = &req->request;
	// Note: request -> current_urb;
	current_urb = ept->tx_urb;

	//TODO: fix
	//txcsr0 = readl(musb->endpoints[ep_num].addr_txcsr0);
	//txcsr0 = readl(musb->p_ep_array[ep_num].addr_txcsr0);
	txcsr0 = readl(U3D_TX1CSR0);

	if (txcsr0 & TX_SENDSTALL) {
		DBG_I("%s: ep %d stalling, txcsr %03x\n",
		      __func__, ept->num, txcsr0);
		return;
	}
//	request->actual = request->length;
//	mu3d_hal_insert_transfer_gpd(ep_num, USB_DIR_IN, (u8 *)request->dma, request->length, true,true,false,(ept->type==USB_ENDPOINT_XFER_ISOC?0:1), ept->packet_sz);
	mu3d_hal_insert_transfer_gpd(ep_num, USB_DIR_IN, (u8 *)current_urb->buf, current_urb->actual_length, true, true, false, (ept->type == USB_EP_XFER_ISO ? 0 : 1), ept->maxpkt);
	mu3d_hal_resume_qmu(ep_num, USB_DIR_IN);
	DBG_I("tx start...length : %d\n", current_urb->actual_length);

}


/* the caller ensures that req is not NULL. */
void rxstate_qmu(struct urb *req)
{
	u8 ep_num;
	struct udc_endpoint     *ept;
	//struct usb_request    *request;
	struct urb      *current_urb;
	u32 rxcsr0 = 0;

	//ep_num = GET_EP_NUM(req->p_ep->endpoint_address);
	ep_num = req->ept->num;

	//TODO: fix
	//struct udc_endpoint       *ept = &musb->endpoints[epnum].ep_out;
	//ept = &musb->p_ep_array[ep_num];
	ept = mt_find_ep(ep_num, USB_DIR_OUT);

	//TODO: fix
	//rxcsr0 = readl(musb->endpoints[ep_num].addr_rxcsr0);
	//rxcsr0 = readl(musb->p_ep_array[ep_num]);
	rxcsr0 = readl(U3D_RX1CSR0);

	//request = &req->request;
	current_urb = ept->rcv_urb;

	if (rxcsr0 & RX_SENDSTALL) {
		DBG_I("%s: ep %d stalling, RXCSR %04x\n",
		      __func__, ept->num, rxcsr0);
		return;
	}

	DBG_I("rxstate_qmu 0\n");
	//mu3d_hal_insert_transfer_gpd(ep_num, USB_DIR_OUT,  (u8*)request->dma, request->length, true, true, false, (ept->type==USB_ENDPOINT_XFER_ISOC?0:1), ept->maxpkt);
	mu3d_hal_insert_transfer_gpd(ep_num, USB_DIR_OUT, (u8 *)current_urb->buf, current_urb->actual_length, true, true, false, (ept->type == USB_EP_XFER_ISO ? 0 : 1), ept->maxpkt);
	mu3d_hal_resume_qmu(ep_num, USB_DIR_OUT);
	DBG_I("rx start\n");

}

/*
 * 1. Find the last gpd HW has executed and update tx_gpd_last[]
 * 2. Set the flag for txstate to know that TX has been completed
 * ported from proc_qmu_tx() from test driver.
 * caller:qmu_interrupt after getting QMU done interrupt and TX is raised
 */
void qmu_tx_interrupt(u8 ep_num)
{

	struct tgpd *gpd = tx_gpd_last[ep_num];
	struct tgpd *gpd_current = (struct tgpd*)(readl(U3D_TXQCPR(ep_num)));
	struct udc_endpoint     *ept;
	ept = mt_find_ep(ep_num, USB_DIR_IN);

#if defined(SUPPORT_VA)
	gpd_current = gpd_phys_to_virt(gpd_current, USB_DIR_IN, ep_num);
#endif
	DBG_I("tx_gpd_last 0x%x, gpd_current 0x%x, gpd_end 0x%x, \n",  (u32)gpd, (u32)gpd_current, (u32)tx_gpd_end[ep_num]);

	if (gpd == gpd_current) {//gpd_current should at least point to the next GPD to the previous last one.
		DBG_I("should not happen: %s %d\n", __func__, __LINE__);
		return;
	}

	while (gpd != gpd_current && !TGPD_IS_FLAGS_HWO(gpd)) {

		DBG_I("Tx gpd %x info { HWO %d, BPD %d, Next_GPD %x , DataBuffer %x, BufferLen %d, Endpoint %d}\n",
		      (u32)gpd, (u32)TGPD_GET_FLAG(gpd), (u32)TGPD_GET_FORMAT(gpd), (u32)TGPD_GET_NEXT(gpd),
		      (u32)TGPD_GET_DATA(gpd), (u32)TGPD_GET_BUF_LEN(gpd), (u32)TGPD_GET_EPaddr(gpd));

		/* required for mt_udc_epx_handler */
		ept->sent = (u32)TGPD_GET_BUF_LEN(gpd);

		gpd = TGPD_GET_NEXT(gpd);
#if defined(SUPPORT_VA)
		gpd = gpd_phys_to_virt(gpd, USB_DIR_IN, ep_num);
#endif

		tx_gpd_last[ep_num] = gpd;
	}

	DBG_I("tx_gpd_last[%d]: 0x%x\n", ep_num, (u32)tx_gpd_last[ep_num]);
	DBG_I("tx_gpd_end[%d]:  0x%x\n", ep_num, (u32)tx_gpd_end[ep_num]);
	DBG_I("Tx %d complete\n", ep_num);

	return;
}

/*
 * When receiving RXQ done interrupt, qmu_interrupt calls this function.
 *
 * 1. Traverse GPD/BD data structures to count actual transferred length.
 * 2. Set the done flag to notify rxstate_qmu() to report status to upper gadget driver.
 *
 *  ported from proc_qmu_rx() from test driver.
 *  caller:qmu_interrupt after getting QMU done interrupt and TX is raised
 */
void qmu_rx_interrupt(u8 ep_num)
{
#ifdef DBG_USB_QMU
	u32 bufferlength;
#endif
	u32 recivedlength = 0;
	struct tgpd *gpd = (struct tgpd*) rx_gpd_last[ep_num];
	struct tgpd *gpd_current = (struct tgpd*)(readl(U3D_RXQCPR(ep_num)));
	struct tbd *bd;
	struct udc_endpoint     *ept;
	struct urb      *current_urb;

	ept = mt_find_ep(ep_num, USB_DIR_OUT);

	current_urb = ept->rcv_urb;

#if defined(SUPPORT_VA)
	gpd_current = gpd_phys_to_virt(gpd_current, USB_DIR_OUT, ep_num);
#endif
	DBG_I("ep_num: %d ,Rx_gpd_last: 0x%x, gpd_current: 0x%x, gpd_end: 0x%x \n",ep_num,(u32)gpd, (u32)gpd_current, (u32)rx_gpd_end[ep_num]);

	if (gpd == gpd_current) {
		DBG_I("should not happen: %s %d\n", __func__, __LINE__);
		return;
	}

	while (gpd != gpd_current && !TGPD_IS_FLAGS_HWO(gpd)) {

		if (TGPD_IS_FORMAT_BDP(gpd)) {

			bd = (struct tbd *)TGPD_GET_DATA(gpd);
#if defined(SUPPORT_VA)
			bd = (struct tbd *)bd_phys_to_virt(bd,USB_DIR_OUT, ep_num);
#endif

			while (1) {

				DBG_I("BD: 0x%x\n", (u32)bd);
				DBG_I("Buf Len: 0x%x\n", (u32)TBD_GET_BUF_LEN(bd));
				//req->transferCount += TBD_GET_BUF_LEN(bd);
				//ept->qmu_done_length += TBD_GET_BUF_LEN(bd);
				current_urb->actual_length += TBD_GET_BUF_LEN(bd);

				//DBG_I("Total Len : 0x%x\n",ept->qmu_done_length);
				if (TBD_IS_FLAGS_EOL(bd)) {
					break;
				}
				bd = TBD_GET_NEXT(bd);
#if defined(SUPPORT_VA)
				bd = bd_phys_to_virt(bd, USB_DIR_OUT, ep_num);
#endif
			}
		} else {
			recivedlength = (u32)TGPD_GET_BUF_LEN(gpd);
#ifdef DBG_USB_QMU
			bufferlength  = (u32)TGPD_GET_DATABUF_LEN(gpd);
#endif

			/* required for mt_udc_epx_handler */
			current_urb->actual_length += recivedlength;
#ifdef DBG_USB_QMU
			DBG_I("%s: recivedlength: %d, bufferlength: %d, current_urb->actual_length: %d\n", __func__, recivedlength, bufferlength, current_urb->actual_length);
#endif
		}

		DBG_I("Rx gpd info { HWO %d, Next_GPD %x ,DataBufferLength %d, DataBuffer %x, Recived Len %d, Endpoint %d, TGL %d, ZLP %d}\n",
		      (u32)TGPD_GET_FLAG(gpd), (u32)TGPD_GET_NEXT(gpd),
		      (u32)TGPD_GET_DATABUF_LEN(gpd), (u32)TGPD_GET_DATA(gpd),
		      (u32)TGPD_GET_BUF_LEN(gpd), (u32)TGPD_GET_EPaddr(gpd),
		      (u32)TGPD_GET_TGL(gpd), (u32)TGPD_GET_ZLP(gpd));

		gpd = TGPD_GET_NEXT(gpd);
#if defined(SUPPORT_VA)
		gpd = gpd_phys_to_virt(gpd,USB_DIR_OUT, ep_num);
#endif

		rx_gpd_last[ep_num] = gpd;
	}

	DBG_I("rx_gpd_last[%d]: 0x%x\n", ep_num, (u32)rx_gpd_last[ep_num]);
	DBG_I("rx_gpd_end[%d]:  0x%x\n", ep_num, (u32)rx_gpd_end[ep_num]);
}

void qmu_done_interrupt(u32 qmu_val)
{
	int i;

	DBG_I("[USB][QMU] qmu_interrupt\n");

	for (i = 1; i <= MAX_QMU_EP; i++) {
		if (qmu_val & QMU_RX_DONE(i)) {
			qmu_rx_interrupt(i);
		}

		if (qmu_val & QMU_TX_DONE(i)) {
			qmu_tx_interrupt(i);
		}
	}
}

void qmu_exception_interrupt(void *base, u32 qmu_val)
{
	u32 err;
	int i = (int) qmu_val;

	if (qmu_val & RXQ_CSERR_INT) {
		DBG_I("Rx %d checksum error!\n", i);
	}

	if (qmu_val & RXQ_LENERR_INT) {
		DBG_I("Rx %d length error!\n", i);
	}

	if (qmu_val & TXQ_CSERR_INT) {
		DBG_I("Tx %d checksum error!\n", i);
	}

	if (qmu_val & TXQ_LENERR_INT) {
		DBG_I("Tx %d length error!\n", i);
	}

	if ((qmu_val & RXQ_CSERR_INT) || (qmu_val & RXQ_LENERR_INT)) {
		err = readl(U3D_RQERRIR0);
		DBG_I("Rx Queue error in QMU mode![0x%x]\n", (unsigned int)err);
		for (i = 1; i <= MAX_QMU_EP; i++) {

			if (err & QMU_RX_CS_ERR(i)) {
				DBG_I("Rx %d length error!\n", i);
			}

			if (err & QMU_RX_LEN_ERR(i)) {
				DBG_I("Rx %d recieve length error!\n", i);
			}
		}
		writel(err, U3D_RQERRIR0);
	}

	if (qmu_val & RXQ_ZLPERR_INT) {
		err = readl(U3D_RQERRIR1);
		DBG_I("Rx Queue error in QMU mode![0x%x]\n", (unsigned int)err);

		for (i = 1; i <= MAX_QMU_EP; i++) {
			if (err &QMU_RX_ZLP_ERR(i)) {
				DBG_I("Rx %d recieve an zlp packet!\n", i);
			}
		}
		writel(err, U3D_RQERRIR1);
	}

	if ((qmu_val & TXQ_CSERR_INT) || (qmu_val & TXQ_LENERR_INT)) {

		err = readl(U3D_TQERRIR0);
		DBG_I("Tx Queue error in QMU mode![0x%x]\n", (unsigned int)err);

		for (i = 1; i <= MAX_QMU_EP; i++) {
			if (err & QMU_TX_CS_ERR(i)) {
				DBG_I("Tx %d checksum error!\n", i);
			}

			if (err & QMU_TX_LEN_ERR(i)) {
				DBG_I("Tx %d buffer length error!\n", i);
			}
		}
		writel(err, U3D_TQERRIR0);

	}

	if ((qmu_val & RXQ_EMPTY_INT) || (qmu_val & TXQ_EMPTY_INT)) {
		u32 empty = readl(U3D_QEMIR);
		DBG_I("Rx Empty in QMU mode![0x%x]\n", empty);
		writel(empty, U3D_QEMIR);
		mu3d_hal_flush_qmu(1, USB_DIR_OUT);
		mu3d_hal_restart_qmu(1, USB_DIR_OUT);
	}

}
#endif /* ifdef SUPPORT_QMU */
