#	$NetBSD: Makefile,v 1.6 2003/08/03 17:19:45 jdolecek Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I. -DHAS_ISBLANK
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes

# gawk INFO file
GDIST=	${NETBSDSRCDIR}/gnu/dist/gawk
.PATH:	${GDIST}/doc

TEXINFO=        awk.info

CLEANFILES+=	${PROG}.texi

${PROG}.texi: gawk.texi
	@-rm -f $@
	sed -e 's/gawk.info/${PROG}.info/g' <$> >$@

.include <bsd.info.mk>
.include <bsd.prog.mk>
