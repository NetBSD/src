# $NetBSD: Makefile,v 1.3 1997/06/05 14:25:52 agc Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
CFLAGS+=	-I${.CURDIR} ${DEBUG}
SRCS=		exec.c file.c global.c msg.c pen.c plist.c str.c
SRCS+=		ftpio.c ftperr.c 
MAN3=		ftpio.3
CLEANFILES+=	ftperr.c

NOPROFILE=	yes
NOPIC=		yes

# only needed during build - prevent installation of library
libinstall::

ftperr.c:	ftp.errors
	@echo '#include <stdio.h>' > ${.TARGET}
	@echo '#include "ftpio.h"' >> ${.TARGET}
	@echo "struct ftperr ftpErrList[] = {" \ >>  ${.TARGET}
	@cat ${.ALLSRC} \
	  | grep -v ^# \
	  | sort \
	  | while read NUM STRING; do \
	    echo "  { $${NUM}, \"$${STRING}\" },"; \
	  done >> ${.TARGET}
	@echo "};" >> ${.TARGET}
	@echo -n "int const ftpErrListLength = " >> ${.TARGET}
	@echo "sizeof(ftpErrList) / sizeof(*ftpErrList);" >> ${.TARGET}

.include <bsd.lib.mk>
