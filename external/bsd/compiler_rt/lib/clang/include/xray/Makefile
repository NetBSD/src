#	$NetBSD: Makefile,v 1.2 2020/02/07 19:00:10 kamil Exp $

TOPDIR= ${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

NOOBJ=	# defined

.PATH:	${TOPDIR}/include/xray

INCS+=	xray_interface.h
INCS+=	xray_log_interface.h
INCS+=	xray_records.h

.include <bsd.own.mk>

INCSDIR=	/usr/lib/clang/9.0.0/include/xray

.include <bsd.prog.mk>
