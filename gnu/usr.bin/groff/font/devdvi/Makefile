# $NetBSD: Makefile,v 1.5 2003/10/21 10:01:20 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/font/devdvi/Makefile,v 1.3 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	font/devdvi

DEV=		dvi
DEVFILES=	DESC \
		TR TI TB TBI CW CWI HR HI HB HBI \
		TREC TIEC TBEC TBIEC CWEC CWIEC HREC HIEC HBEC HBIEC \
		TRTC TITC TBTC TBITC CWTC CWITC HRTC HITC HBTC HBITC \
		MI S EX SA SB SC \
		generate/Makefile \
		generate/msam.map generate/msbm.map \
		generate/texb.map generate/texex.map generate/texi.map \
		generate/texmi.map generate/texr.map generate/texsy.map \
		generate/textt.map generate/textex.map generate/ec.map \
		generate/tc.map
DEVSCRIPTS=	generate/CompileFonts

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
	    test -z '${DVIPRINT}' || echo print '${DVIPRINT}'; \
	) >${.TARGET}

.include "../Makefile.dev"
