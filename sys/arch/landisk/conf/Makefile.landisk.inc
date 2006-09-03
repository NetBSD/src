#	$NetBSD: Makefile.landisk.inc,v 1.1.2.2 2006/09/03 15:23:05 yamt Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=0x8c001000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
