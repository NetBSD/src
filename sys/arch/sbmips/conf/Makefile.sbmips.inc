#	$NetBSD: Makefile.sbmips.inc,v 1.1.10.2 2002/06/23 17:40:03 jdolecek Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true;
