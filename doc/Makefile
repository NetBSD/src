#	$NetBSD: Makefile,v 1.1.2.1 2014/05/22 11:27:14 yamt Exp $

.include <bsd.own.mk>

regen: .PHONY regen-BUILDING
regen-BUILDING: .PHONY
	${TOOL_MANDOC_ASCII} ${.CURDIR}/BUILDING.mdoc \
		| ${TOOL_SED} -e "s,.$$(printf '\b'),,g" \
		>${.CURDIR}/../BUILDING
