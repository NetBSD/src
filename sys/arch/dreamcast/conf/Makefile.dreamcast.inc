#	$NetBSD: Makefile.dreamcast.inc,v 1.8.12.1 2005/02/06 07:45:24 jmc Exp $

MACHINE_ARCH=	sh3el
CPPFLAGS+=	-I$S/dist/ipf

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
