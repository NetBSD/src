#	$NetBSD: Makefile,v 1.17 2002/02/11 21:45:18 tv Exp $

NOOBJ=# defined

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN) && ${MKGCC} != "no" && \
    exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)

SUBDIR+=	backend frontend cpp0 gcov .WAIT \
		cc1 cc1obj cc1plus cpp f771 g++ g77 gcc protoize unprotoize

.include <bsd.subdir.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
