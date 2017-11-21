#	$NetBSD: Makefile.inc,v 1.4 2017/11/21 03:03:31 rin Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# regex sources
TREDIST= ${NETBSDSRCDIR}/external/bsd/tre/dist

CPPFLAGS+=	-I${TREDIST}/lib -I${.CURDIR}/../include
CPPFLAGS+=	-DHAVE_CONFIG_H=1
CPPFLAGS+=	-DTRE_SYSTEM_REGEX_H_PATH=\"${NETBSDSRCDIR}/include/regex.h\"
CPPFLAGS+=	-DTRE_USE_SYSTEM_REGEX_H=1
CPPFLAGS+=	-DTRE_REGEX_T_FIELD=re_g

WARNS=	5
