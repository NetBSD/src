#	$NetBSD: Makefile,v 1.104 2001/08/19 14:21:02 mrg Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils c89 cvs dc diff diff3 egcs gas.new gawk gcc gdb grep
SUBDIR+=	gprof groff gzip ld.new rcs sdiff send-pr texinfo

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

.include <bsd.subdir.mk>
