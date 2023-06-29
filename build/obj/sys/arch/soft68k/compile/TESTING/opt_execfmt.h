/* option `EXEC_ELF_NOTELESS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_ELF_NOTELESS
 .global _KERNEL_OPT_EXEC_ELF_NOTELESS
 .equiv _KERNEL_OPT_EXEC_ELF_NOTELESS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_ELF_NOTELESS\n .global _KERNEL_OPT_EXEC_ELF_NOTELESS\n .equiv _KERNEL_OPT_EXEC_ELF_NOTELESS,0x6e074def\n .endif");
#endif
#define	EXEC_SCRIPT	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_SCRIPT
 .global _KERNEL_OPT_EXEC_SCRIPT
 .equiv _KERNEL_OPT_EXEC_SCRIPT,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_SCRIPT\n .global _KERNEL_OPT_EXEC_SCRIPT\n .equiv _KERNEL_OPT_EXEC_SCRIPT,0x1\n .endif");
#endif
/* option `EXEC_ELF64' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_ELF64
 .global _KERNEL_OPT_EXEC_ELF64
 .equiv _KERNEL_OPT_EXEC_ELF64,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_ELF64\n .global _KERNEL_OPT_EXEC_ELF64\n .equiv _KERNEL_OPT_EXEC_ELF64,0x6e074def\n .endif");
#endif
#define	EXEC_ELF32	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_ELF32
 .global _KERNEL_OPT_EXEC_ELF32
 .equiv _KERNEL_OPT_EXEC_ELF32,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_ELF32\n .global _KERNEL_OPT_EXEC_ELF32\n .equiv _KERNEL_OPT_EXEC_ELF32,0x1\n .endif");
#endif
/* option `EXEC_ECOFF' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_ECOFF
 .global _KERNEL_OPT_EXEC_ECOFF
 .equiv _KERNEL_OPT_EXEC_ECOFF,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_ECOFF\n .global _KERNEL_OPT_EXEC_ECOFF\n .equiv _KERNEL_OPT_EXEC_ECOFF,0x6e074def\n .endif");
#endif
/* option `EXEC_COFF' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_COFF
 .global _KERNEL_OPT_EXEC_COFF
 .equiv _KERNEL_OPT_EXEC_COFF,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_COFF\n .global _KERNEL_OPT_EXEC_COFF\n .equiv _KERNEL_OPT_EXEC_COFF,0x6e074def\n .endif");
#endif
#define	EXEC_AOUT	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_EXEC_AOUT
 .global _KERNEL_OPT_EXEC_AOUT
 .equiv _KERNEL_OPT_EXEC_AOUT,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_EXEC_AOUT\n .global _KERNEL_OPT_EXEC_AOUT\n .equiv _KERNEL_OPT_EXEC_AOUT,0x1\n .endif");
#endif
