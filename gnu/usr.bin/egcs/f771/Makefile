#	$NetBSD: Makefile,v 1.2 1998/03/31 05:04:18 mrg Exp $

PROG=	f771
SRCS=	bad.c bit.c bld.c com.c data.c equiv.c expr.c global.c implic.c \
	info.c intrin.c lab.c lex.c malloc.c name.c parse.c proj.c src.c \
	st.c sta.c stb.c stc.c std.c ste.c storag.c stp.c str.c sts.c stt.c \
	stu.c stv.c stw.c symbol.c target.c top.c type.c version.c where.c
FINIFILES=str-1t.j str-1t.h str-2t.j str-2t.h str-fo.j str-fo.h str-io.j \
	str-io.h str-nq.j str-nq.h str-op.j str-op.h str-ot.j str-ot.h

F=	$(.CURDIR)/../../../dist/gcc/f

$(FINIFILES): fini
	./fini $(F)/str-1t.fin str-1t.j str-1t.h
	./fini $(F)/str-2t.fin str-2t.j str-2t.h
	./fini $(F)/str-fo.fin str-fo.j str-fo.h
	./fini $(F)/str-io.fin str-io.j str-io.h
	./fini $(F)/str-nq.fin str-nq.j str-nq.h
	./fini $(F)/str-op.fin str-op.j str-op.h
	./fini $(F)/str-ot.fin str-ot.j str-ot.h

fini:	fini.o proj.o
	$(CC) -o fini fini.o proj.o

st.c sta.c stb.c stc.c std.c ste.c stt.c stu.c stw.c:	$(FINIFILES)

CPPFLAGS+=	-I$(F) -I.
CLEANFILES+=	$(FINIFILES) fini fini.o
.PATH: $(F)

.include "../Makefile.cc1"
