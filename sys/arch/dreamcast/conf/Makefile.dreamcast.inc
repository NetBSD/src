#	$NetBSD: Makefile.dreamcast.inc,v 1.7.12.2 2002/06/22 04:17:45 thorpej Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
