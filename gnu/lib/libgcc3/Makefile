#	$NetBSD: Makefile,v 1.4 2003/07/27 14:41:44 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		gcc

.cc: # disable .cc->NULL transform

.if exists(${.CURDIR}/${MACHINE_ARCH}.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/${MACHINE_ARCH}.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc
GNUHOSTDIST=	${DIST}
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc3/arch/${MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_MAYBE_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${DIST}/gcc/cp -I.

LIB2FUNCS=	${G_LIB2FUNCS_1:=.c} ${G_LIB2FUNCS_2:=.c} ${G_LIB2FUNCS_ST:=.c}
LIB2DIVMOD=	${G_LIB2_DIVMOD_FUNCS:=.c}
LIB2_EH=	${G_LIB2ADDEH:M*.c:T}

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} ${G_LIB2ADD:M*.c:T}

CLEANFILES+=	${LIB2FUNCS} tconfig.h

${LIB2FUNCS}: ${.CURDIR}/Makefile
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

${LIB2DIVMOD}: ${.CURDIR}/Makefile
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

# XXX
.if ${MACHINE_ARCH} == "m68000"
CPICFLAGS:=
.endif

${G_LIB2ADDEH:=.o}:
	${COMPILE.c} ${CPICFLAGS} -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2_DIVMOD_FUNCS:=.o}:
	${COMPILE.c} ${CPICFLAGS} -fexceptions -fnon-call-exceptions -o ${.TARGET} ${.IMPSRC}

dp-bit.c: ${.CURDIR}/Makefile
	printf '#define FLOAT\n#include <fp-bit.c>\n' >${.TARGET}

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${G_CONFIGDIR}

.include <bsd.lib.mk>

${OBJS} ${SOBJS} ${POBJS} .depend: ${.CURDIR}/${MACHINE_ARCH}.mk tconfig.h

# XXX /bin/sh
tconfig.h:
	TM_DEFINES="$(G_tm_defines)" \
	HEADERS="$(G_xm_file)" XM_DEFINES="$(G_xm_defines)" \
	TARGET_CPU_DEFAULT="" \
	/bin/sh $(GNUHOSTDIST)/gcc/mkconfig.sh tconfig.h

.else
.include <bsd.prog.mk> # do nothing
.endif
