#	$NetBSD: Makefile,v 1.64 1999/01/18 10:18:54 mrg Exp $

# XXX for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar gdb

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "vax") || \
    (${MACHINE_ARCH} == "sparc64")
SUBDIR+=	gas.new
.elif (${MACHINE_ARCH} != "arm32")
SUBDIR+=	gas
.endif

# XXX mips vs mips{el,eb}.  Should be all ELF hosts anyway.
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "sparc64")
SUBDIR+=	ld.new
.elif (${MACHINE_ARCH} != "arm32")
SUBDIR+=	ld
.endif

.if (${OBJECT_FMT} == "ELF") && (${MACHINE_ARCH} != "mips")
SUBDIR+=	gprof
.endif

.include <bsd.subdir.mk>
