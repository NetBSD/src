#	$NetBSD: Makefile,v 1.1.2.2 2019/08/16 18:44:07 martin Exp $

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

NOOBJ=	# defined

.PATH:	${TOPDIR}/lib/asan
INCS+=	asan_blacklist.txt

.PATH:	${TOPDIR}/lib/msan
INCS+=	msan_blacklist.txt

.include <bsd.own.mk>

INCSDIR=	/usr/lib/clang/7.0.0/share

.include <bsd.prog.mk>
