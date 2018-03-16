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

#if !defined(__MRDUMP_ELF_COMMON_H__)
#define __MRDUMP_ELF_COMMON_H__

#define ELFMAG      "\177ELF"
#define SELFMAG     4

#define CORE_STR "CORE"

#ifndef ELF_CORE_EFLAGS
#define ELF_CORE_EFLAGS 0
#endif

#define EI_NIDENT   16

#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_PAD      8

#define EM_ARM 40
#define EM_AARCH64 183

#define ET_CORE 4

#define PT_LOAD 1
#define PT_NOTE 4

#define ELFCLASS32 1
#define ELFCLASS64 2

#define NT_PRSTATUS 1
#define NT_PRFPREG 2
#define NT_PRPSINFO 3


#define PF_R 0x4
#define PF_W 0x2
#define PF_X 0x1

#define ELFOSABI_NONE 0

#define EV_CURRENT 1

#define ELFDATA2LSB 1

#define ELF_PRARGSZ 80

#define MRDUMP_TYPE_FULL_MEMORY 0
#define MRDUMP_TYPE_KERNEL_1 1
#define MRDUMP_TYPE_KERNEL_2 2

#define MRDUMP_TYPE_MASK 0x3

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef uint64_t Elf64_Off;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;

static inline void mrdump_elf_setup_eident(unsigned char e_ident[EI_NIDENT], unsigned char elfclasz)
{
	memcpy(e_ident, ELFMAG, SELFMAG);
	e_ident[EI_CLASS] = elfclasz;
	e_ident[EI_DATA] = ELFDATA2LSB;
	e_ident[EI_VERSION] = EV_CURRENT;
	e_ident[EI_OSABI] = ELFOSABI_NONE;
	memset(e_ident+EI_PAD, 0, EI_NIDENT-EI_PAD);
}

#define mrdump_elf_setup_elfhdr(elfp, machid, elfhdr_t, elfphdr_t)   \
    elfp->e_type = ET_CORE;              \
    elfp->e_machine = machid;                            \
    elfp->e_version = EV_CURRENT;            \
    elfp->e_entry = 0;                                   \
    elfp->e_phoff = sizeof(elfhdr_t);                    \
    elfp->e_shoff = 0;                                   \
    elfp->e_flags = ELF_CORE_EFLAGS;                     \
    elfp->e_ehsize = sizeof(elfhdr_t);                   \
    elfp->e_phentsize = sizeof(elfphdr_t);       \
    elfp->e_phnum = 2;                                   \
    elfp->e_shentsize = 0;               \
    elfp->e_shnum = 0;                                   \
    elfp->e_shstrndx = 0;                \


struct elf_siginfo {
	int si_signo;
	int si_code;
	int si_errno;
};

struct __attribute__((__packed__)) elf_mrdump_machdesc {
	uint32_t flags;
	uint32_t nr_cpus;

	uint64_t phys_offset;
	uint64_t total_memory;

	uint64_t page_offset;
	uint64_t high_memory;

	uint64_t kimage_vaddr;

	uint64_t modules_start;
	uint64_t modules_end;

	uint64_t vmalloc_start;
	uint64_t vmalloc_end;

	uint64_t master_page_table;

	uint64_t dfdmem_pa;
};

#endif
