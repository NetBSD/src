#	$NetBSD: Makefile.sbmips.inc,v 1.1.24.1 2005/02/06 07:45:26 jmc Exp $

CPPFLAGS+=	-I$S/dist/ipf

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true;
