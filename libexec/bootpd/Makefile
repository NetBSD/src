# Remove the -DVEND_CMU if you don't wish to support the "CMU vendor format"
# in addition to the RFC1048 format.

PROG=	bootpd
CFLAGS+=-DSYSLOG -DDEBUG -DVEND_CMU
SRCS=	bootpd.c readfile.c hash.c Version.c
MAN5=	bootptab.0
MAN8=	bootpd.0

.include <bsd.prog.mk>
