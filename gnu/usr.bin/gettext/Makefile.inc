#	$NetBSD: Makefile.inc,v 1.5 2002/09/19 03:09:37 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../include \
		-I${DIST}/gettext/lib -DLOCALEDIR=\"/usr/share/locale\"

BINDIR?=	/usr/bin
