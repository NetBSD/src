#	$NetBSD: Makefile.inc,v 1.2 2018/06/25 18:02:49 kamil Exp $

.include <bsd.own.mk>

SANITIZER_RENAME_SYMBOL+=	regcomp regerror regexec regfree

WARNS=	0

IDIST=	${NETBSDSRCDIR}/external/gpl2/grep/dist

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${.CURDIR}/../include -I${IDIST}/lib -I${IDIST}/src

DOBJDIR!=	cd $(.CURDIR)/../lib && ${PRINTOBJDIR}

LIBGREPUTILS=	${DOBJDIR}/libgreputils.a
BINDIR=	/usr/bin

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
