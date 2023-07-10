#ifndef _ARM_EFIRT_H_
#define _ARM_EFIRT_H_


enum cpu_efirt_mem_type {
	ARM_EFIRT_MEM_CODE,
	ARM_EFIRT_MEM_DATA,
	ARM_EFIRT_MEM_MMIO,
};

void	cpu_efirt_map_range(vaddr_t, paddr_t, size_t, enum cpu_efirt_mem_type);

#endif
