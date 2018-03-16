#ifndef __SSUSB_QMU_H__
#define __SSUSB_QMU_H__

#define USE_SSUSB_QMU

//#include "mt_mu3d_hal_qmu_drv.h"

#ifdef USE_SSUSB_QMU

#define GPD_BUF_SIZE 65532
#define GPD_BUF_SIZE_ALIGN 64512 /* 63 * 1024 */
#define BD_BUF_SIZE 32768 //set to half of 64K of max size

struct qmu_done_isr_data {
	struct musb *musb;
	u32 qmu_val;
};

//EXTERN void usb_initialize_qmu(void);
void usb_initialize_qmu(void);
//EXTERN void txstate_qmu(struct musb *musb, struct musb_request *req);
//EXTERN void txstate_qmu(struct urb *req);
void txstate_qmu(struct urb *req);

void qmu_exception_interrupt(void *base, u32 qmuval);
void qmu_done_interrupt(u32 qmu_val);

void qmu_handler(u32 qmu_val);

#endif //#ifdef USE_SSUSB_QMU

#endif //#ifndef __SSUSB_QMU_H__
