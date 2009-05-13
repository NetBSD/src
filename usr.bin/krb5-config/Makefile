# $NetBSD: Makefile,v 1.6.4.1 2009/05/13 19:19:54 jym Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:          ${DIST}/heimdal/tools

SCRIPTS=	krb5-config
MAN=		krb5-config.1

CLEANFILES+=	krb5-config

krb5-config: krb5-config.in
	${TOOL_SED} -e "s!@PACKAGE\@!heimdal!g" \
	        -e "s!@VERSION\@!1.1!g" \
	        -e "s!@prefix\@!/!g" \
	        -e "s!@exec_prefix\@!/!g" \
	        -e "s!@libdir\@!/usr/lib!g" \
	        -e "s!@includedir\@!/usr/include/krb5!g" \
	        -e "s!@LIB_crypt\@!-lcrypt!g" \
	        -e "s!@LIB_dbopen\@!!g" \
	        -e "s!@INCLUDE_hcrypto\@!!g" \
	        -e "s!@LIB_hcrypto_appl\@!-lcrypto!g" \
	        -e "s!@LIB_dlopen\@!!g" \
	        -e "s!@LIB_door_create\@!!g" \
	        -e "s!@LIB_pkinit\@!-lhx509!g" \
	        -e "s!@LIBS\@!!g" \
	        $(DIST)/heimdal/tools/krb5-config.in > $@
	chmod +x $@

.include <bsd.prog.mk>
