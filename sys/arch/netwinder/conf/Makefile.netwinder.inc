#	$NetBSD: Makefile.netwinder.inc,v 1.7.8.1 2005/02/06 07:45:25 jmc Exp $

CPPFLAGS+=	-D${MACHINE}
CPPFLAGS+=	-I$S/dist/ipf

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
