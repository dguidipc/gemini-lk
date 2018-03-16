#ifndef __ATAG_H__
#define __ATAG_H__

#include <stdint.h>
#include <sys/types.h>
#include <platform/mmc_types.h>
#include <platform/mt_typedefs.h>

struct tag_header {
	u32 size;
	u32 tag;
};

#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define SIZE_1M             (1024 * 1024)
#define SIZE_2M             (2 * SIZE_1M)
#define SIZE_256M           (256 * SIZE_1M)
#define SIZE_512M           (512 * SIZE_1M)

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE	0x54410001
struct tag_core {
	u32 flags;		/* bit 0 = read-only */
	u32 pagesize;
	u32 rootdev;
};

/* it is allowed to have multiple ATAG_MEM nodes */
#define ATAG_MEM  0x54410002
typedef struct {
	uint32_t size;
	uint32_t start_addr;
} mem_info;

#define ATAG_MEM64	0x54420002
typedef struct {
	uint64_t size;
	uint64_t start_addr;
} mem64_info;

#define ATAG_EXT_MEM64	0x54430002
typedef struct {
	uint64_t size;
	uint64_t start_addr;
} ext_mem64_info;

/* command line: \0 terminated string */
#define ATAG_CMDLINE	0x54410009
struct tag_cmdline {
	char cmdline[1];	/* this is the minimum size */
};

/* describes where the compressed ramdisk image lives (physical address) */
#define ATAG_INITRD2	0x54420005
struct tag_initrd {
	u32 start;		/* physical start address */
	u32 size;		/* size of compressed ramdisk image in bytes */
};

#define ATAG_VIDEOLFB	0x54410008
struct tag_videolfb {
	u16		lfb_width;
	u16		lfb_height;
	u16		lfb_depth;
	u16		lfb_linelength;
	u32		lfb_base;
	u32		lfb_size;
	u8		red_size;
	u8		red_pos;
	u8		green_size;
	u8		green_pos;
	u8		blue_size;
	u8		blue_pos;
	u8		rsvd_size;
	u8		rsvd_pos;
};

/* boot information */
#define ATAG_BOOT	0x41000802
struct tag_boot {
	u32 bootmode;
};

/* imix_r information */
#define ATAG_IMIX	0x41000802
struct tag_imix_r {
	u32 bootmode;
};


/*META com port information*/
#define ATAG_META_COM 0x41000803
struct tag_meta_com {
	u32 meta_com_type;	/* identify meta via uart or usb */
	u32 meta_com_id;	/* multiple meta need to know com port id */
	u32 meta_uart_port;	/* identify meta uart port */
};

/*device information*/
#define ATAG_DEVINFO_DATA         0x41000804
#define ATAG_DEVINFO_DATA_SIZE    48
struct tag_devinfo_data {
	u32 devinfo_data[ATAG_DEVINFO_DATA_SIZE];
	u32 devinfo_data_size;
};

#define ATAG_MDINFO_DATA 0x41000806
struct tag_mdinfo_data {
	u8 md_type[4];
};

#define ATAG_VCORE_DVFS_INFO 0x54410007
struct tag_vcore_dvfs_info {
	u32 pllgrpreg_size;
	u32 freqreg_size;
	u32 *low_freq_pll_val;
	u32 *low_freq_cha_val;
	u32 *low_freq_chb_val;
	u32 *high_freq_pll_val;
	u32 *high_freq_cha_val;
	u32 *high_freq_chb_val;
};

#define ATAG_PTP_INFO 0x54410008
struct tag_ptp_info {
	u32 first_volt;
	u32 second_volt;
	u32 third_volt;
	u32 have_550;
};

/* The list ends with an ATAG_NONE node. */
#define ATAG_NONE	0x00000000

extern unsigned *target_atag_nand_data(unsigned *ptr) __attribute__((weak));
extern unsigned *target_atag_partition_data(unsigned *ptr) __attribute__((weak));
extern unsigned *target_atag_boot(unsigned *ptr) __attribute__((weak));
#if defined(MTK_DLPT_SUPPORT)
extern unsigned *target_atag_imix_r(unsigned *ptr) __attribute__((weak));
#endif
extern unsigned *target_atag_devinfo_data(unsigned *ptr) __attribute__((weak));
extern unsigned *target_atag_mem(unsigned *ptr) __attribute__((weak));
extern unsigned *target_atag_meta(unsigned *ptr) __attribute__((weak));
/* todo: give lk strtoul and nuke this */
extern unsigned *target_atag_commmandline(unsigned *ptr, char *commandline) __attribute__((weak));
extern unsigned *target_atag_initrd(unsigned *ptr, ulong initrd_start, ulong initrd_size) __attribute__((weak));
extern unsigned *target_atag_videolfb(unsigned *ptr) __attribute__((weak));
extern unsigned *target_atag_mdinfo(unsigned *ptr) __attribute__((weak));
extern void *target_get_scratch_address(void) __attribute__((weak));
extern int platform_atag_append(void *fdt) __attribute__((weak));

#endif /* __ATAG_H__ */
