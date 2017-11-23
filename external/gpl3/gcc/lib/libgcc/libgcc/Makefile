#	$NetBSD: Makefile,v 1.27.4.1 2017/11/23 11:02:04 martin Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		gcc

.include "../../../Makefile.gcc_path"
.include <bsd.own.mk>

.if ${MKGCC} != "no"

.include <bsd.init.mk>

.cc: # disable .cc->NULL transform

# XXX handle this better?  GCC 4.8 moved them.
.if ${MACHINE} == "emips"
LIB2ADD_HACK+=	${GNUHOSTDIST}/gcc/config/floatunsidf.c \
		${GNUHOSTDIST}/gcc/config/floatunsisf.c
.endif

# XXXGCC5 GCC 5.3 has all these in libgcc and we have them in libc.
.if ${MKSOFTFLOAT} != "no"
LIB2ADD_EDITED= ${G_LIB2ADD:T:Nfloatsisf.c:Nfloatunsisf.c:Nfloatsidf.c:Nfloatunsidf.c:Nfixsfsi.c:Nextendsfdf2.c:Naddsf3.c:Nmulsf3.c:Ndivsf3.c:Nadddf3.c:Nsubdf3.c:Nmuldf3.c:Ndivdf3.c:Nfixdfsi.c:S/.asm/.S/}
.else
LIB2ADD_EDITED=	${G_LIB2ADD:T:S/.asm/.S/}
.endif

SRCS+=		${LIB2FUNCS} ${LIB2FUNCS_ST} ${LIB2DIVMOD} \
		${LIB2ADD_EDITED} \
		${LIB2ADD_HACK:T:S/.asm/.S/} \
		${G_LIB2ADD_ST:T} ${LIB1ASMFUNCS}
.if ${MKPIC} == "no"
.if empty(LIBGCC_MACHINE_ARCH:Mearm*)
SRCS+=		${LIB2_EH} ${LIB2_EHASM}
.endif
PICFLAGS=
.else
CPPFLAGS+=	-DPIC
.endif

# Force building libgcc.a.  It will be PIC from -DPIC above and also
# the -fpic/-FPIC present in ${G_LIBGCC2_CFLAGS}.
MKPIC:= no

COPTS.unwind-dw2.c = -Wno-stack-protector

.include <bsd.lib.mk>


CLEANFILES+=	${SOBJS:=.tmp1} ${SOBJS:=.tmp2}

.c.o:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${CSHLIBFLAGS} ${.IMPSRC} -o ${.TARGET}.tmp1
	${NM} -pg ${.TARGET}.tmp1 | \
	    ${TOOL_AWK} 'NF == 3 { print "\t.hidden", $$3 }' | \
	    ${CC} ${COPTS} -r -nostdinc -nostdlib ${CPUFLAGS} -o ${.TARGET}.tmp2 ${.TARGET}.tmp1 -xassembler -
	${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.tmp2
.if defined(COPTS) && !empty(COPTS:M*-g*)
	mv ${.TARGET}.tmp2 ${.TARGET}
.else
	${LD} -r ${.TARGET}.tmp2 -o ${.TARGET}
	rm -f ${.TARGET}.tmp2
.endif
	rm -f ${.TARGET}.tmp1

.S.o .s.o:
	${_MKTARGET_COMPILE}
	${COMPILE.S} ${PICFLAGS} ${CFLAGS:M-[ID]*} ${.IMPSRC} -o ${.TARGET}.tmp1
	${NM} -pg ${.TARGET}.tmp1 | \
	    ${TOOL_AWK} 'NF == 3 { print "\t.hidden", $$3 }' | \
	    ${CC} ${COPTS} -r -nostdinc -nostdlib -o ${.TARGET}.tmp2 ${.TARGET}.tmp1 -xassembler -
	${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.tmp2
.if defined(COPTS) && !empty(COPTS:M*-g*)
	mv ${.TARGET}.tmp2 ${.TARGET}
.else
	${LD} -r ${.TARGET}.tmp2 -o ${.TARGET}
	rm -f ${.TARGET}.tmp2
.endif
	rm -f ${.TARGET}.tmp1

.else
.include <bsd.prog.mk> # do nothing
.endif
