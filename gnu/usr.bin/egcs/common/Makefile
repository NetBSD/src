#	$NetBSD: Makefile,v 1.28 1999/10/09 16:11:16 sommerfeld Exp $

MKLINKLIB=no
MKMAN=	no
MKPIC=	no

.include "../Makefile.inc"

LIB=	cc1

YHEADER=1

# GNU (gcc,gas) uses "mips" as arch-dependent filename for mipsel and mipseb
.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
GCC_ARCH?=	mips
.elif (${MACHINE_ARCH} == "sparc64")
GCC_ARCH?=	sparc
.else
GCC_ARCH?=	${MACHINE_GNU_ARCH}
.endif

CPPFLAGS+=	-DTARGET_NAME=\"${MACHINE_GNU_ARCH}-netbsd\"
HOST_CPPFLAGS+=	-DTARGET_NAME=\"${MACHINE_GNU_ARCH}-netbsd\"
CPICFLAGS=	-fPIC -DPIC


MD=		${DIST}/config/${GCC_ARCH}/${GCC_ARCH}.md

BCDEF=		${DIST}/bytecode.def

.PATH:		${DIST}

SRCS=		${GCC_ARCH}.c toplev.c version.c tree.c print-tree.c \
		stor-layout.c fold-const.c function.c stmt.c except.c expr.c \
		calls.c expmed.c explow.c optabs.c varasm.c rtl.c print-rtl.c \
		rtlanal.c emit-rtl.c genrtl.c real.c regmove.c dbxout.c \
		sdbout.c dwarfout.c dwarf2out.c xcoffout.c bitmap.c alias.c \
		integrate.c jump.c cse.c loop.c unroll.c flow.c stupid.c \
		combine.c varray.c regclass.c local-alloc.c global.c reload.c \
		reload1.c caller-save.c gcse.c insn-peep.c reorg.c final.c \
		recog.c reg-stack.c insn-opinit.c insn-recog.c insn-extract.c \
		insn-output.c insn-emit.c profile.c insn-attrtab.c getpwd.c \
		convert.c dyn-string.c obstack.c

.if defined(USE_EGCS_HAIFA)
SRCS+=		haifa-sched.c
.else
SRCS+=		sched.c
.endif

DPSRCS+=	s-genrtl genrtl.h s-check tree-check.h
CLEANFILES+=	bitmap.lo obstack.lo print-rtl.lo rtl.lo rtlanal.lo \
		gengenrtl gengenrtl.lo genrtl.c genrtl.h \
		gencheck gencheck.lo tree-check.h \
		s-genrtl s-check

gengenrtl: gengenrtl.lo
	${HOST_LINK.c} -o $@ $>
.NOPATH: genrtl.h genrtl.c  s-genrtl
genrtl.h genrtl.c: s-genrtl
s-genrtl: gengenrtl rtl.h rtl.def gansidecl.h machmode.h machmode.def
	@rm -f ${DIST}/genrtl.h ${DIST}/genrtl.c
	${.OBJDIR}/gengenrtl tmp-genrtl.h tmp-genrtl.c
	${DIST}/move-if-change  tmp-genrtl.h genrtl.h
	${DIST}/move-if-change  tmp-genrtl.c genrtl.c
	touch s-genrtl

gencheck: gencheck.lo
	${HOST_LINK.c} -o $@ $>
.NOPATH: tree-check.h s-check
tree-check.h: s-check
s-check: gencheck
	@rm -f ${DIST}/tree-check.h
	${.OBJDIR}/gencheck >tmp-tree-check.h
	${DIST}/move-if-change  tmp-tree-check.h tree-check.h
	touch s-check

genattrtab: rtlanal.lo

.for _GEN_ in attrtab emit extract opinit output peep recog
DPSRCS+=	insn-${_GEN_}.c s-${_GEN_}
CLEANFILES+=	gen${_GEN_} gen${_GEN_}.lo insn-${_GEN_}.c s-${_GEN_}

gen${_GEN_}.lo: genrtl.h
gen${_GEN_}: gen${_GEN_}.lo bitmap.lo obstack.lo print-rtl.lo rtl.lo ${MD}
	${HOST_LINK.c} -o $@ ${.ALLSRC:M*.lo}

.NOPATH: insn-${_GEN_}.c s-${_GEN_}
insn-${_GEN_}.c: s-${_GEN_}
s-${_GEN_}: gen${_GEN_} ${MD}
	@rm -f ${DIST}/insn-${_GEN_}.c
	${.OBJDIR}/gen${_GEN_} ${MD} > tmp-${_GEN_}
	${DIST}/move-if-change  tmp-${_GEN_} insn-${_GEN_}.c
	touch s-${_GEN_}
.endfor

.for _GEN_ in attr codes config flags
DPSRCS+=	insn-${_GEN_}.h s-${_GEN_}
CLEANFILES+=	gen${_GEN_} gen${_GEN_}.lo insn-${_GEN_}.h s-${_GEN_}

gen${_GEN_}.lo: genrtl.h
gen${_GEN_}: gen${_GEN_}.lo bitmap.lo obstack.lo print-rtl.lo rtl.lo ${MD}
	${HOST_LINK.c} -o $@ ${.ALLSRC:M*.lo}

.NOPATH: insn-${_GEN_}.h s-${_GEN_}
insn-${_GEN_}.h: s-${_GEN_}
s-${_GEN_}: gen${_GEN_} ${MD}
	@rm -f ${DIST}/insn-${_GEN_}.h
	${.OBJDIR}/gen${_GEN_} ${MD} > tmp-${_GEN_}
	${DIST}/move-if-change  tmp-${_GEN_} insn-${_GEN_}.h
	touch s-${_GEN_}
.endfor

.include <bsd.lib.mk>

${OBJS} ${SOBJS} genattrtab.lo genextract.lo: \
		insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h

# this makes "make depend" optional
bitmap.lo genemit.lo genopinit.lo genoutput.lo genpeep.lo \
print-rtl.lo rtl.lo rtlanal.lo: \
		genrtl.h

.PATH:		${DIST}/config/${GCC_ARCH}
