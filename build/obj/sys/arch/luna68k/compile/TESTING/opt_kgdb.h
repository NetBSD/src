/* option `KGDB' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB
 .global _KERNEL_OPT_KGDB
 .equiv _KERNEL_OPT_KGDB,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB\n .global _KERNEL_OPT_KGDB\n .equiv _KERNEL_OPT_KGDB,0x6e074def\n .endif");
#endif
/* option `KGDB_DEVMODE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB_DEVMODE
 .global _KERNEL_OPT_KGDB_DEVMODE
 .equiv _KERNEL_OPT_KGDB_DEVMODE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB_DEVMODE\n .global _KERNEL_OPT_KGDB_DEVMODE\n .equiv _KERNEL_OPT_KGDB_DEVMODE,0x6e074def\n .endif");
#endif
/* option `KGDB_DEVRATE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB_DEVRATE
 .global _KERNEL_OPT_KGDB_DEVRATE
 .equiv _KERNEL_OPT_KGDB_DEVRATE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB_DEVRATE\n .global _KERNEL_OPT_KGDB_DEVRATE\n .equiv _KERNEL_OPT_KGDB_DEVRATE,0x6e074def\n .endif");
#endif
/* option `KGDB_DEVADDR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB_DEVADDR
 .global _KERNEL_OPT_KGDB_DEVADDR
 .equiv _KERNEL_OPT_KGDB_DEVADDR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB_DEVADDR\n .global _KERNEL_OPT_KGDB_DEVADDR\n .equiv _KERNEL_OPT_KGDB_DEVADDR,0x6e074def\n .endif");
#endif
/* option `KGDB_DEVPORT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB_DEVPORT
 .global _KERNEL_OPT_KGDB_DEVPORT
 .equiv _KERNEL_OPT_KGDB_DEVPORT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB_DEVPORT\n .global _KERNEL_OPT_KGDB_DEVPORT\n .equiv _KERNEL_OPT_KGDB_DEVPORT,0x6e074def\n .endif");
#endif
/* option `KGDB_DEVNAME' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB_DEVNAME
 .global _KERNEL_OPT_KGDB_DEVNAME
 .equiv _KERNEL_OPT_KGDB_DEVNAME,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB_DEVNAME\n .global _KERNEL_OPT_KGDB_DEVNAME\n .equiv _KERNEL_OPT_KGDB_DEVNAME,0x6e074def\n .endif");
#endif
/* option `KGDB_DEV' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_KGDB_DEV
 .global _KERNEL_OPT_KGDB_DEV
 .equiv _KERNEL_OPT_KGDB_DEV,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_KGDB_DEV\n .global _KERNEL_OPT_KGDB_DEV\n .equiv _KERNEL_OPT_KGDB_DEV,0x6e074def\n .endif");
#endif
