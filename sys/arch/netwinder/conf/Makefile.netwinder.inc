#	$NetBSD: Makefile.netwinder.inc,v 1.2.4.5 2003/01/03 16:48:29 thorpej Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
