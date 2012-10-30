/*	$NetBSD: tree.h,v 1.1.1.1.14.1 2012/10/30 18:55:24 yamt Exp $	*/

/* tree.h - declare structures used by tree library
 *
 * vix 22jan93 [revisited; uses RCS, ANSI, POSIX; has bug fixes]
 * vix 27jun86 [broken out of tree.c]
 *
 * Id: tree.h,v 1.3 2005/04/27 04:56:18 sra Exp 
 */


#ifndef	_TREE_H_INCLUDED
#define	_TREE_H_INCLUDED


#ifndef __P
# if defined(__STDC__) || defined(__GNUC__)
#  define __P(x) x
# else
#  define __P(x) ()
# endif
#endif

/*%
 * tree_t is our package-specific anonymous pointer.
 */
#if defined(__STDC__) || defined(__GNUC__)
typedef	void *tree_t;
#else
typedef	char *tree_t;
#endif

/*%
 * Do not taint namespace
 */
#define	tree_add	__tree_add
#define	tree_delete	__tree_delete
#define	tree_init	__tree_init
#define	tree_mung	__tree_mung
#define	tree_srch	__tree_srch
#define	tree_trav	__tree_trav


typedef	struct tree_s {
		tree_t		data;
		struct tree_s	*left, *right;
		short		bal;
	}
	tree;


void	tree_init	__P((tree **));
tree_t	tree_srch	__P((tree **, int (*)(), tree_t));
tree_t	tree_add	__P((tree **, int (*)(), tree_t, void (*)()));
int	tree_delete	__P((tree **, int (*)(), tree_t, void (*)()));
int	tree_trav	__P((tree **, int (*)()));
void	tree_mung	__P((tree **, void (*)()));


#endif	/* _TREE_H_INCLUDED */
/*! \file */
