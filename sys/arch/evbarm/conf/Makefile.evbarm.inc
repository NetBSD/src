#	$NetBSD: Makefile.evbarm.inc,v 1.3 2001/11/09 07:39:25 thorpej Exp $

.if (${BOARDTYPE} == "integrator")
SYSTEM_FIRST_OBJ=	intmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/integrator/intmmu.S 
.endif

SYSTEM_LD_TAIL_EXTRA= \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

.if (${BOARDTYPE} == "iq80310")
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec
.endif
