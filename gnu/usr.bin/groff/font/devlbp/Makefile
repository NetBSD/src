# $NetBSD: Makefile,v 1.5 2003/10/21 10:01:20 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/font/devlbp/Makefile,v 1.2 2002/10/11 08:58:13 ru Exp $

DIST_SUBDIR=	font/devlbp

DEV=		lbp
LBPPRINT=	${PSPRINT}
DEVFILES=	DESC HB HBI HI HR HNB HNBI HNI HNR \
		TB TBI TI TR CR CB CI ER EB EI

CLEANFILES+=	DESC

DESC:	DESC.in
	${_MKTARGET_CREATE}
	(\
	    cat ${.ALLSRC}; \
	    if test "${PAGE}" = A4; then \
		echo "papersize a4"; \
	    else \
		echo "papersize letter"; \
	    fi; \
	    test -z '${LBPPRINT}' || echo print '${LBPPRINT}'; \
	) >${.TARGET}

.include "../Makefile.dev"
