#	$NetBSD: Makefile,v 1.8 2001/06/18 16:18:30 tv Exp $

.include <bsd.own.mk>

.ifdef USE_NEW_TOOLCHAIN

SUBDIR+=	backend cc1 cc1obj cc1plus f771 frontend g++ g77 gcc

# Dependencies to allow single-target builds.
all-cc1 all-cc1obj all-cc1plus all-f771: all-backend
all-g++ all-g77 all-gcc: all-frontend

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
MKOBJ=no
.include <bsd.prog.mk>
.endif
