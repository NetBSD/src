#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.6 1994/12/22 11:35:06 cgd Exp $

PROG=	kgmon
MAN=	kgmon.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
#
# This program may safely be run setuid-root to allow non-root
# users to start, stop, and reset profiling buffers.
#
#BINOWN=root
#BINMODE=4555

.include <bsd.prog.mk>
