#	$Id: Makefile,v 1.1 1993/08/23 09:23:04 cgd Exp $

LIB=	cc1
NOPROFILE=

CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../arch -I${.CURDIR}/../arch/${MACHINE}
.PATH:	${.CURDIR}/../arch/${MACHINE}

# machine-independent
SRCS+=	c-common.c caller-save.c calls.c combine.c convert.c cse.c dbxout.c \
	dwarfout.c emit-rtl.c explow.c expmed.c expr.c final.c flow.c \
	fold-const.c function.c getpwd.c global.c integrate.c jump.c \
	local-alloc.c loop.c obstack.c optabs.c print-rtl.c print-tree.c \
	real.c recog.c reg-stack.c regclass.c reload.c reload1.c reorg.c \
	rtl.c rtlanal.c sched.c sdbout.c stmt.c stor-layout.c stupid.c \
	toplev.c tree.c unroll.c varasm.c version.c xcoffout.c

# machine-dependent
SRCS+=	aux-output.c insn-attrtab.c insn-emit.c insn-extract.c insn-opinit.c \
	insn-output.c insn-peep.c insn-recog.c

install:

.include <bsd.lib.mk>
