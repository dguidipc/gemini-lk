#ifndef _SEC_IMG_AUTH
#define _SEC_IMG_AUTH

/**************************************************************************
 * [Software Secure Boot Format]
 **************************************************************************/
typedef enum {
	SW_SUPPORT_DISABLE=0,
	SW_SUPPORT_ENABLE=1,
} SW_SEC_BOOT_FEATURE_SUPPORT;

typedef enum {
	SW_SEC_BOOT_LOCK=1,
	SW_SEC_BOOT_UNLOCK=2,
} SW_SEC_BOOT_LOCK_TYPE;

typedef enum {
	SW_SEC_BOOT_CERT_PER_PROJECT=1,
	SW_SEC_BOOT_CERT_PER_DEVICE=2,
} SW_SEC_BOOT_CERT_TYPE;

typedef enum {
	SW_SEC_BOOT_NOT_TRY=0,
	SW_SEC_BOOT_TRY_LOCK=1,
	SW_SEC_BOOT_TRY_UNLOCK=2
} SW_SEC_BOOT_TRY_TYPE;

typedef enum {
	SW_SEC_BOOT_NOT_DONE=0,
	SW_SEC_BOOT_DONE_LOCKED=1,
	SW_SEC_BOOT_DONE_UNLOCKED=2
} SW_SEC_BOOT_DONE_TYPE;

typedef enum {
	SW_SEC_BOOT_CHECK_IMAGE=1,
	SW_SEC_BOOT_NOT_CHECK_IMAGE=2,
} SW_SEC_BOOT_CHK_TYPE;

typedef enum {
	SEC_LOCK = 0,
	SEC_UNLOCK = 1,
	SEC_VERIFIED = 2,
	SEC_CUSTOM = 3,
	SEC_UNDEFINED = 0xff,
} LOCK_STATE;

typedef struct {
	unsigned int dl_format_lock;
	unsigned int dl_1st_loader_lock;
	unsigned int dl_2nd_loader_lock;
	unsigned int dl_image_lock;
	unsigned int boot_chk_2nd_loader;
	unsigned int boot_chk_logo;
	unsigned int boot_chk_bootimg;
	unsigned int boot_chk_recovery;
	unsigned int boot_chk_system;
	unsigned int boot_chk_others;
	unsigned int boot_chk_cust1;
	unsigned int boot_chk_cust2;
	unsigned int dl_tee_lock;
	unsigned int boot_chk_tee;
} SEC_POLICY_T;

#define MAX_NUM_LOCK_STATE 4

typedef struct {
	unsigned int  magic_number;
	unsigned int  sw_secure_boot_en;
	unsigned int  default_state;
	SEC_POLICY_T  state_policy[MAX_NUM_LOCK_STATE];
	unsigned char reserve[20];
} AND_SW_SEC_BOOT_T;

extern SEC_POLICY_T *g_cur_sec_policy;
/**************************************************************************
 * [seclib extern function]
 **************************************************************************/
extern void seclib_image_buf_init(void);
extern void seclib_image_buf_free(void);

/**************************************************************************
 * [SECURE PARTITION NAME]
 **************************************************************************/
struct sec_part_name_map {
	char fb_name[32];   /*partition name used by fastboot, and gpt since 95*/
	char ft_name[32];   /*partition name used by flash tool*/
	char st_name[32];    /* partition name used by sign tool */
	char sb_name[32];   /*partition name used by secure boot*/
	int partition_idx;  /*partition index*/
	int is_sec_dl_forbit;   /*partition support download in security */
};

extern struct sec_part_name_map g_secure_part_name[];

/* index for secure boot partition name, g_secure_part_name*/
#define SBOOT_PART_PRELOADER            0
#define SBOOT_PART_SECURE               8
#define SBOOT_PART_LK                   9
#define SBOOT_PART_BOOTIMG              10
#define SBOOT_PART_RECOVERY             11
#define SBOOT_PART_SECRO                12
#define SBOOT_PART_LOGO                 14
#define SBOOT_PART_ANDROID              17
#define SBOOT_PART_USRDATA              19
#define SBOOT_PART_CUST1                23
#define SBOOT_PART_CUST2                24
#endif
