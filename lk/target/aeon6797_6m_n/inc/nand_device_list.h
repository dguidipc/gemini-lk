
#ifndef __NAND_DEVICE_LIST_H__
#define __NAND_DEVICE_LIST_H__

#define NAND_MAX_ID		5
#define CHIP_CNT		4
#define RAMDOM_READ		(1<<0)
#define CACHE_READ		(1<<1)

typedef struct
{
   u8 id[NAND_MAX_ID];
   u8 id_length;
   u8 addr_cycle;
   u8 iowidth;
   u16 totalsize;
   u16 blocksize;
   u16 pagesize;
   u16 sparesize;
   u32 timmingsetting;
   u8 devciename[30];
   u32 advancedmode;
}flashdev_info,*pflashdev_info;

static const flashdev_info gen_FlashTable[]={
	{{0xAD,0xBC,0x90,0x55,0x54}, 5,5, 16,512,128,2048,64,0x10801011,"H9DA4GH4JJAMCR_4EM",0}, 
	{{0xEC,0xBC,0x00,0x55,0x54}, 5,5, 16,512,128,2048,64,0x1123,"K524G2GACB_A050",0}, 
	{{0x2C,0xBC,0x90,0x55,0x56}, 5,5, 16,512,128,2048,64,0x21044333,"MT29C4G96MAZAPCJA_5IT",0}, 
	{{0x20,0xBC,0x10,0x55,0x54}, 5,5, 16,512,128,2048,64,0x1123,"EHD013151MA_50",0}, 
};

#endif
