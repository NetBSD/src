#	$NetBSD: Makefile,v 1.79 1999/02/11 00:49:43 tv Exp $

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep gprof groff
SUBDIR+=	gzip ld.new rcs sdiff send-pr sort tar

.if (${MACHINE_ARCH} == "i386")
SUBDIR+=	gdb
.endif

.if (${MACHINE_ARCH} == "m68k") || \
    (${MACHINE_ARCH} == "ns32k")
SUBDIR+=	gas
.else
SUBDIR+=	gas.new
.endif

# for OBJECT_FMT
.include <bsd.own.mk>

.if (${MACHINE_ARCH} == "i386" && ${OBJECT_FMT} != "ELF") || \
    (${MACHINE_ARCH} == "m68k") || \
    (${MACHINE_ARCH} == "ns32k") || \
    (${MACHINE_ARCH} == "sparc" && ${OBJECT_FMT} != "ELF") || \
    (${MACHINE_ARCH} == "vax")
SUBDIR+=	ld
.endif

.include <bsd.subdir.mk>
