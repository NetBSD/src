/* option `makeoptions_COPY_SYMTAB' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_makeoptions_COPY_SYMTAB
 .global _KERNEL_OPT_makeoptions_COPY_SYMTAB
 .equiv _KERNEL_OPT_makeoptions_COPY_SYMTAB,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_makeoptions_COPY_SYMTAB\n .global _KERNEL_OPT_makeoptions_COPY_SYMTAB\n .equiv _KERNEL_OPT_makeoptions_COPY_SYMTAB,0x6e074def\n .endif");
#endif
