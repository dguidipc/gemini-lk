#ifndef __PART_WP_H__
#define __PART_WP_H__

#include "platform/errno.h"
#include "platform/mmc_core.h"

#ifdef MTK_EMMC_POWER_ON_WP
extern int partition_write_prot_set(const char *start_part_name,
                                    const char *end_part_name, EMMC_WP_TYPE type);
#else
static inline int partition_write_prot_set(const char *start_part_name,
        const char *end_part_name, EMMC_WP_TYPE type)
{
	return -EINVAL;
}
#endif


#endif
