#	$NetBSD: Makefile.inc,v 1.1.32.1 2014/05/22 11:36:52 yamt Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

.PATH: ${ARCHDIR}/misc ${.CURDIR}/misc

# constructor
SRCS+=	initfini.c

# for -fstack-protector
SRCS+=	stack_protector.c
