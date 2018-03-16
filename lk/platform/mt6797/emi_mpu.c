#include <debug.h>
#include <platform/mt_typedefs.h>
#include <platform/emi_mpu.h>
#include <kernel/thread.h>

#define DBG_EMI(x...) dprintf(CRITICAL, x)

#define MTK_SIP_KERNEL_EMIMPU_SET           0x82000209

#define emi_mpu_smc_set(start, end, region_permission) \
    mt_secure_call(MTK_SIP_KERNEL_EMIMPU_SET, start, end, region_permission)

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned int start, unsigned int end, int region, unsigned int access_permission)
{
	int ret = 0;
	unsigned long flags;

	DBG_EMI("LK set emi mpu region protection start:%x end=%x region=%d access_permission=%x \n", start, end, region, access_permission);
	access_permission = access_permission & 0x4FFFFFF;
	access_permission = access_permission | ((region & 0x1F)<<27);
	enter_critical_section();
	emi_mpu_smc_set(start, end, access_permission);
	exit_critical_section();

	return ret;
}

