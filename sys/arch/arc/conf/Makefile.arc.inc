#	$NetBSD: Makefile.arc.inc,v 1.10.2.5 2005/11/10 13:55:00 skrll Exp $

CFLAGS+=	-mips2
AFLAGS+=	-mips2

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
