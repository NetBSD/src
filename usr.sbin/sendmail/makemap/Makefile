#	$NetBSD: Makefile,v 1.10 1998/01/09 08:10:44 perry Exp $
#
#	@(#)Makefile	8.4 (Berkeley) 6/10/97

PROG=	makemap
MAN=	makemap.8
CPPFLAGS+=-I${.CURDIR}/../src -DNEWDB -DNOT_SENDMAIL

SRCS=	makemap.c safefile.c

.PATH: ${.CURDIR}/../src
.include <bsd.prog.mk>
