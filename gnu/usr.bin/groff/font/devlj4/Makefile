# $NetBSD: Makefile,v 1.7 2006/02/06 18:29:48 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/font/devlj4/Makefile,v 1.1 2001/04/17 12:36:48 ru Exp $

DIST_SUBDIR=	font/devlj4

DEV=		lj4
LJ4RES=		1200
LJ4PRINT=	${PSPRINT}
DEVFILES=	DESC \
		AB ABI AI AR \
		ALBB ALBR AOB AOI AOR CB CBI CI CR GB GBI GI GR \
		LGB LGI LGR OB OBI OI OR TB TBI TI TR \
		TNRB TNRBI TNRI TNRR \
		UB UBI UI UR \
		UCB UCBI UCI UCR CLARENDON CORONET MARIGOLD S \
		SYMBOL WINGDINGS \
		generate/Makefile generate/symbol.map generate/special.awk \
		generate/special.map generate/text.map generate/wingdings.map

CLEANFILES+=	DESC

DESC: DESC.in
	${_MKTARGET_CREATE}
	(\
	    echo "res ${LJ4RES}"; \
	    echo "unitwidth `expr 7620000 / ${LJ4RES}`"; \
	    cat ${.ALLSRC}; \
	    if test "${PAGE}" = A4; then \
		echo "papersize a4"; \
	    else \
		echo "papersize letter"; \
	    fi; \
	    test -z '${LJ4PRINT}' || echo print '${LJ4PRINT}'; \
	) >${.TARGET}

.include "../Makefile.dev"
