#	$NetBSD: Makefile,v 1.4 1997/03/24 21:59:40 christos Exp $
#	from: @(#)Makefile	5.6 (Berkeley) 3/12/91

.include "../../Makefile.inc"

PROG=	less
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../lesskey
SRCS=	brac.c ch.c charset.c cmdbuf.c command.c decode.c edit.c \
	filename.c forwback.c help.c ifile.c input.c jump.c line.c \
	linenum.c lsystem.c main.c mark.c optfunc.c option.c opttbl.c \
	os.c output.c position.c prompt.c screen.c search.c signal.c \
	tags.c ttyin.c version.c
LDADD+=	-ltermcap
DPADD+=	${LIBTERM}
MLINKS=	less.1 more.1 less.1 page.1
LINKS=	${BINDIR}/less ${BINDIR}/more ${BINDIR}/less ${BINDIR}/page

FILES=more.help
FILESDIR=/usr/share/misc

.include <bsd.prog.mk>
