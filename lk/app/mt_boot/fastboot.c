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

#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/event.h>
#include <dev/udc.h>
#include <platform/mt_typedefs.h>
#include <platform/mtk_wdt.h>
#include "sys_commands.h"

#ifdef USE_G_ORIGINAL_PROTOCOL
#include "dl_commands.h"
#else
#include "download_commands.h"
#endif

#include "fastboot.h"
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#if defined(MTK_SECURITY_SW_SUPPORT) && defined(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT)
#include "sec_unlock.h"
#endif
#include <platform/boot_mode.h>
#ifdef FASTBOOT_WHOLE_FLASH_SUPPORT
#include "partition_parser.h"
#endif
#define MAX_RSP_SIZE 64
/* MAX_USBFS_BULK_SIZE: if use USB3 QMU GPD mode: cannot exceed 63 * 1024 */
#define MAX_USBFS_BULK_SIZE (16 * 1024)

#define EXPAND(NAME) #NAME
#define TARGET(NAME) EXPAND(NAME)

static event_t usb_online;
static event_t txn_done;
static unsigned char buffer[4096] __attribute__((aligned(64)));
static struct udc_endpoint *in, *out;
static struct udc_request *req;
int txn_status;

void *download_base;
unsigned download_max;
unsigned download_size;
extern int sec_usbdl_enabled (void);
extern void mtk_wdt_disable(void);
extern void mtk_wdt_restart(void);
extern unsigned get_secure_status(void);
extern unsigned get_unlocked_status(void);
unsigned fastboot_state = STATE_OFFLINE;

extern void fastboot_oem_key(const char *arg, void *data, unsigned sz);
extern void fastboot_oem_query_lock_state(const char *arg, void *data, unsigned sz);

timer_t wdt_timer;
struct fastboot_cmd *cmdlist;

extern BOOT_ARGUMENT *g_boot_arg;

static void req_complete(struct udc_request *req, unsigned actual, int status)
{
	txn_status = status;
	req->length = actual;
	event_signal(&txn_done, 0);
}


void fastboot_register(const char *prefix, void (*handle)(const char *arg, void *data, unsigned sz),
                       unsigned allowed_when_security_on, unsigned forbidden_when_lock_on)
{
	struct fastboot_cmd *cmd;

	cmd = malloc(sizeof(*cmd));
	if (cmd) {
		cmd->prefix = prefix;
		cmd->prefix_len = strlen(prefix);
		cmd->allowed_when_security_on = allowed_when_security_on;
		cmd->forbidden_when_lock_on = forbidden_when_lock_on;
		cmd->handle = handle;
		cmd->next = cmdlist;
		cmdlist = cmd;
	}
}

struct fastboot_var *varlist;

void fastboot_publish(const char *name, const char *value)
{
	struct fastboot_var *var;
	var = malloc(sizeof(*var));
	if (var) {
		var->name = name;
		var->value = value;
		var->next = varlist;
		varlist = var;
	}
}

void fastboot_update_var(const char *name, const char *value)
{
	struct fastboot_var *var = varlist;

	while (NULL != var) {
		if (!strcmp(name, var->name)) {
			var->value = value;
		}
		var = var->next;
	}

	return;
}

int usb_read(void *_buf, unsigned len)
{
	int r;
	unsigned xfer;
	unsigned char *buf = _buf;
	int count = 0;

	if (fastboot_state == STATE_ERROR)
		goto oops;

	while (len > 0) {
		xfer = (len > MAX_USBFS_BULK_SIZE) ? MAX_USBFS_BULK_SIZE : len;
		req->buf = buf;
		req->length = xfer;
		req->complete = req_complete;
		r = udc_request_queue(out, req);
		if (r < 0) {
			dprintf(INFO, "usb_read() queue failed\n");
			goto oops;
		}
		event_wait(&txn_done);

		if (txn_status < 0) {
			dprintf(INFO, "usb_read() transaction failed\n");
			goto oops;
		}

		count += req->length;
		buf += req->length;
		len -= req->length;

		/* short transfer? */
		if (req->length != xfer) break;
	}

	return count;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

int usb_write(void *buf, unsigned len)
{
	int r;

	if (fastboot_state == STATE_ERROR)
		goto oops;

	req->buf = buf;
	req->length = len;
	req->complete = req_complete;
	r = udc_request_queue(in, req);
	if (r < 0) {
		dprintf(INFO, "usb_write() queue failed\n");
		goto oops;
	}
	event_wait(&txn_done);
	if (txn_status < 0) {
		dprintf(INFO, "usb_write() transaction failed\n");
		goto oops;
	}
	return req->length;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

void fastboot_ack(const char *code, const char *reason)
{
	char response[MAX_RSP_SIZE];

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		reason = "";

	snprintf(response, MAX_RSP_SIZE, "%s%s", code, reason);
	fastboot_state = STATE_COMPLETE;

	usb_write(response, strlen(response));

}

void fastboot_info(const char *reason)
{
	char response[MAX_RSP_SIZE];

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		return;

	snprintf(response, MAX_RSP_SIZE, "INFO%s", reason);

	usb_write(response, strlen(response));
}

void fastboot_fail(const char *reason)
{
	fastboot_ack("FAIL", reason);
}

void fastboot_okay(const char *info)
{
	fastboot_ack("OKAY", info);
}

static void fastboot_command_loop(void)
{
	struct fastboot_cmd *cmd;
	int r;
	dprintf(ALWAYS,"fastboot: processing commands\n");

again:
	while (fastboot_state != STATE_ERROR) {
		memset(buffer, 0, sizeof(buffer));
		r = usb_read(buffer, MAX_RSP_SIZE);
		if (r < 0) break; //no input command
		buffer[r] = 0;
		dprintf(ALWAYS,"[fastboot: command buf]-[%s]-[len=%d]\n", buffer, r);
		dprintf(ALWAYS,"[fastboot]-[download_base:0x%x]-[download_size:0x%x]\n",(unsigned int)download_base,(unsigned int)download_size);

		/*Pick up matched command and handle it*/
		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			fastboot_state = STATE_COMMAND;
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len)) {
				continue;
			}

			dprintf(ALWAYS,"[Cmd process]-[buf:%s]-[lenBuf:%s]\n", buffer,  buffer + cmd->prefix_len);
#ifdef MTK_SECURITY_SW_SUPPORT
			extern unsigned int seclib_sec_boot_enabled(unsigned int);
			//if security boot enable, check cmd allowed
			if ( !(sec_usbdl_enabled() || seclib_sec_boot_enabled(1)) || cmd->allowed_when_security_on )
				if ((!cmd->forbidden_when_lock_on) || (0 != get_unlocked_status()))
#endif
				{
					cmd->handle((const char*) buffer + cmd->prefix_len, (void*) download_base, download_size);
				}
			if (fastboot_state == STATE_COMMAND) {
#ifdef MTK_SECURITY_SW_SUPPORT
				if ((sec_usbdl_enabled() || seclib_sec_boot_enabled(1)) && !cmd->allowed_when_security_on ) {
					fastboot_fail("not support on security");
				} else
				if ((cmd->forbidden_when_lock_on) && (0 == get_unlocked_status())) {
					fastboot_fail("not allowed in locked state");
				} else
#endif
				{
					fastboot_fail("unknown reason");
				}
			}
			goto again;
		}
		dprintf(ALWAYS,"[unknown command]*[%s]*\n", buffer);
		fastboot_fail("unknown command");

	}
	fastboot_state = STATE_OFFLINE;
	dprintf(ALWAYS,"fastboot: oops!\n");
}

static int fastboot_handler(void *arg)
{
	for (;;) {
		event_wait(&usb_online);
		fastboot_command_loop();
	}
	return 0;
}

static void fastboot_notify(struct udc_gadget *gadget, unsigned event)
{
	if (event == UDC_EVENT_ONLINE) {
		event_signal(&usb_online, 0);
	} else if (event == UDC_EVENT_OFFLINE) {
		event_unsignal(&usb_online);
	}
}

static struct udc_endpoint *fastboot_endpoints[2];

static struct udc_gadget fastboot_gadget = {
	.notify     = fastboot_notify,
	.ifc_class  = 0xff,
	.ifc_subclass   = 0x42,
	.ifc_protocol   = 0x03,
	.ifc_endpoints  = 2,
	.ifc_string = "fastboot",
	.ept        = fastboot_endpoints,
};

extern void fastboot_oem_register();
void register_partition_var(void)
{
	int i;
	unsigned long long p_size;
	char *type_buf;
	char *value_buf;
	char *var_name_buf;
	char *p_name_buf;

	for (i=0; i<PART_MAX_COUNT; i++) {
		p_size = partition_get_size(i);
		if ((long long)p_size == -1)
			continue;
		partition_get_name(i,&p_name_buf);

		partition_get_type(i,&type_buf);
		var_name_buf = malloc(30);
		sprintf(var_name_buf,"partition-type:%s",p_name_buf);
		fastboot_publish(var_name_buf,type_buf);
		//printf("%d %s %s\n",i,var_name_buf,type_buf);

		/*reserved for MTK security*/
		if (!strcmp(type_buf,"ext4")) {
			if (!strcmp(p_name_buf,"userdata")) {
				p_size -= (u64)1*1024*1024;
				if (p_size > 800*1024*1024) {
					p_size = 800*1024*1024;
				}
			}
		}
		value_buf = malloc(20);
		sprintf(value_buf,"%llx",p_size);
		var_name_buf = malloc(30);
		sprintf(var_name_buf,"partition-size:%s",p_name_buf);
		fastboot_publish(var_name_buf,value_buf);
		//printf("%d %s %s\n",i,var_name_buf,value_buf);
	}
}

static void register_secure_unlocked_var(void)
{
	char str_buf[2][4] = {"no","yes"};
	char *secure_buf;
	char *unlocked_buf;
	char *warranty_buf;
	unsigned n;
	int warranty;

	/* [FIXME] memory allocated not freed? */

	secure_buf = malloc(10);
#ifdef MTK_SECURITY_SW_SUPPORT
	n = get_secure_status();
#else
	n = 0;
#endif
	sprintf(secure_buf,"%s",str_buf[n]);
	fastboot_publish("secure", secure_buf);

	unlocked_buf = malloc(10);
#ifdef MTK_SECURITY_SW_SUPPORT
	n = get_unlocked_status();
#else
	n = 1;
#endif
	sprintf(unlocked_buf,"%s",str_buf[n]);
	fastboot_publish("unlocked", unlocked_buf);

	warranty_buf = malloc(10);
#ifdef MTK_SECURITY_SW_SUPPORT
	sec_query_warranty(&warranty);
#else
	warranty = 0;
#endif
	if (warranty >= 0 && warranty <= 1) {
		sprintf(warranty_buf,"%s",str_buf[warranty]);
		fastboot_publish("warranty", warranty_buf);
	}

}
#ifdef MTK_OFF_MODE_CHARGE_SUPPORT
static void register_off_mode_charge_var(void)
{
	//INIT VALUE
	char str_buf[2][2] = {"0","1"};
	char *value_buf;
	unsigned n;

	value_buf = malloc(5);
	n = get_off_mode_charge_status();
	if ((n == 0) || (n == 1)) {
		sprintf(value_buf,"%s",str_buf[n]);
		fastboot_publish("off-mode-charge", value_buf);
	} else {
		dprintf(INFO, "off mode charge status 'n' is out off boundary\n");
	}
}
#endif
int fastboot_init(void *base, unsigned size)
{
	thread_t *thr;

	dprintf(ALWAYS, "fastboot_init()\n");

	download_base = base;
	download_max = SCRATCH_SIZE;

	//mtk_wdt_disable(); /*It will re-enable during continue boot*/
	timer_initialize(&wdt_timer);
	timer_set_periodic(&wdt_timer, 5000, (timer_callback)mtk_wdt_restart, NULL);

	fastboot_register("getvar:", cmd_getvar, TRUE, FALSE);
	fastboot_publish("version", "0.5");
	fastboot_publish("version-preloader", g_boot_arg->pl_version);
	fastboot_register("signature", cmd_install_sig, FALSE, TRUE);
#ifdef USE_G_ORIGINAL_PROTOCOL
#if (defined(MTK_EMMC_SUPPORT) || defined(MTK_UFS_BOOTING)) && defined(MTK_SPI_NOR_SUPPORT)
	dprintf(ALWAYS,"Init EMMC device in fastboot mode\n");
	mmc_legacy_init(1);
#endif
	fastboot_register("flash:", cmd_flash_mmc, TRUE, TRUE);
	fastboot_register("erase:", cmd_erase_mmc, TRUE, TRUE);
#else
#if (defined(MTK_UFS_BOOTING) || defined(MTK_EMMC_SUPPORT))
	fastboot_register("flash:", cmd_flash_emmc, TRUE, TRUE);
	fastboot_register("erase:", cmd_erase_emmc, TRUE, TRUE);
#else
	fastboot_register("flash:", cmd_flash_nand, TRUE, TRUE);
	fastboot_register("erase:", cmd_erase_nand, TRUE, TRUE);
#endif

#endif


	fastboot_register("continue", cmd_continue, FALSE, FALSE);
	fastboot_register("reboot", cmd_reboot, TRUE, FALSE);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader, TRUE, FALSE);
	fastboot_publish("product", TARGET(BOARD));
	fastboot_publish("kernel", "lk");
	register_secure_unlocked_var();
#ifdef MTK_OFF_MODE_CHARGE_SUPPORT
	register_off_mode_charge_var();
#endif
	//fastboot_publish("serialno", sn_buf);

	register_partition_var();


	/*LXO: Download related command*/
	fastboot_register("download:", cmd_download, TRUE, FALSE);
	fastboot_publish("max-download-size", "0x8000000"); //128M = 134217728
	/*LXO: END!Download related command*/

	fastboot_oem_register();
#if defined(MTK_SECURITY_SW_SUPPORT)
	fastboot_register("oem p2u", cmd_oem_p2u, TRUE, FALSE);
#endif
	fastboot_register("oem reboot-recovery",cmd_oem_reboot2recovery, TRUE, FALSE);
	fastboot_register("oem append-cmdline",cmd_oem_append_cmdline,FALSE, FALSE);
#ifdef MTK_OFF_MODE_CHARGE_SUPPORT
	fastboot_register("oem off-mode-charge",cmd_oem_off_mode_charge,FALSE, FALSE);
#endif
#if defined(MTK_TC7_COMMON_DEVICE_INTERFACE) && defined(MTK_SECURITY_SW_SUPPORT)
	fastboot_register("oem auto-ADB",cmd_oem_ADB_Auto_Enable,TRUE, FALSE);
#endif
#if defined(MTK_SECURITY_SW_SUPPORT) && defined(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT)
	fastboot_register("oem unlock",fastboot_oem_unlock, TRUE, FALSE);
	fastboot_register("oem lock",fastboot_oem_lock, TRUE, FALSE);
	fastboot_register("oem key",fastboot_oem_key,TRUE, FALSE);
	fastboot_register("oem lks",fastboot_oem_query_lock_state,TRUE, FALSE);
	/* allowed when secure boot and unlocked */
	fastboot_register("boot", cmd_boot, TRUE, TRUE);
	/* command rename */
	fastboot_register("flashing unlock",fastboot_oem_unlock, TRUE, FALSE);
	fastboot_register("flashing lock",fastboot_oem_lock, TRUE, FALSE);
	fastboot_register("flashing get_unlock_ability", fastboot_get_unlock_ability, TRUE, FALSE);
#endif
#ifdef MTK_JTAG_SWITCH_SUPPORT
	/* pin mux switch to ap_jtag */
	fastboot_register("oem ap_jtag",cmd_oem_ap_jtag, TRUE, FALSE);
#endif
#ifdef MTK_TINYSYS_SCP_SUPPORT
	fastboot_register("oem scp_status",cmd_oem_scp_status, FALSE, FALSE);
#endif
#ifdef MTK_USB2JTAG_SUPPORT
	fastboot_register("oem usb2jtag", cmd_oem_usb2jtag, TRUE, FALSE);
#endif
	event_init(&usb_online, 0, EVENT_FLAG_AUTOUNSIGNAL);
	event_init(&txn_done, 0, EVENT_FLAG_AUTOUNSIGNAL);

	in = udc_endpoint_alloc(UDC_TYPE_BULK_IN, 512);
	if (!in)
		goto fail_alloc_in;
	out = udc_endpoint_alloc(UDC_TYPE_BULK_OUT, 512);
	if (!out)
		goto fail_alloc_out;

	fastboot_endpoints[0] = in;
	fastboot_endpoints[1] = out;

	req = udc_request_alloc();
	if (!req)
		goto fail_alloc_req;

	if (udc_register_gadget(&fastboot_gadget))
		goto fail_udc_register;

	thr = thread_create("fastboot", fastboot_handler, 0, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	if (!thr) {
		goto fail_alloc_in;
	}
	thread_resume(thr);
	return 0;

fail_udc_register:
	udc_request_free(req);
fail_alloc_req:
	udc_endpoint_free(out);
fail_alloc_out:
	udc_endpoint_free(in);
fail_alloc_in:
	return -1;
}

