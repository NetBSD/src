#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.5 1994/06/11 08:04:23 mycroft Exp $

PROG=	kgmon
MAN8=	kgmon.0
DPADD=	${LIBKVM}
LDADD=	-lkvm
#
# This program may safely be run setuid-root to allow non-root
# users to start, stop, and reset profiling buffers.
#
#BINOWN=root
#BINMODE=4555

.include <bsd.prog.mk>
