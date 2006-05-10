#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:49 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

# If using an external toolchain, we expect crtbegin/crtend to be
# supplied by that toolchain's run-time support.
.if !defined(EXTERNAL_TOOLCHAIN) && ${MKGCC} != "no" && ${OBJECT_FMT} == "ELF"

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4
GNUHOSTDIST=	${DIST}
GCCARCH=	${NETBSDSRCDIR}/gnu/usr.bin/gcc4/arch/${MACHINE_ARCH}

GALLCFLAGS=	${G_CRTSTUFF_CFLAGS} ${G_CRTSTUFF_T_CFLAGS}

CPPFLAGS+=	-I${GCCARCH} ${GALLCFLAGS:M-D*} ${GALLCFLAGS:M-I*:N-I.*}
CPPFLAGS+=	-I.

GCFLAGS=	${GALLCFLAGS:N-D*:N-I*:N-i*:N./*}

DPSRCS+=	${.CURDIR}/arch/${MACHINE_ARCH}.mk tconfig.h
CLEANFILES+=	cs-tconfig.h tconfig.h

.include "${.CURDIR}/arch/${MACHINE_ARCH}.mk"

SRCS+=		crtbegin.c crtend.c
OBJS+=		crtbegin.o crtend.o
.if ${MKPIC} != "no"
SRCS+=		crtbeginS.c crtendS.c
OBJS+=		crtbeginS.o crtendS.o	# for shared libraries
SRCS+=		crtbeginT.c
OBJS+=		crtbeginT.o		# for -static links
.endif

realall: ${OBJS}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.PATH: ${DIST}/gcc ${DIST}/gcc/config ${G_CONFIGDIR}

tconfig.h:
	${_MKTARGET_CREATE}
	TM_DEFINES="$(G_tm_defines)" \
	HEADERS="$(G_xm_file)" XM_DEFINES="$(G_xm_defines)" \
	TARGET_CPU_DEFAULT="" \
	${HOST_SH} $(GNUHOSTDIST)/gcc/mkconfig.sh tconfig.h

.include <bsd.prog.mk>

# Override the default .c -> .o rule.
.c.o:
	${CC} ${CPPFLAGS} ${GCFLAGS} -c ${.IMPSRC} -o ${.TARGET}.o
	mv ${.TARGET}.o ${.TARGET}

# these aren't necessary but are #include'd
FAKEHEADERS=options.h insn-flags.h insn-constants.h
${FAKEHEADERS}:
	${_MKTARGET_CREATE}
	touch ${.TARGET}
tm.h ${SRCS}: ${FAKEHEADERS}
CLEANFILES+=	${FAKEHEADERS}

.else

.include <bsd.prog.mk>			# do nothing

.endif	# ! EXTERNAL_TOOLCHAIN && MKGCC != no && OBJECT_FMT == ELF
