#	$NetBSD: Makefile,v 1.89 1999/05/01 16:38:17 veego Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep gprof groff
SUBDIR+=	gzip ld.new rcs sdiff send-pr sort tar texinfo

.if (${MACHINE_ARCH} != "powerpc") && \
    (${MACHINE_ARCH} != "sparc64")
SUBDIR+=	gdb
.endif

.if (${MACHINE_ARCH} == "m68k" && ${OBJECT_FMT} != "ELF") ||	\
    (${MACHINE_ARCH} == "ns32k")
SUBDIR+=	gas
.else
SUBDIR+=	gas.new
.endif

.if (${MACHINE_ARCH} == "arm32") ||				\
    (${MACHINE_ARCH} == "i386" && ${OBJECT_FMT} != "ELF") ||	\
    (${MACHINE_ARCH} == "m68k" && ${OBJECT_FMT} != "ELF") ||	\
    (${MACHINE_ARCH} == "ns32k") ||				\
    (${MACHINE_ARCH} == "sparc" && ${OBJECT_FMT} != "ELF") ||	\
    (${MACHINE_ARCH} == "vax")
SUBDIR+=	ld
.endif

.include <bsd.subdir.mk>
