#	$NetBSD: Makefile,v 1.12 1997/05/31 21:21:29 cjs Exp $

LIB=	cc1

NOMAN=
NOPIC=
NOPROFILE=

COMMON!=cd $(.CURDIR)/../common; \
	 printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/$(MACHINE_ARCH) -I$(COMMON)

MD= ${.CURDIR}/../arch/${MACHINE_ARCH}/${MACHINE_ARCH}.md

BCDEF= ${.CURDIR}/bytecode.def

.PATH:	${.CURDIR}/../arch/${MACHINE_ARCH}

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
	$(MACHINE_ARCH).c insn-attr.h insn-codes.h insn-config.h insn-flags.h \
	insn-peep.c insn-opinit.c insn-recog.c insn-extract.c insn-output.c \
	insn-emit.c insn-attrtab.c 

insn-attr.h: genattr $(MD)
	./genattr $(MD) > insn-attr.h
CLEANFILES += insn-attr.h genattr genattr.o

insn-flags.h: genflags $(MD)
	./genflags $(MD) > insn-flags.h
CLEANFILES += insn-flags.h genflags genflags.o

insn-codes.h: gencodes $(MD)
	./gencodes $(MD) > insn-codes.h
CLEANFILES += insn-codes.h gencodes gencodes.o

insn-config.h: genconfig $(MD)
	./genconfig $(MD) > insn-config.h
CLEANFILES += insn-config.h genconfig genconfig.o

bc-opcode.h: bi-opcode $(BCDEF)
	./bi-opcode < $(BCDEF) > bc-opcode.h
CLEANFILES += bc-opcode.h bi-opcode bi-opcode.o

bc-opname.h: bi-opname $(BCDEF)
	./bi-opname < $(BCDEF) > bc-opname.h
CLEANFILES += bc-opname.h bi-opname bi-opname.o

bc-arity.h: bi-arity $(BCDEF)
	./bi-arity < $(BCDEF) > bc-arity.h
CLEANFILES += bc-arity.h bi-arity bi-arity.o

insn-peep.c: genpeep $(MD)
	./genpeep $(MD) > insn-peep.c
CLEANFILES += insn-peep.c genpeep genpeep.o

insn-opinit.c: genopinit $(MD)
	./genopinit $(MD) > insn-opinit.c
CLEANFILES += insn-opinit.c genopinit genopinit.o

insn-recog.c: genrecog $(MD)
	./genrecog $(MD) > insn-recog.c
CLEANFILES += insn-recog.c genrecog genrecog.o

insn-extract.c: genextract $(MD)
	./genextract $(MD) > insn-extract.c
CLEANFILES += insn-extract.c genextract genextract.o

insn-output.c: genoutput $(MD)
	./genoutput $(MD) > insn-output.c
CLEANFILES += insn-output.c genoutput genoutput.o

insn-emit.c: genemit $(MD)
	./genemit $(MD) > insn-emit.c
CLEANFILES += insn-emit.c genemit genemit.o

insn-attrtab.c: genattrtab $(MD)
	./genattrtab $(MD) > insn-attrtab.c
CLEANFILES += insn-attrtab.c genattrtab genattrtab.o

genattr.o: genattr.c

genattr: genattr.o rtl.o obstack.o
	$(CC) -o genattr genattr.o rtl.o obstack.o

genflags.o: genflags.c

genflags: genflags.o rtl.o obstack.o
	$(CC) -o genflags genflags.o rtl.o obstack.o

gencodes.o: gencodes.c

gencodes: gencodes.o rtl.o obstack.o
	$(CC) -o gencodes gencodes.o rtl.o obstack.o

genconfig.o: genconfig.c

genconfig: genconfig.o rtl.o obstack.o
	$(CC) -o genconfig genconfig.o rtl.o obstack.o

genpeep.o: genpeep.c

genpeep: genpeep.o rtl.o obstack.o
	$(CC) -o genpeep genpeep.o rtl.o obstack.o

genopinit.o: genopinit.c

genopinit: genopinit.o rtl.o obstack.o
	$(CC) -o genopinit genopinit.o rtl.o obstack.o

genrecog.o: genrecog.c

genrecog: genrecog.o rtl.o obstack.o
	$(CC) -o genrecog genrecog.o rtl.o obstack.o

genextract.o: genextract.c

genextract: genextract.o rtl.o obstack.o
	$(CC) -o genextract genextract.o rtl.o obstack.o

genoutput.o: genoutput.c

genoutput: genoutput.o rtl.o obstack.o
	$(CC) -o genoutput genoutput.o rtl.o obstack.o

genemit.o: genemit.c

genemit: genemit.o rtl.o obstack.o
	$(CC) -o genemit genemit.o rtl.o obstack.o

genattrtab.o: genattrtab.c

genattrtab: genattrtab.o rtl.o obstack.o print-rtl.o rtlanal.o
	$(CC) -o genattrtab genattrtab.o rtl.o obstack.o print-rtl.o rtlanal.o

bi-opcode.o: bi-opcode.c

bi-opcode: bi-opcode.o bi-parser.o bi-lexer.o bi-reverse.o obstack.o
	$(CC) -o bi-opcode bi-opcode.o bi-parser.o bi-lexer.o bi-reverse.o obstack.o

bi-opname.o: bi-opname.c

bi-opname: bi-opname.o bi-parser.o bi-lexer.o bi-reverse.o obstack.o
	$(CC) -o bi-opname bi-opname.o bi-parser.o bi-lexer.o bi-reverse.o obstack.o

bi-arity.o: bi-arity.c

bi-arity: bi-arity.o bi-parser.o bi-lexer.o bi-reverse.o obstack.o
	$(CC) -o bi-arity bi-arity.o bi-parser.o bi-lexer.o bi-reverse.o obstack.o

CLEANFILES += bi-lexer.o bi-parser.o bi-parser.c bi-parser.h

bi-lexer.o: bi-parser.h

bi-parser.h: bi-parser.c
bi-parser.c: bi-parser.y
	$(YACC) -b bi -d $(.IMPSRC)
	mv -f bi.tab.c bi-parser.c
	mv -f bi.tab.h bi-parser.h

CLEANFILES += c-parse.c c-parse.h

c-lex.o: c-parse.h

c-parse.h: c-parse.c
c-parse.c: c-parse.y
	$(YACC) -b c -d $(.IMPSRC)
	mv -f c.tab.c c-parse.c
	mv -f c.tab.h c-parse.h

# only needed during build
libinstall::

.include <bsd.lib.mk>
