/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2016. All rights reserved.
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
#include <reg.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <platform/mt_reg_base.h>
#include <plat_debug_interface.h>
#include <systracker.h>
#include "utils.h"

static int systracker_dump(char *buf, int *wp)
{
	int i;
	unsigned int reg_value;
	unsigned int entry_valid;
	unsigned int entry_tid;
	unsigned int entry_id;
	unsigned int entry_address;
	unsigned int entry_data_size;
	unsigned int entry_burst_length;

	if (buf == NULL || wp == NULL)
		return -1;

	/* Get tracker info and save to buf */

	/* check if we got a timeout first */
	if (!(readl(BUS_DBG_CON) & BUS_DBG_CON_TIMEOUT))
		return 0;

	*wp += sprintf(buf + *wp, "\n*************************** systracker ***************************\n");

	/* BUS_DBG_AR_TRACK_L(__n)
	 * [31:0] ARADDR: DBG read tracker entry read address
	 */

	/* BUS_DBG_AR_TRACK_H(__n)
	 * [14] Valid:DBG read tracker entry valid
	 * [13:7] ARID:DBG read tracker entry read ID
	 * [6:4] ARSIZE:DBG read tracker entry read data size
	 * [3:0] ARLEN: DBG read tracker entry read burst length
	 */

	/* BUS_DBG_AR_TRACK_TID(__n)
	 * [2:0] BUS_DBG_AR_TRANS0_ENTRY_ID: DBG read tracker entry ID of 1st transaction
	 */

	for (i = 0; i < BUS_DBG_NUM_TRACKER; i++) {
		entry_address = readl(BUS_DBG_AR_TRACK_L(i));
		reg_value = readl(BUS_DBG_AR_TRACK_H(i));
		entry_valid = extract_n2mbits(reg_value, 19, 19);
		entry_id = extract_n2mbits(reg_value, 7, 18);
		entry_data_size = extract_n2mbits(reg_value, 4, 6);
		entry_burst_length = extract_n2mbits(reg_value, 0, 3);
		entry_tid = readl(BUS_DBG_AR_TRANS_TID(i));

		*wp += sprintf(buf + *wp,
		               "read entry = %d, valid = 0x%x, tid = 0x%x, read id = 0x%x, address = 0x%x, data_size = 0x%x, burst_length = 0x%x\n",
		               i, entry_valid, entry_tid, entry_id,
		               entry_address, entry_data_size, entry_burst_length);
	}

	/* BUS_DBG_AW_TRACK_L(__n)
	 * [31:0] AWADDR: DBG write tracker entry write address
	 */

	/* BUS_DBG_AW_TRACK_H(__n)
	 * [14] Valid:DBG   write tracker entry valid
	 * [13:7] ARID:DBG  write tracker entry write ID
	 * [6:4] ARSIZE:DBG write tracker entry write data size
	 * [3:0] ARLEN: DBG write tracker entry write burst length
	 */

	/* BUS_DBG_AW_TRACK_TID(__n)
	 * [2:0] BUS_DBG_AW_TRANS0_ENTRY_ID: DBG write tracker entry ID of 1st transaction
	 */

	for (i = 0; i < BUS_DBG_NUM_TRACKER; i++) {
		entry_address = readl(BUS_DBG_AW_TRACK_L(i));
		reg_value = readl(BUS_DBG_AW_TRACK_H(i));
		entry_valid = extract_n2mbits(reg_value, 19, 19);
		entry_id = extract_n2mbits(reg_value, 7, 18);
		entry_data_size = extract_n2mbits(reg_value, 4, 6);
		entry_burst_length = extract_n2mbits(reg_value, 0, 3);
		entry_tid = readl(BUS_DBG_AW_TRANS_TID(i));

		*wp += sprintf(buf + *wp,
		               "write entry = %d, valid = 0x%x, tid = 0x%x, write id = 0x%x, address = 0x%x, data_size = 0x%x, burst_length = 0x%x\n",
		               i, entry_valid, entry_tid, entry_id, entry_address,
		               entry_data_size, entry_burst_length);
	}

	return strlen(buf);
}

int systracker_get(void **data, int *len)
{
	int ret;

	*len = 0;
	*data = malloc(SYSTRACKER_BUF_LENGTH);
	if (*data == NULL)
		return 0;

	ret = systracker_dump(*data, len);
	if (ret < 0 || *len > SYSTRACKER_BUF_LENGTH) {
		*len = (*len > SYSTRACKER_BUF_LENGTH) ? SYSTRACKER_BUF_LENGTH : *len;
		return ret;
	}

	return 1;
}

void systracker_put(void **data)
{
	free(*data);
}
