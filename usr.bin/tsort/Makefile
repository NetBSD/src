#	$NetBSD: Makefile,v 1.9 2020/08/17 16:26:03 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

.include <bsd.own.mk>
WARNS= 6
PROG=	tsort
.if !defined(HOSTPROGNAME)
LDADD+= -lutil
DPADD+= ${LIBUTIL}
.endif

.include <bsd.prog.mk>
