#	$NetBSD: Makefile,v 1.21 2002/11/12 14:33:49 itohy Exp $

.include <bsd.own.mk>   # For OBJECT_FMT
.include <bsd.shlib.mk>

.if (${OBJECT_FMT} == "a.out" ||	\
    (${OBJECT_FMT} == "ELF" &&		\
    (${MACHINE_ARCH} == "arm" ||	\
     ${MACHINE_ARCH} == "i386" ||	\
     ${MACHINE_ARCH} == "m68k" ||	\
     ${MACHINE_ARCH} == "sparc" ||	\
     ${MACHINE_ARCH} == "vax")))

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I${NETBSDSRCDIR}/libexec/ld.aout_so
.PATH:	${NETBSDSRCDIR}/libexec/ld.aout_so

.if ${SHLIBINSTALLDIR} != ${LIBDIR}
CPPFLAGS+=-DSTANDARD_SEARCH_DIRS=\"${SHLIBINSTALLDIR}\",\"${LIBDIR}\"
.endif

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
