#	$NetBSD: Makefile,v 1.6 2004/04/28 01:44:59 uwe Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_s
NOSTATICLIB=	# defined
NOPROFILE=	# defined
NOPICINSTALL=	# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no"
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif

.cc: # disable .cc->NULL transform

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

LDFLAGS+=	-nodefaultlibs
#LDFLAGS+=	-Wl,--version-script=${DIST}/libgcc/libgcc.map

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} \
		${G_LIB2ADD:T:S/.asm/.S/} ${LIB1ASMFUNCS}

.include <bsd.lib.mk>

.if ${MACHINE_CPU} == "sh3"
# make sure the millicode is not externally visible

CLEANFILES+=	${SOBJS:=.tmp1} ${SOBJS:=.tmp2}

.S.so .s.so:
	${_MKTARGET_COMPILE}
	${COMPILE.S} ${CAPICFLAGS} ${CFLAGS:M-[ID]*} ${.IMPSRC} -o ${.TARGET}.tmp1
	${NM} -pg ${.TARGET}.tmp1 | ${AWK} 'NF == 3 { print "\t.hidden", $$3 }' | \
	    ${CC} -Wl,-x -r -nostdinc -nostdlib -o ${.TARGET}.tmp2 ${.TARGET}.tmp1 -xassembler -
.if defined(COPTS) && !empty(COPTS:M*-g*)
	mv ${.TARGET}.tmp2 ${.TARGET}
.else
	${LD} -x -r ${.TARGET}.tmp2 -o ${.TARGET}
	rm -f ${.TARGET}.tmp2
.endif
	rm -f ${.TARGET}.tmp1
.endif

.else
.include <bsd.prog.mk> # do nothing
.endif
