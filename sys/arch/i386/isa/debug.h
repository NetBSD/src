/*	$Id: debug.h,v 1.4.2.1 1993/10/09 10:31:16 mycroft Exp $ */

/* #define	SHOW_A_LOT */

#ifdef INTR_DEBUG
#define INTRLOCAL(label) label
#else /* not INTR_DEBUG */
#define INTRLOCAL(label) L/**/label
#endif /* INTR_DEBUG */

#define	BDBTRAP(name) \
	ss ; \
	cmpb	$0,_bdb_exists ; \
	je	1f ; \
	testb	$SEL_RPL_MASK,4(%esp) ; \
	jne	1f ; \
	ss ; \
bdb_/**/name/**/_ljmp: ; \
	ljmp	$0,$0 ; \
1:

#ifdef INTR_DEBUG
#define	COUNT_EVENT(group, event)	incl	(group) + (event) * 4
#else /* not INTR_DEBUG */
#define	COUNT_EVENT(group, event)
#endif /* INTR_DEBUG */

#define	COUNT_INTR(group, event)	incl	(group) + (event) * 4

	.data
	.globl	_bdb_exists
_bdb_exists:
	.long	0
	.text
