#	$NetBSD: Makefile,v 1.15 2001/12/31 23:04:12 thorpej Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN) && ${MKGCC} != "no"

SUBDIR+=	backend frontend cpp0 gcov .WAIT \
		cc1 cc1obj cc1plus cpp f771 g++ g77 gcc protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
NOOBJ=# defined
.include <bsd.prog.mk>
.endif
