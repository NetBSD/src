#	$NetBSD: Makefile.netwinder.inc,v 1.7 2003/01/03 02:34:49 thorpej Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
