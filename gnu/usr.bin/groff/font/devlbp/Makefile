# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:46 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/font/devlbp/Makefile,v 1.2 2002/10/11 08:58:13 ru Exp $

DIST_SUBDIR=	font/devlbp

DEV=		lbp
LBPPRINT=	${PSPRINT}
DEVFILES=	DESC HB HBI HI HR HNB HNBI HNI HNR \
		TB TBI TI TR CR CB CI ER EB EI

CLEANFILES+=	DESC

DESC:	DESC.in
	cat ${.ALLSRC} >${.TARGET}
	if test "${PAGE}" = A4; then \
	  echo "papersize a4" >>${.TARGET}; \
        else \
	  echo "papersize letter" >>${.TARGET}; \
        fi
	test -z '${LBPPRINT}' || echo print '${LBPPRINT}' >>${.TARGET}

.include "../Makefile.dev"
