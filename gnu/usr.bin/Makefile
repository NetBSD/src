#	$NetBSD: Makefile,v 1.51 1998/08/27 21:24:59 tv Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar gdb

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "arm32")
SUBDIR+=	gas.new
.else
SUBDIR+=	gas
.endif

.if (${MACHINE_ARCH} == "alpha")
SUBDIR+=	ld.new
.else
SUBDIR+=	ld
.endif

.ifdef USE_EGCS
SUBDIR+=	egcs
.else
SUBDIR+=	gcc
.endif

.include <bsd.subdir.mk>
