#	$NetBSD: Makefile,v 1.15 2002/09/19 03:09:35 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/gawk ${DIST}/gawk/doc

PROG=		awk
SRCS=		alloca.c array.c awk.y builtin.c dfa.c eval.c field.c \
		gawkmisc.c io.c main.c missing.c msg.c \
		node.c random.c re.c regex.c version.c
MLINKS=		awk.1

TEXINFO=	awk.info

CPPFLAGS+=	-DHAVE_CONFIG_H -DDEFPATH=\".:/usr/share/awk\" \
		-I${.CURDIR} -I${DIST}/gawk
LDADD=		-lm
CLEANFILES+=	${PROG}.1 ${PROG}.texi

.if ${MACHINE_ARCH} == "vax" && ${OBJECT_FMT} == "ELF"
COPTS+=		-O1
.endif

${PROG}.1:	gawk.1
		@-rm -f $@
		cp $> $@

${PROG}.texi:	gawk.texi
		@-rm -f $@
		sed -e 's/gawk.info/awk.info/g' <$> >$@

.include <bsd.info.mk>
.include <bsd.prog.mk>
