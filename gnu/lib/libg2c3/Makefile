#	$NetBSD: Makefile,v 1.6 2006/04/09 02:52:20 christos Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		g2c

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc
GNUHOSTDIST=	${DIST}

SRCS=		${G_OBJS:.lo=.c}

GCPPFLAGS=	${G_ALL_CFLAGS:O:u}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-DAllow_TYQUAD

.for f in etime_.c dtime_.c
${f:.c=.d} ${f:.c=.o} ${f:.c=.so} ${f:.c=.po}: ${DIST}/libf2c/libU77/$f
.endfor

.for c in F I U
CLEANFILES+=	Version${c}.c
Version${c}.c: ${DIST}/libf2c/lib${c}77/Version.c
	${_MKTARGET_CREATE}
	cp $> $@
.endfor

.for f in ${G_F2CEXT:N*y2kbug*}
OBJS+=		L${f}.o

L${f}.o: f2cext.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -DL${f} -o $@ ${>:M*.c}

.if ${MKPIC} != "no"
L${f}.so: f2cext.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -DL${f} -o $@ ${>:M*.c}
.endif

.if ${MKDEBUG} != "no"
L${f}.go: f2cext.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CPICFLAGS} -DL${f} -o $@ ${>:M*.c}
.endif

.if ${MKPROFILE} != "no"
L${f}.po: f2cext.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} -pg -DL${f} -o $@ ${>:M*.c}
.endif
.endfor

.PATH: ${DIST}/libf2c ${DIST}/libf2c/libF77 ${DIST}/libf2c/libI77 ${DIST}/libf2c/libU77

.include <bsd.lib.mk>

${OBJS}: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk

.else
.include <bsd.prog.mk> # do nothing
.endif
