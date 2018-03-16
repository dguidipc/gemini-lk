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

#ifndef MMC_TEST_H
#define MMC_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

struct mmc_test_config {
	char           *desc;           /* description */
	int             id;             /* host id to test */
	int             autocmd;        /* auto command */
	int             cmd23_flags;    /* reliable write, force prog, etc */
	int             mode;           /* PIO/DMA mode */
	int             uhsmode;        /* UHS mode */
	int             burstsz;        /* DMA burst size */
	int             piobits;
	unsigned int    flags;          /* DMA flags */
	int             count;          /* repeat counts */
	int             clksrc;         /* clock source */
	unsigned int    clock;          /* clock frequency for testing */
	unsigned int    buswidth;       /* bus width */
	unsigned long   blknr;          /* n'th block number for read/write test */
	unsigned int    total_size;     /* total size to test */
	unsigned int    blksz;          /* block size */
	int             chunk_blks;     /* blocks of chunk */
	char           *buf;            /* read/write buffer */
	unsigned char   chk_result;     /* check write result? */

	unsigned char   tst_single;     /* test single block read/write? */
	unsigned char   tst_multiple;   /* test multiple block read/write? */
	unsigned char   tst_interleave; /* test interleave block read/write ? */
	unsigned char   start_bit;      /* start bit of read data */
	unsigned char   axi_burst_len;  /* axi command length per one burst */
	unsigned char   axi_rd_os;      /* axi read outstanding number */
	unsigned char   axi_wr_os;      /* axi write outstanding number */
};

struct mmc_op_report {
	unsigned long count;        /* the count of this operation */
	unsigned long min_time;     /* the min. time of this operation */
	unsigned long max_time;     /* the max. time of this operation */
	unsigned long total_time;   /* the total time of this operation */
	unsigned long total_size;   /* the total size of this operation */
};
struct mmc_op_perf {
	struct mmc_host      *host;
	struct mmc_card      *card;
	struct mmc_op_report  single_blk_read;
	struct mmc_op_report  single_blk_write;
	struct mmc_op_report  multi_blks_read;
	struct mmc_op_report  multi_blks_write;
};


typedef struct {
	/* register offset */
	u32  offset;

	/* r: read only, w: write only, a:readable & writable
	 * k: read clear, x: don't care, n: do not test
	 * s: readable and write 1 set, c: readable and write 1 clear
	 *
	 * RU:  read only bit value update by the hw, denote as 'r', like MSDC_RXDATA
	 * RO:  read only, denote as 'r', like MSDC_VERSION
	 * RW:  writeable and readable, denote as 'a'
	 * WO:  write only, denote as 'w', like MSDC_TXDATA
	 * W1:  write once, denote as '', not used yet.
	 * RC:  clear on read, denote as 'k', not used yet.
	 * A1:  auto set by the hw.
	 * A0:  auto clear by the hw.
	 * DC:  don't care, denote as 'n' or 'x'
	 * W1C: write 1 to bit wise-clear, denote as 'c', like SDC_CSTS
	 * note: RU means the content of the regiter will change,
	 *       RO means the content of the register will not change
	 */
	char attr[32];

	/* 0: default is 0
	 * 1: default is 1
	 * x: don't care
	 */
	char reset[32];
} reg_desc_t;

typedef void (*mmc_prof_callback)(void *data, ulong id, ulong counts);

#ifdef MMC_PROFILING
extern struct mmc_op_perf *mmc_prof_handle(int id);
extern void mmc_prof_init(int id, struct mmc_host *host, struct mmc_card *card);
extern void mmc_prof_start(void);
extern void mmc_prof_stop(void);
extern unsigned int mmc_prof_count(void);
extern void mmc_prof_update(mmc_prof_callback cb, ulong id, void *data);
extern void mmc_prof_report(struct mmc_op_report *rpt);
extern int mmc_prof_dump(int dev_id);
#else
#define mmc_prof_handle(i)      NULL
#define mmc_prof_init(i,h,c)    do{}while(0)
#define mmc_prof_count()        0
#define mmc_prof_start()        do{}while(0)
#define mmc_prof_stop()         do{}while(0)
#define mmc_prof_update(c,i,d)  do{}while(0)
#define mmc_prof_report(rpt)    do{}while(0)
#define mmc_prof_dump(id)       do{}while(0)
#endif

#ifdef MMC_TEST
extern int msdc_reg_test(struct mmc_host *host, int id, int clock_gate_test);
extern int mmc_test(int argc, char *argv[]);
#else
#define mmc_test(c,v)           0
#endif

#ifdef MTK_EMMC_POWER_ON_WP
extern int mmc_wp_test();
#endif

#ifdef __cplusplus
}
#endif

#endif /* MMC_TEST_H */

