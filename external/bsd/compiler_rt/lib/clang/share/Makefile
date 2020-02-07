#	$NetBSD: Makefile,v 1.2 2020/02/07 19:00:10 kamil Exp $

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

NOOBJ=	# defined

.PATH:	${TOPDIR}/lib/asan
INCS+=	asan_blacklist.txt

.PATH:	${TOPDIR}/lib/msan
INCS+=	msan_blacklist.txt

.include <bsd.own.mk>

INCSDIR=	/usr/lib/clang/9.0.0/share

.include <bsd.prog.mk>
