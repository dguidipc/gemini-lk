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

#ifndef _UFS_AIO_H
#define _UFS_AIO_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"

extern struct ufs_hba g_ufs_hba;

#define UFS_AIO_MAX_DEVICE          (1)

#define MAX_CDB_SIZE                16
#define GENERAL_UPIU_REQUEST_SIZE   32
#define QUERY_DESC_MAX_SIZE         255
#define QUERY_DESC_MIN_SIZE         2
#define QUERY_DESC_HDR_SIZE         2

#define QUERY_OSF_SIZE            (GENERAL_UPIU_REQUEST_SIZE - \
                    (sizeof(struct utp_upiu_header)))

#define UPIU_HEADER_DWORD(byte3, byte2, byte1, byte0)\
            cpu_to_be32((byte3 << 24) | (byte2 << 16) |\
             (byte1 << 8) | (byte0))
/*
 * UFS device may have standard LUs and LUN id could be from 0x00 to
 * 0x7F. Standard LUs use "Peripheral Device Addressing Format".
 * UFS device may also have the Well Known LUs (also referred as W-LU)
 * which again could be from 0x00 to 0x7F. For W-LUs, device only use
 * the "Extended Addressing Format" which means the W-LUNs would be
 * from 0xc100 (SCSI_W_LUN_BASE) onwards.
 * This means max. LUN number reported from UFS device could be 0xC17F.
 */
#define UFS_UPIU_MAX_UNIT_NUM_ID    0x7F
//#define UFS_MAX_LUNS        (SCSI_W_LUN_BASE + UFS_UPIU_MAX_UNIT_NUM_ID)
#define UFS_UPIU_WLUN_ID    (1 << 7)
#define UFS_UPIU_MAX_GENERAL_LUN    3

/* default UFS host ID */
#define UFS_DEFAULT_HOST_ID 0

/* default LU number */
#define UFS_DEFAULT_LUN 0

/* default transfer request slot number */
#define UFS_DEFAULT_TAG 0

/* SCSI command opcodes */
#define TEST_UNIT_READY       0x00

/* SCSI related definitions */
#define SCSI_SENSE_BUFFERSIZE   96

/* Well known logical unit id in LUN field of UPIU */
enum {
	UFS_UPIU_REPORT_LUNS_WLUN    = 0x81,
	UFS_UPIU_UFS_DEVICE_WLUN    = 0xD0,
	UFS_UPIU_BOOT_WLUN        = 0xB0,
	UFS_UPIU_RPMB_WLUN        = 0xC4,
};

/*
 * UFS Protocol Information Unit related definitions
 */

/* Task management functions */
enum {
	UFS_ABORT_TASK        = 0x01,
	UFS_ABORT_TASK_SET    = 0x02,
	UFS_CLEAR_TASK_SET    = 0x04,
	UFS_LOGICAL_RESET    = 0x08,
	UFS_QUERY_TASK        = 0x80,
	UFS_QUERY_TASK_SET    = 0x81,
};

/* UTP UPIU Transaction Codes Initiator to Target */
enum {
	UPIU_TRANSACTION_NOP_OUT    = 0x00,
	UPIU_TRANSACTION_COMMAND    = 0x01,
	UPIU_TRANSACTION_DATA_OUT    = 0x02,
	UPIU_TRANSACTION_TASK_REQ    = 0x04,
	UPIU_TRANSACTION_QUERY_REQ    = 0x16,
};

/* UTP UPIU Transaction Codes Target to Initiator */
enum {
	UPIU_TRANSACTION_NOP_IN        = 0x20,
	UPIU_TRANSACTION_RESPONSE    = 0x21,
	UPIU_TRANSACTION_DATA_IN    = 0x22,
	UPIU_TRANSACTION_TASK_RSP    = 0x24,
	UPIU_TRANSACTION_READY_XFER    = 0x31,
	UPIU_TRANSACTION_QUERY_RSP    = 0x36,
	UPIU_TRANSACTION_REJECT_UPIU    = 0x3F,
};

/* UPIU Read/Write flags */
enum {
	UPIU_CMD_FLAGS_NONE    = 0x00,
	UPIU_CMD_FLAGS_WRITE    = 0x20,
	UPIU_CMD_FLAGS_READ    = 0x40,
};

/* UPIU Task Attributes */
enum {
	UPIU_TASK_ATTR_SIMPLE    = 0x00,
	UPIU_TASK_ATTR_ORDERED    = 0x01,
	UPIU_TASK_ATTR_HEADQ    = 0x02,
	UPIU_TASK_ATTR_ACA    = 0x03,
};

/* UPIU Query request function */
enum {
	UPIU_QUERY_FUNC_STANDARD_READ_REQUEST           = 0x01,
	UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST          = 0x81,
};

/* Flag idn for Query Requests*/
enum flag_idn {
	QUERY_FLAG_IDN_FDEVICEINIT      = 0x01,
	QUERY_FLAG_IDN_PWR_ON_WPE    = 0x03,
	QUERY_FLAG_IDN_BKOPS_EN         = 0x04,
};

/* Attribute idn for Query requests */
enum attr_idn {
	QUERY_ATTR_IDN_ACTIVE_ICC_LVL    = 0x03,
	QUERY_ATTR_IDN_BKOPS_STATUS    = 0x05,
	QUERY_ATTR_IDN_REF_CLK_FREQ    = 0x0A,
	QUERY_ATTR_IDN_EE_CONTROL    = 0x0D,
	QUERY_ATTR_IDN_EE_STATUS    = 0x0E,
};

/* Descriptor idn for Query requests */
enum desc_idn {
	QUERY_DESC_IDN_DEVICE        = 0x0,
	QUERY_DESC_IDN_CONFIGURAION    = 0x1,
	QUERY_DESC_IDN_UNIT            = 0x2,
	QUERY_DESC_IDN_RFU_0        = 0x3,
	QUERY_DESC_IDN_INTERCONNECT    = 0x4,
	QUERY_DESC_IDN_STRING        = 0x5,
	QUERY_DESC_IDN_RFU_1        = 0x6,
	QUERY_DESC_IDN_GEOMETRY        = 0x7,
	QUERY_DESC_IDN_POWER        = 0x8,
	QUERY_DESC_IDN_MAX,
};

enum desc_header_offset {
	QUERY_DESC_LENGTH_OFFSET    = 0x00,
	QUERY_DESC_DESC_TYPE_OFFSET    = 0x01,
};

enum ufs_desc_max_size {
	QUERY_DESC_DEVICE_MAX_SIZE      = 0x40,
	QUERY_DESC_CONFIGURAION_MAX_SIZE    = 0x90,
	QUERY_DESC_UNIT_MAX_SIZE        = 0x23,
	QUERY_DESC_INTERCONNECT_MAX_SIZE    = 0x06,
	/*
	 * Max. 126 UNICODE characters (2 bytes per character) plus 2 bytes
	 * of descriptor header.
	 */
	QUERY_DESC_STRING_MAX_SIZE        = 0xFE,
	QUERY_DESC_GEOMETRY_MAZ_SIZE        = 0x44,
	QUERY_DESC_POWER_MAX_SIZE        = 0x62,
	QUERY_DESC_RFU_MAX_SIZE            = 0x00,
};

/* Unit descriptor parameters offsets in bytes*/
enum unit_desc_param {
	UNIT_DESC_PARAM_LEN            = 0x0,
	UNIT_DESC_PARAM_TYPE            = 0x1,
	UNIT_DESC_PARAM_UNIT_INDEX        = 0x2,
	UNIT_DESC_PARAM_LU_ENABLE        = 0x3,
	UNIT_DESC_PARAM_BOOT_LUN_ID        = 0x4,
	UNIT_DESC_PARAM_LU_WR_PROTECT        = 0x5,
	UNIT_DESC_PARAM_LU_Q_DEPTH        = 0x6,
	UNIT_DESC_PARAM_MEM_TYPE        = 0x8,
	UNIT_DESC_PARAM_DATA_RELIABILITY    = 0x9,
	UNIT_DESC_PARAM_LOGICAL_BLK_SIZE    = 0xA,
	UNIT_DESC_PARAM_LOGICAL_BLK_COUNT    = 0xB,
	UNIT_DESC_PARAM_ERASE_BLK_SIZE        = 0x13,
	UNIT_DESC_PARAM_PROVISIONING_TYPE    = 0x17,
	UNIT_DESC_PARAM_PHY_MEM_RSRC_CNT    = 0x18,
	UNIT_DESC_PARAM_CTX_CAPABILITIES    = 0x20,
	UNIT_DESC_PARAM_LARGE_UNIT_SIZE_M1    = 0x22,
};

/* Device descriptor parameters offsets in bytes*/
enum device_desc_param {
	DEVICE_DESC_PARAM_LEN                   = 0x0,
	DEVICE_DESC_PARAM_TYPE                  = 0x1,
	DEVICE_DESC_PARAM_DEVICE_TYPE           = 0x2,
	DEVICE_DESC_PARAM_DEVICE_CLASS          = 0x3,
	DEVICE_DESC_PARAM_DEVICE_SUB_CLASS      = 0x4,
	DEVICE_DESC_PARAM_PRTCL                 = 0x5,
	DEVICE_DESC_PARAM_NUM_LU                = 0x6,
	DEVICE_DESC_PARAM_NUM_WLU               = 0x7,
	DEVICE_DESC_PARAM_BOOT_ENBL             = 0x8,
	DEVICE_DESC_PARAM_DESC_ACCSS_ENBL       = 0x9,
	DEVICE_DESC_PARAM_INIT_PWR_MODE         = 0xA,
	DEVICE_DESC_PARAM_HIGH_PR_LUN           = 0xB,
	DEVICE_DESC_PARAM_SEC_RMV_TYPE          = 0xC,
	DEVICE_DESC_PARAM_SEC_LU                = 0xD,
	DEVICE_DESC_PARAM_BKOP_TERM_LT          = 0xE,
	DEVICE_DESC_PARAM_ACTVE_ICC_LVL         = 0xF,
	DEVICE_DESC_PARAM_SPEC_VER              = 0x10,
	DEVICE_DESC_PARAM_MANF_DATE             = 0x12,
	DEVICE_DESC_PARAM_MANF_NAME             = 0x14,
	DEVICE_DESC_PARAM_PRDCT_NAME            = 0x15,
	DEVICE_DESC_PARAM_SN                    = 0x16,
	DEVICE_DESC_PARAM_OEM_ID                = 0x17,
	DEVICE_DESC_PARAM_MANF_ID               = 0x18,
	DEVICE_DESC_PARAM_UD_OFFSET             = 0x1A,
	DEVICE_DESC_PARAM_UD_LEN                = 0x1B,
	DEVICE_DESC_PARAM_RTT_CAP               = 0x1C,
	DEVICE_DESC_PARAM_FRQ_RTC               = 0x1D,
};

/* Configuration descriptor offsets in bytes */
enum conf_desc_param {
	CONF_DESC_B_LENGTH                  = 0x0,
	CONF_DESC_B_DESCRIPTOR_TYPE         = 0x1,
	CONF_DESC_B_BOOT_ENABLE             = 0x3,
	CONF_DESC_B_DESCR_ACCESS_EN         = 0x4,
	CONF_DESC_B_INIT_POWER_MODE         = 0x5,
	CONF_DESC_B_HIGH_PRIORITY_LUN       = 0x6,
	CONF_DESC_B_SECURE_REMOVAL_TYPE     = 0x7,
	CONF_DESC_B_INIT_ACTIVE_ICC_LEVEL   = 0x8,
	CONF_DESC_W_PERIOIDIC_RTC_UPDATE    = 0x9,

	CONF_DESC_UNIT_B_LU_ENABLE          = 0x0,
	CONF_DESC_UNIT_B_BOOT_LUN_ID        = 0x1,
	CONF_DESC_UNIT_B_LU_WRITE_PROTECT   = 0x2,
	CONF_DESC_UNIT_B_MEMORY_TYPE        = 0x3,
	CONF_DESC_UNIT_D_NUM_ALLOC_UNITS    = 0x4,
	CONF_DESC_UNIT_B_DATA_RELIABILITY   = 0x8,
	CONF_DESC_UNIT_B_LOGICAL_BLOCK_SIZE = 0x9,
	CONF_DESC_UNIT_B_PROVISIONING_TYPE  = 0xA,
	CONF_DESC_UNIT_W_CONTEXT_CAPABILITY = 0xB,
};

#define CONF_DESC_UNIT_DESC_CFG_PARAM_OFFSET     (16)
#define CONF_DESC_UNIT_DESC_CFG_PARAM_ENTRY_SIZE (16)

#define UFS_MEMORY_TYPE_NORMAL          (0)
#define UFS_MEMORY_TYPE_SYSTEM_CODE     (1)
#define UFS_MEMORY_TYPE_NON_PERSISTENT  (2)
#define UFS_MEMORY_TYPE_ENHANCED_1      (3)
#define UFS_MEMORY_TYPE_ENHANCED_2      (4)
#define UFS_MEMORY_TYPE_ENHANCED_3      (5)
#define UFS_MEMORY_TYPE_ENHANCED_4      (6)

#define UFS_BOOT_LU_SIZE_BYTE           (4 * 1024 * 1024)   // 4 MB

/*
 * Logical Unit Write Protect
 * 00h: LU not write protected
 * 01h: LU write protected when fPowerOnWPEn =1
 * 02h: LU permanently write protected when fPermanentWPEn =1
 */
enum ufs_lu_wp_type {
	UFS_LU_NO_WP        = 0x00,
	UFS_LU_POWER_ON_WP    = 0x01,
	UFS_LU_PERM_WP        = 0x02,
};

/* bActiveICCLevel parameter current units */
enum {
	UFSHCD_NANO_AMP        = 0,
	UFSHCD_MICRO_AMP    = 1,
	UFSHCD_MILI_AMP        = 2,
	UFSHCD_AMP        = 3,
};

#define POWER_DESC_MAX_SIZE            0x62
#define POWER_DESC_MAX_ACTV_ICC_LVLS        16

/* Attribute  bActiveICCLevel parameter bit masks definitions */
#define ATTR_ICC_LVL_UNIT_OFFSET    14
#define ATTR_ICC_LVL_UNIT_MASK        (0x3 << ATTR_ICC_LVL_UNIT_OFFSET)
#define ATTR_ICC_LVL_VALUE_MASK        0x3FF

/* Power descriptor parameters offsets in bytes */
enum power_desc_param_offset {
	PWR_DESC_LEN            = 0x0,
	PWR_DESC_TYPE            = 0x1,
	PWR_DESC_ACTIVE_LVLS_VCC_0    = 0x2,
	PWR_DESC_ACTIVE_LVLS_VCCQ_0    = 0x22,
	PWR_DESC_ACTIVE_LVLS_VCCQ2_0    = 0x42,
};

#define ATTR_B_BOOT_LUN_EN                  (0x0)
#define ATTR_B_CONFIG_DESCR_LOCK            (0xB)

#define ATTR_B_BOOT_LUN_EN_BOOT_DISABLED    (0) //00h: Boot disabled (default)
#define ATTR_B_BOOT_LUN_EN_BOOT_LU_A        (1) //01h: Enabled boot from Boot LU A
#define ATTR_B_BOOT_LUN_EN_BOOT_LU_B        (2) //02h: Enabled boot from Boot LU B

/* Exception event mask values */
enum {
	MASK_EE_STATUS        = 0xFFFF,
	MASK_EE_URGENT_BKOPS    = (1 << 2),
};

/* Background operation status */
enum bkops_status {
	BKOPS_STATUS_NO_OP               = 0x0,
	BKOPS_STATUS_NON_CRITICAL        = 0x1,
	BKOPS_STATUS_PERF_IMPACT         = 0x2,
	BKOPS_STATUS_CRITICAL            = 0x3,
	BKOPS_STATUS_MAX         = BKOPS_STATUS_CRITICAL,
};

/* UTP QUERY Transaction Specific Fields OpCode */
enum query_opcode {
	UPIU_QUERY_OPCODE_NOP        = 0x0,
	UPIU_QUERY_OPCODE_READ_DESC    = 0x1,
	UPIU_QUERY_OPCODE_WRITE_DESC    = 0x2,
	UPIU_QUERY_OPCODE_READ_ATTR    = 0x3,
	UPIU_QUERY_OPCODE_WRITE_ATTR    = 0x4,
	UPIU_QUERY_OPCODE_READ_FLAG    = 0x5,
	UPIU_QUERY_OPCODE_SET_FLAG    = 0x6,
	UPIU_QUERY_OPCODE_CLEAR_FLAG    = 0x7,
	UPIU_QUERY_OPCODE_TOGGLE_FLAG    = 0x8,
};

/* Query response result code */
enum {
	QUERY_RESULT_SUCCESS                    = 0x00,
	QUERY_RESULT_NOT_READABLE               = 0xF6,
	QUERY_RESULT_NOT_WRITEABLE              = 0xF7,
	QUERY_RESULT_ALREADY_WRITTEN            = 0xF8,
	QUERY_RESULT_INVALID_LENGTH             = 0xF9,
	QUERY_RESULT_INVALID_VALUE              = 0xFA,
	QUERY_RESULT_INVALID_SELECTOR           = 0xFB,
	QUERY_RESULT_INVALID_INDEX              = 0xFC,
	QUERY_RESULT_INVALID_IDN                = 0xFD,
	QUERY_RESULT_INVALID_OPCODE             = 0xFE,
	QUERY_RESULT_GENERAL_FAILURE            = 0xFF,
};

/* UTP Transfer Request Command Type (CT) */
enum {
	UPIU_COMMAND_SET_TYPE_SCSI    = 0x0,
	UPIU_COMMAND_SET_TYPE_UFS    = 0x1,
	UPIU_COMMAND_SET_TYPE_QUERY    = 0x2,
};

/* UTP Transfer Request Command Offset */
#define UPIU_COMMAND_TYPE_OFFSET    28

/* Offset of the response code in the UPIU header */
#define UPIU_RSP_CODE_OFFSET        8

enum {
	MASK_SCSI_STATUS        = 0xFF,
	MASK_TASK_RESPONSE              = 0xFF00,
	MASK_RSP_UPIU_RESULT            = 0xFFFF,
	MASK_QUERY_DATA_SEG_LEN         = 0xFFFF,
	MASK_RSP_UPIU_DATA_SEG_LEN    = 0xFFFF,
	MASK_RSP_EXCEPTION_EVENT        = 0x10000,
};

/* Task management service response */
enum {
	UPIU_TASK_MANAGEMENT_FUNC_COMPL        = 0x00,
	UPIU_TASK_MANAGEMENT_FUNC_NOT_SUPPORTED = 0x04,
	UPIU_TASK_MANAGEMENT_FUNC_SUCCEEDED    = 0x08,
	UPIU_TASK_MANAGEMENT_FUNC_FAILED    = 0x05,
	UPIU_INCORRECT_LOGICAL_UNIT_NO        = 0x09,
};

/* UFS device power modes */
enum ufs_dev_pwr_mode {
	UFS_ACTIVE_PWR_MODE    = 1,
	UFS_SLEEP_PWR_MODE    = 2,
	UFS_POWERDOWN_PWR_MODE    = 3,
};

struct ufs_ocs_error {
	const char * name;
	u8 err;
};

/**
 * struct utp_upiu_header - UPIU header structure
 * @dword_0: UPIU header DW-0
 * @dword_1: UPIU header DW-1
 * @dword_2: UPIU header DW-2
 */
struct utp_upiu_header {
	__be32 dword_0;
	__be32 dword_1;
	__be32 dword_2;
};

/**
 * struct utp_upiu_cmd - Command UPIU structure
 * @data_transfer_len: Data Transfer Length DW-3
 * @cdb: Command Descriptor Block CDB DW-4 to DW-7
 */
struct utp_upiu_cmd {
	__be32 exp_data_transfer_len;
	u8 cdb[MAX_CDB_SIZE];
};

/**
 * struct utp_upiu_query - upiu request buffer structure for
 * query request.
 * @opcode: command to perform B-0
 * @idn: a value that indicates the particular type of data B-1
 * @index: Index to further identify data B-2
 * @selector: Index to further identify data B-3
 * @reserved_osf: spec reserved field B-4,5
 * @length: number of descriptor bytes to read/write B-6,7
 * @value: Attribute value to be written DW-5
 * @reserved: spec reserved DW-6,7
 */
struct utp_upiu_query {
	u8 opcode;
	u8 idn;
	u8 index;
	u8 selector;
	__be16 reserved_osf;
	__be16 length;
	__be32 value;
	__be32 reserved[2];
};

/**
 * struct utp_upiu_req - general upiu request structure
 * @header:UPIU header structure DW-0 to DW-2
 * @sc: fields structure for scsi command DW-3 to DW-7
 * @qr: fields structure for query request DW-3 to DW-7
 */
struct utp_upiu_req {
	struct utp_upiu_header header;
	union {
		struct utp_upiu_cmd sc;
		struct utp_upiu_query qr;
	};
};

/**
 * struct utp_cmd_rsp - Response UPIU structure
 * @residual_transfer_count: Residual transfer count DW-3
 * @reserved: Reserved double words DW-4 to DW-7
 * @sense_data_len: Sense data length DW-8 U16
 * @sense_data: Sense data field DW-8 to DW-12
 */
struct utp_cmd_rsp {
	__be32 residual_transfer_count;
	__be32 reserved[4];
	__be16 sense_data_len;
	u8 sense_data[18];
};

/**
 * struct utp_upiu_rsp - general upiu response structure
 * @header: UPIU header structure DW-0 to DW-2
 * @sr: fields structure for scsi command DW-3 to DW-12
 * @qr: fields structure for query request DW-3 to DW-7
 */
struct utp_upiu_rsp {
	struct utp_upiu_header header;
	union {
		struct utp_cmd_rsp sr;
		struct utp_upiu_query qr;
	};
};

/**
 * struct utp_upiu_task_req - Task request UPIU structure
 * @header - UPIU header structure DW0 to DW-2
 * @input_param1: Input parameter 1 DW-3
 * @input_param2: Input parameter 2 DW-4
 * @input_param3: Input parameter 3 DW-5
 * @reserved: Reserved double words DW-6 to DW-7
 */
struct utp_upiu_task_req {
	struct utp_upiu_header header;
	__be32 input_param1;
	__be32 input_param2;
	__be32 input_param3;
	__be32 reserved[2];
};

/**
 * struct utp_upiu_task_rsp - Task Management Response UPIU structure
 * @header: UPIU header structure DW0-DW-2
 * @output_param1: Ouput parameter 1 DW3
 * @output_param2: Output parameter 2 DW4
 * @reserved: Reserved double words DW-5 to DW-7
 */
struct utp_upiu_task_rsp {
	struct utp_upiu_header header;
	__be32 output_param1;
	__be32 output_param2;
	__be32 reserved[3];
};

/**
 * struct ufs_query_req - parameters for building a query request
 * @query_func: UPIU header query function
 * @upiu_req: the query request data
 */
struct ufs_query_req {
	u8 query_func;
	struct utp_upiu_query upiu_req;
};

/**
 * struct ufs_query_resp - UPIU QUERY
 * @response: device response code
 * @upiu_res: query response data
 */
struct ufs_query_res {
	u8 response;
	struct utp_upiu_query upiu_res;
};

/**
 * PIO mode related definitions
 */

// UPIU cmds buffer size
#define UFS_AIO_PIO_UPIU_CMD_BUF_SIZE       (0x20)

// HCI registers settings
#define UFS_HCI_REGISTER_FLAG_CDARES        (0x3 << 20)
#define UFS_HCI_REGISTER_FLAG_CDABUSY       (0x1 << 19)
#define UFS_HCI_REGISTER_FLAG_CDASTA        (0x1 << 18)

//Length Macros
#define LEN_10                        (10)
#define LEN_30                        (30)
#define LEN_32                        (32)
#define LEN_64                        (64)
#define LEN_100                       (100)
#define LEN_1000                      (1000)
#define LEN_3000                      (3000)
#define LEN_10000                     (10000)
#define LEN_1000000                   (1000000)

//UPIU cmds Response
#define UPIU_RESPONSE_SUCCESS         (0x0)
#define UPIU_CMD_STATUS_GOOD          (0x0)
#define UPIU_CMD_STATUS_CHECK_COND    (0x2)

//UPIU cmds buffer size
#define UPIU_CMD_BUF_SIZE             (0x20)

//UPIU data buffer size, max usage 0x40 for Read Device Descriptor, keep some buffer here
#define UFS_TEMP_BUF_SIZE            (0x100)

#endif /* End of Header */

