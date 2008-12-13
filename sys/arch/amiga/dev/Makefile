# $NetBSD: Makefile,v 1.6.84.1 2008/12/13 01:13:00 haad Exp $

.include <bsd.own.mk>

# ite support for A2410, generate C source from TMS34010 assembler 
grf_ultms.c: grf_ultms.g
	echo '/* $$' 'NetBSD$$ */' | ${TOOL_SED} -e 's/ N/N/' > ${.TARGET}
	echo >> ${.TARGET}
	echo '/* Generated from' >> ${.TARGET}
	grep '$$NetBSD' ${.ALLSRC} \
	    | ${TOOL_SED} -e 's/[^$$]*\$$//' -e 's/\$$.*//' -e 's/^/   /' \
	    >> ${.TARGET}
	echo '*/' >> ${.TARGET}
	echo >> ${.TARGET}
	gspa -c tmscode ${.ALLSRC} >> ${.TARGET}
