#	$NetBSD: Makefile.netwinder.inc,v 1.1.2.2 2002/06/23 17:38:40 jdolecek Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
