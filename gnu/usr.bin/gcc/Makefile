#	$NetBSD: Makefile,v 1.9 2001/07/18 14:02:26 tv Exp $

.include <bsd.own.mk>

.ifdef USE_NEW_TOOLCHAIN

SUBDIR+=	cpp0 backend cc1 cc1obj cc1plus f771 frontend cpp g++ g77 gcc

# Dependencies to allow single-target builds.
all-cc1 all-cc1obj all-cc1plus all-f771: all-backend
all-g++ all-g77 all-gcc all-cpp: all-frontend

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
MKOBJ=no
.include <bsd.prog.mk>
.endif
