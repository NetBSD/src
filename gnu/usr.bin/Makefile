#	$NetBSD: Makefile,v 1.62 1998/12/17 22:36:42 thorpej Exp $

# XXX for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar gdb

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "arm32") || \
    (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "vax")
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

.if (${OBJECT_FMT} == "ELF") && (${MACHINE_ARCH} != "mips")
SUBDIR+=	gprof
.endif

.include <bsd.subdir.mk>
