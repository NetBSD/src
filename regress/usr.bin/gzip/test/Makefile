#	$NetBSD: Makefile,v 1.1 2004/07/10 19:32:06 yamt Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

INPUTS=		truncated.gz crcerror.gz
UUDECODE_FILES=	${INPUTS}

regress: ${INPUTS}
.for x in ${INPUTS}
	@sh -c "if ${GZIP} -t ${x}; then false; else true; fi"
	@echo "# Errors messages above are expected."
.endfor

.include <bsd.prog.mk>
