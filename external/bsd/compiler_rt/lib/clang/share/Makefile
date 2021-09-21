#	$NetBSD: Makefile,v 1.3 2021/09/21 20:34:38 christos Exp $

NOOBJ=	# defined
.include <bsd.init.mk>

.PATH:	${TOPDIR}/lib/asan
INCS+=	asan_blacklist.txt

.PATH:	${TOPDIR}/lib/msan
INCS+=	msan_blacklist.txt

INCSDIR=	/usr/lib/clang/${CLANG_VER}/share

.include <bsd.prog.mk>
