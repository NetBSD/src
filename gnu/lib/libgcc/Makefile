#	$NetBSD: Makefile,v 1.45 2003/06/01 02:09:31 thorpej Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		gcc

.cc: # disable .cc->NULL transform

.if exists(${.CURDIR}/${MACHINE_ARCH}.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/${MACHINE_ARCH}.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain
GNUHOSTDIST=	${DIST}
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc/arch/${MACHINE_ARCH}

GCPPFLAGS=	${G_LIBGCC2_CFLAGS} ${G_MAYBE_USE_COLLECT2} ${G_INCLUDES}
CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${DIST}/gcc/cp

LIB2FUNCS=	${G_LIB2FUNCS:=.c}
.if defined(G_LIB2FUNCS_EH)
LIB2FUNCS_EH=	${G_LIB2FUNCS_EH:=.c}
.endif
CXX_LIB2FUNCS=	${G_CXX_LIB2FUNCS:Mop*:S/opdel.o/opdelete.o/:.o=.cc}

SRCS+=		${LIB2FUNCS} ${LIB2FUNCS_EH} ${CXX_LIB2FUNCS} \
		${G_CXX_LIB2FUNCS:Nop*:.o=.cc} ${G_LIB2ADD:M*.c:T}
OBJS+=		${G_LIB2FUNCS_EH:=.o}

# XXX fp-bit.c should be removed from this list after some grace period.
CLEANFILES+=	${LIB2FUNCS} ${CXX_LIB2FUNCS} dp-bit.c _eh.c fp-bit.c

${LIB2FUNCS}: ${.CURDIR}/Makefile
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

.if defined(G_LIB2FUNCS_EH)
${LIB2FUNCS_EH}: ${.CURDIR}/Makefile
	printf '#define L${.PREFIX}\n#include <libgcc2.c>\n' >${.TARGET}

${G_LIB2FUNCS_EH:=.o}:
	${COMPILE.c} -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2FUNCS_EH:=.po}:
	${COMPILE.c} -pg -fexceptions -o ${.TARGET} ${.IMPSRC}

${G_LIB2FUNCS_EH:=.so}:
	${COMPILE.c} ${CPICFLAGS} -fexceptions -o ${.TARGET} ${.IMPSRC}
.endif	# G_LIB2FUNCS_EH

${CXX_LIB2FUNCS}: ${.CURDIR}/Makefile
	printf '#define L_op_${.PREFIX:S/^op//}\n#include <new1.cc>\n#include <new2.cc>\n' >${.TARGET}

dp-bit.c: ${.CURDIR}/Makefile
	printf '#define FLOAT\n#include <fp-bit.c>\n' >${.TARGET}

INCS=		exception new new.h typeinfo
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/cp/inc ${DIST}/gcc/config \
	${G_CONFIGDIR}

.include <bsd.lib.mk>

${OBJS} ${SOBJS} ${POBJS}: ${.CURDIR}/${MACHINE_ARCH}.mk
.else
.include <bsd.prog.mk> # do nothing
.endif
