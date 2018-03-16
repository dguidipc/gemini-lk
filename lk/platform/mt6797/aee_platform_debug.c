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
#include <malloc.h>
#include <dev/aee_platform_debug.h>
#include <platform/mt_reg_base.h>
#include <reg.h>
#include <string.h>

/* FOR DFD Function
*
* This area is applied for dfd functions.
* DFD SRAM is 144KB at address of 0x300000
*/
#define DFD_SRAM_ADDRESS    0x300000
#define DFD_SRAM_LENGTH     0x24000

static int dfd_get(void **data, int *len)
{
	/* DFD SRAM can only be read by CPU,
	 * the caller might use DMA to move data,
	 * to help those guys, we move content to DRAM */
	*data = malloc(DFD_SRAM_LENGTH);
	if (*data == NULL) {
		return 0;
	}

	memcpy(*data, (void *)DFD_SRAM_ADDRESS, DFD_SRAM_LENGTH);
	*len = DFD_SRAM_LENGTH;
	return 1;
}

static void dfd_put(void **data)
{
	free(*data);
}

static unsigned int save_dfd20_data(u64 offset, int *len, CALLBACK dev_write)
{
	char *buf = NULL;
	unsigned int datasize = 0;

	/* Save DFD buffer */
	if (dfd_get((void **)&buf, len)) {
		datasize = dev_write(buf, *len);
		dfd_put((void **)&buf);
	}
	return datasize;
}

/* FOR DRAMC data and DRAM Calibration Log
*
* This area is applied for DRAM related debug.
*/
/* DRAMC data */
#define DRAM_DEBUG_SRAM_ADDRESS 0x12D800
#define DRAM_DEBUG_SRAM_LENGTH  1024
static int plat_dram_debug_get(void **data, int *len)
{
	*data = (void *)DRAM_DEBUG_SRAM_ADDRESS;
	*len = DRAM_DEBUG_SRAM_LENGTH;
	return 1;
}

/* shared SRAM for DRAMLOG calibration log */
#define DRAM_KLOG_SRAM_ADDRESS 0x0012E000
#define DRAM_KLOG_SRAM_LENGTH  8064
#define DRAM_KLOG_VALID_ADDRESS 0x0012E00C
static int plat_dram_klog_get(void **data, int *len)
{
	*data = (void *)DRAM_KLOG_SRAM_ADDRESS;
	*len = DRAM_KLOG_SRAM_LENGTH;
	return 1;
}

static bool plat_dram_has_klog(void)
{
	if (*(volatile unsigned int*)DRAM_KLOG_VALID_ADDRESS)
		return true;

	return false;
}

static unsigned int save_dram_data(u64 offset, int *len, CALLBACK dev_write)
{
	char *buf = NULL;
	unsigned int datasize = 0, allsize = 0;

	if (plat_dram_debug_get((void **)&buf, len)) {
		datasize = dev_write(buf, *len);
		allsize = datasize;
	}

	if (plat_dram_klog_get((void **)&buf, len)) {
		mrdump_read_log(buf, *len, MRDUMP_EXPDB_DRAM_KLOG_OFFSET);
		datasize = dev_write(buf, *len);
		allsize += datasize;
	}

	return allsize;
}

static int plat_write_dram_klog(void)
{
	char *sram_base = NULL;
	int len = 0;

	if (plat_dram_klog_get((void **)&sram_base, (int *)&len)) {
		if (plat_dram_has_klog()) {
			mrdump_write_log(MRDUMP_EXPDB_DRAM_KLOG_OFFSET, sram_base, len);
		}
	}
	return 0;
}

/* SPM Debug Features */
#define PCM_WDT_LATCH_0     (SLEEP_BASE + 0x190)
#define PCM_WDT_LATCH_1     (SLEEP_BASE + 0x194)
#define PCM_WDT_LATCH_2     (SLEEP_BASE + 0x198)
#define PCM_WDT_LATCH_3     (SLEEP_BASE + 0x1C4)
#define DRAMC_DBG_LATCH     (SLEEP_BASE + 0x19C)
#define PCM_WDT_LATCH_4     (SLEEP_BASE + 0x1C8)
#define PCM_WDT_LATCH_NUM   (6)
#define SPM_DATA_BUF_LENGTH (1024)
static unsigned long get_spm_wdt_latch(int index)
{
	unsigned long ret;

	switch (index) {
		case 0:
			ret = readl(PCM_WDT_LATCH_0);
			break;
		case 1:
			ret = readl(PCM_WDT_LATCH_1);
			break;
		case 2:
			ret = readl(PCM_WDT_LATCH_2);
			break;
		case 3:
			ret = readl(PCM_WDT_LATCH_3);
			break;
		case 4:
			ret = readl(DRAMC_DBG_LATCH);
			break;
		case 5:
			ret = readl(PCM_WDT_LATCH_4);
			break;
		default:
			ret = 0;
	}

	return ret;
}

static int spm_dump_data(char *buf, int *wp)
{
	int i;
	unsigned long val;

	if (buf == NULL || wp == NULL)
		return -1;

	/*
	 * Example output:
	 * SPM Suspend debug regs(index 1) = 0x8320535
	 * SPM Suspend debug regs(index 2) = 0xfe114200
	 * SPM Suspend debug regs(index 3) = 0x3920fffe
	 * SPM Suspend debug regs(index 4) = 0x3ac06f4f
	 */

	for (i = 0; i < PCM_WDT_LATCH_NUM; i++) {
		val = get_spm_wdt_latch(i);
		*wp += sprintf(buf + *wp,
		               "SPM Suspend debug regs(index %d) = 0x%x\n",
		               i + 1, val);
	}

	*wp += sprintf(buf + *wp, "\n");
	return 1;
}

int spm_data_get(void **data, int *len)
{
	int ret;

	*len = 0;
	*data = malloc(SPM_DATA_BUF_LENGTH);
	if (*data == NULL)
		return 0;

	ret = spm_dump_data(*data, len);
	if (ret < 0 || *len > SPM_DATA_BUF_LENGTH) {
		*len = (*len > SPM_DATA_BUF_LENGTH) ? SPM_DATA_BUF_LENGTH : *len;
		return ret;
	}

	return 1;
}

void spm_data_put(void **data)
{
	free(*data);
}

static unsigned int save_spm_data(u64 offset, int *len, CALLBACK dev_write)
{
	char *buf = NULL;
	unsigned int datasize = 0;

	/* Save SPM buffer */
	spm_data_get((void **)&buf, len);
	if (buf != NULL) {
		if (*len > 0)
			datasize = dev_write(buf, *len);
		spm_data_put((void **)&buf);
	}

	return datasize;
}

/* platform initial function */
int platform_debug_init(void)
{
	/* function pointer assignment */
	plat_dfd20_get = save_dfd20_data;
	plat_dram_get = save_dram_data;
	plat_spm_data_get = save_spm_data;

	/* routine tasks */
	plat_write_dram_klog();

	return 1;
}
