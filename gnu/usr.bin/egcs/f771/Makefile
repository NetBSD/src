#	$NetBSD: Makefile,v 1.5 1998/08/17 22:29:56 tv Exp $

PROG=		f771
SRCS=		bad.c bit.c bld.c com.c data.c equiv.c expr.c global.c \
		implic.c info.c intrin.c lab.c lex.c malloc.c name.c parse.c \
		proj.c src.c st.c sta.c stb.c stc.c std.c ste.c storag.c \
		stp.c str.c sts.c stt.c stu.c stv.c stw.c symbol.c target.c \
		top.c type.c version.c where.c
F=		${.CURDIR}/../../../dist/gcc/f
CPPFLAGS+=	-I${F} -I.
CLEANFILES+=	${FINIFILES} fini fini.lo proj.lo
DPSRCS+=	${FINIFILES}

fini: fini.lo proj.lo
	${HOST_LINK.c} -o $@ $>

FINIFILES=	str-1t.j str-1t.h str-2t.j str-2t.h str-fo.j str-fo.h \
		str-io.j str-io.h str-nq.j str-nq.h str-op.j str-op.h \
		str-ot.j str-ot.h

.SUFFIXES: .fin .j .h
.fin.j .fin.h: fini
	./fini $< $*.j $*.h

.PATH: ${F}

.include "../Makefile.cc1"

${OBJS}: ${FINIFILES}
