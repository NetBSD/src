#	$NetBSD: Makefile.ev64260.inc,v 1.1 2003/03/05 22:08:24 matt Exp $

TEXTADDR?=	100000

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S

.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
.endif
