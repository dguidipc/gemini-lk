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

#include "blockheader.h"
#include <string.h>

#define ID_NUM1   (12)
#define ID_NUM2   (8)
#define MAX_BL_NUM   (8)


#define SDMMC_HEADER_IDENTIFIER ("EMMC_BOOT")
#define SDMMC_HEADER_VERSION    (1)
#define SDMMC_HEADER_SIZE       (0x200)

#define BLOCK_FILL_VALUE        (0x00)
#define EXTERNAL_HEADER_FILL_VALUE  (0xFF)

// For preloader header
#define MAGIC_VER                         (0x014D4D4D)
#define BRLYT_ID                          ("BRLYT")
#define BRLYT_VER                         (1)
#define BL_EXIT_MAGIC                     (0x42424242)

#define EXT_PRELOADER_MT6582   0x8A000000



typedef enum {
	GFH_FILE_INFO = 0
	,GFH_BL_INFO
} GFH_TYPE;

typedef struct {
	unsigned int magic;
	unsigned short size;
	unsigned short type;
} GFH_Header, *PGFH_Header;

typedef struct {
	GFH_Header m_gfh_hdr;
	char id[ID_NUM1];
	unsigned int file_version;
	unsigned short file_type;
	char flash_dev;
	char sig_type;
	unsigned int load_addr;
	unsigned int length;
	unsigned int max_size;
	unsigned int content_offset;
	unsigned int sig_length;
	unsigned int jump_offset;
	unsigned int addr;
} GFH_FILE_INFO_V1, *PGFH_FILE_INFO_V1;

typedef struct {
	GFH_Header m_gfh_hdr;
	unsigned int m_bl_attr;
} GFH_BL_INFO_V1, *PGFH_BL_INFO_V1;

typedef struct {
	unsigned int   magic;
	char    dev;
	unsigned short   type;
	unsigned int   begin_dev_addr;
	unsigned int   boundary_dev_addr;
	unsigned int   attribute;
} BLDescriptor, PBLDescriptor;


// For header block, first 0x200 bytes.
typedef struct {
	char id[ID_NUM1];
	unsigned int version;
	unsigned int record_region_size;
} SDMMC_HEADER;

// For header block, second 0x200 bytes.
typedef struct {
	char id[ID_NUM2];
	unsigned int version;
	unsigned int boot_region_dev_addr;
	unsigned int main_region_dev_addr;
	BLDescriptor descriptor[MAX_BL_NUM];
} MtkBRLayout;


unsigned int handle_layout_header(char* data, unsigned int sz, char* cache);
unsigned int handle_preloader_gfh(PGFH_Header gfh_hdr, MtkBRLayout* mtk_layout);
unsigned int handle_preloader_ext_gfh(PGFH_Header gfh_hdr, MtkBRLayout* mtk_layout);

/*
block header layout
============================== offset: 0x000
SDMMC_HEADER

============================== offset: 0x200
MtkBRLayout #1

MtkBRLayout #2

MtkBRLayout #3

....

....

MtkBRLayout #8
============================== offset: 0x800
*/
int process_preloader (char* data, /*IN OUT*/unsigned int* psz)
{
	char* cache = data + 0x1000000; // offset 16M for buffer.

	PGFH_Header gfh_hdr = (PGFH_Header)data;
	if (MAGIC_VER != gfh_hdr->magic) {
		return -1;
	}

	memset(cache, 0xFF, HEADER_BLOCK_SIZE/2);
	memset(cache + HEADER_BLOCK_SIZE/2, 0x00, HEADER_BLOCK_SIZE/2);

	SDMMC_HEADER* mmcHdr = (SDMMC_HEADER*)cache;
	memset(mmcHdr, 0, sizeof(SDMMC_HEADER));

	memcpy(mmcHdr->id, SDMMC_HEADER_IDENTIFIER, strlen(SDMMC_HEADER_IDENTIFIER));
	mmcHdr->version = SDMMC_HEADER_VERSION;
	mmcHdr->record_region_size = SDMMC_HEADER_SIZE;

	memcpy(cache+HEADER_BLOCK_SIZE, data, *psz);

	*psz = handle_layout_header(data, *psz, cache);

	memcpy(data, cache, *psz);

	return 0;
}

/*data: orign data
sz: data size
cache: new data cache. will fill data in it.
return :length: new data length.
*/
unsigned int handle_layout_header(char* data, unsigned int sz, char* cache)
{
	unsigned int preloader_max_size = 0;
	char* ext_file_ptr = 0;
	unsigned int ext_file_length = 0;
	bool has_ext_file = false;
	unsigned length = 0;
	bool do_detect_ext_file = true;

	MtkBRLayout* mtk_layout = (MtkBRLayout*)(cache + SDMMC_HEADER_SIZE);
	memset(mtk_layout, 0, sizeof(MtkBRLayout));

	memcpy(mtk_layout->id, BRLYT_ID, strlen(BRLYT_ID));
	mtk_layout->version = BRLYT_VER;
	mtk_layout->boot_region_dev_addr = HEADER_BLOCK_SIZE;

	PGFH_Header gfh_hdr = (PGFH_Header)data;

	//Scan preloader info.
	//will have more than one GFH_Header.
	preloader_max_size = handle_preloader_gfh(gfh_hdr, mtk_layout);

	//need re-locate.
	mtk_layout->main_region_dev_addr = mtk_layout->descriptor[0].boundary_dev_addr;

	memset(cache+sz+HEADER_BLOCK_SIZE, 0xFF, preloader_max_size-sz); //fill 0xFF between gap.
	length += (preloader_max_size-0x800);    // + HEADER_BLOCK_SIZE;

	if (do_detect_ext_file) {
		//=========process ROM(EXT) File=========
		// ROM FILE(or EXT) is at the end of preloader, so we scan for it.
		//(if this block is exist, copy this block after preloader).

		//process rom file will access data behide this pointer.
		//now gfh_hdr point to preloader mid data.
		gfh_hdr = gfh_hdr;

		for (; (unsigned int)gfh_hdr < (unsigned int)(data+sz); gfh_hdr=(PGFH_Header)((char*)gfh_hdr+4)) { //scan to file end.
			if (gfh_hdr->magic == MAGIC_VER
			        && gfh_hdr->size == sizeof(GFH_FILE_INFO_V1)
			        && gfh_hdr->type == GFH_FILE_INFO) {
				PGFH_FILE_INFO_V1 gfh_file_info_v1 = (PGFH_FILE_INFO_V1)gfh_hdr;

				//ROM file maybe have more than 1 ROM blocks.
				if (EXT_PRELOADER_MT6582 == gfh_file_info_v1->file_version) {
					has_ext_file = true;
					ext_file_ptr = (char*)gfh_hdr;
					ext_file_length = handle_preloader_ext_gfh(gfh_hdr, mtk_layout);

					//final re-locate.
					mtk_layout->main_region_dev_addr = mtk_layout->descriptor[1].boundary_dev_addr;
					break;
				}//end if
			}
		}//end for
		if (has_ext_file) {
			memcpy(cache+HEADER_BLOCK_SIZE+preloader_max_size, ext_file_ptr, ext_file_length);
			length += ext_file_length;
		}
	}
	return length;
}

unsigned int handle_preloader_gfh(PGFH_Header gfh_hdr, MtkBRLayout* mtk_layout)
{
	PGFH_FILE_INFO_V1 gfh_file_info_v1;
	PGFH_BL_INFO_V1 gfh_bl_info_v1;
	unsigned int preloader_max_size = 0;

	//will have more than one GFH_Header.
	while (MAGIC_VER == gfh_hdr->magic) {
		switch (gfh_hdr->type) {
			case GFH_FILE_INFO:
				gfh_file_info_v1 = (PGFH_FILE_INFO_V1)gfh_hdr;

				mtk_layout->descriptor[0].magic           = BL_EXIT_MAGIC;
				mtk_layout->descriptor[0].dev                   = gfh_file_info_v1->flash_dev;
				mtk_layout->descriptor[0].type                  = gfh_file_info_v1->file_type;
				mtk_layout->descriptor[0].begin_dev_addr        = HEADER_BLOCK_SIZE;
				mtk_layout->descriptor[0].boundary_dev_addr     = mtk_layout->descriptor[0].begin_dev_addr + gfh_file_info_v1->max_size;
				preloader_max_size = gfh_file_info_v1->max_size;
				//move next Header.
				gfh_hdr = (PGFH_Header)((char*)gfh_hdr + sizeof(GFH_FILE_INFO_V1));
				break;

			case GFH_BL_INFO:
				gfh_bl_info_v1 = (PGFH_BL_INFO_V1)gfh_hdr;
				mtk_layout->descriptor[0].attribute = gfh_bl_info_v1->m_bl_attr;
				//move next Header.
				gfh_hdr = (PGFH_Header)((char*)gfh_hdr + sizeof(GFH_BL_INFO_V1));
				break;

			default:
				gfh_hdr = (PGFH_Header)((char*)gfh_hdr + sizeof(GFH_Header));
				break;
		}
	}

	return preloader_max_size;
}

unsigned int handle_preloader_ext_gfh(PGFH_Header gfh_hdr, MtkBRLayout* mtk_layout)
{
	PGFH_FILE_INFO_V1 gfh_file_info_v1;
	PGFH_BL_INFO_V1 gfh_bl_info_v1;
	unsigned int ext_file_length = 0;

	while (MAGIC_VER == gfh_hdr->magic) {
		switch (gfh_hdr->type) {
			case GFH_FILE_INFO:
				gfh_file_info_v1 = (PGFH_FILE_INFO_V1)gfh_hdr;
				mtk_layout->descriptor[1].magic           = BL_EXIT_MAGIC;
				mtk_layout->descriptor[1].dev                   = mtk_layout->descriptor[0].dev;
				mtk_layout->descriptor[1].type                  = gfh_file_info_v1->file_type;
				mtk_layout->descriptor[1].begin_dev_addr        = mtk_layout->descriptor[0].boundary_dev_addr;
				mtk_layout->descriptor[1].boundary_dev_addr     = mtk_layout->descriptor[1].begin_dev_addr + gfh_file_info_v1->max_size;
				ext_file_length = gfh_file_info_v1->length;
				//ext_file_max_size = gfh_file_info_v1->max_size;
				//move next Header.
				gfh_hdr = (PGFH_Header)((char*)gfh_hdr + sizeof(GFH_FILE_INFO_V1));
				break;

			case GFH_BL_INFO:
				gfh_bl_info_v1 = (PGFH_BL_INFO_V1)gfh_hdr;
				mtk_layout->descriptor[1].attribute = gfh_bl_info_v1->m_bl_attr;
				//move next Header.
				gfh_hdr = (PGFH_Header)((char*)gfh_hdr + sizeof(GFH_BL_INFO_V1));
				break;

			default:
				gfh_hdr = (PGFH_Header)((char*)gfh_hdr + sizeof(GFH_Header));
				break;
		}
	}//end while

	return ext_file_length;
}




