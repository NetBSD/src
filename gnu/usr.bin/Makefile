#	$NetBSD: Makefile,v 1.67 1999/02/03 00:17:35 tv Exp $

# XXX for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

.if (${MACHINE_ARCH} == "i386")
SUBDIR+=	gdb
.endif

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "i386")
# || (${MACHINE_ARCH} == "mips") || \
#    (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "vax") || \
#    (${MACHINE_ARCH} == "sparc64")
SUBDIR+=	gas.new
.elif (${MACHINE_ARCH} != "arm32")
SUBDIR+=	gas
.endif

# XXX mips vs mips{el,eb}.  Should be all ELF hosts anyway.
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "sparc64")
# SUBDIR+=	ld.new
.elif (${MACHINE_ARCH} != "arm32")
SUBDIR+=	ld
.endif

.if (${MACHINE_ARCH} == "i386")
SUBDIR+=	gprof
.endif

.include <bsd.subdir.mk>
