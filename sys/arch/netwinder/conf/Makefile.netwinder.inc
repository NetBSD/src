#	$NetBSD: Makefile.netwinder.inc,v 1.2.4.3 2002/04/17 00:03:55 nathanw Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
