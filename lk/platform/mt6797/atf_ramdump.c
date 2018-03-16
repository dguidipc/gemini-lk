#include <sys/types.h>
#include <debug.h>
#include <platform/mt_typedefs.h>
#include <libfdt.h>
#define INFO_LOG(fmt, args...) do {dprintf(INFO, fmt, ##args);} while (0)
#define ALWAYS_LOG(fmt, args...) do {dprintf(ALWAYS, fmt, ##args);} while (0)
void get_atf_ramdump_memory(void *fdt){
	int offset, sub_offset;
	uint32_t address_hi, address_lo, size_hi, size_lo;
	int len;
	const struct fdt_property *prop;

	offset = fdt_path_offset(fdt, "/reserved-memory");
	sub_offset = fdt_subnode_offset(fdt, offset, "atf-ramdump-memory");
	if (sub_offset) {
		prop = fdt_get_property(fdt, sub_offset, "reg", &len);
		if(prop){
			memcpy((void *)&address_hi, (void *)prop->data, sizeof(uint32_t));
			memcpy((void *)&address_lo, (void *)&prop->data[4], sizeof(uint32_t));
			memcpy((void *)&size_hi, (void *)&prop->data[8], sizeof(uint32_t));
			memcpy((void *)&size_lo, (void *)&prop->data[12], sizeof(uint32_t));
			address_hi = fdt32_to_cpu(address_hi);
			address_lo = fdt32_to_cpu(address_lo);
			size_hi = fdt32_to_cpu(size_hi);
			size_lo = fdt32_to_cpu(size_lo);
			__asm__ volatile("ldr r0, =0x8200020E\n" //MTK_SIP_RAM_DUMP_ADDR_AARCH32
					"mov r1, %0\n"
					"mov r2, %1\n"
					"mov r3, %2\n"
					"mov r4, %3\n"
					"smc #0x0\n"
					::"r"(address_hi),"r"(address_lo),"r"(size_hi),"r"(size_lo):"cc", "r0", "r1" ,"r2", "r3", "r4"
			);
			INFO_LOG("atf ram dump address hi:0x%x, adress lo:0x%x, size hi:%u, size lo:%u\n", address_hi, address_lo, size_hi, size_lo);
		}
		else{
			ALWAYS_LOG("Can not find atf ram dump!\n");
		}
	}
	else{
			ALWAYS_LOG("Can not find atf-ramdump-memory node!\n");
	}
}

