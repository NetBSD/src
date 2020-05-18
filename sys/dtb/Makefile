#	$NetBSD: Makefile,v 1.1 2020/05/18 21:19:35 jmcneill Exp $

TARGETS+=	dtblist

.include <bsd.own.mk>

.if !empty(MACHINE_ARCH:Mearmv[67]*)
SUBDIR+=	arm
.endif

.if !empty(MACHINE_ARCH:Maarch64*)
SUBDIR+=	arm64
.endif

.include <bsd.subdir.mk>

# Regenerate MD set lists in distrib/sets/lists/dtb. Run this whenever
# new dts files are imported.

TARGET_ARCH_LIST=	earmv6 earmv6hf earmv6eb earmv6hfeb
TARGET_ARCH_LIST+=	earmv7 earmv7hf earmv7hf earmv7hfeb
TARGET_ARCH_LIST+=	aarch64 aarch64eb

update-sets:
.for _arch in ${TARGET_ARCH_LIST}
	${MAKE} MACHINE_ARCH=${_arch} dtblist | grep '^[\.#]' | \
	     column -t >${.CURDIR}/../../distrib/sets/lists/dtb/ad.${_arch}
.endfor
