#	$NetBSD: Makefile,v 1.47 1998/05/19 19:09:31 tv Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+=	bc cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

.if (${MACHINE_ARCH} == "arm32")
SUBDIR+=	gas.new
.else
SUBDIR+=	gas
.endif

SUBDIR+=	gdb ld

.ifdef USE_EGCS
SUBDIR+=	egcs
.else
SUBDIR+=	gcc
.endif

.include <bsd.subdir.mk>
