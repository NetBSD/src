#	$NetBSD: Makefile,v 1.13 2001/12/15 03:37:37 thorpej Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN) && !defined(BOOTSTRAP_NEW_TOOLCHAIN)

SUBDIR+=	backend frontend cpp0 gcov .WAIT \
		cc1 cc1obj cc1plus f771 cpp g++ g77 gcc

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
NOOBJ=# defined
.include <bsd.prog.mk>
.endif
