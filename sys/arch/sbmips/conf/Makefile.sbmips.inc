#	$NetBSD: Makefile.sbmips.inc,v 1.1.12.2 2002/03/06 02:13:39 simonb Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true;
