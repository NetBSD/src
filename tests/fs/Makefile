# $NetBSD: Makefile,v 1.14 2010/07/06 14:44:29 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs

SUBDIR+=	ffs kernfs lfs msdosfs nullfs psshfs ptyfs puffs
SUBDIR+=	tmpfs umapfs union
SUBDIR+=	vfs

FILES= h_funcs.subr
FILESDIR= ${TESTSDIR}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
