#	$NetBSD: Makefile,v 1.1 2004/02/15 19:52:27 skrll Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif

.cc: # disable .cc->NULL transform

SRCS+=         ${LIB2FUNCS} ${LIB2DIVMOD} \
               ${G_LIB2ADD:T:S/.asm/.S/} ${LIB1ASMFUNCS}
.if ${MKPIC} == "no"
SRCS+=         ${LIB2_EH}
.endif

.include <bsd.lib.mk>

.c.so:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${CSHLIBFLAGS} ${.IMPSRC} -o ${.TARGET}.tmp
	${NM} -pg ${.TARGET}.tmp | ${AWK} 'NF == 3 { print "\t.hidden", $$3 }' | ${CC} -Wl,-x -r -nostdinc -nostdlib -o ${.TARGET} ${.TARGET}.tmp -xassembler -
	rm -f ${.TARGET}.tmp

.S.so .s.so:
	${COMPILE.S} ${CAPICFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}.tmp
	${NM} -pg ${.TARGET}.tmp | ${AWK} 'NF == 3 { print "\t.hidden", $$3 }' | ${CC} -Wl,-x -r -nostdinc -nostdlib -o ${.TARGET} ${.TARGET}.tmp -xassembler -
	rm -f ${.TARGET}.tmp

.else
.include <bsd.prog.mk> # do nothing
.endif
