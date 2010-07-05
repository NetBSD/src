# $NetBSD: Makefile,v 1.12 2010/07/05 12:53:58 njoly Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+=	ffs kernfs lfs msdosfs nullfs ptyfs puffs tmpfs umapfs union
SUBDIR+=	vfs

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
