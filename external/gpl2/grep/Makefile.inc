#	$NetBSD: Makefile.inc,v 1.1.14.1 2018/07/28 04:37:15 pgoyette Exp $

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
