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

#include "ufs_aio_cfg.h" // Include ufs_aio_cfg.h for defining MTK_UFS_DRV_PRELOADER
#include "ufs_aio_types.h"
#include "ufs_aio.h"
#include "ufs_aio_hcd.h"
#include "ufs_aio_core.h"
#include "ufs_aio_utils.h"
#include "ufs_aio_error.h"

#if defined(MTK_UFS_DRV_DA) || defined(MTK_UFS_DRV_LK) || defined(MTK_UFS_DRV_CTP)
int ufs_aio_scsi_unmap(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, u32 lba, u32 blk_cnt, unsigned long *buf, u32 buf_size)
{
	u32 exp_len;
	struct ufs_aio_unmap_param_list *p;

	memset(cmd->cmd_data, 0, MAX_CDB_SIZE);

	exp_len = sizeof(struct ufs_aio_unmap_param_list);

	p = (struct ufs_aio_unmap_param_list *)buf;

	if (buf_size < exp_len)
		return -1;

	p->unmap_data_length[0] = (exp_len - 2) >> 8;
	p->unmap_data_length[1] = (exp_len - 2) & 0xFF;
	p->unmap_block_descriptor_data_length[0] = (exp_len - 8) >> 8;
	p->unmap_block_descriptor_data_length[1] = (exp_len - 8) & 0xFF;
	p->unmap_block_descriptor.unmap_logical_block_address[0] = 0;   // lba is u32 only currently
	p->unmap_block_descriptor.unmap_logical_block_address[1] = 0;   // lba is u32 only currently
	p->unmap_block_descriptor.unmap_logical_block_address[2] = 0;   // lba is u32 only currently
	p->unmap_block_descriptor.unmap_logical_block_address[3] = 0;   // lba is u32 only currently
	p->unmap_block_descriptor.unmap_logical_block_address[4] = (lba >> 24) & 0xFF;
	p->unmap_block_descriptor.unmap_logical_block_address[5] = (lba >> 16) & 0xFF;
	p->unmap_block_descriptor.unmap_logical_block_address[6] = (lba >> 8) & 0xFF;
	p->unmap_block_descriptor.unmap_logical_block_address[7] = lba & 0xFF;
	p->unmap_block_descriptor.num_logical_blocks[0] = (blk_cnt >> 24) & 0xFF;
	p->unmap_block_descriptor.num_logical_blocks[1] = (blk_cnt >> 16) & 0xFF;
	p->unmap_block_descriptor.num_logical_blocks[2] = (blk_cnt >> 8) & 0xFF;
	p->unmap_block_descriptor.num_logical_blocks[3] = blk_cnt & 0xFF;

	cmd->lun = lun;
	cmd->tag = tag;
	cmd->dir = DMA_TO_DEVICE;
	cmd->exp_len = exp_len;  // UNMAP parameter list (header): 8 bytes + 1 UNMAP block descriptor: 16 bytes
	cmd->attr = ATTR_SIMPLE;
	cmd->cmd_len = 10;

	cmd->cmd_data[0] = UNMAP;               // opcode
	cmd->cmd_data[1] = 0;                   // reserved, anchor (0 for UFS)
	cmd->cmd_data[2] = 0;                   // reserved
	cmd->cmd_data[3] = 0;                   // reserved
	cmd->cmd_data[4] = 0;                   // reserved
	cmd->cmd_data[5] = 0;                   // reserved
	cmd->cmd_data[6] = 0x0;                 // reserved, group number (0 for UFS spec)
	cmd->cmd_data[7] = 0;                   // parameter list length (MSB)
	cmd->cmd_data[8] = exp_len;             // parameter list length (LSB)
	cmd->cmd_data[9] = 0x0;                 // control
	cmd->data_buf = buf;

	return 0;
}

#endif  // MTK_UFS_DRV_DA

void ufs_aio_scsi_cmd_read_capacity(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, unsigned long * buf)
{
	memset(cmd->cmd_data, 0, MAX_CDB_SIZE);

	cmd->lun = lun;
	cmd->tag = tag;
	cmd->dir = DMA_FROM_DEVICE;
	cmd->exp_len = 8;   // size of Read Capacity Parameter Data
	cmd->attr = ATTR_SIMPLE;
	cmd->cmd_len = 10;

	cmd->cmd_data[0] = READ_CAPACITY;       // opcode
	cmd->cmd_data[1] = 0;                   // reserved
	cmd->cmd_data[2] = 0;                   // LBA (MSB), 0 for UFS
	cmd->cmd_data[3] = 0;                   // LBA, 0 for UFS
	cmd->cmd_data[4] = 0;                   // LBA, 0 for UFS
	cmd->cmd_data[5] = 0;                   // LBA (LSB), 0 for UFS
	cmd->cmd_data[6] = 0;                   // reserved
	cmd->cmd_data[7] = 0;                   // reserved
	cmd->cmd_data[8] = 0;                   // reserved, PMI = 0
	cmd->cmd_data[9] = 0;                   // control = 0
	cmd->data_buf = buf;
}

void ufs_aio_scsi_cmd_inquiry(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, unsigned long * buf)
{
	memset(cmd->cmd_data, 0, MAX_CDB_SIZE);

	cmd->lun = lun;
	cmd->tag = tag;
	cmd->dir = DMA_FROM_DEVICE;
	cmd->exp_len = 36;
	cmd->attr = ATTR_SIMPLE;
	cmd->cmd_len = 6;

	cmd->cmd_data[0] = INQUIRY;     // opcode
	cmd->cmd_data[1] = 0x0;         // evpd (0: standared INQUIRY DATA)
	cmd->cmd_data[2] = 0x0;         // page code
	cmd->cmd_data[3] = 0x0;         // allocation length (MSB)
	cmd->cmd_data[4] = 36;          // allocation length (LSB)
	cmd->cmd_data[5] = 0x0;         // control
	cmd->data_buf = buf;
}

void ufs_aio_scsi_cmd_read10(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, u32 lba, u32 blk_cnt, unsigned long * buf, u32 attr)
{
	memset(cmd->cmd_data, 0, MAX_CDB_SIZE);

	cmd->lun = lun;
	cmd->tag = tag;
	cmd->dir = DMA_FROM_DEVICE;
	cmd->exp_len = blk_cnt * UFS_BLOCK_SIZE;
	cmd->attr = attr;
	cmd->cmd_len = 10;
	cmd->cmd_data[0] = READ_10;             // opcode
	cmd->cmd_data[1] = (1 << 3);            // flags, FUA = 1
	cmd->cmd_data[2] = (lba >> 24) & 0xff;  // LBA (MSB)
	cmd->cmd_data[3] = (lba >> 16) & 0xff;  // LBA
	cmd->cmd_data[4] = (lba >> 8) & 0xff;   // LBA
	cmd->cmd_data[5] = (lba) & 0xff;        // LBA (LSB)
	cmd->cmd_data[6] = 0x0;                 // reserved and group number
	cmd->cmd_data[7] = (blk_cnt >> 8);      // transfer length (MSB)
	cmd->cmd_data[8] = blk_cnt;             // transfer length (LSB)
	cmd->cmd_data[9] = 0x0;                 // control
	cmd->data_buf = buf;
}

void ufs_aio_scsi_cmd_write10(struct ufs_aio_scsi_cmd * cmd, u32 tag, u32 lun, u32 lba, u32 blk_cnt, unsigned long * buf, u32 attr)
{
	memset(cmd->cmd_data, 0, MAX_CDB_SIZE);

	cmd->lun = lun;
	cmd->tag = tag;
	cmd->dir = DMA_TO_DEVICE;
	cmd->exp_len = blk_cnt * UFS_BLOCK_SIZE;
	cmd->attr = attr;
	cmd->cmd_len = 10;
	cmd->cmd_data[0] = WRITE_10;            // opcode
	cmd->cmd_data[1] = (1<<3);              // flags, FUA = 1
	cmd->cmd_data[2] = (lba >> 24) & 0xff;  // LBA (MSB)
	cmd->cmd_data[3] = (lba >> 16) & 0xff;  // LBA
	cmd->cmd_data[4] = (lba >> 8) & 0xff;   // LBA
	cmd->cmd_data[5] = (lba) & 0xff;        // LBA (LSB)
	cmd->cmd_data[6] = 0x0;                 // reserved and group number
	cmd->cmd_data[7] = (blk_cnt>>8);        // transfer length (MSB)
	cmd->cmd_data[8] = blk_cnt;             // transfer length (LSB)
	cmd->cmd_data[9] = 0x0;                 // control
	cmd->data_buf = buf;
}

void ufs_aio_scsi_cmd_test_unit_ready(struct ufs_aio_scsi_cmd *cmd, u32 tag, u32 lun)
{
	memset(cmd->cmd_data, 0, MAX_CDB_SIZE);

	cmd->lun = lun;
	cmd->tag = tag;
	cmd->dir = DMA_NONE;
	cmd->exp_len = 0;
	cmd->attr = ATTR_SIMPLE;
	cmd->cmd_len = 6;
	cmd->cmd_data[0] = TEST_UNIT_READY; //opcode
	cmd->cmd_data[1] = 0x0; //rsvd, BIT 0: 0
	cmd->cmd_data[2] = 0x0; //rsvd
	cmd->cmd_data[3] = 0x0; //rsvd
	cmd->cmd_data[4] = 0x0; //rsvd
	cmd->cmd_data[5] = 0x0; //control
	cmd->data_buf = NULL;
}

int ufs_aio_block_read(u32 host_id, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf)
{
	int ret;
	u32 limit_blk_cnt = UFS_AIO_MAX_DATA_SIZE_PER_CMD_MB * 1024 * 1024 / UFS_BLOCK_SIZE;
	u32 read_blk_cnt;

	struct ufs_hba * hba = ufs_aio_get_host(host_id);

	if (hba->blk_read) {

		while (blk_cnt) {
			read_blk_cnt = (blk_cnt > limit_blk_cnt) ? limit_blk_cnt : blk_cnt;

			ret = hba->blk_read(hba, lun, blk_start, read_blk_cnt, buf);

			if (ret)
				return ret;

			blk_start += read_blk_cnt;
			blk_cnt -= read_blk_cnt;
			buf += ((read_blk_cnt * UFS_BLOCK_SIZE) / sizeof(unsigned long));
		}
	} else {
		UFS_DBG_LOGD("[UFS] err: null blk_read ptr\n");
		return -1;
	}

	return 0;
}

int ufs_aio_block_write(u32 host_id, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf)
{
	int ret;
	u32 limit_blk_cnt = UFS_AIO_MAX_DATA_SIZE_PER_CMD_MB * 1024 * 1024 / UFS_BLOCK_SIZE;
	u32 write_blk_cnt;

	struct ufs_hba * hba = ufs_aio_get_host(host_id);

	if (hba->blk_write) {

		while (blk_cnt) {
			write_blk_cnt = (blk_cnt > limit_blk_cnt) ? limit_blk_cnt : blk_cnt;

			ret = hba->blk_write(hba, lun, blk_start, blk_cnt, buf);

			if (ret)
				return ret;

			blk_start += write_blk_cnt;
			blk_cnt -= write_blk_cnt;
			buf += ((write_blk_cnt * UFS_BLOCK_SIZE) / sizeof(unsigned long));
		}
	} else {
		UFS_DBG_LOGD("[UFS] err: null blk_write ptr\n");
		return -1;
	}

	return 0;
}

int ufs_aio_dma_erase(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt)
{
	struct ufs_aio_scsi_cmd cmd;
	int tag;
	int ret;

	if (!ufshcd_get_free_tag(hba, &tag))
		return -1;

	ret = ufs_aio_scsi_unmap(&cmd, tag, lun, blk_start, blk_cnt, (unsigned long *)&g_ufs_temp_buf[0], UFS_TEMP_BUF_SIZE);

	if (0 != ret)
		return ret;

	ret = ufshcd_queuecommand(hba, &cmd);

	ufshcd_put_tag(hba, tag);

	return ret;
}

int ufs_aio_dma_read(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf)
{
	struct ufs_aio_scsi_cmd cmd;
	int tag;
	int ret;

	UFS_DBG_LOGV("[UFS] info: ufs_aio_dma_read,L:%d,S:%d,C:%d,buf:0x%x\n", lun, blk_start, blk_cnt, buf);

	if (!ufshcd_get_free_tag(hba, &tag))
		return -1;

	ufs_aio_scsi_cmd_read10(&cmd, tag, lun, blk_start, blk_cnt, buf, ATTR_SIMPLE);
	ret = ufshcd_queuecommand(hba, &cmd);

	ufshcd_put_tag(hba, tag);

	return ret;
}

int ufs_aio_dma_write(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf)
{
	struct ufs_aio_scsi_cmd cmd;
	int tag;
	int ret;

	UFS_DBG_LOGV("[UFS] info: ufs_aio_dma_write,L:%d,S:%d,C:%d,buf:0x%x\n", lun, blk_start, blk_cnt, buf);

	if (!ufshcd_get_free_tag(hba, &tag))
		return -1;

	ufs_aio_scsi_cmd_write10(&cmd, tag, lun, blk_start, blk_cnt, buf, ATTR_SIMPLE);
	ret = ufshcd_queuecommand(hba, &cmd);

	ufshcd_put_tag(hba, tag);

	return ret;
}

#ifdef UFS_CFG_ENABLE_PIO
int ufs_aio_pio_read(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf)  //The ail return value must >0xFF, size at most 4096 here
{
	int ret = UFS_ERR_NONE;
	u32 cBufLen = 0, cAccumulatedDataLength = 0, cDataLength = 0;
	u8 *pBufIn = (u8 *)buf;
	u32 size = blk_cnt * UFS_BLOCK_SIZE;

	u8 cmd_read10[32] = {
		0x01, 0x40, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x28, 0x08, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};

	u8 cmd_device[UPIU_CMD_BUF_SIZE]= {0x0};

	if (size == 0)
		return 0;

	cmd_read10[2] = lun;

	//Prepare the scsi-read10 command to read the blocks we just wrote to device
	//Modify the length and address of the header first
	cmd_read10[12] = ((blk_cnt*UFS_BLOCK_SIZE)>>24) & 0xFF; //Expected Data Transfer Length  (4KB block)
	cmd_read10[13] = ((blk_cnt*UFS_BLOCK_SIZE)>>16) & 0xFF; //Expected Data Transfer Length
	cmd_read10[14] = ((blk_cnt*UFS_BLOCK_SIZE)>>8) & 0xFF;; //Expected Data Transfer Length
	cmd_read10[15] = (blk_cnt*UFS_BLOCK_SIZE) & 0xFF; //Expected Data Transfer Length

	//modify logical address
	cmd_read10[18] = (blk_start>>24) & 0xFF; //Logical block address
	cmd_read10[19] = (blk_start>>16) & 0xFF; //Logical block address
	cmd_read10[20] = (blk_start>>8) & 0xFF; //Logical block address
	cmd_read10[21] = (blk_start) & 0xFF; //Logical block address

	//modify Transfer Length
	cmd_read10[23] = (blk_cnt>>8) & 0xFF; //TRANSFER LENGTH
	cmd_read10[24] = (blk_cnt) & 0xFF;    //TRANSFER LENGTH

#ifdef MSG_DEBUG
#ifdef UFS_UPIU_DEBUG
	if (1) {
		unsigned long i;
		UFS_DBG_LOGD("get read10 upiu (byte:%d)\n", 32);
		for (i=0; i<32; i++) {
			UFS_DBG_LOGD("0x%x ", cmd_read10[i]);
		}
		UFS_DBG_LOGD("\n");
	}
#endif
#endif


	//Send the scsi-read10 command to device
	UFS_DBG_LOGD("**** sending read10 of 0x%x blocks\n", blk_cnt);
	ret = ufs_aio_pio_cport_direct_write(hba, cmd_read10, sizeof(cmd_read10), 1, 1000); //send read10
	if (ret != UFS_ERR_NONE)
		goto __end;

	UFS_DBG_LOGD("**** reading DATA IN UPIU request \n");
	cAccumulatedDataLength = 0;

	//Start to read buffers from device continuously. Read buffer header first.
	ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, UPIU_CMD_BUF_SIZE, &cBufLen, 1000, FALSE, 0); //get ready_tx command

	while ((cBufLen > 0) && (ret == UFS_ERR_NONE) && (cAccumulatedDataLength < blk_cnt * UFS_BLOCK_SIZE)) { //while get ready tx command
		//Need to check this is Data IN UPIU or Response UPIU with error here
		if ((cmd_device[0] != UPIU_TRANSACTION_DATA_IN) || ((cmd_device[0] == UPIU_TRANSACTION_RESPONSE) && (cmd_device[7] != UPIU_CMD_STATUS_GOOD)))
			goto __end;

		cDataLength = (*(cmd_device+16) << 24) | (*(cmd_device+17) << 16) | (*(cmd_device+18) << 8) | (*(cmd_device+19));

		UFS_DBG_LOGD("prepare to read 0x%x bytes data\n", cDataLength);
		UFS_DBG_LOGD("cmd_device[0-3]: 0x%x 0x%x 0x%x 0x%x\n", *(cmd_device+0), *(cmd_device+1), *(cmd_device+2), *(cmd_device+3));
		UFS_DBG_LOGD("cmd_device[16-19]: 0x%x 0x%x 0x%x 0x%x\n", *(cmd_device+16), *(cmd_device+17), *(cmd_device+18), *(cmd_device+19));

		if (cDataLength > 0) {
			UFS_DBG_LOGD("**** Reading data of 0x%x bytes, starting from 0x%x\n", cDataLength, cAccumulatedDataLength);

			if (size%UFS_BLOCK_SIZE) { //Non 4-byte align size, read size, skip the following data. Notice: the code here is only for size <= 4096 case
				ret = ufs_aio_pio_cport_direct_read(hba, pBufIn + cAccumulatedDataLength, cDataLength , &cBufLen, 1000, TRUE, size); //Get Read buffer
			} else {
				ret = ufs_aio_pio_cport_direct_read(hba, pBufIn + cAccumulatedDataLength, cDataLength , &cBufLen, 1000, FALSE, 0); //Get Read buffer
			}
			if (ret != UFS_ERR_NONE)
				goto __end;

			UFS_DBG_LOGD("**** Read data 0x%x complete. final 4 byte: 0x%x 0x%x 0x%x 0x%x\n", cBufLen, *(pBufIn+cBufLen-4), *(pBufIn+cBufLen-3), *(pBufIn+cBufLen-2), *(pBufIn+cBufLen-1));

			if (cBufLen != cDataLength)
				goto __end;

			cAccumulatedDataLength += cDataLength;

			UFS_DBG_LOGD("Accumulated data : 0x%x\n", cAccumulatedDataLength);

			if (cAccumulatedDataLength >= blk_cnt * UFS_BLOCK_SIZE) {
				UFS_DBG_LOGD("Read data complete\n");
				break;
			}
		}

		//get next read command
		UFS_DBG_LOGD("**** reading DATA IN UPIU \n");

		ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, UPIU_CMD_BUF_SIZE, &cBufLen, 1000, FALSE, 0); //get ready_tx command
	}

	if (ret != UFS_ERR_NONE)
		goto __end;

	UFS_DBG_LOGD("**** reading Response UPIU status \n");

	//Read the Response UPIU
	ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, UPIU_CMD_BUF_SIZE, &cBufLen, 1000, FALSE, 0); //get status

	if (ret != UFS_ERR_NONE)
		goto __end;

	if ((cmd_device[0] != UPIU_TRANSACTION_RESPONSE) || ((cmd_device[0] == UPIU_TRANSACTION_RESPONSE) && (cmd_device[7] != UPIU_CMD_STATUS_GOOD)))
		goto __end;

__end:

	return ret;
}

int ufs_aio_pio_write(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt, unsigned long * buf)
{
	int ret = UFS_ERR_NONE;
	u32 cBufLen = 0, cAccumulatedDataLength = 0, cDataLength = 0;
	u8 *pBufOut = NULL;

	u8 cmd_write10[32] = {
		0x01, 0x20, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0xA0, 0x00, 0x00,
		0x2A, 0x08, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};

	u8 cmd_data_out_header[32] = {
		0x02, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x40, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x80, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	u8 cmd_device[UPIU_CMD_BUF_SIZE + 6]= {0}; //for ready and status command from Device. 6 additional bytes is for reading overhead of next data

	//Assign LU #
	cmd_write10[2]=lun;
	cmd_data_out_header[2]=lun;

	//modify data length in blocks
	cmd_write10[12] = ((blk_cnt*UFS_BLOCK_SIZE)>>24) & 0xFF; //Expected Data Transfer Length  (4KB block)
	cmd_write10[13] = ((blk_cnt*UFS_BLOCK_SIZE)>>16) & 0xFF; //Expected Data Transfer Length
	cmd_write10[14] = ((blk_cnt*UFS_BLOCK_SIZE)>>8) & 0xFF;; //Expected Data Transfer Length
	cmd_write10[15] = (blk_cnt*UFS_BLOCK_SIZE) & 0xFF; //Expected Data Transfer Length

	//modify logical address
	cmd_write10[18] = (blk_start>>24) & 0xFF; //Logical block address
	cmd_write10[19] = (blk_start>>16) & 0xFF; //Logical block address
	cmd_write10[20] = (blk_start>>8) & 0xFF; //Logical block address
	cmd_write10[21] = (blk_start) & 0xFF; //Logical block address

	//modify Transfer Length
	cmd_write10[23] = (blk_cnt>>8) & 0xFF; //TRANSFER LENGTH
	cmd_write10[24] = (blk_cnt) & 0xFF;    //TRANSFER LENGTH

	//Prepare out buffer
	pBufOut = (u8 *)buf; //(u8 *)malloc(sizeof(u8) * blk_cnt * UFS_BLOCK_SIZE);

	if (!pBufOut) {
		UFS_DBG_LOGD("%s #%d: malloc pBufOut failed.\n", __FUNCTION__, __LINE__);
		return UFS_EXIT_FAILURE;
	}

	UFS_DBG_LOGD("pBufOut address is 0x%x\n", (u32)pBufOut);

	//Send scsi-write10 command to device
	UFS_DBG_LOGD("**** sending write10 of 0x%x blocks\n", blk_cnt);
	ret = ufs_aio_pio_cport_direct_write(hba, cmd_write10, sizeof(cmd_write10), 1, 1000); //send write10

	if (ret != UFS_ERR_NONE) {
		UFS_DBG_LOGD("%s #%d: send write10 failed.\n", __FUNCTION__, __LINE__);
		return UFS_EXIT_FAILURE;
	}

	//Read ready to transfer command from device repeatedly and send data, until read expected data amount
	UFS_DBG_LOGD("**** reading request \n");
	ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, UPIU_CMD_BUF_SIZE, &cBufLen, 1000, FALSE, 0); //get ready_tx command

	//Error handling
	if (cmd_device[0] == UPIU_TRANSACTION_RESPONSE && cmd_device[7] == UPIU_CMD_STATUS_CHECK_COND) {
		//Only read Sense Data when Data Length >0
		if ((cmd_device[10]<<8|cmd_device[11]) > 0) {
			if ((cmd_device[10]<<8|cmd_device[11])%4 != 0)
				ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, ((cmd_device[10]<<8|cmd_device[11])/4+1)*4, &cBufLen, 1000, FALSE, 0); //get query response
			else
				ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, (cmd_device[10]<<8|cmd_device[11]), &cBufLen, 1000, FALSE, 0); //get query response
#ifdef MSG_DEBUG
			if (cBufLen > 0) {
				unsigned long i;
				UFS_DBG_LOGD("get Sense data (byte:%d)\n", cBufLen);
				for (i=0; i<cBufLen; i++) {
					UFS_DBG_LOGD("0x%x ", cmd_device[i]);
				}
				UFS_DBG_LOGD("\n");
			}
#endif
		}
		return UFS_EXIT_FAILURE;
	}

	while ((cBufLen > 0) && (ret == UFS_ERR_NONE) && (cAccumulatedDataLength < blk_cnt * UFS_BLOCK_SIZE)) { //while get ready tx command
		//update the address and length in the write header
		memcpy(cmd_data_out_header+12, cmd_device+12, 8);//copy 8 bytes of data buffer offset, data buffer count
		memcpy(cmd_data_out_header+10, cmd_data_out_header+18, 2);//duplicate data segment with data buffer count
		//UFS_DBG_LOGD("Copy address : 0x%x%x%x%x%\n", *(cmd_device+12), *(cmd_device+13), *(cmd_device+14), *(cmd_device+15));

		cDataLength = (*(cmd_device+16) << 24) | (*(cmd_device+17) << 16) | (*(cmd_device+18) << 8) | (*(cmd_device+19));
		UFS_DBG_LOGD("prepare to send 0x%x bytes data\n", cDataLength);

		if (cDataLength > 0) {
			//Create a buffer and feed in data for writing

			//Send the write header first
			UFS_DBG_LOGD("**** Sending DATAOUT header....\n");
			ret = ufs_aio_pio_cport_direct_write(hba, cmd_data_out_header, sizeof(cmd_data_out_header), 0, 1000); //send data header

			if (ret != UFS_ERR_NONE) {
				UFS_DBG_LOGD("%s #%d: send DATAOUT failed.\n", __FUNCTION__, __LINE__);

				return UFS_EXIT_FAILURE;
			}

			//Then send the buffer
			UFS_DBG_LOGD("**** Sending data of 0x%x bytes starting from 0x%x\n", sizeof(u8) * cDataLength, cAccumulatedDataLength);
			ret = ufs_aio_pio_cport_direct_write(hba, pBufOut + cAccumulatedDataLength, sizeof(u8) * cDataLength, 1, 1000); //send write buffer

			if (ret != UFS_ERR_NONE) {
				UFS_DBG_LOGD("%s #%d: send pBufOut failed.\n", __FUNCTION__, __LINE__);

				return UFS_EXIT_FAILURE;
			}

			cAccumulatedDataLength += cDataLength;
			UFS_DBG_LOGD("Accumulated data : 0x%x\n", cAccumulatedDataLength);

			if (cAccumulatedDataLength >= blk_cnt * UFS_BLOCK_SIZE) {
				UFS_DBG_LOGD("Send data complete\n");
				break;
			}
		}
		//get next read_tx command
		UFS_DBG_LOGD("**** reading next ready-to-transfer request \n");

		//Read the next ready to transfer request
		ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, UPIU_CMD_BUF_SIZE, &cBufLen, 1000, FALSE, 0); //get ready_tx command
	}

	//Get status command from device (the while loop is for resend status if any)
	ret = ufs_aio_pio_cport_direct_read(hba, cmd_device, UPIU_CMD_BUF_SIZE, &cBufLen, 1000, FALSE, 0); //get status

	return ret;
}

int ufs_aio_pio_erase(struct ufs_hba * hba, u32 lun, u32 blk_start, u32 blk_cnt)
{
	return -1;
}

#endif  // UFS_CFG_ENABLE_PIO

int ufs_aio_cfg_callback(struct ufs_hba * hba, u8 mode)
{
	int ret = 0;

	if (UFS_MODE_DMA == mode) {
		hba->blk_read = ufs_aio_dma_read;
		hba->blk_write = ufs_aio_dma_write;
		hba->nopin_nopout = ufs_aio_dma_nopin_nopout;
		hba->query_flag = ufs_aio_dma_query_flag;
		hba->read_descriptor = ufs_aio_dma_read_desc;
		hba->dme_get = ufs_aio_dma_dme_get;
		hba->dme_peer_get = ufs_aio_dma_dme_peer_get;
		hba->dme_set = ufs_aio_dma_dme_set;
#if defined(MTK_UFS_DRV_DA) || defined(MTK_UFS_DRV_LK)
		hba->blk_erase = ufs_aio_dma_erase;
		hba->query_attr = ufs_aio_dma_query_attr;
#endif
#if defined(MTK_UFS_DRV_DA)
		hba->write_descriptor = ufs_aio_dma_write_desc;
#endif
	}
#ifdef UFS_CFG_ENABLE_PIO
	else if (UFS_MODE_PIO == mode) {
		hba->blk_read = ufs_aio_pio_read;
		hba->blk_write = ufs_aio_pio_write;
		hba->nopin_nopout = ufs_aio_pio_nopin_nopout;
		hba->query_flag = ufs_aio_pio_query_flag;
		hba->read_descriptor = ufs_aio_pio_read_desc;
		hba->dme_get = ufs_aio_pio_dme_get;
		hba->dme_peer_get = ufs_aio_pio_dme_peer_get;
		hba->dme_set = ufs_aio_pio_dme_set;
#ifdef MTK_UFS_DRV_DA
		hba->blk_erase = ufs_aio_pio_erase;
		hba->query_attr = ufs_aio_pio_query_attr;
		hba->write_descriptor = ufs_aio_pio_write_desc;
#endif
#if 0
		hba->dme_peer_set = ufs_aio_pio_dme_peer_set;
#endif
	}
#endif
	else {
		UFS_DBG_LOGD("[UFS] err: ufs_aio_cfg_callback - invalid mode\n");
		ret = -1;
	}

	return ret;
}

int ufs_aio_cfg_mode(u32 host_id, u8 mode)
{
	int ret;
	struct ufs_hba * hba = ufs_aio_get_host(host_id);

	if (UFS_MODE_DEFAULT == mode) {
#if defined(UFS_CFG_DEFAULT_DMA)
		hba->mode = UFS_MODE_DMA;
#elif defined(UFS_CFG_DEFAULT_PIO)
		hba->mode = UFS_MODE_PIO;
#else
#error "ufs_aio_cfg_mode: unknown default mode"
#endif

		ret = ufs_aio_cfg_callback(hba, hba->mode);
	} else if (UFS_MODE_MAX > mode) {
		hba->mode = mode;
		ret = ufs_aio_cfg_callback(hba, hba->mode);
	} else {
		UFS_DBG_LOGD("[UFS] err: unknown mode\n");
		ret = -1;
	}

	return ret;
}

