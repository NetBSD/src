#	$NetBSD: Makefile.inc,v 1.1.4.2 2008/01/09 01:34:10 matt Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

.PATH: ${.CURDIR}/misc

# constructor
SRCS+=	initfini.c

# for -fstack-protector
SRCS+=	stack_protector.c
