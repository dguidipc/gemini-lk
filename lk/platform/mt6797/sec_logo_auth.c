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

#include <platform/sec_status.h>
#include <platform/mt_typedefs.h>
#include <debug.h>
#include "sec_logo_auth.h"

#define MOD                             "SBC"

int sec_logo_check()
{
	int ret = B_OK;
	unsigned int policy_entry_idx = 0;
	unsigned int img_auth_required = 0;
	U32 single_verify_time = 0;

	dprintf(CRITICAL,"[%s] Enter logo check \n", MOD);

	seclib_image_buf_init();

	single_verify_time = get_timer(0);


	policy_entry_idx = get_policy_entry_idx("logo");
	img_auth_required = get_vfy_policy(policy_entry_idx);

	if (img_auth_required) {
		ret = sec_img_auth_init(g_secure_part_name[SBOOT_PART_LOGO].sb_name, g_secure_part_name[SBOOT_PART_LOGO].sb_name);
		if (B_OK != ret)
			goto _fail;

		ret = sec_img_auth_stor(g_secure_part_name[SBOOT_PART_LOGO].sb_name, g_secure_part_name[SBOOT_PART_LOGO].sb_name);
		if (B_OK != ret)
			goto _fail;
	}

	dprintf(CRITICAL,"[%s] Consume (%d) ms\n", MOD, get_timer(single_verify_time));

	// free buffer memory
	seclib_image_buf_free();

	return B_OK;

_fail:
	dprintf(CRITICAL,"[%s] Fail (0x%x)\n",MOD,ret);
	return ret;
}
