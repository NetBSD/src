#	$NetBSD: Makefile,v 1.11 2001/10/08 22:27:34 tv Exp $

.include <bsd.own.mk>

.ifdef USE_NEW_TOOLCHAIN

SUBDIR+=	backend frontend cpp0 gcov .WAIT \
		cc1 cc1obj cc1plus f771 cpp g++ g77 gcc

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
MKOBJ=no
.include <bsd.prog.mk>
.endif
