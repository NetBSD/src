/* option `M68010' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_M68010
 .global _KERNEL_OPT_M68010
 .equiv _KERNEL_OPT_M68010,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_M68010\n .global _KERNEL_OPT_M68010\n .equiv _KERNEL_OPT_M68010,0x6e074def\n .endif");
#endif
/* option `M68020' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_M68020
 .global _KERNEL_OPT_M68020
 .equiv _KERNEL_OPT_M68020,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_M68020\n .global _KERNEL_OPT_M68020\n .equiv _KERNEL_OPT_M68020,0x6e074def\n .endif");
#endif
#define	M68030	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_M68030
 .global _KERNEL_OPT_M68030
 .equiv _KERNEL_OPT_M68030,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_M68030\n .global _KERNEL_OPT_M68030\n .equiv _KERNEL_OPT_M68030,0x1\n .endif");
#endif
#define	M68040	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_M68040
 .global _KERNEL_OPT_M68040
 .equiv _KERNEL_OPT_M68040,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_M68040\n .global _KERNEL_OPT_M68040\n .equiv _KERNEL_OPT_M68040,0x1\n .endif");
#endif
/* option `M68060' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_M68060
 .global _KERNEL_OPT_M68060
 .equiv _KERNEL_OPT_M68060,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_M68060\n .global _KERNEL_OPT_M68060\n .equiv _KERNEL_OPT_M68060,0x6e074def\n .endif");
#endif
