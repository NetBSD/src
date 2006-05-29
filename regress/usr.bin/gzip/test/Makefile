#	$NetBSD: Makefile,v 1.3 2006/05/29 16:47:07 simonb Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

BADINPUTS=		truncated.gz crcerror.gz
GOODINPUTS=		good.gz
UUDECODE_FILES=	${BADINPUTS} ${GOODINPUTS}

regress: ${BADINPUTS} ${GOODINPUTS}
.for x in ${BADINPUTS}
	@sh -c "if ${GZIPPROG} -t ${x}; then false; else true; fi"
	@echo "# Errors messages above are expected."
.endfor
.for x in ${GOODINPUTS}
	@${GZIPPROG} -t ${x}
.endfor

.include <bsd.prog.mk>
