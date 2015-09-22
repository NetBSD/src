/*	$NetBSD: kern_ksyms_buf.c,v 1.1.2.1 2015/09/22 12:06:07 skrll Exp $	*/

#if defined(_KERNEL_OPT)
#include "opt_copy_symtab.h"
#endif

#define		SYMTAB_FILLER	"|This is the symbol table!"

#ifdef makeoptions_COPY_SYMTAB
#ifndef SYMTAB_SPACE
char		db_symtab[] = SYMTAB_FILLER;
#else
char		db_symtab[SYMTAB_SPACE] = SYMTAB_FILLER;
#endif
int		db_symtabsize = sizeof(db_symtab);
#endif
