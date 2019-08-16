#	$NetBSD: Makefile,v 1.1.2.2 2019/08/16 18:44:10 martin Exp $

TOPDIR= ${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

NOOBJ=	# defined

.PATH:	${TOPDIR}/include/xray

INCS+=	xray_interface.h
INCS+=	xray_log_interface.h
INCS+=	xray_records.h

.include <bsd.own.mk>

INCSDIR=	/usr/lib/clang/7.0.0/include/xray

.include <bsd.prog.mk>
