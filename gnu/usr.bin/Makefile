#	$NetBSD: Makefile,v 1.49 1998/08/16 17:16:01 tv Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

.if (${MACHINE_ARCH} == "arm32")
SUBDIR+=	gas.new
.else
SUBDIR+=	gas
.endif

SUBDIR+=	gdb ld

.ifdef USE_EGCS
#SUBDIR+=	egcs			- currently in flux
.else
SUBDIR+=	gcc
.endif

.include <bsd.subdir.mk>
