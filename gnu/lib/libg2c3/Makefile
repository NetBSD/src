#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:41 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		g2c

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc
GNUHOSTDIST=	${DIST}

SRCS=		${G_OBJS:.lo=.c}

GCPPFLAGS=	${G_ALL_CFLAGS:O:u}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-DAllow_TYQUAD

# ugly; there is a wrong etime_.c in libF77 - we want libU77's
.for f in etime_.c dtime_.c
CLEANFILES+=	${f}
${f:.c=.o} ${f:.c=.po} ${f:.c=.so}: ./${f}
${f}: ${DIST}/libf2c/libU77/${f}
	rm -f ${f}
	ln -s $> ${f}
.endfor

.for c in F I U
CLEANFILES+=	Version${c}.c
Version${c}.c: ${DIST}/libf2c/lib${c}77/Version.c
	cp $> $@
.endfor

.for f in ${G_F2CEXT:N*y2kbug*}
OBJS+=		L${f}.o

L${f}.o: f2cext.c
	${COMPILE.c} -DL${f} -o $@ ${>:M*.c}

.if ${MKPIC} != "no"
L${f}.so: f2cext.c
	${COMPILE.c} ${CPICFLAGS} -DL${f} -o $@ ${>:M*.c}
.endif

.if ${MKPROFILE} != "no"
L${f}.po: f2cext.c
	${COMPILE.c} -pg -DL${f} -o $@ ${>:M*.c}
.endif
.endfor

.PATH: ${DIST}/libf2c ${DIST}/libf2c/libF77 ${DIST}/libf2c/libI77 ${DIST}/libf2c/libU77

.include <bsd.lib.mk>

${OBJS}: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk

.else
.include <bsd.prog.mk> # do nothing
.endif
