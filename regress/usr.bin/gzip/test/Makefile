#	$NetBSD: Makefile,v 1.2 2004/07/11 09:21:50 yamt Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

BADINPUTS=		truncated.gz crcerror.gz
GOODINPUTS=		good.gz
UUDECODE_FILES=	${BADINPUTS} ${GOODINPUTS}

regress: ${BADINPUTS} ${GOODINPUTS}
.for x in ${BADINPUTS}
	@sh -c "if ${GZIP} -t ${x}; then false; else true; fi"
	@echo "# Errors messages above are expected."
.endfor
.for x in ${GOODINPUTS}
	@${GZIP} -t ${x}
.endfor

.include <bsd.prog.mk>
