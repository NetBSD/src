#	$NetBSD: Makefile,v 1.12 2001/12/12 01:49:01 tv Exp $

.include <bsd.own.mk>

.ifdef USE_NEW_TOOLCHAIN

SUBDIR+=	backend frontend cpp0 gcov .WAIT \
		cc1 cc1obj cc1plus f771 cpp g++ g77 gcc

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
NOOBJ=# defined
.include <bsd.prog.mk>
.endif
