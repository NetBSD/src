#	$NetBSD: Makefile,v 1.71 1999/02/03 02:14:55 tv Exp $

SUBDIR+=	bc binutils cpio dc diff diff3 egcs gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

.if (${MACHINE_ARCH} == "i386")
SUBDIR+=	gdb
.endif

.if (${MACHINE_ARCH} == "m68k") || \
    (${MACHINE_ARCH} == "ns32k")
SUBDIR+=	gas
.elif (${MACHINE_ARCH} == "alpha") || \
      (${MACHINE_ARCH} == "arm32") || \
      (${MACHINE_ARCH} == "i386")
SUBDIR+=	gas.new
.endif

.if (${MACHINE_ARCH} == "i386") || \
    (${MACHINE_ARCH} == "m68k") || \
    (${MACHINE_ARCH} == "ns32k") || \
    (${MACHINE_ARCH} == "sparc") || \
    (${MACHINE_ARCH} == "vax")
SUBDIR+=	ld
.else
#SUBDIR+=	ld.new
.endif

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "i386")
SUBDIR+=	gprof
.endif

.include <bsd.subdir.mk>
