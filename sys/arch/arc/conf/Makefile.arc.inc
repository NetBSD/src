#	$NetBSD: Makefile.arc.inc,v 1.13 2005/05/07 14:15:05 tsutsui Exp $

CFLAGS+=	-mips2
AFLAGS+=	-mips2

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
