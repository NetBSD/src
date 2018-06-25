#	$NetBSD: Makefile.inc,v 1.2 2018/06/25 18:01:13 kamil Exp $

WARNS=	0

IDIST=	${NETBSDSRCDIR}/external/gpl2/diffutils/dist

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H \
		-I${IDIST}/../include -I${IDIST}/lib

SANITIZER_RENAME_SYMBOL+=	regcomp regerror regexec regfree
