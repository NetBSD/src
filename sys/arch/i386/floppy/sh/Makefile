#	$Id: Makefile,v 1.1 1994/04/18 08:47:36 cgd Exp $

PROG=	sh
NOMAN=
SRCS=	builtins.c cd.c dirent.c echo.c error.c eval.c exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c nodes.c options.c parser.c redir.c show.c \
	syntax.c trap.c output.c var.c
OBJS+=	init.o
SRCDIR=	${.CURDIR}/../../../../../bin/sh
CFLAGS+=-DSHELL -I. -I${SRCDIR}

BINDIR=/bin

.PATH:	${SRCDIR} ${SRCDIR}/bltin

CLEANFILES+=\
	builtins.c builtins.h init.c mkinit mknodes mksyntax \
	nodes.c nodes.h syntax.c syntax.h token.def

.depend parser.o: token.def
token.def: mktokens
	sh ${SRCDIR}/mktokens

builtins.h builtins.c: ${SRCDIR}/mkbuiltins ${SRCDIR}/builtins
	cd ${SRCDIR}; sh mkbuiltins ${.OBJDIR}

init.c: mkinit ${SRCS}
	./mkinit '${CC} -c ${CFLAGS} init.c' ${.ALLSRC}
	touch ${.TARGET}

nodes.c nodes.h: mknodes ${SRCDIR}/nodetypes ${SRCDIR}/nodes.c.pat
	./mknodes ${SRCDIR}/nodetypes ${SRCDIR}/nodes.c.pat

syntax.c syntax.h: mksyntax
	./mksyntax

.include <bsd.prog.mk>

mkinit: ${LIBCRT0} ${SRCDIR}/mkinit.c ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${SRCDIR}/mkinit.c -o $@ ${LDADD}

mknodes: ${LIBCRT0} ${SRCDIR}/mknodes.c ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${SRCDIR}/mknodes.c -o $@ ${LDADD}

mksyntax: ${LIBCRT0} ${SRCDIR}/mksyntax.c ${SRCDIR}/parser.h ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${SRCDIR}/mksyntax.c -o $@ ${LDADD}
