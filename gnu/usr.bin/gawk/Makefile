#	$NetBSD: Makefile,v 1.12 2000/11/01 15:27:46 wiz Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/gawk ${DIST}/gawk/doc

PROG=		awk
SRCS=		alloca.c array.c awk.y builtin.c dfa.c eval.c field.c \
		gawkmisc.c io.c main.c missing.c msg.c \
		node.c random.c re.c regex.c version.c
MLINKS=		awk.1

TEXINFO=	awk.info

CPPFLAGS+=	-DHAVE_CONFIG_H -DDEFPATH=\".:/usr/share/awk\" \
		-I${.CURDIR} -I${.CURDIR}/../../dist/gawk
LDADD=		-lm
CLEANFILES+=	${PROG}.1 ${PROG}.texi

.if (${MACHINE_ARCH} == "sparc64")
CFLAGS+=	-msoft-quad-float
.endif

${PROG}.1:	gawk.1
		@-rm -f $@
		cp $> $@

${PROG}.texi:	gawk.texi
		@-rm -f $@
		sed -e 's/gawk.info/awk.info/g' <$> >$@

.include <bsd.info.mk>
.include <bsd.prog.mk>
