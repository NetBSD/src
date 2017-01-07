#	$NetBSD: Makefile,v 1.10.28.1 2017/01/07 08:56:59 pgoyette Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

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

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
