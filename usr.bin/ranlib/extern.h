/*	$NetBSD: extern.h,v 1.1 1997/10/20 01:59:59 lukem Exp $	*/

void	badfmt __P((void));
int	build __P((void));
void   *emalloc __P((size_t));
char   *rname __P((char *));
void	settime __P((int));
int	tmp __P((void));
int	touch __P((void));

extern	char   *archive;
extern	CHDR	chdr;
extern	char   *tname;
