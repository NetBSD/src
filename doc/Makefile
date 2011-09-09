#	$NetBSD: Makefile,v 1.1 2011/09/09 14:23:38 apb Exp $

.include <bsd.own.mk>

regen: .PHONY regen-BUILDING
regen-BUILDING: .PHONY
	${TOOL_GROFF} -mdoc -Tascii -P-bou ${.CURDIR}/BUILDING.mdoc \
		>${.CURDIR}/../BUILDING
