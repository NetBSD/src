#	$NetBSD: Makefile,v 1.53 1998/09/30 16:47:47 jonathan Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar gdb

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "arm32") || \
    (${MACHINE_ARCH} == "mips")
SUBDIR+=	gas.new
.else
SUBDIR+=	gas
.endif

# XXX mips vs mips{el,eb}.  Should be all ELF hosts anyway.
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "mips")
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

