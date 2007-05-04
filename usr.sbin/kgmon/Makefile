#	$NetBSD: Makefile,v 1.9 2007/05/04 16:19:43 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	kgmon
MAN=	kgmon.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
WARNS=	4
#
# This program may safely be run setuid-root to allow non-root
# users to start, stop, and reset profiling buffers.
#
#BINOWN=root
#BINMODE=4555

.include <bsd.prog.mk>
