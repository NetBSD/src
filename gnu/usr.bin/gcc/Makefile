#	$NetBSD: Makefile,v 1.16 2002/01/22 00:59:46 thorpej Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN) && ${MKGCC} != "no"

SUBDIR+=	backend frontend cpp0 gcov .WAIT \
		cc1 cc1obj cc1plus cpp f771 g++ g77 gcc protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
