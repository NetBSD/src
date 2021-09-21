#	$NetBSD: Makefile,v 1.3 2021/09/21 20:34:37 christos Exp $

NOOBJ=	# defined

.include <bsd.init.mk>

.PATH:	${TOPDIR}/include/xray

INCS+=	xray_interface.h
INCS+=	xray_log_interface.h
INCS+=	xray_records.h

INCSDIR=	/usr/lib/clang/${CLANG_VER}/include/xray

.include <bsd.prog.mk>
