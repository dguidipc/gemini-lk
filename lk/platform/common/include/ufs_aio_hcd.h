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

#ifndef _UFS_AIO_HCD_H
#define _UFS_AIO_HCD_H

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"
#include "ufs_aio_platform.h"
#include "ufs_aio_hci.h"
#include "ufs_aio.h"

#if defined(MTK_UFS_DRV_LK)
#include "reg.h"    // for writel, readl
#endif

// SCSI commands
#define INQUIRY         0x12
#define READ_CAPACITY   0x25
#define READ_10         0x28
#define WRITE_10        0x2A
#define UNMAP           0x42
#define SECURITY_PROTOCOL_OUT           0xB5
#define SECURITY_PROTOCOL_IN            0xA2

#define UFS_AIO_MAX_NUTRS               (1)             // number of UTP Transfer Requset Slot
#define UFS_AIO_MAX_NUTMRS              (1)             // number of UTP Task Management Requset Slot

// alignment requirement
#define UFS_ALIGN_UCD                   UFS_CACHE_ALIGNED_SIZE(128)   // UTP command descriptors
#define UFS_ALIGN_UTRD                  UFS_CACHE_ALIGNED_SIZE(32)    // UTP transfer request descriptors. 32 is enough for UFSHCI 2.1 spec after applying SW trick
#define UFS_ALIGN_UTMRD                 UFS_CACHE_ALIGNED_SIZE(32)    // UTP task management descriptors. 32 is enough for UFSHCI 2.1 spec after applying SW trick

#define UFS_BLOCK_BITS                  (12)
#define UFS_BLOCK_SIZE                  (1 << UFS_BLOCK_BITS)   // 4 KB

#define UFS_1KB                         (0x400)
#define UFS_1KB_MASK                    (UFS_1KB - 1)

// Original UFSHCD_ENABLE_INTRS (defined in kernel) does not have UIC_POWER_MODE!
#define UFSHCD_ENABLE_INTRS    (UTP_TRANSFER_REQ_COMPL |\
                 UTP_TASK_REQ_COMPL |\
                 UIC_POWER_MODE |\
                 UIC_HIBERNATE_ENTER |\
                 UIC_HIBERNATE_EXIT |\
                 UFSHCD_ERROR_MASK)

/* UIC command timeout, unit: ms */
#define UIC_CMD_TIMEOUT    500

/* NOP OUT retries waiting for NOP IN response */
#define NOP_OUT_RETRIES    10
/* Timeout after 30 msecs if NOP OUT hangs without response */
#define NOP_OUT_TIMEOUT    30 /* msecs */

/* Query request retries */
#define QUERY_REQ_RETRIES 10
/* Query request timeout */
#define QUERY_REQ_TIMEOUT 500 /* msec */    // original: 30

/* Task management command timeout */
#define TM_CMD_TIMEOUT    100 /* msecs */

/* maximum number of link-startup retries */
#define DME_LINKSTARTUP_RETRIES 3

/* maximum number of reset retries before giving up */
#define MAX_HOST_RESET_RETRIES 5

/* Expose the flag value from utp_upiu_query.value */
#define MASK_QUERY_UPIU_FLAG_LOC 0xFF

/* Interrupt aggregation default timeout, unit: 40us */
#define INT_AGGR_DEF_TO    0x02

#define UTP_TRANSFER_REQ_TIMEOUT_MS (5000)  // 5 seconds for a single UTP transfer request
// TODO: need different timeout for different operations

#define ufshcd_writel(hba, val, reg)    \
    writel((val), (hba)->hci_base + (reg))
#define ufshcd_readl(hba, reg)    \
    readl((hba)->hci_base + (reg))

#define ufsphy_writel(hba, val, reg)    \
    writel((val), (hba)->mphy_base + (reg))
#define ufsphy_readl(hba, reg)    \
    readl((hba)->mphy_base + (reg))

/*
 * This quirk needs to be enabled for MTK MPHY test chip.
 */
#define UFSHCD_QUIRK_MTK_MPHY_TESTCHIP          UFS_BIT(31)

enum {
    SW_RST_TARGET_HCI         = 0x1,
    SW_RST_TARGET_UNIPRO      = 0x2,
    SW_RST_TARGET_CPT         = 0x4,	// crypto engine
    SW_RST_TARGET_MPHY        = 0x8,
    SW_RST_TARGET_MAC_ALL     = (SW_RST_TARGET_HCI | SW_RST_TARGET_UNIPRO | SW_RST_TARGET_CPT),
};

enum {
	ATTR_SIMPLE = 0,
	ATTR_ORDERED = 1,
	ATTR_HEAD_OF_QUEUE = 2,
};

enum dev_cmd_type {
	DEV_CMD_TYPE_NOP        = 0x0,
	DEV_CMD_TYPE_QUERY        = 0x1,
};

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

typedef enum {
	UFS_LU_0 = 0
	,UFS_LU_1 = 1
	,UFS_LU_2 = 2
	,UFS_LU_INTERNAL_CNT = 3
} ufs_logical_unit_internal;

/*
 * UFS driver/device status
 */
#define UFS_DRV_STS_TEST_UNIT_READY_ALL_DEVICE  (0x00000001)

/**
 * struct uic_command - UIC command structure
 * @command: UIC command
 * @argument1: UIC command argument 1
 * @argument2: UIC command argument 2
 * @argument3: UIC command argument 3
 * @cmd_active: Indicate if UIC command is outstanding
 * @result: UIC command result
 * @done: UIC command completion
 */
struct uic_command {
	u32 command;
	u32 argument1;
	u32 argument2;
	u32 argument3;
	int cmd_active;
	int result;
};

/**
 * struct ufshcd_sg_entry - UFSHCI PRD Entry
 * @base_addr: Lower 32bit physical address DW-0
 * @upper_addr: Upper 32bit physical address DW-1
 * @reserved: Reserved for future use DW-2
 * @size: size of physical segment DW-3
 */
struct ufshcd_sg_entry {
	__le32    base_addr;
	__le32    upper_addr;
	__le32    reserved;
	__le32    size;
};

/**
 * struct ufs_query - holds relevent data structures for query request
 * @request: request upiu and function
 * @descriptor: buffer for sending/receiving descriptor
 * @response: response upiu and response
 */
struct ufs_query {
	struct ufs_query_req request;
	u8 *descriptor;
	struct ufs_query_res response;
};

/**
 * struct ufshcd_lrb - local reference block
 * @utr_descriptor_ptr: UTRD address of the command
 * @ucd_req_ptr: UCD address of the command
 * @ucd_rsp_ptr: Response UPIU address for this command
 * @ucd_prdt_ptr: PRDT address of the command
 * @cmd: pointer to SCSI command
 * @sense_buffer: pointer to sense buffer address of the SCSI command
 * @sense_bufflen: Length of the sense buffer
 * @scsi_status: SCSI status of the command
 * @command_type: SCSI, UFS, Query.
 * @task_tag: Task tag of the command
 * @lun: LUN of the command
 * @intr_cmd: Interrupt command (doesn't participate in interrupt aggregation)
 */
struct ufshcd_lrb {
	struct utp_transfer_req_desc *utr_descriptor_ptr;
	struct utp_upiu_req *ucd_req_ptr;
	struct utp_upiu_rsp *ucd_rsp_ptr;
	struct ufshcd_sg_entry *ucd_prdt_ptr;

	struct ufs_aio_scsi_cmd *cmd;
	u8 *sense_buffer;
	unsigned int sense_bufflen;
	int scsi_status;

	int command_type;
	int task_tag;
	u8 lun; /* UPIU LUN id field is only 8-bit wide */

	bool intr_cmd;

#ifdef UFS_CFG_CRYPTO
	u32 crypto_en;
	u32 crypto_cfgid;
	u32 crypto_dunl;
	u32 crypto_dunu;
#endif
};

struct ufs_pa_layer_attr {
	u32 gear_rx;
	u32 gear_tx;
	u32 lane_rx;
	u32 lane_tx;
	u32 pwr_rx;
	u32 pwr_tx;
	u32 hs_rate;
};

struct ufs_pwr_mode_info {
	bool is_valid;
	struct ufs_pa_layer_attr info;
};

struct ufs_aio_scsi_cmd {
	unsigned int lun;
	int tag;
	enum dma_data_direction dir;
	unsigned char attr;
	unsigned char cmd_data[MAX_CDB_SIZE];
	unsigned short cmd_len;
	unsigned int exp_len;
	void * data_buf;
};

struct ufs_aio_unmap_block_descriptor { // 16 bytes
	u8 unmap_logical_block_address[8];
	u8 num_logical_blocks[4];
	u8 reserved[4];
};

struct ufs_aio_unmap_param_list {       // 24 bytes
	u8 unmap_data_length[2];
	u8 unmap_block_descriptor_data_length[2];
	u8 reserved[4];
	struct ufs_aio_unmap_block_descriptor unmap_block_descriptor;
};

#define UFS_MAX_CMD_DATA_SIZE   (64)

/**
 * struct ufs_dev_cmd - all assosiated fields with device management commands
 * @type: device management command type - Query, NOP OUT
 * @lock: lock to allow one command at a time
 * @complete: internal commands completion
 * @tag_wq: wait queue until free command slot is available
 */
struct ufs_dev_cmd {
	enum dev_cmd_type type;
	struct ufs_query query;
};

#define MAX_PRODUCT_ID_LEN              (16)
#define MAX_PRODUCT_REVISION_LEVEL_LEN  (4)

struct ufs_device_info {
	u16 wmanufacturerid;                     // from Device Descriptor
	u8  num_active_lu;                       // from Device Descriptor
	char product_id[MAX_PRODUCT_ID_LEN + 1];
	char product_revision_level[MAX_PRODUCT_REVISION_LEVEL_LEN + 1];
};

struct ufs_geometry_info {
	u8  b_allocation_units_size;     // number of segment
	u16 w_adj_factor_enahnced_1;
	u32 d_segment_size;              // in 512 bytes
	u64 q_total_raw_device_capacity; // in 512 bytes
};

struct ufs_unit_desc_cfg_param {
	u8 b_lu_enable;
	u8 b_boot_lun_id;
	u8 b_lu_write_protect;
	u8 b_memory_type;
	u8 d_num_alloc_units[4];
	u8 b_data_reliability;
	u8 b_logical_block_size;
	u8 b_provisioning_type;
	u8 w_context_capabilities[2];
	u8 reserved[3];
};

struct ufs_hba {
	void    *hci_base;
	void    *pericfg_base;
	void    *mphy_base;
	int     nutrs;
	//int     nutmrs;

	/* Virtual memory reference */
	struct utp_transfer_cmd_desc *ucdl_base_addr;
	struct utp_transfer_req_desc *utrdl_base_addr;
	//struct utp_task_req_desc *utmrdl_base_addr;
	//void * sense_buf_base_addr[UFS_AIO_MAX_NUTRS];

	/* DMA memory reference */
	ufs_paddr_t ucdl_dma_addr;
	ufs_paddr_t utrdl_dma_addr;
	//ufs_paddr_t utmrdl_dma_addr;
	//ufs_paddr_t sense_buf_dma_addr[UFS_AIO_MAX_NUTRS];

	unsigned int hci_quirks;
	unsigned int dev_quirks;

	struct uic_command *active_uic_cmd;
	struct ufs_pa_layer_attr pwr_info;

	struct ufshcd_lrb *lrb;
	unsigned long lrb_in_use;

	struct ufs_device_info dev_info;

	u8  active_tr_tag;
	u8  mode;
#if defined(MTK_UFS_DRV_DA)
	u8  unit_desc_cfg_param_valid;
#endif
	//u8  active_tm_tag;
	int  active_lun;

	unsigned long outstanding_reqs;

	struct ufs_pwr_mode_info max_pwr_info;

	/* Device management request data */
	struct ufs_dev_cmd dev_cmd;

	int (* blk_read)(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf);
	int (* blk_write)(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf);
	int (* blk_erase)(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt);
	int (* nopin_nopout)(struct ufs_hba * hba);
	int (* query_flag)(struct ufs_hba *hba, enum query_opcode opcode, enum flag_idn idn, bool *flag_res);
	int (* query_attr)(struct ufs_hba *hba, enum query_opcode opcode, enum attr_idn idn, u8 index, u8 selector, u32 *attr_val);
	int (* read_descriptor)(struct ufs_hba * hba, enum desc_idn desc_id, int desc_index, u8 *buf, u32 size);
	int (* write_descriptor)(struct ufs_hba * hba, enum desc_idn desc_id, int desc_index, u8 *buf, u32 size);
	int (* dme_get)(struct ufs_hba *hba, u32 attr_sel, u32 *mib_val);
	int (* dme_peer_get)(struct ufs_hba *hba, u32 attr_sel, u32 *mib_val);
	int (* dme_set)(struct ufs_hba *hba, u32 attr_sel, u32 mib_val);
	int (* dme_peer_set)(struct ufs_hba *hba, u32 attr_sel, u32 mib_val);

#if defined(MTK_UFS_DRV_DA)
	// unit descriptor configurable parameters (in Configuration Descriptor)
	struct ufs_unit_desc_cfg_param unit_desc_cfg_param[UFS_UPIU_MAX_GENERAL_LUN];
#endif

#if defined(MTK_UFS_DRV_LK)
	u32 blk_cnt[UFS_LU_INTERNAL_CNT];
#endif

	u32 drv_status;

};

#if !defined(MTK_UFS_DRV_LK)
#define writel(val, reg) (*((volatile u32 *)(reg)) = (val))
#define readl(reg) (*((volatile u32 *)(reg)))
#endif

#define ufs_aio_writel(hba, val, reg)    \
    writel((val), (hba)->hci_base + (reg))

#define ufs_aio_readl(hba, reg)    \
    readl((hba)->hci_base + (reg))

/**
 * ufs_aio_hba_stop - Send controller to reset state
 * @hba: per adapter instance
 */
static inline void ufs_aio_hba_stop(struct ufs_hba *hba)
{
	ufs_aio_writel(hba, CONTROLLER_DISABLE,  REG_CONTROLLER_ENABLE);
}

#ifdef CONFIG_MTK_UFS_TEST
int ufs_aio_query_attr(struct ufs_hba *hba, enum query_opcode opcode,
                       enum attr_idn idn, u8 index, u8 selector, u32 *attr_val);
int ufs_aio_query_flag(struct ufs_hba *hba, enum query_opcode opcode,
                       enum flag_idn idn, bool *flag_res);
int ufs_aio_query_desc(struct ufs_hba *hba, enum query_opcode opcode,
                       enum desc_idn idn, u8 index, void *desc, u16 len);
#endif

#define UIC_COMMAND(cmd, attr, index, value)            \
    (struct uic_command) {                    \
        .command = (cmd),                \
        .argument1 = UIC_ARG_MIB_SEL(attr, index),     \
        .argument2 = 0,                    \
        .argument3 = (value),                \
        .result = 0xff,                    \
    }

#define UIC_CMD_DME_GET(attr, index, value) \
        UIC_COMMAND(UIC_CMD_DME_GET, attr, index, value)

#define UIC_CMD_DME_PEER_GET(attr, index, value) \
        UIC_COMMAND(UIC_CMD_DME_PEER_GET, attr, index, value)

#define UIC_CMD_DME_SET(attr, index, value) \
        UIC_COMMAND(UIC_CMD_DME_SET, attr, index, value)

#define UIC_CMD_DME_PEER_SET(attr, index, value) \
        UIC_COMMAND(UIC_CMD_DME_PEER_SET, attr, index, value)

/* UIC command interfaces for DME primitives */
#define DME_LOCAL    0
#define DME_PEER    1
#define ATTR_SET_NOR    0    /* NORMAL */
#define ATTR_SET_ST    1    /* STATIC */

/*------------------------------------------------
 * UFS quirks related definitions and declarations
 *------------------------------------------------*/

/* return true if s1 is a prefix of s2 */
#define STR_PRFX_EQUAL(s1, s2) !strncmp(s1, s2, strlen(s1))

#define UFS_ANY_VENDOR 0xFFFF
#define UFS_ANY_MODEL  "ANY_MODEL"

#define UFS_VENDOR_TOSHIBA     0x198
#define UFS_VENDOR_SAMSUNG     0x1CE
#define UFS_VENDOR_SKHYNIX     0x1AD

/**
 * ufs_dev_fix - ufs device quirk info
 * @card: ufs card details
 * @quirk: device quirk
 */
struct ufs_dev_fix {
	struct ufs_device_info card;
	unsigned int quirk;
};

#define END_FIX { { 0 }, 0 }

/* add specific device quirk */
#define UFS_FIX(_vendor, _model, _quirk) \
               {                                         \
                       .card.wmanufacturerid = (_vendor),\
                       .card.product_id = (_model),           \
                       .quirk = (_quirk),                \
               }
/*
 * Some UFS memory device needs to lock configuration descriptor after programming LU.
 */
#define UFS_DEVICE_QUIRK_NEED_LOCK_CONFIG_DESC	(1 << 31)

/*
 * UFS crypto related
 */

enum {
	UFS_CRYPTO_ALGO_AES_XTS             = 0,
	UFS_CRYPTO_ALGO_BITLOCKER_AES_CBC   = 1,
	UFS_CRYPTO_ALGO_AES_ECB             = 2,
	UFS_CRYPTO_ALGO_ESSIV_AES_CBC       = 3,
};

enum {
	UFS_CRYPTO_DATA_UNIT_SIZE_4KB       = 3,
};

union ufs_cpt_cap {
	u32 cap_raw;
	struct {
		u8 cap_cnt;
		u8 cfg_cnt;
		u8 resv;
		u8 cfg_ptr;
	} cap;
};

union ufs_cpt_capx {
	u32 capx_raw;
	struct {
		u8 alg_id;
		u8 du_size;
		u8 key_size;
		u8 resv;
	} capx;
};

union ufs_cap_cfg {
	u32 cfgx_raw[32];
	struct {
		u32 key[16];
		u8 du_size;
		u8 cap_id;
		u16 resv0  : 15;
		u16 cfg_en : 1;
		u8 mu1ti_host;
		u8 resv1;
		u16 vsb;
		u32 resv2[14];
	} cfgx;
};

#define UPIU_COMMAND_CRYPTO_EN_OFFSET	23

/**
 * Exported API declaratoins
 */
extern int          ufs_aio_dma_nopin_nopout(struct ufs_hba * hba);
extern int          ufs_aio_pio_nopin_nopout(struct ufs_hba * hba);

extern int          ufs_aio_dma_query_flag(struct ufs_hba *hba, enum query_opcode opcode, enum flag_idn idn, bool *flag_res);
extern int          ufs_aio_pio_query_flag(struct ufs_hba *hba, enum query_opcode opcode, enum flag_idn idn, bool *flag_res);

extern int          ufs_aio_dma_read_desc(struct ufs_hba * hba, enum desc_idn desc_id, int desc_index, u8 *buf, u32 size);
extern int          ufs_aio_pio_read_desc(struct ufs_hba * hba, enum desc_idn desc_id, int desc_index, u8 * buf, u32 buf_size);

extern int          ufs_aio_dma_write_desc(struct ufs_hba *hba, enum desc_idn idn, int index, u8 *src_buf, u32 buf_len);
extern int          ufs_aio_pio_write_desc(struct ufs_hba *hba, enum desc_idn idn, int index, u8 *src_buf, u32 buf_len);

extern int          ufs_aio_dma_dme_get(struct ufs_hba *hba, u32 attr_sel, u32 *mib_val);
extern int          ufs_aio_pio_dme_get(struct ufs_hba *hba, u32 attr_sel, u32 *mib_val);

extern int          ufs_aio_dma_dme_peer_get(struct ufs_hba *hba, u32 attr_sel, u32 *mib_val);
extern int          ufs_aio_pio_dme_peer_get(struct ufs_hba *hba, u32 attr_sel, u32 *mib_val);

extern int          ufs_aio_dma_dme_set(struct ufs_hba *hba, u32 attr_sel, u32 mib_val);
extern int          ufs_aio_pio_dme_set(struct ufs_hba *hba, u32 attr_sel, u32 mib_val);

extern int          ufs_aio_dma_dme_peer_set(struct ufs_hba *hba, u32 attr_sel, u32 mib_val);
extern int          ufs_aio_pio_dme_peer_set(struct ufs_hba *hba, u32 attr_sel, u32 mib_val);

extern int          ufs_aio_dma_query_attr(struct ufs_hba *hba, enum query_opcode opcode, enum attr_idn idn, u8 index, u8 selector, u32 *attr_val);
extern int          ufs_aio_pio_query_attr(struct ufs_hba *hba, enum query_opcode opcode, enum attr_idn idn, u8 index, u8 selector, u32 *attr_val);

extern int          ufs_aio_crypto_init(struct ufs_hba *hba);
extern int          ufs_aio_get_device_info(struct ufs_hba *hba);
extern int          ufs_aio_get_lu_size(struct ufs_hba *hba, u32 lun, u32 *blk_size_in_byte, u32 *blk_cnt);
extern int          ufs_aio_read_unit_desc_cfg_param(struct ufs_hba *hba);
extern int          ufs_aio_set_boot_lu(struct ufs_hba *hba, u32 b_boot_lun_en);

extern struct ufs_hba * ufs_aio_get_host(u8 host_id);
extern int          ufshcd_init(void);

extern int          ufs_aio_init_mphy(struct ufs_hba *hba);

extern bool         ufshcd_get_free_tag(struct ufs_hba *hba, int *tag_out);
extern void         ufshcd_put_tag(struct ufs_hba *hba, int tag);

extern int          ufshcd_queuecommand(struct ufs_hba *hba, struct ufs_aio_scsi_cmd *cmd);
extern int          ufshcd_read_device_desc(struct ufs_hba *hba, u8 *buf, u32 size);
extern int          ufshcd_read_string_desc(struct ufs_hba *hba, int desc_index, u8 *buf, u32 size, bool ascii);

extern int          ufs_aio_pio_cport_direct_write(struct ufs_hba * hba, const unsigned char *data, unsigned long len, int eom, int retry_ms);
extern int          ufs_aio_pio_cport_direct_read(struct ufs_hba * hba, unsigned char *data, unsigned long buflen, u32 *plen, int retry_ms, bool isDummy, u32 real_byte);

#if defined(MTK_UFS_DRV_CTP)
extern int          ufshcd_config_pwr_mode(struct ufs_hba *hba, struct ufs_pa_layer_attr *desired_pwr_mode);
extern u8           ufs_crypto_rw_en;
#endif

extern unsigned char g_ufs_temp_buf[];

#endif /* _UFS_AIO_HCD_H */

