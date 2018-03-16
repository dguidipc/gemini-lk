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
#include <stdint.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <libfdt.h>
#include <platform/mt_gpt.h>
#include <platform/emi_mpu.h>
#include <debug.h>
#define MODULE_NAME "LK_LD_MD"
#include "ccci_ld_md_core.h"
#include "ccci_ld_md_errno.h"

/***************************************************************************************************
** Feature Option setting part
***************************************************************************************************/
#define ENABLE_EMI_PROTECTION


/***************************************************************************************************
** HW remap section
***************************************************************************************************/
extern unsigned int ddr_enable_4gb(void)__attribute__((weak));
static int is_4gb_ddr_support_en(void)
{
	int ret;
	if (ddr_enable_4gb) {
		ret = ddr_enable_4gb();
		ALWAYS_LOG("ddr_enable_4GB sta:%d\n", ret);
		return ret;
	} else {
		ALWAYS_LOG("ddr 4GB disable\n");
		return 0;
	}
}

/*-------- Register base part -------------------------------*/
/* HW remap for MD1 */
#define MD1_BANK0_MAP0 (0x10001300)
#define MD1_BANK0_MAP1 (0x10001304)
#define MD1_BANK1_MAP0 (0x10001308)
#define MD1_BANK1_MAP1 (0x1000130C)
#define MD1_BANK4_MAP0 (0x10001310)
#define MD1_BANK4_MAP1 (0x10001314)

/* HW remap for MD3(C2K) */
#define MD2_BANK0_MAP0 (0x10001370)
#define MD2_BANK0_MAP1 (0x10001374)
#define MD2_BANK4_MAP0 (0x10001380)
#define MD2_BANK4_MAP1 (0x10001384)

/* HW remap lock register */
#define MD_HW_REMAP_LOCK (0x10001F80)
#define MD1_LOCK         (1<<16)
#define MD3_LOCK         (1<<17)

static int md_mem_ro_rw_remapping(unsigned int md_id, unsigned int addr)
{
	unsigned int md_img_start_addr;
	unsigned int hw_remapping_bank0_map0;
	unsigned int hw_remapping_bank0_map1 = 0;
	unsigned int write_val;

	switch (md_id) {
		case 0: // MD1
			hw_remapping_bank0_map0 = MD1_BANK0_MAP0;
			hw_remapping_bank0_map1 = MD1_BANK0_MAP1;
			break;
		case 2: // MD3
			hw_remapping_bank0_map0 = MD2_BANK0_MAP0;
			hw_remapping_bank0_map1 = MD2_BANK0_MAP1;
			break;
		default:
			ALWAYS_LOG("Invalid md id:%d\n", md_id);
			return -1;
	}

	if (is_4gb_ddr_support_en()) {
		md_img_start_addr = addr & 0xFFFFFFFF;
		ALWAYS_LOG("---> Map 0x00000000 to 0x%x for MD%d(4G)\n", addr, md_id+1);
	} else {
		md_img_start_addr = addr - 0x40000000;
		ALWAYS_LOG("---> Map 0x00000000 to 0x%x for MD%d\n", addr, md_id+1);
	}

	/* For MDx_BANK0_MAP0 */
	write_val = (((md_img_start_addr >> 24) | 1) & 0xFF) \
	            + ((((md_img_start_addr + 0x02000000) >> 16) | 1<<8) & 0xFF00) \
	            + ((((md_img_start_addr + 0x04000000) >> 8) | 1<<16) & 0xFF0000) \
	            + ((((md_img_start_addr + 0x06000000) >> 0) | 1<<24) & 0xFF000000);
	DRV_WriteReg32(hw_remapping_bank0_map0, write_val);
	ALWAYS_LOG("BANK0_MAP0 value:0x%X\n", DRV_Reg32(hw_remapping_bank0_map0));

	/* For MDx_BANK0_MAP1 */
	write_val = ((((md_img_start_addr + 0x08000000) >> 24) | 1) & 0xFF) \
	            + ((((md_img_start_addr + 0x0A000000) >> 16) | 1<<8) & 0xFF00) \
	            + ((((md_img_start_addr + 0x0C000000) >> 8) | 1<<16) & 0xFF0000) \
	            + ((((md_img_start_addr + 0x0E000000) >> 0) | 1<<24) & 0xFF000000);
	DRV_WriteReg32(hw_remapping_bank0_map1, write_val);
	ALWAYS_LOG("BANK0_MAP1 value:0x%X\n", DRV_Reg32(hw_remapping_bank0_map1));

#ifdef DUMMY_AP_MODE
	/* For 256~512MB */
	if (md_id == MD_SYS1) {
		/* For MD1_BANK1_MAP0 */
		write_val = ((((md_img_start_addr + 0x10000000) >> 24) | 1) & 0xFF) \
		            + ((((md_img_start_addr + 0x12000000) >> 16) | 1<<8) & 0xFF00) \
		            + ((((md_img_start_addr + 0x14000000) >> 8) | 1<<16) & 0xFF0000) \
		            + ((((md_img_start_addr + 0x16000000) >> 0) | 1<<24) & 0xFF000000);
		DRV_WriteReg32(MD1_BANK1_MAP0, write_val);
		ALWAYS_LOG("BANK1_MAP0 value:0x%X\n", DRV_Reg32(MD1_BANK1_MAP0));

		/* For MD1_BANK1_MAP1 */
		write_val = ((((md_img_start_addr + 0x18000000) >> 24) | 1) & 0xFF) \
		            + ((((md_img_start_addr + 0x1A000000) >> 16) | 1<<8) & 0xFF00) \
		            + ((((md_img_start_addr + 0x1C000000) >> 8) | 1<<16) & 0xFF0000) \
		            + ((((md_img_start_addr + 0x1E000000) >> 0) | 1<<24) & 0xFF000000);
		DRV_WriteReg32(MD1_BANK1_MAP1, write_val);
		ALWAYS_LOG("BANK1_MAP1 value:0x%X\n", DRV_Reg32(MD1_BANK1_MAP1));
	}
#endif

	return 0;
}

static int md_smem_rw_remapping(unsigned int md_id, unsigned int addr)
{
	unsigned int md_smem_start_addr;
	unsigned int hw_remapping_bank4_map0;
	unsigned int hw_remapping_bank4_map1 = 0;
	unsigned int write_val;

	switch (md_id) {
		case 0: // MD1
			hw_remapping_bank4_map0 = MD1_BANK4_MAP0;
			hw_remapping_bank4_map1 = MD1_BANK4_MAP1;
			break;
		case 2: // MD3
			hw_remapping_bank4_map0 = MD2_BANK4_MAP0;
			hw_remapping_bank4_map1 = MD2_BANK4_MAP1;
			break;
		default:
			ALWAYS_LOG("Invalid md id:%d\n", md_id);
			return -LD_ERR_PLAT_INVALID_MD_ID;
	}

	if (is_4gb_ddr_support_en()) {
		md_smem_start_addr = addr & 0xFFFFFFFF;
		ALWAYS_LOG("---> Map 0x40000000 to 0x%x for MD%d(4G)\n", addr, md_id+1);
	} else {
		md_smem_start_addr = addr - 0x40000000;
		ALWAYS_LOG("---> Map 0x40000000 to 0x%x for MD%d\n", addr, md_id+1);
	}
	/* For MDx_BANK0_MAP0 */
	write_val = (((md_smem_start_addr >> 24) | 1) & 0xFF) \
	            + ((((md_smem_start_addr + 0x02000000) >> 16) | 1<<8) & 0xFF00) \
	            + ((((md_smem_start_addr + 0x04000000) >> 8) | 1<<16) & 0xFF0000) \
	            + ((((md_smem_start_addr + 0x06000000) >> 0) | 1<<24) & 0xFF000000);
	DRV_WriteReg32(hw_remapping_bank4_map0, write_val);
	ALWAYS_LOG("BANK4_MAP0 value:0x%X\n", DRV_Reg32(hw_remapping_bank4_map0));

	/* For MDx_BANK0_MAP1 */
	write_val = ((((md_smem_start_addr + 0x08000000) >> 24) | 1) & 0xFF) \
	            + ((((md_smem_start_addr + 0x0A000000) >> 16) | 1<<8) & 0xFF00) \
	            + ((((md_smem_start_addr + 0x0C000000) >> 8) | 1<<16) & 0xFF0000) \
	            + ((((md_smem_start_addr + 0x0E000000) >> 0) | 1<<24) & 0xFF000000);
	DRV_WriteReg32(hw_remapping_bank4_map1, write_val);
	ALWAYS_LOG("BANK4_MAP1 value:0x%X\n", DRV_Reg32(hw_remapping_bank4_map1));

	return 0;
}

static void md_emi_remapping_lock(unsigned int md_id)
{
	unsigned int reg_val;
	unsigned int lock_bit;

	switch (md_id) {
		case 0: // MD1
			lock_bit = MD1_LOCK;
			break;
		case 2: // MD3
			lock_bit = MD3_LOCK;
			break;
		default:
			ALWAYS_LOG("Invalid md id:%d for lock\n", md_id);
			return;
	}

	reg_val = DRV_Reg32(MD_HW_REMAP_LOCK);
	ALWAYS_LOG("before hw remap lock: MD1[%d], MD3[%d]\n", !!(reg_val&MD1_LOCK), !!(reg_val&MD3_LOCK));
	DRV_WriteReg32(MD_HW_REMAP_LOCK, (reg_val|lock_bit));
	reg_val = DRV_Reg32(MD_HW_REMAP_LOCK);
	ALWAYS_LOG("before hw remap lock: MD1[%d], MD3[%d]\n", !!(reg_val&MD1_LOCK), !!(reg_val&MD3_LOCK));
}

/* =================================================== */
/* MPU Region defination                               */
/* =================================================== */
/* Note: This structure should sync with Kernel!!!!    */
typedef struct _mpu_cfg {
	unsigned int start;
	unsigned int end;
	int region;
	unsigned int permission;
	int relate_region;
} mpu_cfg_t;

#define MPU_REGION_ID_SEC_OS            0
#define MPU_REGION_ID_ATF               1
/* #define MPU_REGION_ID_MD32_SMEM     2 */
#define MPU_REGION_SCP_OS               2
#define MPU_REGION_SVP_Shared_Secure    3
#define MPU_REGION_ID_TRUSTED_UI        4
#define MPU_REGION_ID_MD1_SEC_SMEM      5

#define MPU_REGION_ID_MD1_SMEM          7
#define MPU_REGION_ID_MD3_SMEM          8
#define MPU_REGION_ID_MD1MD3_SMEM       9
#define MPU_REGION_ID_MD1_MCURW_HWRW    10
#define MPU_REGION_ID_MD1_ROM           11  /* contain DSP in Everest */
#define MPU_REGION_ID_MD1_MCURW_HWRO    12
#define MPU_REGION_ID_MD1_MCURO_HWRW    13

#define MPU_REGION_ID_MD1_RW            14
#define MPU_REGION_ID_MDLOG             15  /* Reserved only */
#define MPU_REGION_ID_MD3_ROM           16
#define MPU_REGION_ID_MD3_RW            17

#define MPU_REGION_ID_WIFI_EMI_FW       18
#define MPU_REGION_ID_WMT               19
#define MPU_REGION_ID_AP                23
#define MPU_REGION_ID_TOTAL_NUM         (MPU_REGION_ID_AP + 1)

#define MPU_MDOMAIN_ID_AP       0
#define MPU_MDOMAIN_ID_MD       1
#define MPU_MDOMAIN_ID_MD3      5
#define MPU_MDOMAIN_ID_MDHW     7
#define MPU_MDOMAIN_ID_TOTAL_NUM    8

static const unsigned int mpu_att_default[MPU_REGION_ID_TOTAL_NUM][MPU_MDOMAIN_ID_TOTAL_NUM] = {
	/*===================================================================================================================*/
	/* No |  | D0(AP)    | D1(MD1)      | D2(CONN) | D3(Res)  | D4(MM)       | D5(MD3 )      | D6(MFG)      | D7(MDHW)   */
	/*--------------+----------------------------------------------------------------------------------------------------*/
	/*0*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*1*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*2*/{ FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*3*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*4*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*5*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
	/*6*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
	/*7*/{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*8*/{ NO_PROTECTION, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
	/*9*/{ NO_PROTECTION , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
	/*10*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, NO_PROTECTION},
	/*11*/{ SEC_R_NSEC_R , SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_R_NSEC_R },
	/*12*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_R_NSEC_R },
	/*13*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, NO_PROTECTION},
	/*14*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R },
	/*15*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*16*/{ SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION , FORBIDDEN    , FORBIDDEN},
	/*17*/{ SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
	/*18*/{ FORBIDDEN    , FORBIDDEN    , NO_PROTECTION, FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*19*/{ NO_PROTECTION, FORBIDDEN    , NO_PROTECTION, FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
	/*20*/{ NO_PROTECTION, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, NO_PROTECTION, FORBIDDEN    , NO_PROTECTION, FORBIDDEN},
}; /*=================================================================================================================*/

static void get_mpu_attr_str(int lock, int curr_attr, char buf[], int size)
{
	char ch = lock?'L':'U';
	snprintf(buf, 24, "%d-%d-%d-%d-%d-%d-%d-%d(%c)",
	         curr_attr&7, (curr_attr>>3)&7, (curr_attr>>6)&7, (curr_attr>>9)&7,
	         (curr_attr>>12)&7, (curr_attr>>15)&7, (curr_attr>>18)&7, (curr_attr>>21)&7, ch);
}

static const unsigned char region_mapping_at_hdr_md1[] = {
	MPU_REGION_ID_MD1_ROM, MPU_REGION_ID_MD1_MCURW_HWRO, MPU_REGION_ID_MD1_MCURO_HWRW,
	MPU_REGION_ID_MD1_MCURW_HWRW, MPU_REGION_ID_MD1_RW
};

static const int free_mpu_region[] = {-1};
static int curr_free_mpu_idx;
static int get_free_mpu_region(void)
{
	int ret;
	if (curr_free_mpu_idx < (int)(sizeof(free_mpu_region)/sizeof(int))) {
		ret = free_mpu_region[curr_free_mpu_idx];
		curr_free_mpu_idx++;
	} else
		ret = -LD_ERR_PLAT_MPU_REGION_EMPTY;
	return ret;
}

static unsigned int get_mpu_region_default_access_att(int region, int lock)
{
	return SET_ACCESS_PERMISSON(lock, mpu_att_default[region][7], mpu_att_default[region][6],
					mpu_att_default[region][5], mpu_att_default[region][4],
					mpu_att_default[region][3], mpu_att_default[region][2],
					mpu_att_default[region][1], mpu_att_default[region][0]);
}

static unsigned int mpu_attr_calculate(int region_id, unsigned int request_attr)
{
	/* |D0(AP)|D1(MD1)|D2(CONN)|D3(Res)|D4(MM)|D5(MD3)|D6(MFG)|D7(MDHW)*/
	unsigned int tmp_mpu_att[8], i;
	for (i = 0; i < 8; i++)
		tmp_mpu_att[i] = mpu_att_default[region_id][i];

	/* AP MD1 MDHW: AP */
	if ((request_attr & 0xF) <= FORBIDDEN)
		tmp_mpu_att[0] = (request_attr & 0xF);
	/* AP MD1 MDHW: MD1 */
	request_attr = (request_attr >> 4);
	if ((request_attr & 0xF) <= FORBIDDEN)
		tmp_mpu_att[1] = (request_attr & 0xF);
	/* AP MD1 MDHW: MDHW */
	request_attr = (request_attr >> 4);
	if ((request_attr & 0xF) <= FORBIDDEN)
		tmp_mpu_att[7] = (request_attr & 0xF);

	/* MPU_REGION_ID_MD1_MCURO_HWRW can't lock, other region lock OK */
	if (region_id != MPU_REGION_ID_MD1_MCURO_HWRW) {
		return  SET_ACCESS_PERMISSON(1, tmp_mpu_att[7], tmp_mpu_att[6], tmp_mpu_att[5], tmp_mpu_att[4],
		                             tmp_mpu_att[3], tmp_mpu_att[2], tmp_mpu_att[1], tmp_mpu_att[0]);
	} else {
		return  SET_ACCESS_PERMISSON(0, tmp_mpu_att[7], tmp_mpu_att[6], tmp_mpu_att[5], tmp_mpu_att[4],
		                             tmp_mpu_att[3], tmp_mpu_att[2], tmp_mpu_att[1], tmp_mpu_att[0]);
	}
}

static void ccci_mem_access_cfg(mpu_cfg_t *mpu_cfg_list, int clear)
{
#ifdef ENABLE_EMI_PROTECTION
	mpu_cfg_t *curr;
	unsigned int curr_attr;
	char buf[24];
	if (NULL == mpu_cfg_list)
		return;

	curr = mpu_cfg_list;
	curr_attr = SET_ACCESS_PERMISSON(0, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
	                                 NO_PROTECTION, NO_PROTECTION, NO_PROTECTION);
	while (curr->region != -1) {
		if (clear) {
			emi_mpu_set_region_protection(  0,      /*START_ADDR*/
			                                0,      /*END_ADDR*/
			                                curr->region,   /*region*/
			                                curr_attr);
			ALWAYS_LOG("Clr MPU:S:0x%x E:0x%x A:<%d>[0~7]%d-%d-%d-%d-%d-%d-%d-%d\n",  0, 0,
			          curr->region, curr_attr&7, (curr_attr>>3)&7, (curr_attr>>6)&7,
			          (curr_attr>>9)&7, (curr_attr>>12)&7, (curr_attr>>15)&7,
			          (curr_attr>>18)&7, (curr_attr>>21)&7);
		} else {
			emi_mpu_set_region_protection(  curr->start,    /*START_ADDR*/
			                                curr->end,  /*END_ADDR*/
			                                curr->region,   /*region*/
			                                curr->permission);
			curr_attr = curr->permission;
			get_mpu_attr_str(0, curr_attr, buf, 24);
			ALWAYS_LOG("Set MPU:S:0x%x E:0x%x A:<%d>[0~7]%s\n", curr->start,
			           curr->end, curr->region, buf);
		}
		curr++;
	}
#endif
}

/*--------- Implement one by one -------------------------------------------------------------------------------*/
int plat_get_padding_mpu_num(void)
{
	return (int)(sizeof(free_mpu_region)/sizeof(unsigned int)) - 1;
}

int plat_dt_reserve_mem_size_fixup(void *fdt)
{
	int offset, sub_offset;

	/* -------- Remove device tree item ----------------------- */
	offset = fdt_path_offset(fdt, "/reserved-memory");

	sub_offset = fdt_subnode_offset(fdt, offset, "reserve-memory-ccci_md1");
	if (sub_offset) {
		DT_DBG_LOG("get md1 reserve-mem success. offset: %d, sub_off: %d\n",
		           offset, sub_offset);

		/* kill this node */
		fdt_del_node(fdt, sub_offset);
	} else
		ALWAYS_LOG("ignore md1 reserve-mem.\n");

	sub_offset = fdt_subnode_offset(fdt, offset, "reserve-memory-ccci_md3_ccif");
	if (sub_offset) {
		DT_DBG_LOG("get md3 reserve-mem success. offset: %d, sub_off: %d\n",
		           offset, sub_offset);

		/* kill this node */
		fdt_del_node(fdt, sub_offset);
	} else
		ALWAYS_LOG("ignore md3 reserve-mem.\n");

	sub_offset = fdt_subnode_offset(fdt, offset, "reserve-memory-ccci_share");
	if (sub_offset) {
		DT_DBG_LOG("get ap md reserve-smem szie success. offset: %d, sub_off: %d\n",
		           offset, sub_offset);

		/* kill this node */
		fdt_del_node(fdt, sub_offset);
	} else
		ALWAYS_LOG("ignore reserve-share-mem.\n");

	return 0;
}

/*---------------------------------------------------------------------------------------------------*/
/* Global variable for share memory                                                                  */
/*---------------------------------------------------------------------------------------------------*/
static unsigned int ap_md1_smem_size_at_lk_env;
static unsigned int md1_md3_smem_size_at_lk_env;
static unsigned int ap_md3_smem_size_at_lk_env;

static unsigned int ap_md1_smem_size_at_img;
static unsigned int md1_md3_smem_size_at_img;
static unsigned int ap_md3_smem_size_at_img;

#define AP_MD1_SMEM_SIZE    0x200000
#define MD1_MD3_SMEM_SIZE   0x200000
#define AP_MD3_SMEM_SIZE    0x200000
#define MAX_SMEM_SIZE       0x8000000/*Change from 6M to 128M,0x600000*/
typedef struct _smem_layout {
	unsigned long long base_addr;
	unsigned int ap_md1_smem_offset;
	unsigned int ap_md1_smem_size;
	unsigned int ap_md3_smem_offset;
	unsigned int ap_md3_smem_size;
	unsigned int md1_md3_smem_offset;
	unsigned int md1_md3_smem_size;
	unsigned int total_smem_size;
} smem_layout_t;
static smem_layout_t smem_info;

/*---------------------------------------------------------------------------------------------------*/
/* HW remap function implement                                      */
/*---------------------------------------------------------------------------------------------------*/
int plat_apply_hw_remap_for_md_ro_rw(void* info)
{
	modem_info_t *md_ld_info = (modem_info_t *)info;
	return md_mem_ro_rw_remapping((unsigned int)md_ld_info->md_id, (unsigned int)md_ld_info->base_addr);
}

int plat_apply_hw_remap_for_md_smem(void *addr, int size)
{
	/* For share memory final size depends on MD number, just store start address and size
	** actual setting will do later
	*/
	smem_info.base_addr = (unsigned long long)((unsigned long)addr);
	return 0;
}

/*---------------------------------------------------------------------------------------------------*/
/* check header info collection by plat_post_hdr_info                                                */
/*---------------------------------------------------------------------------------------------------*/
void plat_post_hdr_info(void* hdr, int ver, int id)
{
	if (id == MD_SYS1) {
		ap_md1_smem_size_at_img = ((struct md_check_header_v5*)hdr)->ap_md_smem_size;
		md1_md3_smem_size_at_img = ((struct md_check_header_v5*)hdr)->md_to_md_smem_size;
	} else if (id == MD_SYS3) {
		ap_md3_smem_size_at_img = ((struct md_check_header_v1*)hdr)->ap_md_smem_size;
	}
}

/*---------------------------------------------------------------------------------------------------*/
/* MPU static global variable and mpu relate function implement                                      */
/*---------------------------------------------------------------------------------------------------*/
#define MPU_REGION_TOTAL_NUM	(16) /* = MD1+MD3 */
static mpu_cfg_t mpu_tbl[MPU_REGION_TOTAL_NUM];
static int s_g_curr_mpu_num;
/*
** if set start=0x0, end=0x10000, the actural protected area will be 0x0-0x1FFFF,
** here we use 64KB align, MPU actually request 32KB align since MT6582, but this works...
** we assume emi_mpu_set_region_protection will round end address down to 64KB align.
*/
static void dump_received_pure_mpu_setting(struct image_section_desc *mem_info, int item_num)
{
	int i;
	for (i =0; i < item_num; i++)
		MPU_DBG_LOG("mpu sec dec %d: offset:%x, size:%x, mpu_attr:%x, ext_flag:%x, relate_idx:%x\n", i,
		            mem_info[i].offset, mem_info[i].size, mem_info[i].mpu_attr,
		            mem_info[i].ext_flag, mem_info[i].relate_idx);
}
static int find_bind_mpu_region(mpu_cfg_t *mpu_tbl_hdr, int item_num, unsigned int bind_key)
{
	int i;
	for (i = 0; i < item_num; i++) {
		if (mpu_tbl_hdr[i].relate_region == (int)bind_key)
			return i;
	}

	return -1;
}
static int md1_mpu_setting_process(void *p_md_ld_info, void *p_mem_info, mpu_cfg_t *mpu_tbl_hdr)
{
	modem_info_t *md_ld_info = (modem_info_t *)p_md_ld_info;
	struct image_section_desc *mem_info = (struct image_section_desc *)p_mem_info;
	int normal_region_num = 0;
	int total_region_num = 0;
	int curr_idx = 0;
	int i;
	int had_padding = 0;
	int all_range_region_idx = -1;
	int bind_idx;
	int free_region_id;

	/* Calculate mpu num and padding num */
	for (i = 0; i < MPU_REGION_TOTAL_NUM; i++) {
		if ((mem_info[i].offset == 0) && (mem_info[i].size == 0))
			break;

		if (mem_info[i].ext_flag & VALID_PADDING)
			had_padding = 1;
		if (mem_info[i].ext_flag & MD_ALL_RANGE)
			all_range_region_idx = i;
	}
	total_region_num = i;

	dump_received_pure_mpu_setting(mem_info, total_region_num);

	for (i = 0; i < total_region_num; i++) {
		if (mem_info[i].ext_flag & (MD_ALL_RANGE|NEED_REMOVE|NEED_MPU_MORE))
			continue;
		/* Process normal case first */
		if (curr_idx >= (int)(sizeof(region_mapping_at_hdr_md1)/sizeof(unsigned char))) {
			ALWAYS_LOG("[error]md%d: mpu region too more %d\n", md_ld_info->md_id+1,
			           (int)(sizeof(region_mapping_at_hdr_md1)/sizeof(unsigned char)));
			return -LD_ERR_PLAT_MPU_REGION_TOO_MORE;
		}
		mpu_tbl_hdr[curr_idx].start = (unsigned int)md_ld_info->base_addr + mem_info[i].offset;
		mpu_tbl_hdr[curr_idx].end = mpu_tbl_hdr[curr_idx].start + mem_info[i].size;
		mpu_tbl_hdr[curr_idx].end = ((mpu_tbl_hdr[curr_idx].end + 0xFFFF)&(~0xFFFF)) - 1;/* 64K align */
		mpu_tbl_hdr[curr_idx].permission =
		    mpu_attr_calculate(region_mapping_at_hdr_md1[curr_idx], mem_info[i].mpu_attr);
		mpu_tbl_hdr[curr_idx].region = (int)region_mapping_at_hdr_md1[curr_idx];
		mpu_tbl_hdr[curr_idx].relate_region = mem_info[i].relate_idx;
		curr_idx++;
		normal_region_num++;
	}
	if (normal_region_num != (int)(sizeof(region_mapping_at_hdr_md1)/sizeof(unsigned char))) {
		ALWAYS_LOG("[error]md%d: mpu region not sync %d:%d\n", md_ld_info->md_id+1, normal_region_num,
		           (int)(sizeof(region_mapping_at_hdr_md1)/sizeof(unsigned char)));
		return -LD_ERR_PLAT_MPU_REGION_NUM_NOT_SYNC;
	}
	for (i = 0; i < total_region_num; i++) {
		if (mem_info[i].ext_flag & NEED_MPU_MORE) {
			bind_idx = find_bind_mpu_region(mpu_tbl_hdr, normal_region_num, mem_info[i].relate_idx);
			if (bind_idx >= 0) {
				mpu_tbl_hdr[curr_idx].start = (unsigned int)md_ld_info->base_addr + mem_info[i].offset;
				mpu_tbl_hdr[curr_idx].end = mpu_tbl_hdr[curr_idx].start + mem_info[i].size;
				/* 64K align */
				mpu_tbl_hdr[curr_idx].end = ((mpu_tbl_hdr[curr_idx].end + 0xFFFF)&(~0xFFFF)) - 1;
				mpu_tbl_hdr[curr_idx].permission = mpu_tbl_hdr[bind_idx].permission;
				/* setting relate region */
				free_region_id = get_free_mpu_region();
				if (free_region_id < 0) {
					ALWAYS_LOG("[error]abnormal free region id %d +\n", free_region_id);
					return -LD_ERR_PLAT_ABNORMAL_FREE_REGION;
				}
				mpu_tbl_hdr[curr_idx].region = free_region_id;
				mpu_tbl_hdr[curr_idx].relate_region = mpu_tbl_hdr[bind_idx].region;
				mpu_tbl_hdr[bind_idx].relate_region = free_region_id;
				curr_idx++;
			} else {
				ALWAYS_LOG("md%d: padding array abnormal\n", md_ld_info->md_id+1);
				return -LD_ERR_PLAT_ABNORMAL_PAD_ARRAY;
			}
		}
	}
	/* Apply MD all range mpu protect setting */
	if (had_padding) {
		free_region_id = get_free_mpu_region();
		if (free_region_id < 0) {
			ALWAYS_LOG("[error]no more free region\n");
			return -LD_ERR_PLAT_NO_MORE_FREE_REGION;
		}
		mpu_tbl_hdr[curr_idx].permission = get_mpu_region_default_access_att(free_region_id, 1);
		mpu_tbl_hdr[curr_idx].start = (unsigned int)md_ld_info->base_addr + mem_info[all_range_region_idx].offset;
		mpu_tbl_hdr[curr_idx].end = mpu_tbl_hdr[curr_idx].start + mem_info[all_range_region_idx].size;
		/* 64K align */
		mpu_tbl_hdr[curr_idx].end = ((mpu_tbl_hdr[curr_idx].end + 0xFFFF)&(~0xFFFF)) - 1;
		mpu_tbl_hdr[curr_idx].region = free_region_id;
		mpu_tbl_hdr[curr_idx].relate_region = 0;
		curr_idx++;
	}

	return curr_idx;
}

int plat_send_mpu_info_to_platorm(void *p_md_ld_info, void *p_mem_info)
{
	modem_info_t *md_ld_info = (modem_info_t *)p_md_ld_info;
	struct image_section_desc *mem_info = (struct image_section_desc *)p_mem_info;
	int md_id = md_ld_info->md_id;
	int ret;
	int i;
	char buf[24];

	if (md_id == MD_SYS1) {
		ret = md1_mpu_setting_process(p_md_ld_info, p_mem_info, &mpu_tbl[s_g_curr_mpu_num]);
		if (ret > 0)
			s_g_curr_mpu_num += ret;
	} else if (md_id == MD_SYS3) {
		/* RO part */
		mpu_tbl[s_g_curr_mpu_num].permission = get_mpu_region_default_access_att(MPU_REGION_ID_MD3_ROM, 1);
		mpu_tbl[s_g_curr_mpu_num].start = (unsigned int)md_ld_info->base_addr + mem_info[0].offset;
		mpu_tbl[s_g_curr_mpu_num].end = mpu_tbl[s_g_curr_mpu_num].start + mem_info[0].size;
		mpu_tbl[s_g_curr_mpu_num].region = MPU_REGION_ID_MD3_ROM;
		/* 64K align */
		mpu_tbl[s_g_curr_mpu_num].end = ((mpu_tbl[s_g_curr_mpu_num].end + 0xFFFF)&(~0xFFFF)) - 1;
		s_g_curr_mpu_num++;

		/* RW part */
		mpu_tbl[s_g_curr_mpu_num].permission = get_mpu_region_default_access_att(MPU_REGION_ID_MD3_RW, 1);
		mpu_tbl[s_g_curr_mpu_num].start = (unsigned int)md_ld_info->base_addr + mem_info[1].offset;
		mpu_tbl[s_g_curr_mpu_num].end = mpu_tbl[s_g_curr_mpu_num].start + mem_info[1].size;
		mpu_tbl[s_g_curr_mpu_num].region = MPU_REGION_ID_MD3_RW;
		/* 64K align */
		mpu_tbl[s_g_curr_mpu_num].end = ((mpu_tbl[s_g_curr_mpu_num].end + 0xFFFF)&(~0xFFFF)) - 1;
		s_g_curr_mpu_num++;
	}

	for (i =0; i < s_g_curr_mpu_num; i++) {
		get_mpu_attr_str(0, mpu_tbl[i].permission, buf, 24);
		MPU_DBG_LOG("plat mpu dec %d: region:%d[%d], start:0x%x, end:0x%x, attr:%s\n", i,
		            mpu_tbl[i].region, mpu_tbl[i].relate_region, mpu_tbl[i].start, mpu_tbl[i].end, buf);
	}

	return 0;
}

/*------------------------------------------------------------------------------------------------*/
/* Suppor function for share memory calculate */
/*------------------------------------------------------------------------------------------------*/
static int cal_share_mem_layout(int load_flag)
{
	ap_md1_smem_size_at_lk_env = str2uint(get_env("apmd1_smem"));
	md1_md3_smem_size_at_lk_env = str2uint(get_env("md1md3_smem"));
	ap_md3_smem_size_at_lk_env = str2uint(get_env("apmd3_smem"));
	ALWAYS_LOG("env[apmd1_smem]%x.\n", ap_md1_smem_size_at_lk_env);
	ALWAYS_LOG("env[md1md3_smem]%x.\n", md1_md3_smem_size_at_lk_env);
	ALWAYS_LOG("env[apmd3_smem]%x.\n", ap_md3_smem_size_at_lk_env);
	/* MD Share memory layout */
	/*    AP    <-->   MD1    */
	/*    MD1   <-->   MD3    */
	/*    AP    <-->   MD3    */
	if (load_flag & (1<<MD_SYS3)) {
		smem_info.ap_md1_smem_offset = 0;
		if (ap_md1_smem_size_at_lk_env)
			smem_info.ap_md1_smem_size = ap_md1_smem_size_at_lk_env;
		else if (ap_md1_smem_size_at_img)
			smem_info.ap_md1_smem_size = ap_md1_smem_size_at_img;
		else
			smem_info.ap_md1_smem_size  = AP_MD1_SMEM_SIZE;

		smem_info.md1_md3_smem_offset = (smem_info.ap_md1_smem_offset + smem_info.ap_md1_smem_size + 0x10000 - 1)&(~(0x10000 - 1));
		if (md1_md3_smem_size_at_lk_env)
			smem_info.md1_md3_smem_size = md1_md3_smem_size_at_lk_env;
		else if (md1_md3_smem_size_at_img)
			smem_info.md1_md3_smem_size = md1_md3_smem_size_at_img;
		else
			smem_info.md1_md3_smem_size = MD1_MD3_SMEM_SIZE;

		smem_info.ap_md3_smem_offset = smem_info.md1_md3_smem_offset + smem_info.md1_md3_smem_size;
		if (ap_md3_smem_size_at_lk_env)
			smem_info.ap_md3_smem_size = ap_md3_smem_size_at_lk_env;
		else if (ap_md3_smem_size_at_img)
			smem_info.ap_md3_smem_size = ap_md3_smem_size_at_img;
		else
			smem_info.ap_md3_smem_size = AP_MD3_SMEM_SIZE;

		smem_info.total_smem_size = smem_info.ap_md3_smem_offset + smem_info.ap_md3_smem_size;
	} else {
		smem_info.ap_md1_smem_offset = 0;
		if (ap_md1_smem_size_at_lk_env)
			smem_info.ap_md1_smem_size = ap_md1_smem_size_at_lk_env;
		else if (ap_md1_smem_size_at_img)
			smem_info.ap_md1_smem_size = ap_md1_smem_size_at_img;
		else
			smem_info.ap_md1_smem_size  = AP_MD1_SMEM_SIZE;

		smem_info.md1_md3_smem_offset = 0;
		smem_info.md1_md3_smem_size = 0;
		smem_info.ap_md3_smem_offset = 0;
		smem_info.ap_md3_smem_size = 0;
		smem_info.total_smem_size = smem_info.ap_md1_smem_size;
	}

	/* insert share memory layout to lk info */
	if (insert_ccci_tag_inf("smem_layout", (char*)&smem_info, sizeof(smem_layout_t)) < 0)
		ALWAYS_LOG("insert smem_layout fail\n");

	ALWAYS_LOG("smem_info.base_addr: %x\n", (unsigned int)smem_info.base_addr);
	ALWAYS_LOG("smem_info.ap_md1_smem_offset: %x\n", smem_info.ap_md1_smem_offset);
	ALWAYS_LOG("smem_info.ap_md1_smem_size: %x\n", smem_info.ap_md1_smem_size);
	ALWAYS_LOG("smem_info.ap_md3_smem_offset: %x\n", smem_info.ap_md3_smem_offset);
	ALWAYS_LOG("smem_info.ap_md3_smem_size: %x\n", smem_info.ap_md3_smem_size);
	ALWAYS_LOG("smem_info.md1_md3_smem_offset: %x\n", smem_info.md1_md3_smem_offset);
	ALWAYS_LOG("smem_info.md1_md3_smem_size: %x\n", smem_info.md1_md3_smem_size);
	ALWAYS_LOG("smem_info.total_smem_size: %x\n", smem_info.total_smem_size);

	return (int)smem_info.total_smem_size;
}

static void boot_to_dummy_ap_mode(int load_md_flag);
/*------------------------------------------------------------------------------------------------*/
/* Note: This function using global variable
** if set start=0x0, end=0x10000, the actural protected area will be 0x0-0x1FFFF,
** here we use 64KB align, MPU actually request 32KB align since MT6582, but this works...
** we assume emi_mpu_set_region_protection will round end address down to 64KB align.
*/
int plat_apply_platform_setting(int load_md_flag)
{
	int smem_final_size;

#ifdef DUMMY_AP_MODE
	/* This function will never return */
	boot_to_dummy_ap_mode(load_md_flag);
	return 0;
#endif

	/* Check loading validation */
	if (((load_md_flag & (1<<MD_SYS1)) == 0) && (load_md_flag & (1<<MD_SYS3))) {
		ALWAYS_LOG("md3 depends on md1,but md1 not loaded\n");
		return -LD_ERR_PLAT_MD1_NOT_RDY;
	}
	if ((load_md_flag & ((1<<MD_SYS1)|(1<<MD_SYS3))) == 0) {
		ALWAYS_LOG("both md1 and md3 not enable\n");
		return 0;
	}

	smem_final_size = cal_share_mem_layout(load_md_flag);
	ALWAYS_LOG("ap md1 share mem MPU need configure\n");
	mpu_tbl[s_g_curr_mpu_num].region = MPU_REGION_ID_MD1_SMEM;
	mpu_tbl[s_g_curr_mpu_num].permission = get_mpu_region_default_access_att(MPU_REGION_ID_MD1_SMEM, 0);
	mpu_tbl[s_g_curr_mpu_num].start = (unsigned int)smem_info.base_addr + smem_info.ap_md1_smem_offset;
	mpu_tbl[s_g_curr_mpu_num].end = (unsigned int)smem_info.base_addr + smem_info.ap_md1_smem_offset
	                  + smem_info.ap_md1_smem_size;
	mpu_tbl[s_g_curr_mpu_num].end = ((mpu_tbl[s_g_curr_mpu_num].end + 0xFFFF)&(~0xFFFF)) - 1;
	s_g_curr_mpu_num++;

	if (load_md_flag & (1<<MD_SYS3)) {
		ALWAYS_LOG("md1 md3 share mem MPU need configure\n");
		mpu_tbl[s_g_curr_mpu_num].region = MPU_REGION_ID_MD1MD3_SMEM;
		mpu_tbl[s_g_curr_mpu_num].permission = get_mpu_region_default_access_att(MPU_REGION_ID_MD1MD3_SMEM, 0);
		mpu_tbl[s_g_curr_mpu_num].start = (unsigned int)smem_info.base_addr + smem_info.md1_md3_smem_offset;
		mpu_tbl[s_g_curr_mpu_num].end = (unsigned int)smem_info.base_addr + smem_info.md1_md3_smem_offset
		                  + smem_info.md1_md3_smem_size;
		mpu_tbl[s_g_curr_mpu_num].end = ((mpu_tbl[s_g_curr_mpu_num].end + 0xFFFF)&(~0xFFFF)) - 1;
		s_g_curr_mpu_num++;
		ALWAYS_LOG("ap md3 share mem MPU need configure\n");
		mpu_tbl[s_g_curr_mpu_num].region = MPU_REGION_ID_MD3_SMEM;
		mpu_tbl[s_g_curr_mpu_num].permission = get_mpu_region_default_access_att(MPU_REGION_ID_MD3_SMEM, 0);
		mpu_tbl[s_g_curr_mpu_num].start = (unsigned int)smem_info.base_addr + smem_info.ap_md3_smem_offset;
		mpu_tbl[s_g_curr_mpu_num].end = (unsigned int)smem_info.base_addr + smem_info.ap_md3_smem_offset
		                  + smem_info.ap_md3_smem_size;
		mpu_tbl[s_g_curr_mpu_num].end = ((mpu_tbl[s_g_curr_mpu_num].end + 0xFFFF)&(~0xFFFF)) - 1;
		s_g_curr_mpu_num++;
	}

	mpu_tbl[s_g_curr_mpu_num].region = -1; /* mark for end */
	/* Insert mpu tag info */
	if (insert_ccci_tag_inf("md_mpu_inf", (char*)mpu_tbl, sizeof(mpu_cfg_t)*s_g_curr_mpu_num) < 0)
		ALWAYS_LOG("insert md_mpu_inf fail\n");
	if (insert_ccci_tag_inf("md_mpu_num", (char*)s_g_curr_mpu_num, sizeof(int)) < 0)
		ALWAYS_LOG("insert md_mpu_num fail\n");

	/* Apply all MPU setting */
	ccci_mem_access_cfg(mpu_tbl, 0);

	/* Apply share memory HW remap setting and lock it */
	if (load_md_flag & (1<<MD_SYS1)) {
		md_smem_rw_remapping(MD_SYS1, (unsigned int)(smem_info.base_addr + smem_info.ap_md1_smem_offset));
		md_emi_remapping_lock(MD_SYS1);
	}

	if (load_md_flag & (1<<MD_SYS3)) {
		md_smem_rw_remapping(MD_SYS3, (unsigned int)(smem_info.base_addr + smem_info.md1_md3_smem_offset));
		md_emi_remapping_lock(MD_SYS3);
	}

	return smem_final_size;
}

/*------------------------------------------------------------------------------------------------*/
/* platform configure setting info.                                                               */
/*------------------------------------------------------------------------------------------------*/
long long plat_ccci_get_ld_md_plat_setting(char cfg_name[])
{
	if (strcmp(cfg_name, "share_memory_size") == 0) {

#ifdef DUMMY_AP_MODE
		return 0x200000;
#endif
		return (long long)(MAX_SMEM_SIZE);
	}

	if (strcmp(cfg_name, "share_mem_limit") == 0)
		return 0x90000000LL;

	if (strcmp(cfg_name, "ro_rw_mem_limit") == 0) {
#ifdef DUMMY_AP_MODE
		return 0xA0000000LL;
#endif
		return 0xC0000000LL;
	}

	if (strcmp(cfg_name, "ro_rw_mem_align") == 0)
		return 0x2000000LL;

	if (strcmp(cfg_name, "share_mem_align") == 0)
		return 0x2000000LL;

	if (strcmp(cfg_name, "ld_version") == 0) {

#ifdef DUMMY_AP_MODE
		return 0x10001;
#endif
		return 0x10000;/* xxxx_yyyy, xxxx: main id, yyyy sub id */
	}

	return -1LL;
}

#ifdef DUMMY_AP_MODE
#include <platform/mt_irq.h>
extern void dummy_ap_boot_up_md(int md_en_flag);
extern void load_modem_image(void);
extern int dummy_ap_irq_helper(unsigned int);

/* Remember add this function to file platform.c(platform code) */
void dummy_ap_entry(void)
{
	load_modem_image();
}

/* Remember add this function to file interrupts.c(platform code) */
void dummy_ap_irq_handler(unsigned int irq)
{
	if (dummy_ap_irq_helper(irq)) {
		mt_irq_ack(irq);
		mt_irq_unmask(irq);
	}
}

void boot_to_dummy_ap_mode(int load_md_flag)
{
	md_smem_rw_remapping(MD_SYS1, (unsigned int)smem_info.base_addr);
	md_smem_rw_remapping(MD_SYS3, (unsigned int)smem_info.base_addr);
	/* Before boot dummy AP, clear share memory */
	memset((void*)((unsigned long)smem_info.base_addr), 0, 0x200000);

	dummy_ap_boot_up_md(load_md_flag);
}


#endif
