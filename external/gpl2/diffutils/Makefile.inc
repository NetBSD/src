#	$NetBSD: Makefile.inc,v 1.1.14.1 2018/07/28 04:37:15 pgoyette Exp $

WARNS=	0

IDIST=	${NETBSDSRCDIR}/external/gpl2/diffutils/dist

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${IDIST}/../include -I${IDIST}/lib

SANITIZER_RENAME_SYMBOL+=	regcomp regerror regexec regfree
