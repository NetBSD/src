#	$NetBSD: Makefile,v 1.101 2001/07/27 23:01:22 eeh Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils cvs dc diff diff3 egcs gas.new gawk gdb grep gprof
SUBDIR+=	groff gzip ld.new rcs sdiff send-pr texinfo

SUBDIR+=	cpio
SUBDIR+=	tar
SUBDIR+=	gettext

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
.if (${MACHINE_ARCH} == "sparc64")
SUBDIR+= gcc
.endif

.include <bsd.subdir.mk>
