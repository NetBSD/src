#	$NetBSD: Makefile,v 1.6 2004/04/18 04:32:51 lukem Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif

.cc: # disable .cc->NULL transform

SRCS+=		${LIB2FUNCS} ${LIB2FUNCS_ST} ${LIB2DIVMOD} \
		${G_LIB2ADD:T:S/.asm/.S/} ${LIB1ASMFUNCS}
.if ${MKPIC} == "no"
SRCS+=		${LIB2_EH}
.endif

.include <bsd.lib.mk>

CLEANFILES+=	${SOBJS:=.tmp1} ${SOBJS:=.tmp2}

.c.so:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CSHLIBFLAGS} ${.IMPSRC} -o ${.TARGET}.tmp1
	${NM} -pg ${.TARGET}.tmp1 | ${AWK} 'NF == 3 { print "\t.hidden", $$3 }' | \
	    ${CC} -Wl,-x -r -nostdinc -nostdlib -o ${.TARGET}.tmp2 ${.TARGET}.tmp1 -xassembler -
.if defined(COPTS) && !empty(COPTS:M*-g*)
	mv ${.TARGET}.tmp2 ${.TARGET}
.else
	${LD} -x -r ${.TARGET}.tmp2 -o ${.TARGET}
	rm -f ${.TARGET}.tmp2
.endif
	rm -f ${.TARGET}.tmp1

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

.else
.include <bsd.prog.mk> # do nothing
.endif
