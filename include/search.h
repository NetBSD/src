/*	$NetBSD: search.h,v 1.11 1999/02/16 18:23:01 kleink Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>
 * Public domain.
 */

#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <sys/cdefs.h>
#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

typedef struct entry {
	char *key;
	void *data;
} ENTRY;

typedef enum {
	FIND, ENTER
} ACTION;

typedef enum {
	preorder,
	postorder,
	endorder,
	leaf
} VISIT;

__BEGIN_DECLS
extern void	*bsearch __P((const void *, const void *, size_t, size_t,
			      int (*)(const void *, const void *)));
extern int	 hcreate __P((size_t));
extern void	 hdestroy __P((void));
extern ENTRY	*hsearch __P((ENTRY, ACTION));

extern void	*lfind __P((const void *, const void *, size_t *, size_t,
			      int (*)(const void *, const void *)));
extern void	*lsearch __P((const void *, const void *, size_t *, size_t,
			      int (*)(const void *, const void *)));
extern void	 insque __P((void *, void *));
extern void	 remque __P((void *));

extern void	*tdelete __P((const void *, void **,
			      int (*)(const void *, const void *)));
extern void	*tfind __P((const void *, void **,
			      int (*)(const void *, const void *)));
extern void	*tsearch __P((const void *, void **, 
			      int (*)(const void *, const void *)));
extern void      twalk __P((const void *, void (*)(const void *, VISIT, int)));
__END_DECLS

#endif /* !_SEARCH_H_ */
