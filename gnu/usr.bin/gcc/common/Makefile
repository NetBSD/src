#	$NetBSD: Makefile,v 1.17 1998/09/12 19:45:37 wrstuden Exp $

LIB=	cc1

# XXX temporary for mips -> mipsel migration
TARGET_ARCH=${MACHINE_ARCH:C/^mips.*/mips/}

NOMAN=
NOPIC=
NOPROFILE=
NOLINT=nolint
YHEADER=1

COMMON!=cd $(.CURDIR)/../common; \
	 printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/$(TARGET_ARCH) -I$(COMMON)

HOST_CFLAGS+=${CPPFLAGS}

MD= ${.CURDIR}/../arch/${TARGET_ARCH}/${TARGET_ARCH}.md

BCDEF= ${.CURDIR}/bytecode.def

.PATH:	${.CURDIR}/../arch/${TARGET_ARCH}

SRCS=	bc-arity.h bc-opcode.h bc-opname.h c-parse.h bc-emit.c \
	bc-optab.c bi-reverse.c c-aux-info.c c-common.c c-convert.c \
	c-decl.c c-iterate.c c-lex.c c-pragma.c c-typeck.c \
	caller-save.c calls.c combine.c convert.c cse.c \
	dbxout.c dwarfout.c emit-rtl.c explow.c expmed.c \
	expr.c final.c flow.c fold-const.c function.c \
	getpwd.c global.c integrate.c jump.c local-alloc.c \
	loop.c obstack.c optabs.c print-rtl.c print-tree.c \
	real.c recog.c reg-stack.c regclass.c reload.c \
	reload1.c reorg.c rtl.c rtlanal.c sched.c \
	sdbout.c stmt.c stor-layout.c stupid.c toplev.c \
	tree.c unroll.c varasm.c version.c xcoffout.c \
	$(TARGET_ARCH).c insn-attr.h insn-codes.h insn-config.h insn-flags.h \
	insn-peep.c insn-opinit.c insn-recog.c insn-extract.c insn-output.c \
	insn-emit.c insn-attrtab.c 

insn-attr.h: genattr $(MD)
	./genattr $(MD) > insn-attr.h
CLEANFILES += insn-attr.h genattr genattr.lo

insn-flags.h: genflags $(MD)
	./genflags $(MD) > insn-flags.h
CLEANFILES += insn-flags.h genflags genflags.lo

insn-codes.h: gencodes $(MD)
	./gencodes $(MD) > insn-codes.h
CLEANFILES += insn-codes.h gencodes gencodes.lo

insn-config.h: genconfig $(MD)
	./genconfig $(MD) > insn-config.h
CLEANFILES += insn-config.h genconfig genconfig.lo

bc-opcode.h: bi-opcode $(BCDEF)
	./bi-opcode < $(BCDEF) > bc-opcode.h
CLEANFILES += bc-opcode.h bi-opcode bi-opcode.lo

bc-opname.h: bi-opname $(BCDEF)
	./bi-opname < $(BCDEF) > bc-opname.h
CLEANFILES += bc-opname.h bi-opname bi-opname.lo

bc-arity.h: bi-arity $(BCDEF)
	./bi-arity < $(BCDEF) > bc-arity.h
CLEANFILES += bc-arity.h bi-arity bi-arity.lo

insn-peep.c: genpeep $(MD)
	./genpeep $(MD) > insn-peep.c
CLEANFILES += insn-peep.c genpeep genpeep.lo

insn-opinit.c: genopinit $(MD)
	./genopinit $(MD) > insn-opinit.c
CLEANFILES += insn-opinit.c genopinit genopinit.lo

insn-recog.c: genrecog $(MD)
	./genrecog $(MD) > insn-recog.c
CLEANFILES += insn-recog.c genrecog genrecog.lo

insn-extract.c: genextract $(MD)
	./genextract $(MD) > insn-extract.c
CLEANFILES += insn-extract.c genextract genextract.lo

insn-output.c: genoutput $(MD)
	./genoutput $(MD) > insn-output.c
CLEANFILES += insn-output.c genoutput genoutput.lo

insn-emit.c: genemit $(MD)
	./genemit $(MD) > insn-emit.c
CLEANFILES += insn-emit.c genemit genemit.lo

insn-attrtab.c: genattrtab $(MD)
	./genattrtab $(MD) > insn-attrtab.c
CLEANFILES += insn-attrtab.c genattrtab genattrtab.lo

genattr.lo: genattr.c

genattr: genattr.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genattr ${.ALLSRC}

genflags.lo: genflags.c

genflags: genflags.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genflags ${.ALLSRC}

gencodes.lo: gencodes.c

gencodes: gencodes.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o gencodes ${.ALLSRC}

genconfig.lo: genconfig.c

genconfig: genconfig.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genconfig ${.ALLSRC}

genpeep.lo: genpeep.c

genpeep: genpeep.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genpeep ${.ALLSRC}

genopinit.lo: genopinit.c

genopinit: genopinit.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genopinit ${.ALLSRC}

genrecog.lo: genrecog.c

genrecog: genrecog.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genrecog ${.ALLSRC}

genextract.lo: genextract.c

genextract: genextract.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genextract ${.ALLSRC}

genoutput.lo: genoutput.c

genoutput: genoutput.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genoutput ${.ALLSRC}

genemit.lo: genemit.c

genemit: genemit.lo rtl.lo obstack.lo
	$(HOST_LINK.c) -o genemit ${.ALLSRC}

genattrtab.lo: genattrtab.c

genattrtab: genattrtab.lo rtl.lo obstack.lo print-rtl.lo rtlanal.lo
	$(HOST_LINK.c) -o genattrtab ${.ALLSRC}

bi-opcode.lo: bi-opcode.c

bi-opcode: bi-opcode.lo bi-parser.lo bi-lexer.lo bi-reverse.lo obstack.lo
	$(HOST_LINK.c) -o bi-opcode ${.ALLSRC}

bi-opname.lo: bi-opname.c

bi-opname: bi-opname.lo bi-parser.lo bi-lexer.lo bi-reverse.lo obstack.lo
	$(HOST_LINK.c) -o bi-opname ${.ALLSRC}

bi-arity.lo: bi-arity.c

bi-arity: bi-arity.lo bi-parser.lo bi-lexer.lo bi-reverse.lo obstack.lo
	$(HOST_LINK.c) -o bi-arity ${.ALLSRC}

CLEANFILES += bi-lexer.lo bi-parser.lo bi-parser.c bi-parser.h

bi-lexer.lo: bi-parser.h

bi-parser.h bi-parser.lo: bi-parser.c

CLEANFILES += c-parse.c c-parse.h

c-lex.lo: c-parse.h

c-parse.h c-parse.lo: c-parse.c

CLEANFILES += bi-reverse.lo obstack.lo print-rtl.lo rtl.lo rtlanal.lo

bi-reverse.lo: bi-reverse.c

print-rtl.lo: print-rtl.c

obstack.lo: obstack.c obstack.h

rtl.lo: rtl.c rtl.h

rtlanal.lo: rtlanal.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
