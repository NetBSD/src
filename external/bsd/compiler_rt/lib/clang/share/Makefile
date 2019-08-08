#	$NetBSD: Makefile,v 1.1 2019/08/08 14:00:32 kamil Exp $

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

NOOBJ=	# defined

.PATH:	${TOPDIR}/lib/asan
INCS+=	asan_blacklist.txt

.PATH:	${TOPDIR}/lib/msan
INCS+=	msan_blacklist.txt

.include <bsd.own.mk>

INCSDIR=	/usr/lib/clang/7.0.0/share

.include <bsd.prog.mk>
