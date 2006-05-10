#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		objc

.if ${MKGCC} != "no"

# ?? bump this?
SHLIB_MAJOR=	2
SHLIB_MINOR=	0

# Machine-independent definitions (include file names).
.include "${.CURDIR}/defs.mk"

LIBGCCOBJ!=    cd ${.CURDIR}/../libgcc4/libgcc && ${PRINTOBJDIR}

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4
GNUHOSTDIST=	${NETBSDSRCDIR}/gnu/dist/gcc4
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc4/arch/${MACHINE_ARCH}
GCCLIBXX=	${NETBSDSRCDIR}/gnu/lib/libstdc++-v3_4/arch/${MACHINE_ARCH}

SRCS=		${G_OBJS:N[A-Z]*:Nlinking.lo:.lo=.c} ${G_OBJS:M[A-Z]*:.lo=.m} linking.m

GCPPFLAGS=	${G_ALL_CFLAGS} ${G_INCLUDES}
CPPFLAGS+=	-I. -I${GCCARCH} -I${LIBGCCOBJ} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}
CPPFLAGS+=	-I${GCCLIBXX}

BUILDSYMLINKS=	${.CURDIR}/../libstdc++-v3_4/arch/${MACHINE_ARCH} bits
DPSRCS+=	bits

# XXX this doesn't seem to work?
.for f in Protocol.m Object.m NXConstStr.m linking.m
COPTS.{$f}=	-fgnu-runtime -Wno-error
.endfor
# XXX
NOGCCERROR=1

INCS=		${G_OBJC_H}
INCSDIR=	/usr/include/objc

DPSRCS+=	runtime-info.h
CLEANFILES+=	rtscratch rtscratch.s runtime-info.h
runtime-info.h:
	${_MKTARGET_CREATE}
	touch rtscratch
	`${OBJC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

.PATH: ${DIST}/libobjc ${DIST}/libobjc/objc

.include <bsd.lib.mk>

${OBJS} ${POBJS} ${SOBJS}: runtime-info.h

# these aren't necessary but are #include'd
FAKEHEADERS=options.h insn-flags.h insn-constants.h
${FAKEHEADERS}:
	${_MKTARGET_CREATE}
	touch ${.TARGET}
tm.h ${SRCS}: ${FAKEHEADERS}
CLEANFILES+=	${FAKEHEADERS}

${SRCS}: unwind.h
unwind.h: ${G_UNWIND_H}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${G_UNWIND_H} ${.TARGET}
CLEANFILES+= unwind.h

.else
.include <bsd.prog.mk>	# do nothing
.endif
