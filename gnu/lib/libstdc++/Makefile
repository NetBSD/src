#	$NetBSD: Makefile,v 1.22 2003/06/01 02:08:08 thorpej Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		stdc++

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	4
SHLIB_MINOR=	0

SUBDIR=		include io config

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain
GNUHOSTDIST=	${DIST}

SRCS=		${G_OBJS:.o=.cc} ${G_LIBIBERTY_OBJS:.o=.c}
INCS=		_G_config.h
INCSDIR=	/usr/include/g++

# Ugly, but they aren't identified individually in libio/Makefile:
.for f in ${G_LIBIOSTREAM_USE}
.if exists(${DIST}/libio/${f:.o=.c})
SRCS+=		${f:.o=.c}
.else
SRCS+=		${f:.o=.cc}
.endif
.endfor

GCPPFLAGS=	${G_XCFLAGS} ${G_CINCLUDES} ${G_MT_CFLAGS} ${G_CXXINCLUDES}
CPPFLAGS+=	-nostdinc++ -I${.CURDIR}/arch/${MACHINE_ARCH} \
		${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${DIST}/gcc/cp/inc \
		-I${NETBSDSRCDIR}/gnu/lib/libiberty -I${DIST}/include

CXXFLAGS+=	${G_XCXXFLAGS:M-f*}

TEXINFO=	iostream.texi

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl ${DIST}/libio ${DIST}/libiberty

.for t in C
.for v in STRFUNCS STRIO
CLEANFILES+=	${t}${v}.cc
SRCS+=		${t}${v}.cc

${t}${v}.cc: Makefile ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk sinst.cc
	echo '#define ${t}' >$@
.for f in ${G_${v}}
	echo '#define ${f}' >>$@
.endfor
	cat ${>:M*.cc} >>$@
.endfor
.endfor

.for t in F D LD
.for v in COMFUNCS COMIO
CLEANFILES+=	${t}${v}.cc
SRCS+=		${t}${v}.cc

${t}${v}.cc: Makefile ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk cinst.cc
	echo '#define ${t}' >$@
.for f in ${G_${v}}
	echo '#define ${f}' >>$@
.endfor
	cat ${>:M*.cc} >>$@
.endfor
.endfor

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.info.mk>

${OBJS}: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif

.if (${OBJECT_FMT} == "a.out")
LDADD=	${DESTDIR}/usr/lib/c++rt0.o
DPADD=	${DESTDIR}/usr/lib/c++rt0.o
.endif
