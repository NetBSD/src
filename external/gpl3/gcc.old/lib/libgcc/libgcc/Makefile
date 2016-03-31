#	$NetBSD: Makefile,v 1.4 2016/03/31 15:06:39 martin Exp $

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
G_LIB2ADD_HACK+=	${GNUHOSTDIST}/gcc/config/floatunsidf.c \
			${GNUHOSTDIST}/gcc/config/floatunsisf.c
.endif

# XXX avoid duplicate softfloat symbols in libc and libgcc.a,
# which would break the /rescue build
.if ${MKSOFTFLOAT} == "yes"
LIB2ADD_EDITED=	${G_LIB2ADD:T:Nfloatsisf.c:Nfloatunsisf.c:Nfloatsidf.c:Nfloatunsidf.c:Nfixsfsi.c:Nextendsfdf2.c:Naddsf3.c:Nmulsf3.c:Ndivsf3.c:Nadddf3.c:Nsubdf3.c:Nmuldf3.c:Ndivdf3.c:Nfixdfsi.c:Nxfgnulib.c:Nfpgnulib.c:S/.asm/.S/}
LIB2FUNCS_EDITED=	${LIB2FUNCS:N_floatundisf.c:N_floatundidf.c:N_floatundixf.c:N_floatunditf.c:N_fixsfdi.c:N_fixdfdi.c:N_fixxfdi.c:N_fixtfdi.c:N_addvsi3.c:N_addvdi3.c:N_subvsi3.c:N_subvdi3.c:N_mulvsi3.c:N_mulvdi3.c:N_muldi3.c}
LIB1ASMFUNCS_EDITED=	${LIB1ASMFUNCS:N_float.S:N_double.S:N_gedf2.S:N_eqdf2.S:N_nedf2.S:N_ltdf2.S:N_gesf2.S:N_gtdf2.S:N_ledf2.S:N_lesf2.S}
.else
LIB2ADD_EDITED= ${G_LIB2ADD:T:S/.asm/.S/}
LIB2FUNCS_EDITED=	${LIB2FUNCS}
LIB1ASMFUNCS_EDITED=	${LIB1ASMFUNCS}
.endif
SRCS+=		${LIB2FUNCS_EDITED} ${LIB2FUNCS_ST} ${LIB2DIVMOD} \
		${LIB2ADD_EDITED} ${G_LIB2ADD_HACK:T:S/.asm/.S/} \
		${G_LIB2ADD_ST:T} ${LIB1ASMFUNCS_EDITED}
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
	${COMPILE.c} ${COPTS.${.IMPSRC:T}} ${CSHLIBFLAGS} ${.IMPSRC} -o ${.TARGET}.tmp1
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
