#	$NetBSD: Makefile,v 1.2 1996/03/01 00:13:51 jtc Exp $

LIB=	cc1

NOMAN=
NOPIC=
NOPROFILE=

XXXX != cd $(.CURDIR)/../common; \
	 printf "xxxx:\n\techo -I\$${.OBJDIR}\n" | ${MAKE} -r -s -f - xxxx

CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/$(MACHINE_ARCH) $(XXXX)

MD= ${.CURDIR}/../arch/${MACHINE_ARCH}/${MACHINE_ARCH}.md

BCDEF= ${.CURDIR}/bytecode.def

.PATH:	${.CURDIR}/../arch/${MACHINE_ARCH}

SRCS=	bc-emit.c bc-optab.c bi-reverse.c c-aux-info.c c-common.c c-convert.c \
c-decl.c c-iterate.c c-lex.c c-pragma.c c-typeck.c caller-save.c \
calls.c combine.c convert.c cse.c dbxout.c dwarfout.c \
emit-rtl.c explow.c expmed.c expr.c final.c flow.c \
fold-const.c function.c getpwd.c global.c integrate.c jump.c \
local-alloc.c loop.c obstack.c optabs.c print-rtl.c print-tree.c \
real.c recog.c reg-stack.c regclass.c reload.c reload1.c \
reorg.c rtl.c rtlanal.c sched.c sdbout.c stmt.c \
stor-layout.c stupid.c toplev.c tree.c unroll.c varasm.c \
version.c xcoffout.c $(MACHINE_ARCH).c insn-peep.c insn-opinit.c insn-recog.c \
insn-extract.c insn-output.c insn-emit.c insn-attrtab.c 

all: insn-attr.h insn-flags.h insn-codes.h insn-config.h bc-opcode.h bc-opname.h bc-arity.h lib$(LIB).a

beforedepend: insn-attr.h insn-flags.h insn-codes.h insn-config.h bc-opcode.h bc-opname.h bc-arity.h 

insn-attr.h: genattr
	./genattr $(MD) > insn-attr.h

CLEANFILES += insn-attr.h genattr genattr.o

insn-flags.h: genflags
	./genflags $(MD) > insn-flags.h

CLEANFILES += insn-flags.h genflags genflags.o

insn-codes.h: gencodes
	./gencodes $(MD) > insn-codes.h

CLEANFILES += insn-codes.h gencodes gencodes.o

insn-config.h: genconfig
	./genconfig $(MD) > insn-config.h

CLEANFILES += insn-config.h genconfig genconfig.o

bc-opcode.h: bi-opcode
	./bi-opcode < $(BCDEF) > bc-opcode.h

CLEANFILES += bc-opcode.h bi-opcode bi-opcode.o

bc-opname.h: bi-opname
	./bi-opname < $(BCDEF) > bc-opname.h

CLEANFILES += bc-opname.h bi-opname bi-opname.o

bc-arity.h: bi-arity
	./bi-arity < $(BCDEF) > bc-arity.h

CLEANFILES += bc-arity.h bi-arity bi-arity.o

insn-peep.c: genpeep
	./genpeep $(MD) > insn-peep.c
CLEANFILES += insn-peep.c genpeep genpeep.o

insn-opinit.c: genopinit
	./genopinit $(MD) > insn-opinit.c
CLEANFILES += insn-opinit.c genopinit genopinit.o

insn-recog.c: genrecog
	./genrecog $(MD) > insn-recog.c
CLEANFILES += insn-recog.c genrecog genrecog.o

insn-extract.c: genextract
	./genextract $(MD) > insn-extract.c
CLEANFILES += insn-extract.c genextract genextract.o

insn-output.c: genoutput
	./genoutput $(MD) > insn-output.c
CLEANFILES += insn-output.c genoutput genoutput.o

insn-emit.c: genemit
	./genemit $(MD) > insn-emit.c
CLEANFILES += insn-emit.c genemit genemit.o

insn-attrtab.c: genattrtab
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
	$(YACC) -d $(.IMPSRC)
	mv -f y.tab.c bi-parser.c
	mv -f y.tab.h bi-parser.h

install:

.include <bsd.lib.mk>

