/*	$NetBSD: search.h,v 1.15 2003/07/26 17:35:01 salo Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>
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

#ifdef _SEARCH_PRIVATE
typedef struct node {
	char         *key;
	struct node  *llink, *rlink;
} node_t;
#endif

__BEGIN_DECLS
#ifndef __BSEARCH_DECLARED
#define __BSEARCH_DECLARED
/* also in stdlib.h */
void	*bsearch __P((const void *, const void *, size_t, size_t,
		      int (*)(const void *, const void *)));
#endif /* __BSEARCH_DECLARED */
int	 hcreate __P((size_t));
void	 hdestroy __P((void));
ENTRY	*hsearch __P((ENTRY, ACTION));

void	*lfind __P((const void *, const void *, size_t *, size_t,
		      int (*)(const void *, const void *)));
void	*lsearch __P((const void *, const void *, size_t *, size_t,
		      int (*)(const void *, const void *)));
void	 insque __P((void *, void *));
void	 remque __P((void *));

void	*tdelete __P((const void *, void **,
		      int (*)(const void *, const void *)));
void	*tfind __P((const void *, void **,
		      int (*)(const void *, const void *)));
void	*tsearch __P((const void *, void **, 
		      int (*)(const void *, const void *)));
void      twalk __P((const void *, void (*)(const void *, VISIT, int)));
__END_DECLS

#endif /* !_SEARCH_H_ */
