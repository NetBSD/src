#	$NetBSD: Makefile.dreamcast.inc,v 1.8 2002/10/30 12:22:55 tsutsui Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
