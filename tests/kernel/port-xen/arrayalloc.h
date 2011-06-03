/* $NetBSD: arrayalloc.h,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $ */

void arrayalloc_init(char *);
void arrayalloc_fini(void);

void *arrayalloc(size_t);

void arrayfree(void *, size_t);
