#	$NetBSD: Makefile.sbmips.inc,v 1.1 2002/03/06 02:13:38 simonb Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true;
