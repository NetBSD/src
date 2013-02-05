#	$NetBSD: Makefile,v 1.26 2013/02/05 07:23:00 matt Exp $

.include <bsd.own.mk>   # For MKPIC
.include <bsd.shlib.mk>
.include <bsd.endian.mk>

.if ${MKPIC} != "no" && \
    ((${MACHINE_CPU} == "arm" && ${TARGET_ENDIANNESS} == "1234") || \
     ${MACHINE_ARCH} == "i386" ||	\
     ${MACHINE_ARCH} == "m68k" ||	\
     ${MACHINE_ARCH} == "sparc" ||	\
     ${MACHINE_ARCH} == "vax")

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

.if ${SHLIBINSTALLDIR} != ${LIBDIR}
CPPFLAGS+=-DSTANDARD_SEARCH_DIRS=\"${SHLIBINSTALLDIR}\",\"${LIBDIR}\"
.endif

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
