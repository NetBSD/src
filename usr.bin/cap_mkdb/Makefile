#	$NetBSD: Makefile,v 1.9 2011/06/22 02:49:44 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	cap_mkdb

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.cap_mkdb.c+=	-Wno-error
.endif
