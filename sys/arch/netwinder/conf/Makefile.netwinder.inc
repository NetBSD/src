#	$NetBSD: Makefile.netwinder.inc,v 1.5 2002/04/03 02:06:33 thorpej Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
