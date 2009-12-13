/*	$NetBSD: ntp_lists.h,v 1.1.1.1 2009/12/13 16:54:51 kardel Exp $	*/

/*
 * ntp_lists.h - singly-linked lists common code
 *
 * These macros implement a simple singly-linked list template.  Both
 * the listhead and per-entry next fields are declared as pointers to
 * the list entry struct type.  Initialization to NULL is typically
 * implicit (for globals and statics) or handled by zeroing of the
 * containing structure.
 *
 * The name of the next link field is passed as an argument to allow
 * membership in several lists at once using multiple next link fields.
 *
 * When possible, placing the link field first in the entry structure
 * allows slightly smaller code to be generated on some platforms.
 *
 * LINK_SLIST(listhead, pentry, nextlink)
 *	add entry at head
 *
 * LINK_TAIL_SLIST(listhead, pentry, nextlink, entrytype)
 *	add entry at tail
 *
 * UNLINK_HEAD_SLIST(punlinked, listhead, nextlink)
 *	unlink first entry and point punlinked to it, or set punlinked
 *	to NULL if the list is empty.
 *
 * UNLINK_SLIST(punlinked, listhead, ptounlink, nextlink, entrytype)
 *	unlink entry pointed to by ptounlink.  punlinked is set to NULL
 *	if the entry is not found on the list, otherwise it is set to
 *	ptounlink.
 *
 * UNLINK_EXPR_SLIST(punlinked, listhead, expr, nextlink, entrytype)
 *	unlink entry where expression expr is nonzero.  expr can refer
 *	to the entry being tested using UNLINK_EXPR_SLIST_CURRENT().
 *	See the	implementation of UNLINK_SLIST() below for an example.
 *	punlinked is pointed to the removed entry or NULL if none
 *	satisfy expr.
 */
#ifndef NTP_LISTS_H
#define NTP_LISTS_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <isc/list.h>


#define LINK_SLIST(listhead, pentry, nextlink)			\
do {								\
	(pentry)->nextlink = (listhead);			\
	(listhead) = (pentry);					\
} while (0)

#define LINK_TAIL_SLIST(listhead, pentry, nextlink, entrytype)	\
do {								\
	entrytype **pptail;					\
								\
	pptail = &(listhead);					\
	while (*pptail != NULL)					\
		pptail = &((*pptail)->nextlink);		\
								\
	(pentry)->nextlink = NULL;				\
	*pptail = (pentry);					\
} while (0)

#define UNLINK_HEAD_SLIST(punlinked, listhead, nextlink)	\
do {								\
	(punlinked) = (listhead);				\
	if (NULL != (punlinked)) {				\
		(listhead) = (punlinked)->nextlink;		\
		(punlinked)->nextlink = NULL;			\
	}							\
} while (0)

#define UNLINK_EXPR_SLIST(punlinked, listhead, expr, nextlink,	\
			  entrytype)				\
do {								\
	entrytype **ppentry;					\
								\
	ppentry = &(listhead);					\
								\
	while (!(expr))						\
		if ((*ppentry)->nextlink != NULL)		\
			ppentry = &((*ppentry)->nextlink);	\
		else {						\
			ppentry = NULL;				\
			break;					\
		}						\
								\
	if (ppentry != NULL) {					\
		(punlinked) = *ppentry;				\
		*ppentry = (punlinked)->nextlink;		\
		(punlinked)->nextlink = NULL;			\
	} else							\
		(punlinked) = NULL;				\
} while (0)
#define UNLINK_EXPR_SLIST_CURRENT()	(*ppentry)

#define UNLINK_SLIST(punlinked, listhead, ptounlink, nextlink,	\
		     entrytype)					\
	UNLINK_EXPR_SLIST(punlinked, listhead, (ptounlink) ==	\
	    UNLINK_EXPR_SLIST_CURRENT(), nextlink, entrytype)

#endif	/* NTP_LISTS_H */
