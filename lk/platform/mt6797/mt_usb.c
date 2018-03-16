/*
 * Copyright (c) 2012 MediaTek Inc.
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
#include <platform/mt_ssusb_sifslv_ippc.h>
#include <platform/mt_ssusb_usb3_mac_csr.h>
#include <platform/mt_typedefs.h>
#include <platform/timer.h>
#include <kernel/thread.h>

#include <dev/udc.h>

#ifdef SUPPORT_QMU
#include <platform/mt_ssusb_qmu.h>
#include <platform/mt_mu3d_hal_qmu_drv.h>
#endif

#define USB_GINTR

/* DEBUG INFO Sections */
#ifdef USB_DEBUG
#define DBG_USB_DUMP_DESC 0
#define DBG_USB_DUMP_DATA 0
#define DBG_USB_DUMP_SETUP 1
#define DBG_USB_FIFO 0
#define DBG_USB_GENERAL 1
#define DBG_USB_IRQ 0
#define DBG_PHY_CALIBRATION 0
#endif

#define DBG_C(x...) dprintf(CRITICAL, "[USB] " x)
#define DBG_I(x...) dprintf(INFO, "[USB] " x)
#define DBG_S(x...) dprintf(SPEW, "[USB] " x)

#if DBG_USB_GENERAL
#define DBG_IRQ(x...) dprintf(INFO, x)
#else
#define DBG_IRQ(x...) do{} while(0)
#endif

/* bits used in all the endpoint status registers */
#define EPT_TX(n) (1 << ((n) + 16))
#define EPT_RX(n) (1 << (n))

/* udc.h wrapper for usbdcore */

static unsigned char usb_config_value = 0;
EP0_STATE ep0_state = EP0_IDLE;
int set_address = 0;

u8 g_qmu_init = 0;

#define EP0 0

/* USB transfer directions */
#define USB_DIR_IN  DEVICE_WRITE    /* val: 0x80 */
#define USB_DIR_OUT DEVICE_READ /* val: 0x00 */

/* Request types */
#define USB_TYPE_STANDARD   (0x00 << 5)
#define USB_TYPE_CLASS      (0x01 << 5)
#define USB_TYPE_VENDOR     (0x02 << 5)
#define USB_TYPE_RESERVED   (0x03 << 5)

/* values used in GET_STATUS requests */
#define USB_STAT_SELFPOWERED    0x01

/* USB recipients */
#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02
#define USB_RECIP_OTHER     0x03

/* Endpoints */
#define USB_EP_NUM_MASK 0x0f        /* in bEndpointAddress */
#define USB_EP_DIR_MASK 0x80

#define USB_TYPE_MASK   0x60
#define USB_RECIP_MASK  0x1f

#if CFG_FPGA_PLATFORM
/* for usb phy */
#include <platform/mt_i2c.h>
#endif

#define URB_BUF_SIZE 512

/* function prototype */
/* usbphy.c */
extern void mt_usb_phy_recover(void);
extern void mt_usb_phy_savecurrent(void);
extern void mt_usb_phy_poweron(void);

static struct udc_endpoint *ep0in, *ep0out;
static struct udc_request *ep0req;
static unsigned char ep0_buf[4096] __attribute__((aligned(32)));
struct urb mt_ep0_urb;
struct urb mt_tx_urb;
struct urb mt_rx_urb;
struct urb *ep0_urb = &mt_ep0_urb;
struct urb *tx_urb = &mt_tx_urb;
struct urb *rx_urb = &mt_rx_urb;


/* mt_usb related function declaration */
void mt_setup_ep(unsigned int ep, struct udc_endpoint *endpoint);

/* from mt_usbtty.h */
#define NUM_ENDPOINTS   3

/* origin endpoint_array */
struct udc_endpoint ep_list[NUM_ENDPOINTS + 1]; /* one extra for control endpoint */

static int usb_online = 0;

u32 g_speed = 0;

static u8 dev_address = 0;

u32 g_tx_fifo_addr = USB_TX_FIFO_START_ADDRESS;
u32 g_rx_fifo_addr = USB_RX_FIFO_START_ADDRESS;
int g_enable_u3 = 0;

static struct udc_device *the_device;
static struct udc_gadget *the_gadget;
/* end from hsusb.c */

/* declare ept_complete handle */
static void handle_ept_complete(struct udc_endpoint *ept);

extern void mt_usb_phy_poweron(void);
extern void mt_usb_phy_savecurrent(void);
extern void mt_usb_phy_recover(void);

/* get_max_packet_size */
int get_max_packet_size(int enable_u3)
{
	if (enable_u3 != 0) {
		return EP0_MAX_PACKET_SIZE;
	} else {
		return EP0_MAX_PACKET_SIZE_U3;
	}
}

void board_usb_init(void)
{
	mt_usb_phy_poweron();
	mt_usb_phy_savecurrent();
}

struct udc_descriptor {
	struct udc_descriptor *next;
	unsigned short tag; /* ((TYPE << 8) | NUM) */
	unsigned short len; /* total length */
	unsigned char data[0];
};

#if DBG_USB_DUMP_SETUP
static void dump_setup_packet(char *str, struct setup_packet *sp)
{
	DBG_I("\n");
	DBG_I("%s", str);
	DBG_I("	   bmRequestType = %x\n", sp->type);
	DBG_I("	   bRequest = %x\n", sp->request);
	DBG_I("	   wValue = %x\n", sp->value);
	DBG_I("	   wIndex = %x\n", sp->index);
	DBG_I("	   wLength = %x\n", sp->length);
}
#else
static void dump_setup_packet(char *str, struct setup_packet *sp) {}
#endif

#if defined(DBG_USB_EP0CSR)
void explain_csr0(u32 csr0)
{
	if (csr0 & EP0_EP_RESET) {
		DBG_I("EP0_EP_RESET is set");
	}
	if (csr0 & EP0_AUTOCLEAR) {
		DBG_I("EP0_AUTOCLEAR is set");
	}
	if (csr0 & EP0_AUTOSET) {
		DBG_I("EP0_AUTOSET is set");
	}
	if (csr0 & EP0_DMAREQEN) {
		DBG_I("EP0_DMAREQEN is set");
	}
	if (csr0 & EP0_SENDSTALL) {
		DBG_I("EP0_SENDSTALL is set");
	}
	if (csr0 & EP0_FIFOFULL) {
		DBG_I("EP0_FIFOFULL is set");
	}
	if (csr0 & EP0_SENTSTALL) {
		DBG_I("EP0_SENTSTALL is set");
	}
	if (csr0 & EP0_DPHTX) {
		DBG_I("EP0_DPHTX is set");
	}
	if (csr0 & EP0_DATAEND) {
		DBG_I("EP0_DATAEND is set");
	}
	if (csr0 & EP0_TXPKTRDY) {
		DBG_I("EP0_TXPKTRDY is set");
	}
	if (csr0 & EP0_SETUPPKTRDY) {
		DBG_I("EP0_SETUPPKTRDY is set");
	}
	if (csr0 & EP0_RXPKTRDY) {
		DBG_I("EP0_RXPKTRDY is set");
	}
}
#endif

static void copy_desc(struct urb *urb, void *data, int length)
{

#if DBG_USB_FIFO
	DBG_I("%s: urb: %p, data %p, length: %d, actual_length: %d\n",
	      __func__, urb->buf, data, length, urb->actual_length);
#endif

	//memcpy(urb->buf + urb->actual_length, data, length);
	memcpy(urb->buf, data, length);
	//urb->actual_length += length;
	urb->actual_length = length;
#if DBG_USB_FIFO
	DBG_I("%s: urb: %p, data %p, length: %d, actual_length: %d\n",
	      __func__, urb->buf, data, length, urb->actual_length);
#endif
}

struct udc_descriptor *udc_descriptor_alloc(unsigned type, unsigned num,
        unsigned len)
{
	struct udc_descriptor *desc;
	if ((len > 255) || (len < 2) || (num > 255) || (type > 255))
		return 0;

	if (!(desc = malloc(sizeof(struct udc_descriptor) + len)))
		return 0;

	desc->next = 0;
	desc->tag = (type << 8) | num;
	desc->len = len;
	desc->data[0] = len;
	desc->data[1] = type;

	return desc;
}

static struct udc_descriptor *desc_list = 0;
static struct udc_descriptor *desc_list_u3 = 0;
static unsigned next_string_id = 1;
static unsigned next_string_id_u3 = 1;

void udc_descriptor_register(struct udc_descriptor *desc)
{
	desc->next = desc_list;
	desc_list = desc;
}

#ifdef SUPPORT_U3
void udc_descriptor_register_u3(struct udc_descriptor *desc)
{
	desc->next = desc_list_u3;
	desc_list_u3 = desc;
}
#endif

unsigned udc_string_desc_alloc(const char *str)
{
	unsigned len;
	struct udc_descriptor *desc;
	unsigned char *data;

	if (next_string_id > 255)
		return 0;

	if (!str)
		return 0;

	len = strlen(str);
	desc = udc_descriptor_alloc(TYPE_STRING, next_string_id, len * 2 + 2);
	if (!desc)
		return 0;
	next_string_id++;

	/* expand ascii string to utf16 */
	data = desc->data + 2;
	while (len-- > 0) {
		*data++ = *str++;
		*data++ = 0;
	}

	udc_descriptor_register(desc);
	return desc->tag & 0xff;
}

#ifdef SUPPORT_U3
unsigned udc_string_desc_alloc_u3(const char *str)
{
	unsigned len;
	struct udc_descriptor *desc;
	unsigned char *data;

	if (next_string_id_u3 > 255)
		return 0;

	if (!str)
		return 0;

	len = strlen(str);
	desc = udc_descriptor_alloc(TYPE_STRING, next_string_id_u3, len * 2 + 2);
	if (!desc)
		return 0;
	next_string_id_u3++;

	/* expand ascii string to utf16 */
	data = desc->data + 2;
	while (len-- > 0) {
		*data++ = *str++;
		*data++ = 0;
	}

	udc_descriptor_register_u3(desc);
	return desc->tag & 0xff;
}
#endif

/* mt_udc_state related functions */
/*
 * If abnormal DATA transfer happened, like USB unplugged,
 * we cannot fix this after mt_udc_reset().
 * Because sometimes there will come reset twice.
 */
static void mt_udc_suspend(void)
{
	/* handle abnormal DATA transfer if we had any */
	struct udc_endpoint *endpoint;
	int i;
	extern int txn_status;

	/* initialize flags */
	usb_online = 0;
	usb_config_value = 0;
	the_gadget->notify(the_gadget, UDC_EVENT_OFFLINE);

	/* error out any pending reqs */
	for (i = 1; i < MT_EP_NUM; i++) {
		/* ensure that ept_complete considers
		 * this to be an error state
		 */

		/* End operation when encounter uninitialized ept */
		if (ep_list[i].num == 0)
			break;
#if DBG_USB_GENERAL
		DBG_I("%s: ep: %i, in: %s, req: %p\n",
		      __func__, ep_list[i].num, ep_list[i].in ? "IN" : "OUT", ep_list[i].req);
#endif
		/* do nothing if usb_online == 0 */
		if ((ep_list[i].req && (ep_list[i].in == 0)) || /* USB_DIR_OUT */
		        (ep_list[i].req && (ep_list[i].in == 1))) { /* USB_DIR_IN */
			ep_list[i].status = -1; /* HALT */
			endpoint = &ep_list[i];
			handle_ept_complete(endpoint);
		}
	}

	/* this is required for error handling during data transfer */
	txn_status = -1;

#if defined(SUPPORT_QMU)

	/* stop qmu engine */
	mu3d_hal_stop_qmu(1, USB_DIR_IN);
	mu3d_hal_stop_qmu(1, USB_DIR_OUT);

	/* disable qmu interrupts */
	writel(0x0, U3D_QGCSR);
	writel(0x0, U3D_QIESR0);
	writel(0x0, U3D_QIESR1);

	/* do qmu flush */
	mu3d_hal_flush_qmu(1, USB_DIR_IN);
	mu3d_hal_flush_qmu(1, USB_DIR_OUT);

	/* mu3d_hal_reset_ep: we do reset here only, do not start qmu here */
	mu3d_hal_reset_qmu_ep(1, USB_DIR_IN);
	mu3d_hal_reset_qmu_ep(1, USB_DIR_OUT);
#endif
}

/* mu3d hal related functions */
/* functinos used by mu3d hal functions */
int wait_for_value(int addr, unsigned int msk, unsigned int value, int ms_intvl, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if ((readl(addr) & msk) == value)
			return RET_SUCCESS;

		mdelay(ms_intvl);
	}

	return RET_FAIL;
}

/*
 * mu3d_hal_pio_read_fifo - pio read one packet
 * @args - arg1: ep number,  arg2: data buffer
 */
int mu3d_hal_pio_read_fifo(int ep_num, u8 *p_buf)
{
	u32 count = 0, residue = 0;
	u32 temp = 0;
	u8 *p_tmpbuf = p_buf;

	if (ep_num == 0) {
		residue = count = readl(U3D_RXCOUNT0);
	} else {
		residue = count = USB_READCSR32(U3D_RX1CSR3, ep_num) >> 16;
	}

	while (residue > 0) {
		temp = readl(USB_FIFO(ep_num));

		*p_tmpbuf = temp&0xFF;
		if (residue > 1)
			*(p_tmpbuf + 1) = (temp >> 8)&0xFF;

		if (residue > 2)
			*(p_tmpbuf + 2) = (temp >> 16)&0xFF;

		if (residue > 3)
			*(p_tmpbuf + 3) = (temp >> 24)&0xFF;

		if (residue > 4) {
			p_tmpbuf = p_tmpbuf + 4;
			residue -= 4;
		} else {
			residue = 0;
		}
	}

	return count;
}

/*
 * mu3d_hal_pio_write_fifo - pio write one packet
 * @args - arg1: ep number,  arg2: data buffer
 */
int mu3d_hal_pio_write_fifo(int ep_num, int length, u8 *p_buf, int maxp)
{
	u32 residue = length;
	u32 temp = 0;

#if DBG_USB_FIFO
	DBG_I("%s process p_num: %d, length: %d, buf: %p, maxp: %d\n",
	      __func__, ep_num, length, p_buf, maxp);
#endif

	while (residue > 0) {
		switch (residue) {
			case 1:
				temp = ((*p_buf) & 0xFF);
				writeb(temp, USB_FIFO(ep_num));
				p_buf += 1;
				residue -= 1;
				break;
			case 2:
				temp = ((*p_buf) & 0xFF)
				       + (((*(p_buf + 1)) << 8)&0xFF00);
				writew(temp, USB_FIFO(ep_num));
				p_buf += 2;
				residue -= 2;
				udelay(50);
				break;
			case 3:
				temp = ((*p_buf) & 0xFF)
				       + (((*(p_buf+1)) << 8)&0xFF00);
				writew(temp, USB_FIFO(ep_num));
				p_buf += 2;

				temp = ((*p_buf)&0xFF);
				writeb(temp, USB_FIFO(ep_num));
				p_buf += 1;
				residue -= 3;
				break;
			default:
				temp = ((*p_buf) & 0xFF)
				       + (((*(p_buf + 1)) <<  8)&0xFF00)
				       + (((*(p_buf + 2)) << 16)&0xFF0000)
				       + (((*(p_buf + 3)) << 24)&0xFF000000);
				writel(temp, USB_FIFO(ep_num));
				p_buf += 4;
				residue -= 4;
				break;
		};
	}

	return length;
}

/*
 * mu3d_hal_check_clk_sts - check sys125,u3 mac,u2 mac clock status
 */
int mu3d_hal_check_clk_sts(void)
{
	int ret;

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_SYS125_RST_B_STS, SSUSB_SYS125_RST_B_STS, 1, 10);
	if (ret == RET_FAIL) {
		DBG_I("SSUSB_SYS125_RST_B_STS NG\n");
		goto CHECK_ERROR;
	} else {
		DBG_I("clk sys125:OK\n");
	}

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 1, 10);
	if (ret == RET_FAIL) {
		DBG_I("SSUSB_U3_MAC_RST_B_STS NG\n");
		goto CHECK_ERROR;
	} else {
		DBG_I("clk mac3:OK\n");
	}

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS2, SSUSB_U2_MAC_SYS_RST_B_STS, SSUSB_U2_MAC_SYS_RST_B_STS, 1, 10);
	if (ret == RET_FAIL) {
		DBG_I("SSUSB_U2_MAC_SYS_RST_B_STS NG\n");
		goto CHECK_ERROR;
	} else {
		DBG_I("clk mac2:OK\n");
	}

	return RET_SUCCESS;

CHECK_ERROR:
	DBG_I("Reference clock stability check failed!\n");
	return RET_FAIL;
}

/*
 * mu3d_hal_ssusb_en - disable ssusb power down & enable u2/u3 ports
 */
void mu3d_hal_ssusb_en(void)
{
	clrbits(U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
	clrbits(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
#if defined(SUPPORT_U3) || defined(CFG_FPGA_PLATFORM)
	clrbits(U3D_SSUSB_U3_CTRL_0P, (SSUSB_U3_PORT_DIS | SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_HOST_SEL));
#endif
	clrbits(U3D_SSUSB_U2_CTRL_0P, (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_HOST_SEL));

	setbits(U3D_SSUSB_REF_CK_CTRL, (SSUSB_REF_MAC_CK_GATE_EN | SSUSB_REF_PHY_CK_GATE_EN | SSUSB_REF_CK_GATE_EN | SSUSB_REF_MAC3_CK_GATE_EN));

	/* check U3D sys125,u3 mac,u2 mac clock status. */
	mu3d_hal_check_clk_sts();
}

/*
 * mu3d_hal_u2dev_connect - u2 device softconnect
 */
void mu3d_hal_u2dev_connect(void)
{
	setbits(U3D_POWER_MANAGEMENT, SOFT_CONN);
}

void mu3d_hal_u2dev_disconnect(void)
{
	clrbits(U3D_POWER_MANAGEMENT, SOFT_CONN);
}

void mu3d_hal_u3dev_connect(void)
{
#ifdef SUPPORT_U3
	setbits(U3D_USB3_CONFIG, USB3_EN);
	mdelay(40);
#endif
}

void mu3d_hal_u3dev_disconnect(void)
{
#ifdef SUPPORT_U3
	clrbits(U3D_USB3_CONFIG, USB3_EN);
#endif
}

/*
 * mu3d_hal_set_speed - enable ss or connect to hs/fs
 * @args - arg1: speed
 */
void mu3d_hal_set_speed(USB_SPEED speed)
{
	/* clear ltssm state */
#ifdef DBG_USB_GENERAL
	DBG_I("%s\n", __func__);
#endif
	switch (speed) {
		case SSUSB_SPEED_FULL:
			clrbits(U3D_POWER_MANAGEMENT, HS_ENABLE);
			break;
		case SSUSB_SPEED_HIGH:
			enable_highspeed(); /* usbtty.c */
			setbits(U3D_POWER_MANAGEMENT, HS_ENABLE);
			break;
#if defined(SUPPORT_U3)
		case SSUSB_SPEED_SUPER:
			enable_superspeed(); /* usbtty.c */
			setbits(U3D_POWER_MANAGEMENT, HS_ENABLE);
			g_enable_u3 = true;
			break;
#endif
		default:
			DBG_C("Unsupported speed %d!!\n", speed);
			/*
			 * work around: Disconnect U3 will stop here,
			 * we need to call mt_udc_suspend since there
			 * is no SUSPEND interrupt when U3 was connected before.
			 */
			mt_udc_suspend();
			break;
	};
}

void mu3d_hal_rst_dev(void)
{
	int ret;

	writel(SSUSB_DEV_SW_RST, U3D_SSUSB_DEV_RST_CTRL);
	writel(0, U3D_SSUSB_DEV_RST_CTRL);

	/* do not check when SSUSB_U2_PORT_DIS = 1, because U2 port stays in reset state */
	if (!(readl(U3D_SSUSB_U2_CTRL_0P) & SSUSB_U2_PORT_DIS)) {
		ret = wait_for_value(U3D_SSUSB_IP_PW_STS2, SSUSB_U2_MAC_SYS_RST_B_STS, SSUSB_U2_MAC_SYS_RST_B_STS, 1, 10);
		if (ret == RET_FAIL)
			DBG_C("[ERR]: SSUSB_U2_MAC_SYS_RST_B_STS NG\n");
	}

#ifdef SUPPORT_U3
	/* do not check when SSUSB_U3_PORT_PDN = 1, because U3 port stays in reset state */
	if (!(readl(U3D_SSUSB_U3_CTRL_0P) & SSUSB_U3_PORT_PDN)) {
		ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 1, 10);
		if (ret == RET_FAIL)
			DBG_C("[ERR]: SSUSB_U3_MAC_RST_B_STS NG\n");
	}
#endif

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_QMU_RST_B_STS, SSUSB_DEV_QMU_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
		DBG_C("[ERR]: %d SSUSB_DEV_QMU_RST_B_STS NG\n", __LINE__);

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_BMU_RST_B_STS, SSUSB_DEV_BMU_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
		DBG_C("[ERR]: %d SSUSB_DEV_BMU_RST_B_STS NG\n", __LINE__);

	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_RST_B_STS, SSUSB_DEV_RST_B_STS, 1, 10);
	if (ret == RET_FAIL)
		DBG_C("[ERR]: %d SSUSB_DEV_RST_B_STS NG\n", __LINE__);

	mdelay(50);
}

/*
 * mu3d_hal_system_intr_en - enable system global interrupt
 */
void mu3d_hal_system_intr_en(void)
{
	u32 int_en;
	u32 ltssm_int_en;

	writel(readl(U3D_EPIER), U3D_EPIECR);
	writel(readl(U3D_DMAIER), U3D_DMAIECR);

	/* clear and enable common USB interrupts */
	writel(0, U3D_COMMON_USB_INTR_ENABLE);
	writel(readl(U3D_COMMON_USB_INTR), U3D_COMMON_USB_INTR);
	int_en = SUSPEND_INTR_EN | RESUME_INTR_EN | RESET_INTR_EN | CONN_INTR_EN |
	         DISCONN_INTR_EN  | VBUSERR_INTR_EN | LPM_INTR_EN | LPM_RESUME_INTR_EN;
	writel(int_en, U3D_COMMON_USB_INTR_ENABLE);

#ifdef SUPPORT_U3
	/* clear and enable LTSSM interrupts */
	writel(0, U3D_LTSSM_INTR_ENABLE);
	writel(readl(U3D_LTSSM_INTR), U3D_LTSSM_INTR);
	ltssm_int_en = SS_INACTIVE_INTR_EN | SS_DISABLE_INTR_EN | COMPLIANCE_INTR_EN |
	               LOOPBACK_INTR_EN  | HOT_RST_INTR_EN | WARM_RST_INTR_EN | RECOVERY_INTR_EN |
	               ENTER_U0_INTR_EN | ENTER_U1_INTR_EN | ENTER_U2_INTR_EN | ENTER_U3_INTR_EN |
	               EXIT_U1_INTR_EN | EXIT_U2_INTR_EN | EXIT_U3_INTR_EN | RXDET_SUCCESS_INTR_EN |
	               VBUS_RISE_INTR_EN | VBUS_FALL_INTR_EN | U3_LFPS_TMOUT_INTR_EN |
	               U3_RESUME_INTR_EN;
	writel(ltssm_int_en, U3D_LTSSM_INTR_ENABLE);
#endif

	writel(SSUSB_DEV_SPEED_CHG_INTR_EN, U3D_DEV_LINK_INTR_ENABLE);
}

void mu3d_hal_disable_intr(void)
{
	writel(0xffffffff, U3D_EPISR);
	writel(0xffffffff, U3D_DMAISR);
	writel(0xffffffff, U3D_QISAR0);
	writel(0xffffffff, U3D_QISAR1);
	writel(0xffffffff, U3D_QEMIR);
	writel(0xffffffff, U3D_TQERRIR0);
	writel(0xffffffff, U3D_RQERRIR0);
	writel(0xffffffff, U3D_RQERRIR1);
	writel(0xffffffff, U3D_LV1IECR);
	writel(0xffffffff, U3D_EPIECR);
	writel(0xffffffff, U3D_DMAIECR);

	/* clear registers */
	writel(0xffffffff, U3D_QIECR0);
	writel(0xffffffff, U3D_QIECR1);
	writel(0xffffffff, U3D_QEMIECR);
	writel(0xffffffff, U3D_TQERRIECR0);
	writel(0xffffffff, U3D_RQERRIECR0);
	writel(0xffffffff, U3D_RQERRIECR1);
}

/* usb generic functions */
static int mt_read_fifo(struct udc_endpoint *endpoint)
{

	struct urb *urb = endpoint->rcv_urb;
	int len = 0, count = 0;
	int ep_num = endpoint->num;
	unsigned char *cp;

	if (ep_num == EP0)
		urb = ep0_urb;

	if (urb) {
		cp = (u8 *) (urb->buf + urb->actual_length);

#if DBG_USB_FIFO
		DBG_I("%s: ep_num: %d, urb: %p, urb->buf: %p, urb->actual_length = %d\n",
		      __func__, ep_num, urb, urb->buf, urb->actual_length);
#endif

		count = len = mu3d_hal_pio_read_fifo(ep_num, cp);

		if (ep_num != 0) {
#if DBG_USB_FIFO
			DBG_I("%s: ep_num: %d count = %d\n",
			      __func__, ep_num, count);
#endif
		}

#if DBG_USB_DUMP_DATA
		if (ep_num != 0) {
			DBG_I("%s: &urb->buf: %p\n", __func__, urb->buf);
			DBG_I("dump data:\n");
			hexdump8(urb->buf, count);
		}
#endif

		urb->actual_length += count;
	}

	return count;
}

/* Linux txstate() ?? */
static int mt_write_fifo(struct udc_endpoint *ept)
{
	int ep_num = ept->num;
	struct urb *urb = ept->tx_urb;
	int last = 0, count = 0;
	unsigned char *buf = NULL;
	u32 wrote = 0;

	if (ep_num == EP0)
		urb = ep0_urb;

	if (urb) {
#if DBG_USB_DUMP_DESC
		DBG_I("%s: dump desc\n", __func__);
		hexdump8(urb->buf, urb->actual_length);
#endif

#if DBG_USB_FIFO
		DBG_I("%s: ep_num: %d urb: %p, actual_length: %d\n",
		      __func__, ep_num, urb, urb->actual_length);
		DBG_I("%s: sent: %d, tx_pkt_size: %d\n", __func__, ept->sent, ept->maxpkt);
#endif

		count = last = MIN (urb->actual_length - ept->sent,  ept->maxpkt);

#if DBG_USB_FIFO
		DBG_I ("[%s] urb->actual_length = %d, ept->sent = %d\n",
		       __func__, urb->actual_length, ept->sent);
#endif

		do {
			buf = urb->buf + ept->sent;

			/* do not use "urb->ept->tx_pktsz */
			wrote = mu3d_hal_pio_write_fifo(ep_num, count, buf, ept->maxpkt);
			count -= wrote;

		} while (count > 0);

		ept->last = last;
		ept->sent += last;
	}

	return last;
}

struct udc_endpoint *mt_find_ep(int ep_num, u8 dir)
{
	int i;
	u8 in;

	/* convert dir to in */
	if (dir == USB_DIR_IN) /* dir == USB_DIR_IN */
		in = 1;
	else
		in = 0;

	for (i = 0; i < MT_EP_NUM; i++) {
		if ((ep_list[i].num == ep_num) && (ep_list[i].in == in)) {
#if DBG_USB_GENERAL
			DBG_I("%s: find ep!\n", __func__);
#endif
			return &ep_list[i];
		}
	}
	return NULL;
}

static void mt_udc_flush_fifo(u8 ep_num, u8 dir)
{
	struct udc_endpoint *endpoint;

	if (ep_num == 0) {
		setbits(U3D_EP_RST, EP0_RST);
		clrbits(U3D_EP_RST, EP0_RST);
	} else {
		endpoint = mt_find_ep(ep_num, dir);
		if (endpoint->in == 0) { /* USB_DIR_OUT: val is 0x0; in == 0 here */
			setbits(U3D_EP_RST, (1 << ep_num));//reset RX EP
			clrbits(U3D_EP_RST, (1 << ep_num));//reset reset RX EP
		} else {
			setbits(U3D_EP_RST, (BIT16 << ep_num));//reset TX EP
			clrbits(U3D_EP_RST, (BIT16 << ep_num));//reset reset TX EP
		}
	}
}

static void mt_udc_flush_ep0_fifo(void)
{
	mt_udc_flush_fifo(0, 0);
}

/* is_tx_ep_busy: used by usbtty.c */
int mt_ep_busy(struct udc_endpoint *endpoint)
{
	int ep_num = endpoint->num;
	u32 csr = 0;

	if (endpoint->in == 0) { /* USB_DIR_OUT: val is 0x0; in == 0 here */
		DBG_I("mt_ep_busy: ep%d is RX endpoint\n", ep_num);
	} else {
		csr = USB_READCSR32(U3D_TX1CSR0, ep_num);
	}

	return (csr & TX_TXPKTRDY);
}

static void udc_clean_sentstall(unsigned int ep_num, u8 dir)
{
	if (ep_num == 0) {
		ep0csr_setbits(EP0_SENDSTALL);
	} else {
		if (dir == USB_DIR_OUT) {
			USB_WRITECSR32(U3D_RX1CSR0, ep_num, USB_READCSR32(U3D_RX1CSR0, ep_num) | RX_SENTSTALL);
			USB_WRITECSR32(U3D_RX1CSR0, ep_num, USB_READCSR32(U3D_RX1CSR0, ep_num) &~ RX_SENDSTALL);
		} else {
			USB_WRITECSR32(U3D_TX1CSR0, ep_num, USB_READCSR32(U3D_TX1CSR0, ep_num) | TX_SENTSTALL);
			USB_WRITECSR32(U3D_TX1CSR0, ep_num, USB_READCSR32(U3D_TX1CSR0, ep_num) &~ TX_SENDSTALL);
		}
	}
}

/* the endpoint does not support the received command, stall it!! */
static void udc_stall_ep(unsigned int ep_num, u8 dir)
{
	struct udc_endpoint *endpoint = mt_find_ep(ep_num, dir);
	u32 csr;

	DBG_C("%s\n", __func__);

	if (ep_num == 0) {
		csr = 0;
		mt_udc_flush_ep0_fifo();
		ep0csr_setbits(EP0_SENDSTALL);

		//TODO: check whether we have to wait for SENTSTALL here
		while (!(readl(U3D_EP0CSR) & EP0_SENTSTALL));

		udc_clean_sentstall(0, USB_DIR_OUT);
	} else {
		if (endpoint->in == 0) { /* USB_DIR_OUT: val is 0x0; in == 0 here */
			csr = USB_READCSR32(U3D_RX1CSR0, ep_num);

			csr &= RX_W1C_BITS;
			csr |= RX_SENDSTALL;
			USB_WRITECSR32(U3D_RX1CSR0, ep_num, csr);
			//TODO: do we have to wait for SENTSTALL here?
			while (!(USB_READCSR32(U3D_RX1CSR0, ep_num) & RX_SENTSTALL));

			udc_clean_sentstall(ep_num, USB_DIR_OUT);

			mt_udc_flush_fifo(ep_num, USB_DIR_OUT);
		} else {
			csr = USB_READCSR32(U3D_TX1CSR0, ep_num);

			csr &= TX_W1C_BITS;
			csr |= TX_SENDSTALL;
			USB_WRITECSR32(U3D_TX1CSR0, ep_num, csr);
			//TODO: do we have to wait for SENTSTALL here?
			while (!(USB_READCSR32(U3D_TX1CSR0, ep_num) & TX_SENTSTALL));

			udc_clean_sentstall(ep_num, USB_DIR_IN);

			mt_udc_flush_fifo(ep_num, USB_DIR_IN);
		}
	}

	ep0_state = EP0_IDLE;

	return;
}


/*
 * mu3d_hal_dft_reg() - apply default register settings
 * no use? deleted
 */

/* from mt_usbtty.c - start */
void enable_highspeed(void)
{
	int i;

	g_enable_u3 = 0;
	g_speed = SSUSB_SPEED_HIGH;

	for (i = 1; i < MT_EP_NUM; i++) {
		if (ep_list[i].num != 0) { /* allocated */
			ep_list[i].maxpkt = 512;
			mt_setup_ep(ep_list[i].num, &ep_list[i]);
		}
	}
}

void enable_superspeed(void)
{
	int i;

	g_enable_u3 = true;

	g_speed = SSUSB_SPEED_SUPER;

	for (i = 1; i < MT_EP_NUM; i++) {
		if (ep_list[i].num != 0) { /* allocated */
			ep_list[i].maxpkt = 1024;
			mt_setup_ep(ep_list[i].num, &ep_list[i]);
		}
	}
}

/* from mt_usbtty.c - end */

USB_SPEED mt_udc_get_speed(void)
{
	u32 speed = readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED;
	switch (speed) {
		case 1:
			DBG_I("FS is detected\n");
			return SSUSB_SPEED_FULL;
			break;
		case 3:
			DBG_I("HS is detected\n");
			return SSUSB_SPEED_HIGH;
			break;
		case 4:
			DBG_I("SS is detected\n");
			return SSUSB_SPEED_SUPER;
			break;
		default:
			DBG_I("Unrecognized Speed %d\n", speed);
			break;
	};

	return SSUSB_SPEED_UNKNOWN;
}

/*
 * u3d_ep0en - enable ep0 function
 */
void u3d_ep0en(void)
{
	u32 temp = 0;
	struct udc_endpoint *ep0 = &ep_list[EP0];

	if (mt_udc_get_speed() == SSUSB_SPEED_SUPER) {
		ep0->maxpkt = EP0_MAX_PACKET_SIZE_U3;
	} else {
		ep0->maxpkt = EP0_MAX_PACKET_SIZE;
	}

	/* EP0CSR */
	temp = readl(U3D_EP0CSR);
	temp = ((temp & ~0x3ff) | (ep0->maxpkt & 0x3ff));

	//No DMA
	//temp |= EP0_DMAREQEN;
	writel(temp, U3D_EP0CSR);

	/* enable EP0 interrupts */
	setbits(U3D_EPIESR, (EP0ISR | SETUPENDISR));
}

void u3d_irq_en()
{
	writel(0xFFFFFFFF, U3D_LV1IESR);
}

void mu3d_initialize_drv(void)
{
	/* enable LV1 ISR */
	u3d_irq_en();

	writel(readl(U3D_POWER_MANAGEMENT) & ~LPM_MODE, U3D_POWER_MANAGEMENT);
	writel(readl(U3D_POWER_MANAGEMENT) | (LPM_MODE&0x1), U3D_POWER_MANAGEMENT);

#ifdef EXT_VBUS_DET
	/* force VBUS on */
	writel(0x3, U3D_MISC_CTRL);
#else
	writel(0x0, U3D_MISC_CTRL);
#endif

	/* enable common USB ISR and L3 LTSSM ISR */
	mu3d_hal_system_intr_en();

	u3d_ep0en();
}

/* reset USB hardware */
void mt_udc_reset(USB_SPEED speed)
{
	u32 dwtmp1 = 0, dwtmp2 = 0, dwtmp3 = 0;

	dwtmp1 = readl(U3D_SSUSB_PRB_CTRL1);
	dwtmp2 = readl(U3D_SSUSB_PRB_CTRL2);
	dwtmp3 = readl(U3D_SSUSB_PRB_CTRL3);

	mu3d_hal_rst_dev();

	writel(dwtmp1, U3D_SSUSB_PRB_CTRL1);
	writel(dwtmp2, U3D_SSUSB_PRB_CTRL2);
	writel(dwtmp3, U3D_SSUSB_PRB_CTRL3);

	mdelay(50);

	mu3d_hal_ssusb_en();
	mu3d_hal_set_speed(speed);

	//TODO: Any more global variable needs to be reset here?
	dev_address = 0;
	g_tx_fifo_addr = USB_TX_FIFO_START_ADDRESS;
	g_rx_fifo_addr = USB_RX_FIFO_START_ADDRESS;

	mu3d_initialize_drv();
}

static void mt_udc_ep0_write(void)
{
	struct udc_endpoint *ept = &ep_list[EP0];
	unsigned int count = 0;
	u32 csr0;

	csr0 = readl(U3D_EP0CSR);
	if (csr0 & EP0_TXPKTRDY) {
		DBG_I("mt_udc_ep0_write: ep0 is not ready to be written\n");
		return;
	}

	count = mt_write_fifo(ept);

#if DBG_USB_GENERAL
	DBG_I("%s: count = %d\n", __func__, count);
#endif

	/* hardware limitiation: can't set (EP0_TXPKTRDY | EP0_DATAEND) at same time */
	csr0 |= (EP0_TXPKTRDY);
	writel(csr0, U3D_EP0CSR);

	if (count <= ept->maxpkt) {
		/* last packet */
		ep0_urb->actual_length = 0;
		ept->sent = 0;
	} else {
		DBG_C(" %s wrote %d bytes n there's more, maxp is %d",
		      __func__, count, ept->maxpkt);
	}
}

static void mt_udc_ep0_read(void)
{

	struct udc_endpoint *ept = &ep_list[EP0];
	unsigned int count = 0;
	u32 csr0 = 0;

	csr0 = readl(U3D_EP0CSR);

	/* erroneous ep0 interrupt */
	if (!(csr0 & EP0_RXPKTRDY)) {
		return;
	}

	count = mt_read_fifo(ept);

	/* work around: cannot set  (EP0_RXPKTRDY | EP0_DATAEND) at same time */
	csr0 |= (EP0_RXPKTRDY);
	writel(csr0, U3D_EP0CSR);

	if (count <= ept->maxpkt) {
		/* last packet */
		csr0 |= EP0_DATAEND;
		ep0_state = EP0_IDLE;
	} else {
		/* more packets are waiting to be transferred */
		DBG_C(" %s wrote %d bytes n there's more, maxp is %d",
		      __func__, count, ept->maxpkt);
		csr0 |= EP0_RXPKTRDY;
	}

	udelay(100);
	writel(csr0, U3D_EP0CSR);
}

static void u3d_set_address(int addr)
{
	writel((addr << DEV_ADDR_OFST), U3D_DEVICE_CONF);
}

static int ep0_standard_setup(struct urb *urb)
{
	struct setup_packet *request;
	struct udc_descriptor *desc;
	u8 *cp = urb->buf;

	/* for CLEAR FEATURE */
	u8 ep_num;  /* ep number */
	u8 dir; /* DIR */
	u32 csr;

	request = &urb->device_request;

	dump_setup_packet("Device Request\n", request);

	if ((request->type & USB_TYPE_MASK) != 0) {
		return false;           /* Class-specific requests are handled elsewhere */
	}

	/* handle all requests that return data (direction bit set on bm RequestType) */
	if ((request->type & USB_EP_DIR_MASK)) {
		/* send the descriptor */
		ep0_state = EP0_TX;

		switch (request->request) {
			/* data stage: from device to host */
			case GET_STATUS:
#if DBG_USB_GENERAL
				DBG_I("GET_STATUS\n");
#endif
				urb->actual_length = 2;
				cp[0] = cp[1] = 0;

				switch (request->type & USB_RECIP_MASK) {
					case USB_RECIP_DEVICE:
						cp[0] = USB_STAT_SELFPOWERED;
						break;
					case USB_RECIP_OTHER:
						urb->actual_length = 0;
						break;
					default:
						break;
				}

				return 0;

			case GET_DESCRIPTOR:
#if DBG_USB_GENERAL
				DBG_I("GET_DESCRIPTOR\n");
#endif
				/* usb_highspeed? */

#ifdef SUPPORT_U3
				if (g_enable_u3 == true) {
					desc = desc_list_u3;
#ifdef DBG_USB_GENERAL
					DBG_I("g_enable_u3 == true, g_speed: %d\n", g_speed);
#endif
				} else {
					desc = desc_list;
#ifdef DBG_USB_GENERAL
					DBG_I("g_enable_u3 != true, g_speed: %d\n", g_speed);
#endif
				}

				/* fix build warning here: using "desc = desc" */
				for (desc = desc; desc; desc = desc->next) {
#else
				for (desc = desc_list; desc; desc = desc->next) {
#endif

#if DBG_USB_DUMP_DESC
					DBG_I("desc->tag: %x: request->value: %x\n", desc->tag, request->value);
#endif
					if (desc->tag == request->value) {

#if DBG_USB_DUMP_DESC
						DBG_I("Find packet!\n");
#endif
						unsigned len = desc->len;
						if (len > request->length)
							len = request->length;

#if DBG_USB_GENERAL
						DBG_I("%s: urb: %p, cp: %p\n", __func__, urb, cp);
#endif
						copy_desc(urb, desc->data, len);

						return 0;
					}
				}
				/* descriptor lookup failed */
				return false;

			case GET_CONFIGURATION:
#if DBG_USB_GENERAL
				DBG_I("GET_CONFIGURATION\n");
#endif
				urb->actual_length = 1;
				cp[0] = 1;

				return 0;

			case GET_INTERFACE:
#if DBG_USB_GENERAL
				DBG_I("GET_INTERFACE\n");
#endif
			default:
				DBG_C("Unsupported command with TX data stage\n");
				break;
		}
	}
	else {

		switch (request->request) {

			case SET_ADDRESS:
#if DBG_USB_GENERAL
				DBG_I("SET_ADDRESS\n");
#endif

				dev_address = (request->value);
				set_address = 1;

				u3d_set_address(dev_address);

				return 0;

			case SET_CONFIGURATION:
#if DBG_USB_GENERAL
				DBG_I("SET_CONFIGURATION\n");
#endif
				if (request->value == 1) {
					usb_config_value = 1;

					/* work around: restore epx status after BUS RESET?? */
					mu3d_hal_set_speed(g_speed);
					udelay(50);

					/* restore QMU */
#ifdef SUPPORT_U3
#ifdef SUPPORT_QMU
					usb_initialize_qmu();

					mu3d_hal_start_qmu(1, USB_DIR_OUT);
					mu3d_hal_start_qmu(1, USB_DIR_IN);

					udelay(50);
#endif
#endif
					the_gadget->notify(the_gadget, UDC_EVENT_ONLINE);
				} else {
					usb_config_value = 0;
					the_gadget->notify(the_gadget, UDC_EVENT_OFFLINE);
				}

				usb_online = request->value ? 1 : 0;
#ifdef DBG_USB_GENERAL
				DBG_I("usb_online: %d\n", usb_online);
#endif

				return 0;
#ifdef SUPPORT_U3
			case SET_SEL:
				ep0_state = EP0_RX;
				return 0;

			case SET_FEATURE:
				if (request->value == USB3_U1_ENABLE)
#if DBG_USB_GENERAL
					DBG_I("USB3_U1_ENABLE\n");
#endif
				if (request->value == USB3_U2_ENABLE)
#if DBG_USB_GENERAL
					DBG_I("USB3_U2_ENABLE\n");
#endif
				return 0;
#endif

			case CLEAR_FEATURE:
#if DBG_USB_GENERAL
				DBG_I("CLEAR_FEATURE\n");
#endif
				ep_num = request->index & 0xf;
				dir = request->index & 0x80;

				if ((request->value == 0) && (request->length == 0)) {
#if DBG_USB_GENERAL
					DBG_I("Clear Feature: ep: %d dir: %d\n", ep_num, dir);
#endif
					switch (dir) {
						case USB_DIR_OUT:
							csr = USB_READCSR32(U3D_TX1CSR0, ep_num) & TX_W1C_BITS;
							csr = (csr & (~TX_SENDSTALL)) | TX_SENTSTALL;
							USB_WRITECSR32(U3D_TX1CSR0, ep_num, csr);
#if DBG_USB_GENERAL
							DBG_I("clear tx stall ep: %d dir: %d\n", ep_num, dir);
#endif
							setbits(U3D_EP_RST, (BIT16 << ep_num));//reset TX EP
							clrbits(U3D_EP_RST, (BIT16 << ep_num));//reset reset TX EP

							break;
						case USB_DIR_IN:
							csr = USB_READCSR32(U3D_RX1CSR0, ep_num) & RX_W1C_BITS;
							csr = (csr & (~RX_SENDSTALL)) | RX_SENTSTALL;
							USB_WRITECSR32(U3D_RX1CSR0, ep_num, csr);
#if DBG_USB_GENERAL
							DBG_I("clear rx stall ep: %d dir: %d\n", ep_num, dir);
#endif
							setbits(U3D_EP_RST, (1 << ep_num));//reset RX EP
							clrbits(U3D_EP_RST, (1 << ep_num));//reset reset RX EP

							break;
						default:
							break;
					}
					return 0;
				}
			default:
#ifdef DBG_USB_DUMP_SETUP
				DBG_I("desc->tag: %x: request->request: %x, request->value: %x\n", desc->tag, request->request, request->value);
				DBG_C("Unsupported command with RX data stage\n");
#endif
				break;

		} /* switch request */
	}
	return FALSE;
}

static void mt_udc_ep0_setup(void) {
	struct udc_endpoint *endpoint = &ep_list[0];
	u8 stall = 0;
	u32 csr0;
	struct setup_packet *request;

#ifdef USB_DEBUG
	u16 count;
#endif

	/* Read control status register for endpiont 0 */
	csr0 = readl(U3D_EP0CSR);

	/* check whether RxPktRdy is set? */
	if (!(csr0 & EP0_SETUPPKTRDY))
		return;

	/* unload fifo */
	ep0_urb->actual_length = 0;

#ifndef USB_DEBUG
	mt_read_fifo(endpoint);
#else
	count = mt_read_fifo(endpoint);

#if DBG_USB_FIFO
	DBG_I("%s: mt_read_fifo count = %d\n", __func__, count);
#endif
#endif
	/* decode command */
	request = &ep0_urb->device_request;
	memcpy(request, ep0_urb->buf, sizeof(struct setup_packet));

	if (((request->type) & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
#if DBG_USB_GENERAL
		DBG_I("Standard Request\n");
#endif
		stall = ep0_standard_setup(ep0_urb);
		if (stall) {
			dump_setup_packet("STANDARD REQUEST NOT SUPPORTED\n", request);
		}
	} else if (((request->type) & USB_TYPE_MASK) == USB_TYPE_CLASS) {
#if DBG_USB_GENERAL
		DBG_I("Class-Specific Request\n");
#endif
		if (stall) {
			dump_setup_packet("CLASS REQUEST NOT SUPPORTED\n", request);
		}
	} else if (((request->type) & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
#if DBG_USB_GENERAL
		DBG_I("Vendor-Specific Request\n");
		/* do nothing now */
		DBG_I("ALL VENDOR-SPECIFIC REQUESTS ARE NOT SUPPORTED!!\n");
#endif
	}

	if (stall) {
		/* the received command is not supported */
		udc_stall_ep(0, USB_DIR_OUT);
		return;
	}

	/* handle EP0 state */
	csr0 = readl(U3D_EP0CSR);
	switch (ep0_state) {
		case EP0_TX:
			/* data stage: from device to host */
#if DBG_USB_GENERAL
			DBG_I("%s: EP0_TX\n", __func__);
#endif
			/* move out from udc_chg_ep0_state() */
			csr0 = readl(U3D_EP0CSR);
			csr0 |= (EP0_SETUPPKTRDY | EP0_DPHTX);
			writel(csr0, U3D_EP0CSR);

			mt_udc_ep0_write();

			break;
		case EP0_RX:
			/* data stage: from host to device */
#if DBG_USB_GENERAL
			DBG_I("%s: EP0_RX\n", __func__);
#endif
			/* move out from udc_chg_ep0_state(): no need? */
			csr0 = readl(U3D_EP0CSR);
			csr0 |= (EP0_SETUPPKTRDY);
			writel(csr0, U3D_EP0CSR);

			break;
		case EP0_IDLE:
			/* no data stage */
#if DBG_USB_GENERAL
			DBG_I("%s: EP0_IDLE\n", __func__);
#endif
			csr0 = readl(U3D_EP0CSR);
			csr0 |= (EP0_RXPKTRDY | EP0_DATAEND);
			writel(csr0, U3D_EP0CSR);

			break;
		default:
			break;
	}
}

static void mt_udc_ep0_handler(void) {

	u32 csr0;

	/*
	 * EP0 interrupt is generated when
	 * - EP0CSR.RxPktRdy bit is set after data packet has been received and stored into FIFO0
	 * - data packet in FIFO0 has been sent to host successfully
	 * - EP0CSR.SentStall bit is set after host receives STALL
	 * - EP0CSR.SetupEnd bit is set after receiving SETUP transaction in DATA/STATUS phase
	 */
	csr0 = readl(U3D_EP0CSR);

	if (csr0 & EP0_SENTSTALL) {
#if DBG_USB_GENERAL
		DBG_I("USB: [EP0] SENTSTALL\n");
#endif
		/* needs implementation for exception handling here */
		//ep0_state = EP0_IDLE;
		// writel((readl(U3D_EP0CSR) & EP0_W1C_BITS) | EP0_SENTSTALL, U3D_EP0CSR);
		//udc_chg_ep0_state(EP0_IDLE);
		ep0_state = EP0_IDLE;
		ep0csr_setbits(EP0_SENTSTALL);
	}

	switch (ep0_state) {
		case EP0_IDLE:
#if DBG_USB_GENERAL
			DBG_I("%s: EP0_IDLE\n", __func__);
#endif
			if (set_address) {
				u3d_set_address(dev_address);
				set_address = 0;
			}
			mt_udc_ep0_setup();
			break;
		case EP0_TX:
#if DBG_USB_GENERAL
			DBG_I("%s: EP0_TX\n", __func__);
#endif
			/* complete and sendout buffer */
			csr0 |= EP0_DATAEND;
			writel(csr0, U3D_EP0CSR);
			ep0_state = EP0_IDLE;

			break;
		case EP0_RX:
#if DBG_USB_GENERAL
			DBG_I("%s: EP0_RX\n", __func__);
#endif
			mt_udc_ep0_read();
			ep0_state = EP0_IDLE;
			break;
		default:
			DBG_I("[ERR]: Unrecognized ep0 state%d", ep0_state);
			break;
	}

	return;
}

static void mt_udc_epx_handler(u8 ep_num, u8 dir) {
	u32 csr;
	u32 count;
	struct udc_endpoint *ept;
	struct urb *urb;
	struct udc_request *req;    /* for event signaling */

	ept = mt_find_ep(ep_num, dir);

#if DBG_USB_GENERAL
	DBG_I("EP%d Interrupt\n", ep_num);
	DBG_I("dir: %x\n", dir);
#endif

	switch (dir) {
		case USB_DIR_OUT:
			/* transfer direction is from host to device */
			/* from the view of usb device, it's RX */
			csr = USB_READCSR32(U3D_RX1CSR0, ep_num);

			if (csr & RX_SENTSTALL) {
				DBG_C("EP %d(RX): STALL\n", ep_num);
				/* exception handling: implement this!! */
				return;
			}

#ifdef SUPPORT_QMU /* SUPPORT_QMU */
			count = ept->rcv_urb->actual_length;
#ifdef DBG_USB_QMU
			DBG_I("%s: QMU: count: %d\n", __func__, count);
#endif
#else /* PIO MODE */

			if (!(csr & RX_RXPKTRDY)) {
#if DBG_USB_GENERAL
				DBG_I("EP %d: ERRONEOUS INTERRUPT\n", ep_num); // normal
#endif
				return;
			}


			count = mt_read_fifo(ept);

#if DBG_USB_GENERAL
			DBG_I("EP%d(RX), count = %d\n", ep_num, count);
#endif

			/* write 1 to clear RXPKTRDY */
			csr |= RX_RXPKTRDY;
			USB_WRITECSR32(U3D_RX1CSR0, ep_num, csr);

			if (USB_READCSR32(U3D_RX1CSR0, ep_num) & RX_RXPKTRDY) {
#if DBG_USB_GENERAL
				DBG_I("%s: rxpktrdy clear failed\n", __func__);
#endif
			}
#endif /* ifndef SUPPORT_QMU */

			/* do signaling */
			req = ept->req;
			/* workaround: if req->lenth == 64 bytes (not huge data transmission)
			 * do normal return */
#if DBG_USB_GENERAL
			DBG_I("%s: req->length: %x, ept->rcv_urb->actual_length: %x\n",
			      __func__, req->length, ept->rcv_urb->actual_length);
#endif

			/* Deal with FASTBOOT command */
			if ((req->length >= ept->rcv_urb->actual_length) && req->length == 64) {
				req->length = count;

				/* mask EPx INTRRXE */
				/* The buffer is passed from the AP caller.
				 * It happens that AP is dealing with the buffer filled data by driver,
				 * but the driver is still receiving the next data packets onto the buffer.
				 * Data corrupted happens if the every request use the same buffer.
				 * Mask the EPx to ensure that AP and driver are not accessing the buffer parallely.
				 */
				/* set 1 to disable (clear) enable mask */
				writel((BIT16 << ep_num), U3D_EPIECR);
			}

			/* Deal with DATA transfer */
			if ((req->length == ept->rcv_urb->actual_length) ||
			        ((req->length >= ept->rcv_urb->actual_length) && req->length == 64)) {
				handle_ept_complete(ept);

				/* mask EPx INTRRXE */
				/* The buffer is passed from the AP caller.
				 * It happens that AP is dealing with the buffer filled data by driver,
				 * but the driver is still receiving the next data packets onto the buffer.
				 * Data corrupted happens if the every request use the same buffer.
				 * Mask the EPx to ensure that AP and driver are not accessing the buffer parallely.
				 */
				/* set 1 to disable (clear) enable mask */
				writel((BIT16 << ep_num), U3D_EPIECR);
			}
			break;
		case USB_DIR_IN:
			/* transfer direction is from device to host */
			/* from the view of usb device, it's tx */
			csr = USB_READCSR32(U3D_TX1CSR0, ep_num);

			if (csr & TX_SENTSTALL) {
				DBG_C("EP %d(TX): STALL\n", ep_num);
				ept->status = -1;
				handle_ept_complete(ept);
				/* exception handling: implement this!! */
				return;
			}

#ifndef SUPPORT_QMU
			if (csr & TX_TXPKTRDY) {
				DBG_C
				("%s: ep%d is not ready to be written\n",
				 __func__, ep_num);
				return;
			}
#endif
			urb = ept->tx_urb;

			if (ept->sent == urb->actual_length) {
				/* do signaling */
				handle_ept_complete(ept);
				break;
			}

			/* send next packet of the same urb */
#ifndef SUPPORT_QMU
			count = mt_write_fifo(ept);
#if DBG_USB_GENERAL
			DBG_I("EP%d(TX), count = %d\n", ep_num, ept->sent);
#endif

			if (count != 0) {
				/* not the interrupt generated by the last tx packet of the transfer */
				csr |= TX_TXPKTRDY;
				USB_WRITECSR32(TX_TXPKTRDY, ep_num, csr);
			}
#endif
			break;
		default:
			break;
	}
}

#if defined(DBG_USB_LTSSM)
void report_ltssm_type(u32 ltssm) {
	if (ltssm & RXDET_SUCCESS_INTR) {
		DBG_I("RXDET_SUCCESS_INTR\n");
	}

	if (ltssm & HOT_RST_INTR) {
		DBG_I("HOT_RST_INTR\n");
	}

	if (ltssm & WARM_RST_INTR) {
		DBG_I("WARM_RST_INTR\n");
	}

	if (ltssm & ENTER_U0_INTR) {
		DBG_I("ENTER_U0_INTR\n");
	}

	if (ltssm & VBUS_RISE_INTR) {
		DBG_I("VBUS_RISE_INTR\n");
	}

	if (ltssm & VBUS_FALL_INTR) {
		DBG_I("VBUS_FALL_INTR\n");
	}

	if (ltssm & ENTER_U1_INTR) {
		DBG_I("ENTER_U1_INTR\n");
	}

	if (ltssm & ENTER_U2_INTR) {
		DBG_I("ENTER_U2_INTR\n");
	}

	if (ltssm & ENTER_U3_INTR) {
		DBG_I("ENTER_U3_INTR\n");
	}

	if (ltssm & EXIT_U1_INTR) {
		DBG_I("EXIT_U1_INTR\n");
	}

	if (ltssm & EXIT_U2_INTR) {
		DBG_I("EXIT_U2_INTR\n");
	}

	if (ltssm & EXIT_U3_INTR) {
		DBG_I("EXIT_U3_INTR\n");
	}
}
#endif

/*
 * get_seg_size()
 *
 * Return value indicates the TxFIFO size of 2^n bytes, (ex: value 10 means 2^10 =
 * 1024 bytes.) TXFIFOSEGSIZE should be equal or bigger than 4. The TxFIFO size of
 * 2^n bytes also should be equal or bigger than TXMAXPKTSZ. This EndPoint occupy
 * total memory size  (TX_SLOT + 1 )*2^TXFIFOSEGSIZE bytes.
 */
u8 get_seg_size(u32 max_packet_size) {
	/* Set fifo size(double buffering is currently not enabled) */
	switch (max_packet_size) {
		case 8:
		case 16:
			return USB_FIFOSZ_SIZE_16;
		case 32:
			return USB_FIFOSZ_SIZE_32;
		case 64:
			return USB_FIFOSZ_SIZE_64;
		case 128:
			return USB_FIFOSZ_SIZE_128;
		case 256:
			return USB_FIFOSZ_SIZE_256;
		case 512:
			return USB_FIFOSZ_SIZE_512;
		case 1023:
		case 1024:
		case 2048:
		case 3072:
		case 4096:
			return USB_FIFOSZ_SIZE_1024;
		default:
			DBG_I("The max_packet_size %d is not supported\n", max_packet_size);
			return USB_FIFOSZ_SIZE_512;
	}
}


/*
 * udc_setup_ep - setup endpoint
 *
 * Associate a physical endpoint with endpoint_instance and initialize FIFO
 */
void mt_setup_ep(unsigned int ep_num, struct udc_endpoint *endpoint) {
	u32 csr0, csr1, csr2;
	u32 max_packet_size;
	u8 seg_size;
	u8 max_pkt;

	u8 burst = endpoint->burst;
	u8 mult = endpoint->mult;
	u8 type = endpoint->type;
	u8 slot = endpoint->slot;

	/* EP table records in bits hence bit 1 is ep0 */
	/* Nothing needs to be done for ep0 */
	if (ep_num == 0) { // or (endpoint->type == USB_EP_XFER_CTRL)
		return;
	}

	/* Configure endpoint fifo */
	/* Set fifo address, fifo size, and fifo max packet size */
#if DBG_USB_GENERAL
	DBG_I("%s: endpoint->in: %d, maxpkt: %d\n",
	      __func__, endpoint->in, endpoint->maxpkt);
#endif
	if (endpoint->in == 0) { /* USB_DIR_OUT: val is 0x0; in == 0 here */
		/* RX case */
		mt_udc_flush_fifo(ep_num, USB_DIR_OUT);

		max_packet_size = endpoint->maxpkt;
		/* Set fifo size(double buffering is currently not enabled) */
		seg_size = get_seg_size(max_packet_size);

		/* CSR0 */
		csr0 = USB_READCSR32(U3D_RX1CSR0, ep_num) &~ RX_RXMAXPKTSZ;
		csr0 |= (max_packet_size & RX_RXMAXPKTSZ);

#ifndef SUPPORT_QMU /* PIO_MODE */
		csr0 &= ~RX_DMAREQEN;
#endif

		/* CSR1 */
		max_pkt = (burst + 1) * (mult + 1) - 1;
		csr1 = (burst & SS_RX_BURST);
		csr1 |= (slot << RX_SLOT_OFST) & RX_SLOT;
		csr1 |= (max_pkt << RX_MAX_PKT_OFST) & RX_MAX_PKT;
		csr1 |= (mult << RX_MULT_OFST) & RX_MULT;

		/* CSR2 */
		csr2 = (g_rx_fifo_addr >> 4) & RXFIFOADDR;
		csr2 |= (seg_size << RXFIFOSEGSIZE_OFST) & RXFIFOSEGSIZE;

		/* In LK (FASTBOOT) will use BULK transfer only */
		if (type == USB_EP_XFER_BULK) {
			csr1 |= TYPE_BULK;
		} else if (type == USB_EP_XFER_INT) {
			csr1 |= TYPE_INT;
			csr2 |= (endpoint->binterval << RXBINTERVAL_OFST) & RXBINTERVAL;
		}

		/* Write 1 to clear EPIER */
		setbits(U3D_EPIECR, (BIT16 << ep_num));
		USB_WRITECSR32(U3D_RX1CSR0, ep_num, csr0);
		USB_WRITECSR32(U3D_RX1CSR1, ep_num, csr1);
		USB_WRITECSR32(U3D_RX1CSR2, ep_num, csr2);

		/* Write 1 to set EPIER */
#ifndef SUPPORT_QMU /* For QMU, we don't enable EP interrupts. */
		writel(readl(U3D_EPIER) | (BIT16 << ep_num), U3D_EPIESR);
#endif

		if (max_packet_size == 1023) {
			g_rx_fifo_addr += (1024 * (slot + 1));
		} else {
			g_rx_fifo_addr += (max_packet_size * (slot + 1));
		}

		if (g_rx_fifo_addr > readl(U3D_CAP_EPNRXFFSZ)) {
			DBG_I("[ERR]g_rx_fifo_addr is %x and U3D_CAP_EPNTXFFSZ is %x for ep%d\n",
			      g_rx_fifo_addr, readl(U3D_CAP_EPNRXFFSZ), ep_num);
			DBG_I("max_packet_size = %d\n", max_packet_size);
			DBG_I("slot = %d\n", slot);
		}

	} else {    /* USB_DIR_IN: val is 0x80; in == 1 here */
		/* TX case */
		mt_udc_flush_fifo(ep_num, USB_DIR_IN);

		max_packet_size = endpoint->maxpkt;
		/* Set fifo size(double buffering is currently not enabled) */
		seg_size = get_seg_size(max_packet_size);

		/* CSR0 */
		csr0 = USB_READCSR32(U3D_TX1CSR0, ep_num) &~ TX_TXMAXPKTSZ;
		csr0 |= (max_packet_size & TX_TXMAXPKTSZ);

#ifndef SUPPORT_QMU /* PIO_MODE */
		csr0 &= ~TX_DMAREQEN;
#endif

		/* CSR1 */
		max_pkt = (burst + 1)*(mult + 1) - 1;
		csr1 = (burst & SS_TX_BURST);
		csr1 |= (slot << TX_SLOT_OFST) & TX_SLOT;
		csr1 |= (max_pkt << TX_MAX_PKT_OFST) & TX_MAX_PKT;
		csr1 |= (mult << TX_MULT_OFST) & TX_MULT;

		/* CSR2 */
		csr2 = (g_tx_fifo_addr >> 4) & TXFIFOADDR;
		csr2 |= (seg_size << TXFIFOSEGSIZE_OFST) & TXFIFOSEGSIZE;

		/* In LK (FASTBOOT) will use BULK transfer only */
		if (type == USB_EP_XFER_BULK) {
			csr1 |= TYPE_BULK;
		} else if (type == USB_EP_XFER_INT) {
			csr1 |= TYPE_INT;
			csr2 |= (endpoint->binterval << TXBINTERVAL_OFST) & TXBINTERVAL;
		}

		/* Write 1 to clear EPIER */
		setbits(U3D_EPIECR, (BIT0 << ep_num));
		USB_WRITECSR32(U3D_TX1CSR0, ep_num, csr0);
		USB_WRITECSR32(U3D_TX1CSR1, ep_num, csr1);
		USB_WRITECSR32(U3D_TX1CSR2, ep_num, csr2);

		/* Write 1 to set EPIER */
#ifndef SUPPORT_QMU /* For QMU, we don't enable EP interrupts. */
		writel(readl(U3D_EPIER) | (BIT0 << ep_num), U3D_EPIESR);
#endif

		if (max_packet_size == 1023) {
			g_tx_fifo_addr += (1024 * (slot + 1));
		} else {
			g_tx_fifo_addr += (max_packet_size * (slot + 1));
		}

		if (g_tx_fifo_addr > readl(U3D_CAP_EPNTXFFSZ)) {
			DBG_I("[ERR]g_tx_fifo_addr is %x and U3D_CAP_EPNTXFFSZ is %x for ep%d",
			      g_tx_fifo_addr, readl(U3D_CAP_EPNTXFFSZ), ep_num);
			DBG_I("max_packet_size = %d\n", max_packet_size);
			DBG_I("slot = %d]n", slot);
		}
	}
}

struct udc_endpoint *_udc_endpoint_alloc(unsigned char num, unsigned char in,
        unsigned short max_pkt) {
	int i;

	/*
	 * find an unused slot in ep_list from EP1 to MAX_EP
	 * for example, EP1 will use 2 slot one for IN and the other for OUT
	 */
	if (num != EP0) {
		for (i = 1; i < MT_EP_NUM; i++) {
			if (ep_list[i].num == 0) /* usable */
				break;
		}

		if (i == MT_EP_NUM) /* ep has been exhausted. */
			return NULL;

		/* EPx Type */
		ep_list[i].type = USB_EP_XFER_BULK;

		/* set urb */
		if (in) { /* usb EP1 tx */
			ep_list[i].tx_urb = tx_urb;
			tx_urb->ept = &ep_list[i];
		} else {    /* usb EP1 rx */
			ep_list[i].rcv_urb = rx_urb;
			rx_urb->ept = &ep_list[i];
		}
	} else {
		i = EP0;    /* EP0 */
	}

	ep_list[i].maxpkt = max_pkt;
	ep_list[i].num = num;
	ep_list[i].in = in;
	ep_list[i].req = NULL;

	/* store EPT_TX/RX info */
	if (ep_list[i].in) {
		ep_list[i].bit = EPT_TX(num);
	} else {
		ep_list[i].bit = EPT_RX(num);
	}

	/* write parameters to this ep (write to hardware) */
	mt_setup_ep(num, &ep_list[i]);

	DBG_I("ept%d %s @%p/%p max=%d bit=%x\n",
	      num, in ? "in" : "out", &ep_list[i], &ep_list, max_pkt, ep_list[i].bit);

	return &ep_list[i];
}

#define SETUP(type,request) (((type) << 8) | (request))

static unsigned long ept_alloc_table = EPT_TX(0) | EPT_RX(0);

struct udc_endpoint *udc_endpoint_alloc(unsigned type, unsigned maxpkt) {
	struct udc_endpoint *ept;
	unsigned n;
	unsigned in;

	if (type == UDC_TYPE_BULK_IN) {
		in = 1;
	} else if (type == UDC_TYPE_BULK_OUT) {
		in = 0;
	} else {
		return 0;
	}

	/* udc_endpoint_alloc is used for EPx except EP0 */
	for (n = 1; n < 16; n++) {
		unsigned long bit = in ? EPT_TX(n) : EPT_RX(n);
		if (ept_alloc_table & bit)
			continue;
		ept = _udc_endpoint_alloc(n, in, maxpkt);
		if (ept)
			ept_alloc_table |= bit;
		return ept;
	}

	return 0;
}

/* must use with request. If req == NULL should be error handling */
static void handle_ept_complete(struct udc_endpoint *ept) {
	unsigned int actual;
	int status;
	struct udc_request *req;

	req = ept->req;
	if (req) {
#if DBG_USB_GENERAL
		DBG_I("%s: req: %p: req->length: %d: status: %d\n", __func__, req, req->length, ept->status);
#endif
		/*
		 * This request releasement will cause udc_request_queue abnormal when INTR comes later.
		 * but will only affect QMU mode because signal didn't return correctly.
		 */
		ept->req = NULL;

		if (ept->status == -1) {
			actual = 0;
			status = -1;    /* txn_status */
			DBG_C("%s: EP%d/%s FAIL status: %x\n",
			      __func__, ept->num, ept->in ? "in" : "out", status);
		} else {
			actual = req->length;
			status = 0;
		}

		if (req->complete)
			req->complete(req, actual, status);
	}
}

void mt_udc_irq(u32 ltssm, u32 intrusb, u32 dmaintr, u16 intrtx, u16 intrrx,
                u32 intrqmu, u32 intrqmudone, u32 linkint) {
	u32 temp = 0;
	u32 ep_num = 0;

#if DBG_USB_IRQ
	DBG_I("mt_udc_irq\n");
	DBG_I("ltssm : %x\n", ltssm);
	DBG_I("intrusb : %x\n", intrusb);
	DBG_I("dmaintr : %x\n", dmaintr);
	DBG_I("intrtx : %x\n", intrtx);
	DBG_I("intrrx : %x\n", intrrx);
	DBG_I("intrqmu : %x\n", intrqmu);
	DBG_I("intrqmudone : %x\n", intrqmudone);
	DBG_I("linkint : %x\n", linkint);
#ifdef DBG_EP0ISR
	DBG_I("U3D_EP0CSR : %x\n", readl(U3D_EP0CSR));
	DBG_I("USB_EPIER : %x\n", readl(U3D_EPIER));
	DBG_I("USB_EPISR :%x\n", readl(U3D_EPISR));
	DBG_I("U3D_DEVICE_CONF: %x\n", readl(U3D_DEVICE_CONF));
	DBG_I("U3D_DEVICE_MONITOR: %x\n", readl(U3D_DEVICE_MONITOR));
#endif
#ifdef SBG_EP1ISR
	DBG_I("[CSR] U3D_RX1CSR0: %p: val: %x\n", (void *)U3D_RX1CSR0, USB_READCSR32(U3D_RX1CSR0, 1));
	DBG_I("[CSR] U3D_RX1CSR1: %p: val: %x\n", (void *)U3D_RX1CSR1, USB_READCSR32(U3D_RX1CSR1, 1));
	DBG_I("[CSR] U3D_RX1CSR2: %p: val: %x\n", (void *)U3D_RX1CSR2, USB_READCSR32(U3D_RX1CSR2, 1));
	DBG_I("[CSR] U3D_TX1CSR0: %p: val: %x\n", (void *)U3D_TX1CSR0, USB_READCSR32(U3D_TX1CSR0, 1));
	DBG_I("[CSR] U3D_TX1CSR1: %p: val: %x\n", (void *)U3D_TX1CSR1, USB_READCSR32(U3D_TX1CSR1, 1));
	DBG_I("[CSR] U3D_TX1CSR2: %p: val: %x\n", (void *)U3D_TX1CSR2, USB_READCSR32(U3D_TX1CSR2, 1));
#endif
#ifdef DBG_USB_QMU
	DBG_I("U3D_RXQCSR1: %p, val: %x\n", (void *)U3D_RXQCSR(1), readl(U3D_RXQCSR(1)));
	DBG_I("U3D_RXQSAR1: %p, val: %x\n", (void *)U3D_RXQSAR(1), readl(U3D_RXQSAR(1)));
	DBG_I("U3D_RXQCPR1: %p, val: %x\n", (void *)U3D_RXQCPR(1), readl(U3D_RXQCPR(1)));
	DBG_I("U3D_RXQLDPR1: %p, val: %x\n", (void *)U3D_RXQLDPR(1), readl(U3D_RXQLDPR(1)));

	DBG_I("U3D_TXQCSR1: %p, val: %x\n", (void *)U3D_TXQCSR(1), readl(U3D_TXQCSR(1)));
	DBG_I("U3D_TXQSAR1: %p, val: %x\n", (void *)U3D_TXQSAR(1), readl(U3D_TXQSAR(1)));
	DBG_I("U3D_TXQCPR1: %p, val: %x\n", (void *)U3D_TXQCPR(1), readl(U3D_TXQCPR(1)));
#endif /* ifdef DBG_USB_QMU */
#endif /* ifdef DBG_USB_IRQ */

#ifdef SUPPORT_QMU
	if (intrqmudone) {
		qmu_done_interrupt(intrqmudone);

		if (intrqmudone & QMU_RX_DONE(1)) {
			mt_udc_epx_handler(1, USB_DIR_OUT);
		}

		if (intrqmudone & QMU_TX_DONE(1)) {
			mt_udc_epx_handler(1, USB_DIR_IN);
		}
	}
	if (intrqmu) {
		qmu_handler(intrqmu);
	}
#endif
	if (linkint & SSUSB_DEV_SPEED_CHG_INTR) {
		DBG_I("[INTR] Speed Change\n");
		g_speed = mt_udc_get_speed();
		mu3d_hal_set_speed(g_speed);
	}

	/* Check for reset interrupt */
	if (intrusb & RESET_INTR) {
		DBG_I("[INTR] Reset\n");
		udelay(20);

		/* set device address to 0 after reset */
		set_address = 0;
		intrtx = 0;
		intrrx = 0;
	}

#ifdef SUPPORT_U3
	if (ltssm) {
		if (ltssm & SS_DISABLE_INTR) {
			DBG_I("[INTR] SS is Disabled, %d\n",
			      (readl(U3D_LTSSM_INFO) & DISABLE_CNT) >> DISABLE_CNT_OFST);

			/* Set soft_conn to enable U2 termination */
			mu3d_hal_u2dev_connect();
			u3d_ep0en(); /* u3d_ep0en will update g_speed. */
			mu3d_hal_set_speed(g_speed); /* do we really need set speed here ?? */

			ltssm = 0;
		}

#if defined(DBG_USB_LTSSM)
		report_ltssm_type(ltssm);
#endif

		if (ltssm & ENTER_U0_INTR) {
#if DBG_USB_IRQ
			DBG_I("ENTER_U0_INTR");
#endif
			mu3d_initialize_drv();
			g_tx_fifo_addr = USB_TX_FIFO_START_ADDRESS;
			g_rx_fifo_addr = USB_RX_FIFO_START_ADDRESS;
			set_address = 0;
			intrtx = 0;
			intrrx = 0;
		}
#ifndef POWER_SAVING_MODE
		if (ltssm & U3_RESUME_INTR) {

			DBG_I("[INTR] Resume \n");
			clrbits(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN);
			clrbits(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
			while (!(readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));
			setbits(U3D_LINK_POWER_CONTROL, UX_EXIT);
		}
#endif
	} // ltssm
#endif  // #if SUPPORT_U3

	if (intrusb & DISCONN_INTR) {
		DBG_I("[INTR] DISCONN_INTR\n");
	}

	if (intrusb & CONN_INTR) {
		DBG_I("[INTR] CONN_INTR\n");
	}

	if (intrusb & SUSPEND_INTR) {
		DBG_I("[INTR] SUSPEND_INTR\n");

		/* mt_udc_suspend will also call stop qmu */
		mt_udc_suspend();

		/*
		 * work around: Only disconnect U2 will stop here,
		 * Deal with 1. U2->U2 or U2->U3
		 * we need to "reset" or to "clean" SSUSB fail counnter
		 * to enable U3 for next enumeration.
		 *
		 * Otherwise U3 cannot detect speed.
		 */
#if defined(USB_RESET_AFTER_SUSPEND)
		mt_udc_reset(U3D_DFT_SPEED);
		mt_usb_disconnect_internal();
		/* soft connect U2 or U3 by g_speed */
		mt_usb_connect_internal();
#else
		mt_usb_disconnect_internal();
		mu3d_hal_u3dev_connect();
#endif
	}

	//TODO: Possibly don't need to handle this interrupt
	if (intrusb & LPM_INTR) {
		DBG_I("[INTR] LPM Interrupt\n");

		temp = readl(U3D_USB20_LPM_PARAM);
		DBG_I("[INTR] %x, BESL: %x, x <= %x <= %x\n",
		      temp&0xf, (temp >> 8) & 0xf, (temp >> 12) & 0xf, (temp >> 4) & 0xf);

		temp = readl(U3D_POWER_MANAGEMENT);
		DBG_I("[RWP]: %x\n", (temp & LPM_RWP) >> 11);

		//if (g_sw_rw)
		{
			// s/w LPM only
			setbits(U3D_USB20_MISC_CONTROL, LPM_U3_ACK_EN);

			//wait a while before remote wakeup, so xHCI PLS status is not affected
			mdelay(20);
			setbits(U3D_POWER_MANAGEMENT, RESUME);

			DBG_I("RESUME: %d\n", readl(U3D_POWER_MANAGEMENT) & RESUME);
		}
	}

	if (intrusb & LPM_RESUME_INTR) {
		DBG_I("[INTR] LPM Resume\n");

		if (!(readl(U3D_POWER_MANAGEMENT) & LPM_HRWE)) {
			setbits(U3D_USB20_MISC_CONTROL, LPM_U3_ACK_EN);
		}
	}

	/* Check for resume from suspend mode */
	if (intrusb & RESUME_INTR) {
		DBG_I("[INTR] Resume Interrupt\n");
	}

#ifdef SUPPORT_DMA
	if (dmaintr) {
		u3d_dma_handler(dmaintr);
	}
#endif

	/* For EP0 */
	if ((intrtx & 0x1) || (intrrx & 0x1)) {
		if (intrrx & 0x1) {
			DBG_I("Service SETUPEND");
			intrrx = intrrx & ~0x1; //SETUPENDISR of EP0
		}

		mt_udc_ep0_handler();
		intrtx = intrtx & ~0x1; //EPISR
		//intrrx = intrrx & ~0x1; //SETUPENDISR of EP0
		//ep0_state = EP0_IDLE;
	}

	/* For EPx (TX) */
	if (intrtx) {
		for (ep_num = 1; ep_num < MT_EP_NUM; ep_num++) {
			if (intrtx & EPMASK(ep_num)) {
				mt_udc_epx_handler(ep_num, USB_DIR_IN);
			}
		}
	}

	/* For EPx (RX) */
	if (intrrx) {
		for (ep_num = 1; ep_num < MT_EP_NUM; ep_num++) {
			if (intrrx & EPMASK(ep_num)) {
				mt_udc_epx_handler(ep_num, USB_DIR_OUT);
			}
		}
	}
}

/* from usbtty.c */
void service_interrupts(void) {
	u32 ltssm;
	u32 intrusb;
	u32 dmaintr;
	u16 intrtx;
	u16 intrrx;
	u32 intrqmu;
	u32 intrqmudone;
	u32 linkint;
	u32 intrep;
	u32 lv1_isr;

	/* give ltssm and intrusb initial value */
	ltssm = 0;
	intrusb = 0;
	intrqmu = 0;
	intrqmudone = 0;

	/* read */
	lv1_isr = readl(U3D_LV1ISR);

	if (lv1_isr & MAC2_INTR) {
		intrusb = readl(U3D_COMMON_USB_INTR) & readl(U3D_COMMON_USB_INTR_ENABLE);
	}

#ifdef SUPPORT_U3
	if (lv1_isr & MAC3_INTR) {
		ltssm = readl(U3D_LTSSM_INTR) & readl(U3D_LTSSM_INTR_ENABLE);
	}
#endif

	dmaintr = readl(U3D_DMAISR) & readl(U3D_DMAIER);
	intrep = readl(U3D_EPISR) & readl(U3D_EPIER);
#ifdef SUPPORT_QMU
	intrqmu = readl(U3D_QISAR1);
	intrqmudone = readl(U3D_QISAR0) & readl(U3D_QIER0);
#endif
	intrtx = intrep & 0xffff;
	intrrx = (intrep >> 16);
	linkint = readl(U3D_DEV_LINK_INTR) & readl(U3D_DEV_LINK_INTR_ENABLE);

#if DBG_USB_IRQ
	DBG_I("intrqmudone: %x\n",intrqmudone);
	DBG_I("intrep: %x\n",intrep);
	DBG_I("interrupt: intrusb [%x] intrtx[%x] intrrx [%x] intrdma[%x] intrqmu [%x] intrltssm [%x]\r\n"
	      , intrusb, intrtx, intrrx, dmaintr, intrqmu, ltssm);
#endif
	if (!(intrusb || intrep || dmaintr || intrqmu || ltssm || intrqmudone)) {
		DBG_I("[NULL INTR] REG_INTRL1 = 0x%08X\n",
		      (u32)lv1_isr);
	}

#ifdef SUPPORT_QMU
	writel(intrqmudone, U3D_QISAR0);
#endif

	if (lv1_isr & MAC2_INTR) {
		writel(intrusb, U3D_COMMON_USB_INTR);
	}
#ifdef SUPPORT_U3
	if (lv1_isr & MAC3_INTR) {
		writel(ltssm, U3D_LTSSM_INTR);
	}
#endif
	writel(intrep, U3D_EPISR);
	writel(linkint, U3D_DEV_LINK_INTR);

	if (ltssm | intrusb | dmaintr | intrtx
	        | intrrx | intrqmu | intrqmudone | linkint) {
		mt_udc_irq(ltssm, intrusb, dmaintr, intrtx, intrrx,
		           intrqmu, intrqmudone, linkint);
	}
}

void lk_usb_scheduler(void) {
	mt_irq_ack(MT_SSUSB_IRQ_ID);
	service_interrupts();

	return;
}

int mt_usb_irq_init(void) {
	/* disable all endpoint interrupts */
	mu3d_hal_disable_intr();

	/* 2. Ack all gpt irq if needed */
	//writel(0x3F, GPT_IRQ_ACK);

	/* 3. Register usb irq */
	mt_irq_set_sens(MT_SSUSB_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
	mt_irq_set_polarity(MT_SSUSB_IRQ_ID, MT65xx_POLARITY_LOW);

	return 0;
}

/* Turn on the USB connection by enabling the pullup resistor */
void mt_usb_connect_internal(void) {
	if (g_speed != SSUSB_SPEED_SUPER) {
		mu3d_hal_u2dev_connect();
	} else {
		mu3d_hal_u3dev_connect();
	}
}

/* Turn off the USB connection by disabling the pullup resistor */
void mt_usb_disconnect_internal(void) {
	mu3d_hal_u2dev_disconnect();
	mu3d_hal_u3dev_disconnect();
}

int udc_init(struct udc_device *dev) {
	struct udc_descriptor *desc = NULL;
#ifdef USB_GINTR
#ifdef USB_HSDMA_ISR
	u32 usb_dmaintr;
#endif
#endif

	DBG_I("%s:\n", __func__);

	DBG_I("ep0_urb: %p\n", ep0_urb);
	/* RESET */
	mt_usb_disconnect_internal();
	thread_sleep(20);

	/* usb phy init */
	board_usb_init();

	thread_sleep(20);

	/* allocate ep0 */
	ep0out = _udc_endpoint_alloc(EP0, 0, EP0_MAX_PACKET_SIZE);
	ep0in = _udc_endpoint_alloc(EP0, 1, EP0_MAX_PACKET_SIZE);
	ep0req = udc_request_alloc();
	ep0req->buf = malloc(4096);
	ep0_urb->buf = ep0_buf;

	/* create and register a language table descriptor */
	/* language 0x0409 is US English */
	desc = udc_descriptor_alloc(TYPE_STRING, EP0, 4);
	desc->data[2] = 0x09;
	desc->data[3] = 0x04;
	udc_descriptor_register(desc);

#ifdef SUPPORT_U3
	/* allocate this twice otherwise desc_list and desc_list_u3 will have 2 same head */
	desc = udc_descriptor_alloc(TYPE_STRING, EP0, 4);
	desc->data[2] = 0x09;
	desc->data[3] = 0x04;
	udc_descriptor_register_u3(desc);

#endif

#ifdef SUPPORT_U3
#ifdef SUPPORT_QMU
	mu3d_hal_alloc_qmu_mem();
#endif
#endif

#ifdef USB_HSDMA_ISR
	/* setting HSDMA interrupt register */
	usb_dmaintr = (0xff | 0xff << USB_DMA_INTR_UNMASK_SET_OFFSET);
	writel(usb_dmaintr, USB_DMA_INTR);
#endif

	the_device = dev;
	return 0;
}

void udc_endpoint_free(struct udc_endpoint *ept) {
	/* todo */
}

struct udc_request *udc_request_alloc(void) {
	struct udc_request *req;
	req = malloc(sizeof(*req));
	req->buf = NULL;
	req->length = 0;
	return req;
}

void udc_request_free(struct udc_request *req) {
	free(req);
}

/* Called to start packet transmission. */
/*
 * It must be applied in udc_request_queue when polling mode is used.
 * (When USB_GINTR is undefined).
 * If interrupt mode is used, you can use
 * mt_udc_epx_handler(ept->num, USB_DIR_IN); to replace mt_ep_write make ISR
 * do it for you.
 */
static int mt_ep_write(struct udc_endpoint *ept) {
	int ep_num = ept->num;
	int count;
	u32 csr;

	/* udc_endpoint_write: cannot write ep0 */
	if (ep_num == 0)
		return FALSE;

	/* udc_endpoint_write: cannot write USB_DIR_OUT */
	if (ept->in == 0) /* USB_DIR_OUT: val is 0x0; in == 0 here */
		return FALSE;

	csr = USB_READCSR32(U3D_TX1CSR0, ep_num);
	if (csr & TX_TXPKTRDY) {
#if DBG_USB_GENERAL
		DBG_I("udc_endpoint_write: ep%d is not ready to be written\n",
		      ep_num);

#endif
		return FALSE;
	}
	count = mt_write_fifo(ept);

	csr |= TX_TXPKTRDY;
	USB_WRITECSR32(U3D_TX1CSR0, ep_num, csr);

	return count;
}

int udc_request_queue(struct udc_endpoint *ept, struct udc_request *req) {
#ifdef SUPPORT_QMU
	u8 *pbuf;
#endif

#ifdef SUPPORT_QMU
	/* don't dump debug message here, will cause ISR fail */
	/* work around for abnormal disconnect line during qmu transfer */
	if (!usb_online)
		return 0;
#endif

#if DBG_USB_GENERAL
	DBG_I("%s: ept %d %s queue req=%p, req->length=%x\n",
	      __func__, ept->num, ept->in ? "in" : "out", req, req->length);
	DBG_I("%s: ept %d: %p, ept->in: %s, ept->rcv_urb->buf: %p, ept->tx_urb->buf: %p, req->buf: %p\n",
	      __func__, ept->num, ept, ept->in ? "IN" : "OUT" , ept->rcv_urb->buf, ept->tx_urb->buf, req->buf);
#endif

	enter_critical_section();
	ept->req = req;
	ept->status = 0;    /* ACTIVE */

	ept->sent = 0;
	ept->last = 0;

	/* read */
	if (!ept->in) {
		/* update dst buffer with request's address */
		ept->rcv_urb->buf = req->buf;
		ept->rcv_urb->actual_length = 0;

		/* unmask EPx INTRRXE */
		/*
		 * To avoid the parallely access the buffer,
		 * it is umasked here and umask at complete.
		 */
#ifdef SUPPORT_QMU /* For QMU, don't enable EP interrupts. */
		pbuf = ept->rcv_urb->buf;

		/* FASTBOOT COMMAND */
		if (req->length <= GPD_BUF_SIZE_ALIGN) {
			mu3d_hal_insert_transfer_gpd(ept->num, USB_DIR_OUT, pbuf, req->length, true, true, false, (ept->type == USB_EP_XFER_ISO ? 0 : 1), ept->maxpkt);

		} else { /* FASTBOOT DATA */
			DBG_C("udc_request exceeded the maximum QMU buffer size GPD_BUF_SIZE_ALIGN\n");
		}
		/* start transfer */
		arch_clean_invalidate_cache_range((addr_t) ept->rcv_urb->buf, req->length);
		mu3d_hal_resume_qmu(ept->num, USB_DIR_OUT);
#else   /* For PIO, enable EP interrupts */
		writel(readl(U3D_EPIER) | (BIT16 << ept->num), U3D_EPIESR);
#endif
	}

	/* write */
	if (ept->in) {
		ept->tx_urb->buf = req->buf;
		ept->tx_urb->actual_length = req->length;

#ifdef SUPPORT_QMU /* For QMU, we don't call mt_ep_write() */
		//mu3d_hal_insert_transfer_gpd(ept->num, USB_DIR_IN, ept->tx_urb->buf, req->length, true, true, false, (ept->type == USB_EP_XFER_ISO ? 0 : 1), ept->maxpkt);
		/* According to usb_read implementation on PC side, ZLP flag should not be set */
		mu3d_hal_insert_transfer_gpd(ept->num, USB_DIR_IN, ept->tx_urb->buf, req->length, true, true, false, 0, ept->maxpkt);
		arch_clean_invalidate_cache_range((addr_t) ept->tx_urb->buf, req->length);
		mu3d_hal_resume_qmu(ept->num, USB_DIR_IN);
#else   /* For PIO, call mt_ep_write() */
		mt_ep_write(ept);
#endif
	}
	exit_critical_section();
	return 0;
}

int udc_register_gadget(struct udc_gadget *gadget) {
	if (the_gadget) {
		DBG_C("only one gadget supported\n");
		return FALSE;
	}
	the_gadget = gadget;
	return 0;
}

static void udc_ept_desc_fill(struct udc_endpoint *ept, unsigned char *data) {
	data[0] = 7;
	data[1] = TYPE_ENDPOINT;
	data[2] = ept->num | (ept->in ? USB_DIR_IN : USB_DIR_OUT);
	data[3] = 0x02;     /* bulk -- the only kind we support */
	data[4] = 0x00;     // ept->maxpkt; 512bytes
	data[5] = 0x02;     // ept->maxpkt >> 8; 512bytes
	data[6] = ept->in ? 0x00 : 0x01;
}

#ifdef SUPPORT_U3
static void udc_ept_desc_fill_u3(struct udc_endpoint *ept, unsigned char *data) {
	data[0] = 7;
	data[1] = TYPE_ENDPOINT;
	data[2] = ept->num | (ept->in ? USB_DIR_IN : USB_DIR_OUT);
	data[3] = 0x02;     /* bulk -- the only kind we support */
	data[4] = 0x00;     /* ept->maxpkt; 1024bytes */
	data[5] = 0x04;     /* ept->maxpkt >> 8; 1024bytes */
	data[6] = ept->in ? 0x00 : 0x01;
}
#endif

static unsigned udc_ifc_desc_size(struct udc_gadget *g) {
	return 9 + g->ifc_endpoints * 7;
}

static void udc_ifc_desc_fill(struct udc_gadget *g, unsigned char *data) {
	unsigned n;

	data[0] = 0x09;
	data[1] = TYPE_INTERFACE;
	data[2] = 0x00;     /* ifc number */
	data[3] = 0x00;     /* alt number */
	data[4] = g->ifc_endpoints; /* 0x02 */
	data[5] = g->ifc_class;     /* 0xff */
	data[6] = g->ifc_subclass;  /* 0x42 */
	data[7] = g->ifc_protocol;  /* 0x03 */
	data[8] = udc_string_desc_alloc(g->ifc_string);

	data += 9;
	for (n = 0; n < g->ifc_endpoints; n++) {
		udc_ept_desc_fill(g->ept[n], data);
		data += 7;
	}
}

#ifdef SUPPORT_U3
static void udc_companion_desc_fill(unsigned char *data) {
	data[0] = 6;
	data[1] = TYPE_SS_EP_COMP;
	data[2] = 0x0f; /* max burst: 0x0~0xf */
	data[3] = 0x00;
	data[4] = 0x00;
	data[5] = 0x00;
}

static unsigned udc_ifc_desc_size_u3(struct udc_gadget *g) {
	return 9 + g->ifc_endpoints * (7 + 6);  /* Endpoint + Companion desc */
}

static void udc_ifc_desc_fill_u3(struct udc_gadget *g, unsigned char *data) {
	unsigned n;

	data[0] = 0x09;
	data[1] = TYPE_INTERFACE;
	data[2] = 0x00;     /* ifc number */
	data[3] = 0x00;     /* alt number */
	data[4] = g->ifc_endpoints; /* 0x02 */
	data[5] = g->ifc_class;     /* 0xFF */
	data[6] = g->ifc_subclass;  /* 0x42 */
	data[7] = g->ifc_protocol;  /* 0x03 <-> ref: 0x01 */
	data[8] = udc_string_desc_alloc_u3(g->ifc_string);

	data += 9;
	for (n = 0; n < g->ifc_endpoints; n++) {
		udc_ept_desc_fill_u3(g->ept[n], data);
		data += 7;
		udc_companion_desc_fill(data);
		data += 6;
	}
}

static void udc_devcapa_desc_fill_u2(unsigned char *data) {
	/* data 0-6 (BOS 5-11) */
	data[0] = 0x07;     /* bLength: 7 */
	data[1] = 0x10;     /* bDescriptorType: DEVICE CAPABILITY */
	data[2] = 0x02;     /* bDevCapabilityType: USB 2.0 Ext Descriptor */
	data[3] = 0x02;     /* bmAttributes[4]: LPM (SuperSpeed) */
	data[4] = 0x00;
	data[5] = 0x00;
	data[6] = 0x00;
}

static void udc_devcapa_desc_fill_u3(unsigned char *data) {
	/* data 0-9 (BOS 12-21) */
	data[0] = 0x0A;     /* bLength: 10 */
	data[1] = 0x10;     /* bDescriptorType: DEVICE CAPABILITY */
	data[2] = 0x03;     /* bDevCapabilityType: SuperSpeed */
	data[3] = 0x00;     /* bmAttributes: Don't support LTM */
	data[4] = 0x0E;     /* wSpeedsSupported[0]: b'1110 */
	data[5] = 0x00;     /* wSpeedsSupported[1] */
	data[6] = 0x01;     /* bFunctionalitySupport */
	data[7] = 0x0A;     /* bU1DevExitLat: Less than 10us */
	data[8] = 0x20;     /* wU2DevExitLat[0]: 32us */
	data[9] = 0x00;     /* wU2DevExitLat[1] */
}
#endif

int udc_start(void) {
	struct udc_descriptor *desc;
	unsigned char *data;
	unsigned size;

	if (!the_device) {
		DBG_C("udc cannot start before init\n");
		return FALSE;
	}
	if (!the_gadget) {
		DBG_C("udc has no gadget registered\n");
		return FALSE;
	}

	/* create our device descriptor - USB2 */
	desc = udc_descriptor_alloc(TYPE_DEVICE, EP0, 18);
	data = desc->data;
	data[2] = 0x00;     /* usb spec minor rev */
	data[3] = 0x02;     /* usb spec major rev */
	data[4] = 0x00;     /* class */
	data[5] = 0x00;     /* subclass */
	data[6] = 0x00;     /* protocol */
	data[7] = 0x40;     /* max packet size on ept 0 */
	memcpy(data + 8, &the_device->vendor_id, sizeof(short));
	memcpy(data + 10, &the_device->product_id, sizeof(short));
	memcpy(data + 12, &the_device->version_id, sizeof(short));
	data[14] = udc_string_desc_alloc(the_device->manufacturer);
	data[15] = udc_string_desc_alloc(the_device->product);
	data[16] = udc_string_desc_alloc(the_device->serialno);
	data[17] = 1;       /* number of configurations */
	udc_descriptor_register(desc);

#ifdef SUPPORT_U3
	/* create our device descriptor - USB3 */
	desc = udc_descriptor_alloc(TYPE_DEVICE, EP0, 18);
	data = desc->data;
	data[2] = 0x00;     /* usb spec minor rev */
	data[3] = 0x03;     /* usb spec major rev */
	data[4] = 0x00;     /* class */
	data[5] = 0x00;     /* subclass */
	data[6] = 0x00;     /* protocol */
	data[7] = 0x09;     /* max packet size on ept 0 (USB3) */
	memcpy(data + 8, &the_device->vendor_id, sizeof(short));
	memcpy(data + 10, &the_device->product_id, sizeof(short));
	memcpy(data + 12, &the_device->version_id, sizeof(short));
	data[14] = udc_string_desc_alloc_u3(the_device->manufacturer);
	data[15] = udc_string_desc_alloc_u3(the_device->product);
	data[16] = udc_string_desc_alloc_u3(the_device->serialno);
	data[17] = 1;       /* number of configurations */
	udc_descriptor_register_u3(desc);

	/* create our BOS Binary Device Object descriptor - USB3 */
	desc = udc_descriptor_alloc(TYPE_BOS, EP0, 5);
	data = desc->data;
	/* wTotalLength: Length of this descriptor and all of its sub descriptors */
	data[2] = 0x16;     /* wTotalLength[0] */
	data[3] = 0x00;     /* wTotalLength[1] */
	data[4] = 0x02;     /* bNumDeviceCaps: number of separate device capability descriptors in BOS */
	udc_descriptor_register_u3(desc);

	/* create our BOS Binary Device Object descriptor - USB3 (FULL) */
	desc = udc_descriptor_alloc(TYPE_BOS, EP0, 22);
	data = desc->data;
	data[0] = 0x05;     /* bLength of BOS Header */
	data[2] = 0x16;     /* wTotalLength[0] */
	data[3] = 0x00;     /* wTotalLength[1] */
	data[4] = 0x02;     /* bNumDeviceCaps: number of separate device capability descriptors in BOS */

	/* BOS 1 */
	udc_devcapa_desc_fill_u2(data + 5);

	/* BOS 2 */
	udc_devcapa_desc_fill_u3(data + 5 + 7);

	udc_descriptor_register_u3(desc);
#endif

	/* create our configuration descriptor */
	size = 9 + udc_ifc_desc_size(the_gadget);
	desc = udc_descriptor_alloc(TYPE_CONFIGURATION, EP0, size);
	data = desc->data;
	data[0] = 0x09;
	data[2] = size;
	data[3] = size >> 8;
	data[4] = 0x01;     /* number of interfaces */
	data[5] = 0x01;     /* configuration value */
	data[6] = 0x00;     /* configuration string */
	data[7] = 0x80;     /* attributes */
	data[8] = 0x80;     /* max power (250ma) -- todo fix this */

	udc_ifc_desc_fill(the_gadget, data + 9);
	udc_descriptor_register(desc);

#ifdef SUPPORT_U3
	/* create our configuration descriptor - USB3 */
	size = 9 + udc_ifc_desc_size_u3(the_gadget);
	desc = udc_descriptor_alloc(TYPE_CONFIGURATION, EP0, size);
	data = desc->data;
	data[0] = 0x09;
	data[2] = size;
	data[3] = size >> 8;
	data[4] = 0x01;     /* number of interfaces */
	data[5] = 0x01;     /* configuration value */
	data[6] = 0x00;     /* configuration string */
	data[7] = 0x80;     /* attrib-power: bus: 0x80, self: 0xC0 */
	data[8] = 0x32;     /* max power: bus: 0x32, self: 0x18 */

	udc_ifc_desc_fill_u3(the_gadget, data + 9);
	udc_descriptor_register_u3(desc);
#endif

#if DBG_USB_DUMP_DESC
	DBG_I("%s: dump desc_list\n", __func__);
	for (desc = desc_list; desc; desc = desc->next) {
		DBG_I("tag: %04x\n", desc->tag);
		DBG_I("len: %d\n", desc->len);
		DBG_I("data:");
		hexdump8(desc->data, desc->len);
	}

#ifdef SUPPORT_U3
	DBG_I("%s: dump desc_list_u3\n", __func__);
	for (desc = desc_list_u3; desc; desc = desc->next) {
		DBG_I("tag: %04x\n", desc->tag);
		DBG_I("len: %d\n", desc->len);
		DBG_I("data:");
		hexdump8(desc->data, desc->len);
	}

#endif /* ifdef SUPPORT_U3 */
#endif

	/* register interrupt handler */
	mt_usb_irq_init();

	/* go to RUN mode */
	mt_usb_phy_recover();

	/* unmask usb irq */
#ifdef USB_GINTR
	mt_irq_unmask(MT_SSUSB_IRQ_ID);
#endif
	/* udc_enable end */
	mt_udc_reset(U3D_DFT_SPEED);
	udelay(100);

	/* usb_disconnect */
	mt_usb_disconnect_internal();

	/* usb_connect */
	mt_usb_connect_internal();

#ifndef USB_GINTR
	while (1) {
		service_interrupts();
	}
#endif

	return 0;
}

int udc_stop(void) {
	thread_sleep(10);

	mt_usb_disconnect_internal();
	mt_usb_phy_savecurrent();

	return 0;
}
