/*	$NetBSD: ealloc.h,v 1.2 2002/06/30 14:17:44 lukem Exp $	*/

void	*emalloc(size_t);
char	*estrdup(const char *);
void	*erealloc(void *, size_t);
void	*ecalloc(size_t, size_t);
