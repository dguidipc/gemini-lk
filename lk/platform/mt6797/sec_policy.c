#include "sec_policy.h"
#include "lock_state.h"
#include <debug.h>
#define SHA256_HASH_SZ (32)

unsigned char g_binding_hash[SHA256_HASH_SZ] = {0};

struct policy_part_map g_policy_map[] = {
	{0,                 "default",     0, 0, 0, DEFAULT_SEC_POLICY_1, DEFAULT_SEC_POLICY_2, DEFAULT_SEC_POLICY_3, DEFAULT_SEC_POLICY_4, 0},
	{PL_SW_ID,          "preloader",   0, 0, 0, PL_SEC_POLICY_1, PL_SEC_POLICY_2, PL_SEC_POLICY_3, PL_SEC_POLICY_4, PL_BINDING_HASH},
	{LK_SW_ID,          "lk",          0, 0, 0, LK_SEC_POLICY_1, LK_SEC_POLICY_2, LK_SEC_POLICY_3, LK_SEC_POLICY_4, LK_BINDING_HASH},
	{LOGO_SW_ID,        "logo",        0, 0, 0, LOGO_SEC_POLICY_1, LOGO_SEC_POLICY_2, LOGO_SEC_POLICY_3, LOGO_SEC_POLICY_4, LOGO_BINDING_HASH},
	{BOOT_SW_ID,        "boot",        0, 0, 0, BOOT_SEC_POLICY_1, BOOT_SEC_POLICY_2, BOOT_SEC_POLICY_3, BOOT_SEC_POLICY_4, BOOT_BINDING_HASH},
	{RECOVERY_SW_ID,    "recovery",    0, 0, 0, RECOVERY_SEC_POLICY_1, RECOVERY_SEC_POLICY_2, RECOVERY_SEC_POLICY_3, RECOVERY_SEC_POLICY_4, RECOVERY_BINDING_HASH},
	{SYSTEM_SW_ID,      "system",      0, 0, 0, SYSTEM_SEC_POLICY_1, SYSTEM_SEC_POLICY_2, SYSTEM_SEC_POLICY_3, SYSTEM_SEC_POLICY_4, SYSTEM_BINDING_HASH},
	{TEE1_SW_ID,        "tee1",        0, 0, 0, TEE1_SEC_POLICY_1, TEE1_SEC_POLICY_2, TEE1_SEC_POLICY_3, TEE1_SEC_POLICY_4, TEE1_BINDING_HASH},
	{TEE2_SW_ID,        "tee2",        0, 0, 0, TEE2_SEC_POLICY_1, TEE2_SEC_POLICY_2, TEE2_SEC_POLICY_3, TEE2_SEC_POLICY_4, TEE2_BINDING_HASH},
	{OEMKEYSTORE_SW_ID, "oemkeystore", 0, 0, 0, OEMKEYSTORE_SEC_POLICY_1, OEMKEYSTORE_SEC_POLICY_2, OEMKEYSTORE_SEC_POLICY_3, OEMKEYSTORE_SEC_POLICY_4, OEMKEYSTORE_BINDING_HASH},
	{KEYSTORE_SW_ID,    "keystore",    0, 0, 0, KEYSTORE_SEC_POLICY_1, KEYSTORE_SEC_POLICY_2, KEYSTORE_SEC_POLICY_3, KEYSTORE_SEC_POLICY_4, KEYSTORE_BINDING_HASH},
	{USERDATA_SW_ID,    "userdata",    0, 0, 0, USERDATA_SEC_POLICY_1, USERDATA_SEC_POLICY_2, USERDATA_SEC_POLICY_3, USERDATA_SEC_POLICY_4, USERDATA_BINDING_HASH},
	{MD1IMG_SW_ID,      "md1img",      0, 0, 0, MD1IMG_SEC_POLICY_1, MD1IMG_SEC_POLICY_2, MD1IMG_SEC_POLICY_3, MD1IMG_SEC_POLICY_4, MD1IMG_BINDING_HASH},
	{MD1DSP_SW_ID,      "md1dsp",      0, 0, 0, MD1DSP_SEC_POLICY_1, MD1DSP_SEC_POLICY_2, MD1DSP_SEC_POLICY_3, MD1DSP_SEC_POLICY_4, MD1DSP_BINDING_HASH},
	{MD1ARM7_SW_ID,     "md1arm7",     0, 0, 0, MD1ARM7_SEC_POLICY_1, MD1ARM7_SEC_POLICY_2, MD1ARM7_SEC_POLICY_3, MD1ARM7_SEC_POLICY_4, MD1ARM7_BINDING_HASH},
	{MD3IMG_SW_ID,      "md3img",      0, 0, 0, MD3IMG_SEC_POLICY_1, MD3IMG_SEC_POLICY_2, MD3IMG_SEC_POLICY_3, MD3IMG_SEC_POLICY_4, MD3IMG_BINDING_HASH},	
	{SCP1_SW_ID,        "scp1",      0, 0, 0, SCP1_SEC_POLICY_1, SCP1_SEC_POLICY_2, SCP1_SEC_POLICY_3, SCP1_SEC_POLICY_4, SCP1_BINDING_HASH},
	{SCP2_SW_ID,        "scp2",      0, 0, 0, SCP2_SEC_POLICY_1, SCP2_SEC_POLICY_2, SCP2_SEC_POLICY_3, SCP2_SEC_POLICY_4, SCP2_BINDING_HASH},
	/*Custom img*/
	{0, CUST1_IMG_NAME, 0, 0, 0, CUST1_SEC_POLICY_1, CUST1_SEC_POLICY_2, CUST1_SEC_POLICY_3, CUST1_SEC_POLICY_4 , CUST1_BINDING_HASH},
	{0, CUST2_IMG_NAME, 0, 0, 0, CUST2_SEC_POLICY_1, CUST2_SEC_POLICY_2, CUST2_SEC_POLICY_3, CUST2_SEC_POLICY_4 , CUST2_BINDING_HASH},
	/*End of array*/
	{0, "NULL", "NULL", "NULL", "NULL", 0 , 0, 0, 0, 0},
};

static int strcmp(const char *cs, const char *ct)
{
	signed char __res;
	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}
	return __res;
}

unsigned int get_policy_entry_idx(unsigned char *part_name)
{
	unsigned int i = 0;
	unsigned int num_of_entries = sizeof(g_policy_map)/sizeof(struct policy_part_map);

	while (i < num_of_entries) {
		if (0 == strcmp(part_name, g_policy_map[i].part_name1))
			break;
		else if (0 == strcmp(part_name, g_policy_map[i].part_name2))
			break;
		else if (0 == strcmp(part_name, g_policy_map[i].part_name3))
			break;
		else if (0 == strcmp(part_name, g_policy_map[i].part_name4))
			break;
		i++;
	}

	if (i == num_of_entries) {
		dprintf(CRITICAL, "[SEC_POLICY] reached the end, use default policy\n");
		i = 0;
	}

	return i;
}

static unsigned int get_default_lock_state(unsigned int *lock_state)
{
	unsigned int ret = 0;

	*lock_state = LKS_LOCK;

	return ret;
}

static unsigned int get_sec_state(unsigned int *sboot_state, unsigned int *lock_state)
{
	unsigned int ret = 0;
	unsigned int default_lock_state = 0;

	ret = get_sboot_state(sboot_state);
	if (ret)
		*sboot_state = 0;

	ret = get_lock_state(lock_state);
	/* dont't process return value here */

	if ((LKS_DEFAULT == *lock_state) || (LKS_MP_DEFAULT == *lock_state)) {
		ret = get_default_lock_state(&default_lock_state);
		if (ret)
			*lock_state = LKS_LOCK;
		*lock_state = default_lock_state;
	}

	dprintf(CRITICAL, "[SEC_POLICY] sboot_state = 0x%x\n", *sboot_state);
	if (!default_lock_state)
		dprintf(CRITICAL, "[SEC_POLICY] lock_state = 0x%x\n", *lock_state);
	else
		dprintf(CRITICAL, "[SEC_POLICY] lock_state(default) = 0x%x\n", *lock_state);

	return 0;
}

static char get_sec_policy(unsigned int policy_entry_idx)
{
	unsigned int sboot_state = 0;
	unsigned int lock_state = 0;

	unsigned char sec_policy = 0;
	unsigned int ret = 0;

	ret = get_sec_state(&sboot_state, &lock_state);
	/* this API won't return error, so we don't process it here */

	if (sboot_state == 0 && lock_state == LKS_UNLOCK)
		sec_policy = g_policy_map[policy_entry_idx].sec_sbcdis_unlock_policy;
	else if (sboot_state == 0 && lock_state != LKS_UNLOCK)
		sec_policy = g_policy_map[policy_entry_idx].sec_sbcdis_lock_policy;
	else if (sboot_state == 1 && lock_state == LKS_UNLOCK)
		sec_policy = g_policy_map[policy_entry_idx].sec_sbcen_unlock_policy;
	else if (sboot_state == 1 && lock_state != LKS_UNLOCK)
		sec_policy = g_policy_map[policy_entry_idx].sec_sbcen_lock_policy;

	return sec_policy;
}

static unsigned char chr_to_hex(unsigned char input)
{
	unsigned char output = 0;
	if (input >= '0' && input <= '9')
		output = input - '0' + 0x0;
	if (input >= 'A' && input <= 'F')
		output = input - 'A' + 0xa;
	if (input >= 'a' && input <= 'f')
		output = input - 'a' + 0xa;

	return output;
}

unsigned char *get_binding_hash(unsigned int policy_entry_idx)
{
	unsigned int i = 0;

	if (!g_policy_map[policy_entry_idx].hash)
		return NULL;

	for (i = 0; i < SHA256_HASH_SZ; i++) {
		unsigned char chr0, chr1 = 0;
		chr0 = g_policy_map[policy_entry_idx].hash[i * 2];
		chr1 = g_policy_map[policy_entry_idx].hash[i * 2 + 1];
		g_binding_hash[i] = (chr_to_hex(chr0) << 4) | (chr_to_hex(chr1));
	}

	return &g_binding_hash;
}

/* bypass hashbind: return 0, need hashbind: return 1 */
unsigned int get_hash_binding_policy(unsigned int policy_entry_idx)
{
	unsigned char sec_policy = 0;
	sec_policy = get_sec_policy(policy_entry_idx);

	if (HASH_BIND_BIT_SET(sec_policy))
		/* need hash bind */
		return 1;
	else
		/* skip hash bind */
		return 0;
}

/* bypass auth check: return 0, need auth check: return 1 */
unsigned int get_vfy_policy(unsigned int policy_entry_idx)
{
	unsigned char sec_policy = 0;
	sec_policy = get_sec_policy(policy_entry_idx);

	if (VFY_BIT_SET(sec_policy))
		/* need verify */
		return 1;
	else
		/* skip verify */
		return 0;
}

/* bypass dl check: return 0, need dl check: return 1 */
unsigned int get_dl_policy(unsigned int policy_entry_idx)
{
	unsigned char sec_policy = 0;
	sec_policy = get_sec_policy(policy_entry_idx);

	if (DL_FORBIDDEN_BIT_SET(sec_policy))
		/* dl is forbidden */
		return 1;
	else
		/* dl is permitted */
		return 0;
}

unsigned int get_sw_id(unsigned int policy_entry_idx)
{
	return g_policy_map[policy_entry_idx].sw_id;
}

unsigned int is_all_partition_writable(unsigned int *write_all_perm)
{
	unsigned int ret = 0;
	unsigned int i = 0;
	unsigned int num_of_entries = sizeof(g_policy_map)/sizeof(struct policy_part_map);

	*write_all_perm = 1;

	for (i = 0; i < num_of_entries; i++) {
		/* if any partition in the table is not writeable, then write/format
		without specifying partion name is not allowed */
		if (get_dl_policy(i)) {
			*write_all_perm = 0;
			break;
		}
	}

	return ret;
}


