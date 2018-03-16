#include <string.h>
#include <arch/ops.h>
#include <dev/mrdump.h>
#include <debug.h>
#include "aee.h"
#include "kdump.h"

#ifdef MTK_3LEVEL_PAGETABLE
#include <stdlib.h>
#include <err.h>
#endif

#define SEARCH_SIZE 33554432
#define SEARCH_STEP 1024

#ifdef MTK_3LEVEL_PAGETABLE
#define MAX_MAP_CNT (SECTION_SIZE/SEARCH_STEP)
#define mapflags (MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA)
static struct mrdump_control_block *mrdump_cb_addr(void)
{
	int i, cnt = 0;

	/* to write sig, use 1-on-1 mapping */
	uint64_t paddr = DRAM_PHY_ADDR;
	vaddr_t vaddr = (vaddr_t)paddr;

	for (i = 0; i < SEARCH_SIZE; i += SEARCH_STEP) {
		if (cnt == 0) {
			int map_ok = arch_mmu_map(paddr, vaddr, mapflags, SECTION_SIZE);
			if (map_ok != NO_ERROR) {
				voprintf_debug("%s: arch_mmu_map error.\n", __func__);
				return NULL;
			}
		}

		struct mrdump_control_block *bufp = (struct mrdump_control_block *)vaddr;
		if (memcmp(bufp->sig, MRDUMP_GO_DUMP, 8) == 0) {
			return bufp;
		}

		cnt++;
		cnt %= MAX_MAP_CNT;
		paddr += SEARCH_STEP;
		vaddr += SEARCH_STEP;
	}
	return NULL;
}
#else
static struct mrdump_control_block *mrdump_cb_addr(void)
{
	int i;
	for (i = 0; i < SEARCH_SIZE; i += SEARCH_STEP) {
		struct mrdump_control_block *bufp = (struct mrdump_control_block *)(DRAM_PHY_ADDR + i);
		if (memcmp(bufp->sig, MRDUMP_GO_DUMP, 8) == 0) {
			return bufp;
		}
	}
	return NULL;
}
#endif

struct mrdump_control_block *aee_mrdump_get_params(void)
{
	struct mrdump_control_block *bufp = mrdump_cb_addr();
	if (bufp == NULL) {
		voprintf_debug("mrdump_cb is NULL\n");
		return NULL;
	}
	if (memcmp(bufp->sig, MRDUMP_GO_DUMP, 8) == 0) {
		bufp->sig[0] = 'X';
		aee_mrdump_flush_cblock(bufp);
		voprintf_debug("Boot record found at %p[%02x%02x]\n", bufp, bufp->sig[0], bufp->sig[1]);
		return bufp;
	} else {
		voprintf_debug("No Boot record found\n");
		return NULL;
	}
}

void aee_mrdump_flush_cblock(struct mrdump_control_block *bufp)
{
	if (bufp != NULL) {
		arch_clean_cache_range((addr_t)bufp, sizeof(struct mrdump_control_block));
	}
}
