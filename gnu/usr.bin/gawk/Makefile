#	$NetBSD: Makefile,v 1.17 2003/10/06 16:50:39 wiz Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
IDIST=		${DIST}/gawk
.PATH: ${IDIST} ${IDIST}/doc

PROG=		awk
SRCS=		array.c awkgram.y builtin.c eval.c ext.c \
		field.c gawkmisc.c io.c main.c msg.c node.c profile.c \
		random.c re.c regex.c replace.c version.c

MLINKS=		awk.1

TEXINFO=	awk.info

CPPFLAGS+=	-DHAVE_CONFIG_H -DDEFPATH=\".:/usr/share/awk\" \
		-DGAWK -I${.CURDIR} -I${IDIST} \
		-DLOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale\"
DPADD+=		${LIBINTL} ${LIBM}
LDADD+=		-lintl -lm
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
