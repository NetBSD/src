#	$NetBSD: Makefile,v 1.58 1998/11/05 04:14:56 jonathan Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar gdb

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "arm32") || \
    (${MACHINE_ARCH} == "mips") || (${MACHINE_ARCH} == "vax")
SUBDIR+=	gas.new
.else
SUBDIR+=	gas
.endif

# XXX mips vs mips{el,eb}.  Should be all ELF hosts anyway.
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "mips")
SUBDIR+=	ld.new ld/rtld ld/ldd ld/ldconfig
.else
SUBDIR+=	ld
.endif

.ifdef USE_EGCS
SUBDIR+=	egcs
.else
SUBDIR+=	gcc
.endif

.if (${OBJECT_FMT} == "ELF") && (${MACHINE_ARCH} != "mips")
SUBDIR+=	gprof
.endif

.include <bsd.subdir.mk>
