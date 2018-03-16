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

#if !defined(__MRDUMP_ELF64_H__)
#define __MRDUMP_ELF64_H__

#include <stdint.h>
#include "mrdump_elf_common.h"

#define ELF_NGREGS 34

typedef uint64_t elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREGS];

#define NT_MRDUMP_MACHDESC 0xAEE0

typedef struct elf64_hdr {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_ehdr;

typedef struct elf64_phdr {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Addr p_paddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
} Elf64_phdr;

typedef struct elf64_note {
	Elf64_Word    n_namesz;   /* Name size */
	Elf64_Word    n_descsz;   /* Content size */
	Elf64_Word    n_type;     /* Content type */
} Elf64_nhdr;

struct elf_timeval64 {
	int64_t tv_sec;
	int64_t tv_usec;
};

struct elf_prstatus64 {
	struct elf_siginfo pr_info;
	short pr_cursig;
	uint64_t pr_sigpend;
	uint64_t pr_sighold;

	int32_t pr_pid;
	int32_t pr_ppid;
	int32_t pr_pgrp;

	int32_t pr_sid;
	struct elf_timeval64 pr_utime;
	struct elf_timeval64 pr_stime;
	struct elf_timeval64 pr_cutime;
	struct elf_timeval64 pr_cstime;

	elf_gregset_t pr_reg;

	int pr_fpvalid;
};

struct elf_prpsinfo64 {
	char pr_state;
	char pr_sname;
	char pr_zomb;
	char pr_nice;
	uint64_t pr_flag;

	uint32_t pr_uid;
	uint32_t pr_gid;

	int32_t pr_pid;
	int32_t pr_ppid;
	int32_t pr_pgrp;
	int32_t pr_sid;

	char pr_fname[16];
	char pr_psargs[ELF_PRARGSZ];
};

#endif /* __MRDUMP_ELF64_H__ */
