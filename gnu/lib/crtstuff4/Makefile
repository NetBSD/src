#	$NetBSD: Makefile,v 1.3.10.1 2009/12/22 04:51:26 mrg Exp $

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

# these aren't necessary but are #include'd
FAKEHEADERS=options.h insn-flags.h insn-constants.h
${FAKEHEADERS}:
	${_MKTARGET_CREATE}
	touch ${.TARGET}
DPSRCS+=	${FAKEHEADERS}
CLEANFILES+=	${FAKEHEADERS}

${OBJS}: ${DPSRCS}

.include <bsd.prog.mk>

# Override the default .c -> .o rule.
.c.o:
	${_MKTARGET_COMPILE}
	${CC} ${CPPFLAGS} ${GCFLAGS} ${CPUFLAGS} -c ${.IMPSRC} -o ${.TARGET}.o
	mv ${.TARGET}.o ${.TARGET}

.if (${MACHINE_ARCH} == "mips64eb") || (${MACHINE_ARCH} == "mips64el")
# Turn off as(1) warnings on MIPS, since warnings are fatal with WARNS>0
# and GCC configury passes -finhibit-size-directive which causes mips-gas
# to barf.  Don't know what the real fix for this is...
#
# XXX should be COPTS, but that's too early
CPUFLAGS+=-Wa,--no-warn
.endif

.else

.include <bsd.prog.mk>			# do nothing

.endif	# ! EXTERNAL_TOOLCHAIN && MKGCC != no && OBJECT_FMT == ELF
