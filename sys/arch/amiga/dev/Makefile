# $NetBSD: Makefile,v 1.2.6.1 2002/06/23 17:34:24 jdolecek Exp $

# ite support for A2410, generate C source from TMS34010 assembler 
grf_ultms.c: grf_ultms.g
	echo '/* $$' 'NetBSD$$ */' | sed -e 's/ N/N/' > ${.TARGET}
	echo >> ${.TARGET}
	echo '/* Generated from' >> ${.TARGET}
	grep '$$NetBSD' ${.ALLSRC} | sed -e 's/[^$$]*\$$//' -e 's/\$$.*//' \
		-e 's/^/   /' >> ${.TARGET}
	echo '*/' >> ${.TARGET}
	echo >> ${.TARGET}
	gspa -c tmscode < ${.ALLSRC} >> ${.TARGET}
