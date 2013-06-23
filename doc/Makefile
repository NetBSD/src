#	$NetBSD: Makefile,v 1.1.8.1 2013/06/23 06:26:21 tls Exp $

.include <bsd.own.mk>

regen: .PHONY regen-BUILDING
regen-BUILDING: .PHONY
	${TOOL_MANDOC_ASCII} ${.CURDIR}/BUILDING.mdoc \
		| ${TOOL_SED} -e "s,.$$(printf '\b'),,g" \
		>${.CURDIR}/../BUILDING
