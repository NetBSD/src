#	$NetBSD: Makefile,v 1.95 2000/07/04 17:12:00 thorpej Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils dc diff diff3 egcs gas.new gawk grep gprof
SUBDIR+=	groff gzip ld.new rcs sdiff send-pr sort texinfo

SUBDIR+=	cpio
SUBDIR+=	tar

.if (${MACHINE_ARCH} != "sparc64")
SUBDIR+=	gdb
.endif

.if (${MACHINE_ARCH} == "m68k" && ${OBJECT_FMT} != "ELF") ||	\
    (${MACHINE_ARCH} == "ns32k")
SUBDIR+=	gas
.endif

.if (${OBJECT_FMT} != "ELF") &&		\
    ((${MACHINE_ARCH} == "arm32") ||	\
     (${MACHINE_ARCH} == "i386") ||	\
     (${MACHINE_ARCH} == "m68k") ||	\
     (${MACHINE_ARCH} == "ns32k") ||	\
     (${MACHINE_ARCH} == "sparc") ||	\
     (${MACHINE_ARCH} == "vax"))
SUBDIR+=	ld
.endif

.include <bsd.subdir.mk>
