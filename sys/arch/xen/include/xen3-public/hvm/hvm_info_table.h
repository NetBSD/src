/******************************************************************************
 * hvm/hvm_info_table.h
 * 
 * HVM parameter and information table, written into guest memory map.
 */

#ifndef __XEN_PUBLIC_HVM_HVM_INFO_TABLE_H__
#define __XEN_PUBLIC_HVM_HVM_INFO_TABLE_H__

#define HVM_INFO_PFN         0x09F
#define HVM_INFO_OFFSET      0x800
#define HVM_INFO_PADDR       ((HVM_INFO_PFN << 12) + HVM_INFO_OFFSET)

struct hvm_info_table {
    char        signature[8]; /* "HVM INFO" */
    uint32_t    length;
    uint8_t     checksum;
    uint8_t     acpi_enabled;
    uint8_t     apic_enabled;
    uint8_t     pae_enabled;
    uint32_t    nr_vcpus;
};

#endif /* __XEN_PUBLIC_HVM_HVM_INFO_TABLE_H__ */
