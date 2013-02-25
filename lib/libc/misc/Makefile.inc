#	$NetBSD: Makefile.inc,v 1.1.38.1 2013/02/25 00:27:51 tls Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

.PATH: ${ARCHDIR}/misc ${.CURDIR}/misc

# constructor
SRCS+=	initfini.c

# for -fstack-protector
SRCS+=	stack_protector.c
