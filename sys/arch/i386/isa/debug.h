/*	$Id: debug.h,v 1.4.2.2 1993/11/11 02:16:35 mycroft Exp $ */

/* #define	SHOW_A_LOT */

#ifdef INTR_DEBUG
#define INTRLOCAL(label) label
#else /* not INTR_DEBUG */
#define INTRLOCAL(label) L/**/label
#endif /* INTR_DEBUG */

#ifdef BDB
#define	BDBTRAP(name) \
	ss ; \
	cmpb	$0,_bdb_exists ; \
	jz	1f ; \
	testb	$SEL_RPL_MASK,4(%esp) ; \
	jnz	1f ; \
	ss ; \
bdb_/**/name/**/_ljmp: ; \
	ljmp	$0,$0 ; \
1:
#endif

	.data
	.globl	_bdb_exists
_bdb_exists:
	.long	0
	.text
