#define	NSEEPROM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NSEEPROM
 .global _KERNEL_OPT_NSEEPROM
 .equiv _KERNEL_OPT_NSEEPROM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NSEEPROM\n .global _KERNEL_OPT_NSEEPROM\n .equiv _KERNEL_OPT_NSEEPROM,0x0\n .endif");
#endif
#define	NAT24CXX_EEPROM	0
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NAT24CXX_EEPROM
 .global _KERNEL_OPT_NAT24CXX_EEPROM
 .equiv _KERNEL_OPT_NAT24CXX_EEPROM,0x0
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NAT24CXX_EEPROM\n .global _KERNEL_OPT_NAT24CXX_EEPROM\n .equiv _KERNEL_OPT_NAT24CXX_EEPROM,0x0\n .endif");
#endif
