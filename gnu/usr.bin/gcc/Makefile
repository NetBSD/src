#	$NetBSD: Makefile,v 1.10 2001/08/06 19:46:12 tv Exp $

.include <bsd.own.mk>

.ifdef USE_NEW_TOOLCHAIN

SUBDIR+=	cpp0 gcov backend cc1 cc1obj cc1plus f771 frontend cpp g++ g77 gcc

# Dependencies to allow single-target builds.
all-cc1 all-cc1obj all-cc1plus all-f771: all-backend
all-g++ all-g77 all-gcc all-cpp: all-frontend

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
MKOBJ=no
.include <bsd.prog.mk>
.endif
