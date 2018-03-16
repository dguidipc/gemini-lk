#include <stdint.h>
#include <printf.h>
#include <string.h>
#include <platform/errno.h>
#include <platform/mmc_core.h>
#include <platform/partition.h>
#include <platform/partition_wp.h>


int partition_write_prot_set(const char *start_part_name, const char *end_part_name, EMMC_WP_TYPE type)
{
	int err = 0;
	part_t start, end;
	part_t *pstart, *pend;
	unsigned long nr_wp_sects;

	pstart = mt_part_get_partition((char *)start_part_name);
	if (!pstart) {
		dprintf(CRITICAL, "[Part_WP]Not found start partition %s info\n", start_part_name);
		err = -EINVAL;
		goto out;
	}
	memcpy(&start, pstart, sizeof(start));

	pend = mt_part_get_partition((char *)end_part_name);
	if (!pend) {
		dprintf(CRITICAL, "[Part_WP]Not found end partition %s info\n", end_part_name);
		err = -EINVAL;
		goto out;
	}
	memcpy(&end, pend, sizeof(end));

	if (start.part_id != end.part_id) {
		dprintf(CRITICAL, "[Part_WP]WP Range(region): %d->%d\n", start.part_id, end.part_id);
		err = -EINVAL;
		goto out;
	}

	if (start.start_sect > end.start_sect) {
		dprintf(CRITICAL, "[Part_WP]WP Range(block): 0x%lx->0x%lx\n", start.start_sect, end.start_sect);
		err = -EINVAL;
		goto out;
	}

	nr_wp_sects = end.start_sect + end.nr_sects - start.start_sect;
	dprintf(CRITICAL, "[Part_WP]WP(%s->%s): Region(%d), start_sect(0x%lx), nr_blk(0x%lx), type(%d)\n",
	        start_part_name, end_part_name, start.part_id, start.start_sect, nr_wp_sects, type);
	err = mmc_set_write_protect(0, start.part_id, start.start_sect, nr_wp_sects, type);

out:
	return err;
}
