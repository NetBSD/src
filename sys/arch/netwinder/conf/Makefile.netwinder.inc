#	$NetBSD: Makefile.netwinder.inc,v 1.6 2002/11/03 21:43:32 chris Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
