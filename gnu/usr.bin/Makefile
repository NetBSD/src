#	$NetBSD: Makefile,v 1.76 1999/02/09 17:47:50 tv Exp $

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep groff
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

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "i386")
SUBDIR+=	gprof
.endif

.include <bsd.subdir.mk>
