#	$NetBSD: Makefile,v 1.19 2003/10/21 10:01:20 lukem Exp $

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

BUILDSYMLINKS+=	gawk.1 ${PROG}.1

${PROG}.texi:	gawk.texi
	${_MKTARGET_CREATE}
	rm -f $@
	sed -e 's/gawk.info/awk.info/g' <$> >$@

.include <bsd.info.mk>
.include <bsd.prog.mk>
