/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2011
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifndef _SEC_H_
#define _SEC_H_

#include <platform/mt_typedefs.h>
#include <platform/sec_status.h>

/* exported functions */
extern int security_preprocess(void **data, unsigned *size, unsigned offset, int dev_type);
extern int security_postprocess(int sec_part_index);
extern int security_init(void **data, unsigned *size, int dev_type, int sec_part_index);
extern int security_deinit(void);
/*---------------------------------------------------------------------------*/
extern int install_sig(void *data, unsigned size);
extern int get_chunk_size(unsigned int *chunk_size);
extern int get_img_name(unsigned char *img_name, unsigned int len);
/*---------------------------------------------------------------------------*/
/* API for controlling debug ports                                           */
/*---------------------------------------------------------------------------*/
/* flags returned by sec_dbgport_read_hw_config*/
#define DBGPORT_HW_JTAG_DIS     0x0001
#define DBGPORT_SW_JTAG_CON     0x0002
#define DBGPORT_BROM_LOCK_DIS   0x0004
/*---------------------------------------------------------------------------*/
extern U32 sec_dbgport_read_hw_config(void);
extern void sec_dbgport_enable(bool bSecDebugEnable);
extern void sec_dbgport_disable(void);
extern void sec_dbgport_lock(void);
extern void sec_dbgport_dump(void);
extern bool sec_dbgport_test(bool dbg_en, bool sec_dbg_en);
/*---------------------------------------------------------------------------*/

/*device information*/
#define ATAG_MASP_DATA         0x41000866

#define NUM_SBC_PUBK_HASH           8
#define NUM_CRYPTO_SEED          16
#define NUM_RID 4

struct tag_masp_data {
	unsigned int rom_info_sbc_attr;
	unsigned int rom_info_sdl_attr;
	unsigned int hw_sbcen;
	unsigned int lock_state;
	unsigned int rid[NUM_RID];
	/*rom_info.m_SEC_KEY.crypto_seed*/
	unsigned char crypto_seed[NUM_CRYPTO_SEED];
	unsigned int sbc_pubk_hash[NUM_SBC_PUBK_HASH];
};
#endif /* _SEC_H_ */
