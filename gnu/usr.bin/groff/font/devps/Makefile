# $NetBSD: Makefile,v 1.4 2003/10/20 00:54:37 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/font/devps/Makefile,v 1.3 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	font/devps

DEV=		ps
DISTFILES=	text.enc download \
		S ZD ZDR SS AB ABI AI AR BMB BMBI BMI BMR \
		CB CBI CI CR HB HBI HI HR HNB HNBI HNI HNR \
		NB NBI NI NR PB PBI PI PR TB TBI TI TR ZCMI \
		EURO freeeuro.pfa
PSFILES=	prologue symbolsl.pfa zapfdr.pfa
DEVGENFILES=	generate/Makefile generate/dingbats.map \
		generate/dingbats.rmap generate/lgreekmap \
		generate/symbolchars generate/symbolsl.afm generate/textmap
DEVGENSCRIPTS=	generate/afmname generate/symbol.sed
DEVFILES=	DESC ${PSFILES} ${DISTFILES} ${DEVGENFILES}
DEVSCRIPTS=	${DEVGENSCRIPTS}

CLEANFILES+=	DESC ${PSFILES}

DESC: DESC.in
	${_MKMSGCREATE}
	${_MKCMD}\
	(\
	    cat ${.ALLSRC}; \
	    echo "broken ${BROKEN_SPOOLER_FLAGS}"; \
	    if test "${PAGE}" = A4; then \
		echo "papersize a4"; \
	    else \
		echo "papersize letter"; \
	    fi; \
	    test -z '${PSPRINT}' || echo print '${PSPRINT}'; \
	) >${.TARGET}

${PSFILES}:
	${_MKMSGCREATE}
	${_MKCMD}\
	rm -f $@
	${_MKCMD}\
	sed -f ${DIST_DIR}/psstrip.sed $? >$@

.include "../Makefile.dev"

prologue:	${DIST_DIR}/prologue.ps
zapfdr.pfa:	${DIST_DIR}/zapfdr.ps
symbolsl.pfa:	${DIST_DIR}/symbolsl.ps
