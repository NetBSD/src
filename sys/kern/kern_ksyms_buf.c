#define		SYMTAB_FILLER	"|This is the symbol table!"

#ifdef COPY_SYMTAB
#ifndef SYMTAB_SPACE
char		db_symtab[] = SYMTAB_FILLER;
#else
char		db_symtab[SYMTAB_SPACE] = SYMTAB_FILLER;
#endif
int		db_symtabsize = sizeof(db_symtab);
#endif
