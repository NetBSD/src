/* option `ENVSYS_OBJECTS_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ENVSYS_OBJECTS_DEBUG
 .global _KERNEL_OPT_ENVSYS_OBJECTS_DEBUG
 .equiv _KERNEL_OPT_ENVSYS_OBJECTS_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ENVSYS_OBJECTS_DEBUG\n .global _KERNEL_OPT_ENVSYS_OBJECTS_DEBUG\n .equiv _KERNEL_OPT_ENVSYS_OBJECTS_DEBUG,0x6e074def\n .endif");
#endif
/* option `ENVSYS_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_ENVSYS_DEBUG
 .global _KERNEL_OPT_ENVSYS_DEBUG
 .equiv _KERNEL_OPT_ENVSYS_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_ENVSYS_DEBUG\n .global _KERNEL_OPT_ENVSYS_DEBUG\n .equiv _KERNEL_OPT_ENVSYS_DEBUG,0x6e074def\n .endif");
#endif
