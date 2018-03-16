/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "msdc_cfg.h" //Include msdc_cfg.h for defining MMC_MSDC_DRV_PRELOADER

#if !defined(MMC_MSDC_DRV_CTP) // CTP no need this file
#include "addr_trans.h"
#include "msdc.h"
#include "block_generic_interface.h"
#define MMC_HOST_ID                 0

#define BUF_BLK_NUM                 4   /* 4 * 512bytes = 2KB */

/**************************************************************************
*  DEBUG CONTROL
**************************************************************************/

/**************************************************************************
*  MACRO DEFINITION
**************************************************************************/

/**************************************************************************
*  EXTERNAL DECLARATION
**************************************************************************/
static addr_trans_info_t g_emmc_addr_trans[EMMC_PART_END];
static addr_trans_tbl_t g_addr_trans_tbl;
int mmc_do_erase(int dev_num,u64 start_addr,u64 len,u32 part_id);

#if defined(MMC_MSDC_DRV_PRELOADER)
extern struct nand_chip g_nand_chip;
static blkdev_t g_mmc_bdev;
#endif

#if defined(MMC_MSDC_DRV_LK)
int g_user_virt_addr=0;
#endif
u64 g_emmc_size = 0;
u64 g_emmc_user_size = 0;

int mmc_switch_part(u32 part_id)
{
	int err = MMC_ERR_NONE;
	struct mmc_card *card;
	struct mmc_host *host;
	u8 part = (u8)part_id;
	u8 cfg;
	u8 *ext_csd;

	card = mmc_get_card(MMC_HOST_ID);
	host = mmc_get_host(MMC_HOST_ID);

	if (!card)
		return MMC_ERR_INVALID;

	ext_csd = &card->raw_ext_csd[0];

	if (mmc_card_mmc(card) && ext_csd[EXT_CSD_REV] >= 3) {
#ifdef MTK_EMMC_SUPPORT
		if (part_id == EMMC_PART_USER)
			part = EXT_CSD_PART_CFG_DEFT_PART;
		else if (part_id == EMMC_PART_BOOT1)
			part = EXT_CSD_PART_CFG_BOOT_PART_1;
		else if (part_id == EMMC_PART_BOOT2)
			part = EXT_CSD_PART_CFG_BOOT_PART_2;

		cfg = card->raw_ext_csd[EXT_CSD_PART_CFG];

		/* already set to specific partition */
		if (part == (cfg & 0x7))
			return MMC_ERR_NONE;

		cfg = (cfg & ~0x7) | part;

		err = mmc_switch(host, card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CFG, cfg);

		if (err == MMC_ERR_NONE) {
			err = mmc_read_ext_csd(host, card);
			if (err == MMC_ERR_NONE) {
				ext_csd = &card->raw_ext_csd[0];
				if (ext_csd[EXT_CSD_PART_CFG] != cfg)
					err = MMC_ERR_FAILED;
			}
		}
#endif
	}

	if (err)
		printf("[%s]: failed to switch part_id:%d, part:%d, err=%d\n", __func__, part_id, part, err);

	return err;
}

#if defined(MMC_MSDC_DRV_PRELOADER)
static int mmc_addr_trans_tbl_init(struct mmc_card *card, blkdev_t *bdev)
#elif defined(MMC_MSDC_DRV_LK)
static int mmc_addr_trans_tbl_init(struct mmc_card *card)
#endif
{
	u32 wpg_sz;
	u8 *ext_csd;

	memset(&g_addr_trans_tbl, 0, sizeof(addr_trans_tbl_t));

	ext_csd = &card->raw_ext_csd[0];

#if defined(MMC_MSDC_DRV_PRELOADER)
	bdev->offset = 0;
#endif

	if (mmc_card_mmc(card) && ext_csd[EXT_CSD_REV] >= 3) {
		u64 size[EMMC_PART_END];
		u32 i;

		if ((ext_csd[EXT_CSD_ERASE_GRP_DEF] & EXT_CSD_ERASE_GRP_DEF_EN)
		        && (ext_csd[EXT_CSD_HC_WP_GPR_SIZE] > 0)) {
			wpg_sz = 512 * 1024 * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] *
			         ext_csd[EXT_CSD_HC_WP_GPR_SIZE];
		} else {
			wpg_sz = card->csd.write_prot_grpsz;
		}

		size[EMMC_PART_UNKNOWN] = 0;
		size[EMMC_PART_BOOT1] = ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024;
		size[EMMC_PART_BOOT2] = ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024;
		size[EMMC_PART_RPMB]  = ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128 * 1024;
		size[EMMC_PART_GP1]   = ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] * 256 * 256 +
		                        ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] * 256 +
		                        ext_csd[EXT_CSD_GP1_SIZE_MULT + 0];
		size[EMMC_PART_GP2]   = ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] * 256 * 256 +
		                        ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] * 256 +
		                        ext_csd[EXT_CSD_GP2_SIZE_MULT + 0];
		size[EMMC_PART_GP3]   = ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] * 256 * 256 +
		                        ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] * 256 +
		                        ext_csd[EXT_CSD_GP3_SIZE_MULT + 0];
		size[EMMC_PART_GP4]   = ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] * 256 * 256 +
		                        ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] * 256 +
		                        ext_csd[EXT_CSD_GP4_SIZE_MULT + 0];
		size[EMMC_PART_USER]  = (u64)card->blklen * card->nblks;

		size[EMMC_PART_GP1] *= wpg_sz;
		size[EMMC_PART_GP2] *= wpg_sz;
		size[EMMC_PART_GP3] *= wpg_sz;
		size[EMMC_PART_GP4] *= wpg_sz;

		for (i = EMMC_PART_BOOT1; i < EMMC_PART_END; i++) {
			g_emmc_addr_trans[i].id  = i;
			g_emmc_addr_trans[i].len = size[i] / 512; /* in 512B unit */
#if defined(MMC_MSDC_DRV_LK)
			if (i<EMMC_PART_USER)
				g_user_virt_addr += size[i];
#endif
			g_emmc_size += size[i];
		}

		/* determine user area offset */
#if defined(MMC_MSDC_DRV_PRELOADER)
		for (i = EMMC_PART_BOOT1; i < EMMC_PART_USER; i++) {
			bdev->offset += size[i];
		}
		bdev->offset /= bdev->blksz; /* in blksz unit */
#endif

		g_emmc_user_size = size[EMMC_PART_USER];
		g_addr_trans_tbl.num  = EMMC_PART_END;
		g_addr_trans_tbl.info = &g_emmc_addr_trans[0];
	} else {
		g_addr_trans_tbl.num  = 0;
		g_addr_trans_tbl.info = NULL;
	}

	return 0;
}

int mmc_get_boot_part(u32 *bootpart)
{
	struct mmc_card *card;
	struct mmc_host *host;
	int err = MMC_ERR_NONE;
	u8 *ext_csd;
	u8 part_config;

	if (!bootpart)
		return -1;

	card = mmc_get_card(MMC_HOST_ID);
	host = mmc_get_host(MMC_HOST_ID);

	if (!card || !host)
		return -1;
	err = mmc_read_ext_csd(host, card);

	if (err == MMC_ERR_NONE) {
		ext_csd = &card->raw_ext_csd[0];

		part_config = (ext_csd[EXT_CSD_PART_CFG] >> 3) & 0x07;
		if (part_config == 1) {
			*bootpart = EMMC_PART_BOOT1;
		} else if (part_config == 2) {
			*bootpart = EMMC_PART_BOOT2;
		} else if (part_config == 7) {
			*bootpart = EMMC_PART_USER;
		} else {
			printf("[PL MTK] fail to get boot part\n");
			return -1;
		}
	}

	return err;
}

#if defined(MMC_MSDC_DRV_PRELOADER)
static int mmc_bread(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u32 part_id)
{
	struct mmc_host *host = (struct mmc_host*)bdev->priv;

	mmc_switch_part(part_id);
	return mmc_block_read(host->id, (unsigned long)blknr, blks, (unsigned long*)buf);
}

static int mmc_bwrite(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf, u32 part_id)
{
	struct mmc_host *host = (struct mmc_host*)bdev->priv;

	mmc_switch_part(part_id);
	return mmc_block_write(host->id, (unsigned long)blknr, blks, (unsigned long*)buf);
}

// ==========================================================
// MMC Common Interface - Init
// ==========================================================
u32 mmc_init_device(void)
{
	int ret;
	struct mmc_card *card;

	if (!blkdev_get(BOOTDEV_SDMMC)) {

		ret = mmc_init(MMC_HOST_ID, MSDC_MODE_DEFAULT);
		if (ret != 0) {
			printf("[SD0] init card failed\n");
			return ret;
		}

		memset(&g_mmc_bdev, 0, sizeof(blkdev_t));

		card = mmc_get_card(MMC_HOST_ID);

		g_mmc_bdev.blksz   = MMC_BLOCK_SIZE;
		g_mmc_bdev.erasesz = MMC_BLOCK_SIZE;
		g_mmc_bdev.blks    = card->nblks;
		g_mmc_bdev.bread   = mmc_bread;
		g_mmc_bdev.bwrite  = mmc_bwrite;
		g_mmc_bdev.type    = BOOTDEV_SDMMC;
		g_mmc_bdev.blkbuf  = NULL;
		g_mmc_bdev.priv    = (void*)mmc_get_host(MMC_HOST_ID);

		mmc_addr_trans_tbl_init(card, &g_mmc_bdev);

		blkdev_register(&g_mmc_bdev);
	}
	return 0;
}

u32 mmc_get_device_id(u8 *id, u32 len,u32 *fw_len)
{
	u8 buf[16]; /* CID = 128 bits */
	struct mmc_card *card;
	u8 buf_sanid[512];
	u8 i;

	if (0 != mmc_init_device())
		return -1;

	card = mmc_get_card(MMC_HOST_ID);

	buf[0] = (card->raw_cid[0] >> 24) & 0xFF; /* Manufacturer ID */
	buf[1] = (card->raw_cid[0] >> 16) & 0xFF; /* Reserved(6)+Card/BGA(2) */
	buf[2] = (card->raw_cid[0] >> 8 ) & 0xFF; /* OEM/Application ID */
	buf[3] = (card->raw_cid[0] >> 0 ) & 0xFF; /* Product name [0] */
	buf[4] = (card->raw_cid[1] >> 24) & 0xFF; /* Product name [1] */
	buf[5] = (card->raw_cid[1] >> 16) & 0xFF; /* Product name [2] */
	buf[6] = (card->raw_cid[1] >> 8 ) & 0xFF; /* Product name [3] */
	buf[7] = (card->raw_cid[1] >> 0 ) & 0xFF; /* Product name [4] */
	buf[8] = (card->raw_cid[2] >> 24) & 0xFF; /* Product name [5] */
	buf[9] = (card->raw_cid[2] >> 16) & 0xFF; /* Product revision */
	buf[10] =(card->raw_cid[2] >> 8 ) & 0xFF; /* Serial Number [0] */
	buf[11] =(card->raw_cid[2] >> 0 ) & 0xFF; /* Serial Number [1] */
	buf[12] =(card->raw_cid[3] >> 24) & 0xFF; /* Serial Number [2] */
	buf[13] =(card->raw_cid[3] >> 16) & 0xFF; /* Serial Number [3] */
	buf[14] =(card->raw_cid[3] >> 8 ) & 0xFF; /* Manufacturer date */
	buf[15] =(card->raw_cid[3] >> 0 ) & 0xFF; /* CRC7 + stuff bit*/
	*fw_len = 1;

	/* sandisk */
	if (buf[0] == 0x45) {
		/* before v4.41 */
		if (card->raw_ext_csd[EXT_CSD_REV] <= 5) {
			if (0 == mmc_get_sandisk_fwid(MMC_HOST_ID,buf_sanid)) {
				*fw_len = 6;
			}
		} else if (card->raw_ext_csd[EXT_CSD_REV] == 6) { /* v4.5, v4.51 */
			/* do nothing, same as other vendor */
		} else if (card->raw_ext_csd[EXT_CSD_REV] == 7) { /* v5.0 */
			/* sandisk only use 6 bytes */
			*fw_len = 6;
			for (i==0; i<6; i++) {
				buf_sanid[32+i] = card->raw_ext_csd[EXT_CSD_FIRMWARE_VERSION+i];
			}
		}
	}

	len = len > 16 ? 16 : len;
	memcpy(id, buf, len);
	if (*fw_len == 6)
		memcpy(id+len,buf_sanid+32,6);
	else
		*(id+len) = buf[9];
	return 0;
}

int mmc_bread_boot(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf)
{
	u32 pid, id;
	struct mmc_host *host;

	if (bdev && bdev->priv) {
		host = (struct mmc_host*)bdev->priv;
		id = host->id;
	} else {
		printf("[%s]: invalid bdev, force the host id to 0\n", __func__, pid);
		id = 0;
	}

	if (mmc_get_boot_part(&pid)) {
		printf("[%s]: invalid pid=%d\n", __func__, pid);
		return -1;
	}

	mmc_switch_part(pid);

	return mmc_block_read(id, (unsigned long)blknr, blks, (unsigned long*)buf);
}

int mmc_bwrite_boot(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf)
{
	u32 pid, id;
	struct mmc_host *host;

	if (bdev && bdev->priv) {
		host = (struct mmc_host*)bdev->priv;
		id = host->id;
	} else {
		id = 0;
	}

	if (mmc_get_boot_part(&pid)) {
		printf("[%s]: invalid pid=%d\n", __func__, pid);
		return -1;
	}

	mmc_switch_part(pid);

	return mmc_block_write(id, (unsigned long)blknr, blks, (unsigned long*)buf);
}

#endif //#if defined(MMC_MSDC_DRV_PRELOADER)


#if defined(MMC_MSDC_DRV_LK)
#include "part_dev.h"
static block_dev_desc_t sd_dev[MSDC_MAX_NUM];
static int boot_dev_found = 0;
static part_dev_t boot_dev;

unsigned long mmc_wrap_bread(int dev_num, unsigned long blknr, lbaint_t blkcnt, void *dst, unsigned int part_id)
{
	mmc_switch_part(part_id);
	return mmc_block_read(dev_num, blknr, blkcnt, (unsigned long *)dst) == MMC_ERR_NONE ? blkcnt : (unsigned long) -1;
}
unsigned long mmc_wrap_bwrite(int dev_num, unsigned long blknr, lbaint_t blkcnt, const void *src, unsigned int part_id)
{
	mmc_switch_part(part_id);
	return mmc_block_write(dev_num, blknr, blkcnt, (unsigned long *)src) == MMC_ERR_NONE ? blkcnt : (unsigned long) -1;
}

int mmc_legacy_init(int verbose)
{
	int id = verbose - 1;
	int err = MMC_ERR_NONE;
	struct mmc_host *host;
	struct mmc_card *card;
	block_dev_desc_t *bdev;

	bdev = &sd_dev[id];

	//msdc_hard_reset(host);

	err = mmc_init(id, MSDC_MODE_DEFAULT);

	if (err == MMC_ERR_NONE && !boot_dev_found) {
		/* fill in device description */
		card=mmc_get_card(id);
		host=mmc_get_host(id);
		mmc_addr_trans_tbl_init(card);

		bdev->dev         = id;
		bdev->blksz       = MMC_BLOCK_SIZE;
		bdev->lba         = card->nblks * card->blklen / MMC_BLOCK_SIZE;
		bdev->blk_bits    = 9;
		bdev->part_boot1  = EMMC_PART_BOOT1;
		bdev->part_boot2  = EMMC_PART_BOOT2;
		bdev->part_user   = EMMC_PART_USER;
		bdev->block_read  = mmc_wrap_bread;
		bdev->block_write = mmc_wrap_bwrite;

#if defined(MEM_PRESERVED_MODE_ENABLE)
		if ( id==1 ) {
			host->boot_type = NON_BOOTABLE;
			return err;
		}
#endif

		host->boot_type   = RAW_BOOT;

		/* FIXME. only one RAW_BOOT dev */
		if (host->boot_type == RAW_BOOT) {
			boot_dev.id = id;
			boot_dev.init = 1;
			boot_dev.blkdev = bdev;
			boot_dev.erase = mmc_do_erase;
			mt_part_register_device(&boot_dev);
			boot_dev_found = 1;
			printf("[SD%d] boot device found\n", id);
		} else if (host->boot_type == FAT_BOOT) {
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
			if (0 == fat_register_device(bdev, 1)) {
				boot_dev_found = 1;
				printf("[SD%d] FAT partition found\n", id);
			}
#endif
		}
	}

	return err;
}

// ==========================================================
// MMC Common Interface - Erase
// ==========================================================
static int __mmc_do_erase(struct mmc_host *host,struct mmc_card *card, u64 start_addr, u64 len)
{
	int err = MMC_ERR_NONE;
	u64 end_addr =((start_addr + len + card->blklen - 1)/card->blklen - 1) * card->blklen;
	u8 buf[512];

	if (end_addr/card->blklen > card->nblks - 1) {
		printf("[MSDC%d]Erase address out of range! start<0x%llx>,len<0x%llx>,card_nblks<0x%x>\n",host->id, start_addr, len, card->nblks);
		return MMC_ERR_INVALID;
	}
resend:
	/* Add dummy read to reset erase sequence error. */
	err = mmc_block_read(host->id, 0, 1, (unsigned long*)buf);
	if (err != MMC_ERR_NONE) {
		printf("[MSDC%d]Fail in read card\n", host->id);
		goto out;
	}

	err = mmc_erase_start(card, start_addr);
	if (err != MMC_ERR_NONE) {
		printf("[MSDC%d]Set erase start addrees 0x%llx failed,Err<%d>\n", host->id, start_addr, err);

		if (err == MMC_ERR_ERASE_SEQ) {
			printf("[MSDC%d]Erase sequence error, retry erase.\n", host->id);
			goto resend;
		} else {
			goto out;
		}
	}

	err = mmc_erase_end(card, end_addr);
	if (err != MMC_ERR_NONE) {
		printf("[MSDC%d]Set erase end addrees 0x%llx + 0x%llx failed,Err<%d>\n", host->id, start_addr, len, err);

		if (err == MMC_ERR_ERASE_SEQ) {
			printf("[MSDC%d]Erase sequence error, retry erase.\n", host->id);
			goto resend;
		} else {
			goto out;
		}
	}

	err = mmc_erase(card, MMC_ERASE_TRIM);
	if (err != MMC_ERR_NONE) {
		printf("[MSDC%d]Set erase <0x%llx - 0x%llx> failed,Err<%d>\n", host->id, start_addr, start_addr + len, err);
		goto out;
	}

	printf("[MSDC%d]0x%llx - 0x%llx Erased\n", host->id, start_addr, start_addr + len);
out:
	return err;
}

int mmc_do_erase(int dev_num,u64 start_addr,u64 len,u32 part_id)
{
	struct mmc_host *host = mmc_get_host(dev_num);
	struct mmc_card *card = mmc_get_card(dev_num);
	//struct mmc_erase_part erase_part[EMMC_PART_END];
#if 0
	u32 s_blknr = 0;
	u32 e_blknr = 0;
	u32 s_pid,s_pid_o,e_pid;
#endif
	u32 err;
	if ((!card) || (!host) ) {
		printf("[mmc_do_erase] card or host is NULL\n");
		return MMC_ERR_INVALID;
	}

	if (!len ) {
		printf("[MSDC%d] invalid erase size! len<0x%llx>\n",host->id,len);
		return MMC_ERR_INVALID;
	}
	if ((start_addr % card->blklen) || (len % card->blklen)) {
		printf("[MSDC%d] non-alignment erase address! start<0x%llx>,len<0x%llx>,card_nblks<0x%x>\n",host->id,start_addr,len,card->nblks);
		return MMC_ERR_INVALID;
	}



	if ((err = mmc_switch_part(part_id))) {
		printf("[MSDC%d] mmc swtich failed.part<%d> error <%d> \n", host->id, part_id, err);
		return err;
	}

	if ((err = __mmc_do_erase(host, card, start_addr, len))) {
		printf("[MSDC%d] mmc erase failed.error <%d> \n",host->id, err);
		return err;
	}

	return err;

}

#endif // #if defined(MMC_MSDC_DRV_LK)

#endif //!defined(MMC_MSDC_DRV_CTP)
