#	$NetBSD: Makefile,v 1.18 1998/10/14 16:55:52 tv Exp $

.include "../Makefile.inc"

LIB=	cc1

NOMAN=
NOLINKLIB=
YHEADER=1

CPPFLAGS+=	-DTARGET_NAME=\"${MACHINE_GNU_ARCH}-netbsd\"
CPICFLAGS=	-fPIC -DPIC
HOST_CPPFLAGS=	${CPPFLAGS}

MD=		${DIST}/config/${MACHINE_GNU_ARCH}/${MACHINE_GNU_ARCH}.md

BCDEF=		${DIST}/bytecode.def

.PATH:		${DIST}

SRCS=		${MACHINE_GNU_ARCH}.c toplev.c version.c tree.c print-tree.c \
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

DPSRCS+=	genrtl.h tree-check.h
CLEANFILES+=	bitmap.lo obstack.lo print-rtl.lo rtl.lo rtlanal.lo \
		gengenrtl gengenrtl.lo genrtl.c genrtl.h \
		gencheck gencheck.lo tree-check.h

gengenrtl: gengenrtl.lo
	${HOST_LINK.c} -o $@ $>
.NOPATH: genrtl.h genrtl.c
genrtl.h genrtl.c: gengenrtl
	@rm -f ${DIST}/genrtl.h ${DIST}/genrtl.c
	./gengenrtl genrtl.h genrtl.c

gencheck: gencheck.lo
	${HOST_LINK.c} -o $@ $>
.NOPATH: tree-check.h
tree-check.h: gencheck
	@rm -f ${DIST}/tree-check.h
	./gencheck >tree-check.h

genattrtab: rtlanal.lo

.for _GEN_ in attrtab emit extract opinit output peep recog
DPSRCS+=	insn-${_GEN_}.c
CLEANFILES+=	gen${_GEN_} gen${_GEN_}.lo insn-${_GEN_}.c

gen${_GEN_}.lo: genrtl.h
gen${_GEN_}: gen${_GEN_}.lo bitmap.lo obstack.lo print-rtl.lo rtl.lo ${MD}
	${HOST_LINK.c} -o $@ ${.ALLSRC:M*.lo}

.NOPATH: insn-${_GEN_}.c
insn-${_GEN_}.c: gen${_GEN_}
	@rm -f ${DIST}/insn-${_GEN_}.c
	./gen${_GEN_} ${MD} >$@
.endfor

.for _GEN_ in attr codes config flags
DPSRCS+=	insn-${_GEN_}.h
CLEANFILES+=	gen${_GEN_} gen${_GEN_}.lo insn-${_GEN_}.h

gen${_GEN_}.lo: genrtl.h
gen${_GEN_}: gen${_GEN_}.lo bitmap.lo obstack.lo print-rtl.lo rtl.lo ${MD}
	${HOST_LINK.c} -o $@ ${.ALLSRC:M*.lo}

.NOPATH: insn-${_GEN_}.h
insn-${_GEN_}.h: gen${_GEN_}
	@rm -f ${DIST}/insn-${_GEN_}.h
	./gen${_GEN_} ${MD} >$@
.endfor

.include <bsd.lib.mk>

${OBJS} ${SOBJS} genattrtab.lo genextract.lo: \
		insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h

.PATH:		${DIST}/config/${MACHINE_GNU_ARCH}
