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

#if !defined(__KDUMP_SDHC_H__)
#define __KDUMP_SDHC_H__

#include <stdbool.h>

#define MaxFindFileClusNum 100

typedef enum {
	WRITE_FILE_DIRECT = 0,
	FORMAT_BEF_WRITE = 1
} FileWriteType;


typedef enum FileSysType {
	FAT_16 = 0,
	FAT_32 = 1
} FATType;

typedef struct {
	uint32_t   BPB_BytsPerSec;
	uint32_t   BPB_SecPerClus;
	uint32_t   BPB_RsvdSecCnt;
	uint32_t   BPB_NumFATs;
	uint32_t   BPB_FATSz;
	uint32_t   BPB_RootEntCnt;
	uint32_t   BPB_RootClus;
	uint32_t   BPB_TotSec;
	FATType FileSysType;
	uint32_t   BootStartSec;
	uint32_t   FATStartSec;
	uint32_t   RootDirStartSec;
	uint32_t   ClusStartSec;
} FAT_Para;



typedef struct {
	uint8_t    name[11];            // file name
	uint8_t    attr;                // file attribute bits (system, hidden, etc.)
	uint8_t    NTflags;             // ???
	uint8_t    createdTimeMsec;     // ??? (milliseconds needs 11 bits for 0-2000)
	uint16_t    createdTime;         // time of file creation
	uint16_t    createdDate;         // date of file creation
	uint16_t    lastAccessDate;      // date of last file access
	uint16_t    clusFirstHigh;       // high word of first cluster
	uint16_t    time;                // time of last file change
	uint16_t    date;                // date of last file change
	uint16_t    clusFirst;           // low word of first cluster
	uint32_t   size;                // file size in bytes
} DirEntry;

typedef struct {
	uint8_t    seqNum;              // sequence number
	uint8_t    name1[10];           // name characters (five UTF-16 characters)
	uint8_t    attr;                // attributes (always 0x0F)
	uint8_t    NTflags;             // reserved (alwyas 0x00)
	uint8_t    checksum;            // checksum of DOS file name
	uint8_t    name2[12];           // name characters (six UTF-16 characters)
	uint16_t    clusFirst;           // word of first cluster (always 0x0000)
	uint8_t    name3[4];            // name characters (2 UTF-16 characters)
} LfnEntry;

#define FAT_BUFSIZE 65536

typedef struct {
	uint8_t   FileBuffer[FAT_BUFSIZE];  // File cluster cache, assume maximum cluster size is 64KB
	uint8_t   FATBuffer[512];       // FAT cache
	uint32_t  BufferLen;            // data cached length in FileBuffer
	uint32_t  TotalLen;         // File total length
	uint32_t  PrevClusterNum;       // Prev cluster number
	uint32_t  CurrClusterNum;       // Current cluster number
	uint32_t  FATSector;            // Current FAT sector number
	uint32_t  CheckSum;         // File write content checksum
	bool DiskFull;
} FileHandler;


struct mrdump_dev {
	const char *name;
	void *handle;

	bool (*read)(struct mrdump_dev *dev, uint64_t sector_addr, uint8_t *pdBuf, int32_t blockLen);
	bool (*write)(struct mrdump_dev *dev, uint64_t sector_addr, uint8_t *pdBuf, int32_t blockLen);
};

static inline int mrdump_dev_read(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
	return dev->read(dev, sector_addr, pdBuf, blockLen);
}

static inline int mrdump_dev_write(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
	return dev->write(dev, sector_addr, pdBuf, blockLen);
}

#endif
