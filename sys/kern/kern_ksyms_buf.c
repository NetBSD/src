#define		SYMTAB_FILLER	"|This is the symbol table!"

#ifdef COPY_SYMTAB
#ifndef SYMTAB_SPACE
char		db_symtab[] = SYMTAB_FILLER;
#else
#ifdef __mips64
#define MDADD	32	/* work around a bfd/dbsym/ld bug on mips64 */
#else
#define	MDADD	0
#endif
char		db_symtab[SYMTAB_SPACE+MDADD] = SYMTAB_FILLER;
#endif
int		db_symtabsize = sizeof(db_symtab);
#endif
