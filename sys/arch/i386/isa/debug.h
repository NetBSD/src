/*	$Id: debug.h,v 1.4.2.3 1993/11/11 16:56:35 mycroft Exp $ */

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

	.data
	.globl	_bdb_exists
_bdb_exists:
	.long	0
	.text
#endif
