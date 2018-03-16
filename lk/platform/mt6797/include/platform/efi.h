#ifndef __EFI_H__
#define __EFI_H__


typedef struct {
	u8 b[16];
} __attribute__((packed)) efi_guid_t;


typedef struct {
	u64 signature;
	u32 revision;
	u32 header_size;
	u32 header_crc32;
	u32 reserved;
	u64 my_lba;
	u64 alternate_lba;
	u64 first_usable_lba;
	u64 last_usable_lba;
	efi_guid_t disk_guid;
	u64 partition_entry_lba;
	u32 num_partition_entries;
	u32 sizeof_partition_entry;
	u32 partition_entry_array_crc32;
} __attribute__((packed)) gpt_header;


#define GPT_ENTRY_NAME_LEN  (72 / sizeof(u16))

typedef struct {
	efi_guid_t partition_type_guid;
	efi_guid_t unique_partition_guid;
	u64 starting_lba;
	u64 ending_lba;
	u64 attributes;
	u16 partition_name[GPT_ENTRY_NAME_LEN];
} __attribute__((packed))gpt_entry;

typedef struct {
	u8 boot_ind;
	u8 head;
	u8 sector;
	u8 cyl;
	u8 sys_ind;
	u8 end_head;
	u8 end_sector;
	u8 end_cyl;
	u32 start_sector;
	u32 nr_sects;
} __attribute__((packed)) mbr_partition;

typedef struct {
	u8 boot_code[440];
	u32 unique_mbr_signature;
	u16 unknown;
	mbr_partition partition_record[4];
	u16 signature;
} __attribute__((packed)) pmbr;

#define GPT_HEADER_SIGNATURE    0x5452415020494645ULL

#endif
