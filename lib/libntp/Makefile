#	$NetBSD: Makefile,v 1.2 1997/04/18 14:25:53 christos Exp $

LIB=	ntp
SRCS=	a_md512crypt.c a_md5decrypt.c a_md5encrypt.c adjtime.c atoint.c atolfp.c \
atouint.c auth12crypt.c authdecrypt.c authdes.c authencrypt.c authkeys.c \
authparity.c authreadkeys.c authusekey.c buftvtots.c caljulian.c calleapwhen.c \
caltontp.c calyearstart.c clocktime.c clocktypes.c decodenetnum.c dofptoa.c \
dolfptoa.c emalloc.c findconfig.c fptoa.c fptoms.c getopt.c \
hextoint.c hextolfp.c humandate.c inttoa.c lib_strbuf.c lib_strbuf.h \
machines.c memmove.c mexit.c mfptoa.c mfptoms.c modetoa.c \
mstolfp.c msutotsf.c msyslog.c netof.c numtoa.c numtohost.c \
octtoint.c prettydate.c ranny.c refnumtoa.c statestr.c syssignal.c \
systime.c tsftomsu.c tstotv.c tvtoa.c tvtots.c uglydate.c \
uinttoa.c utvtoa.c version.c

CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}/../../usr.sbin/xntp/include

.include <bsd.lib.mk>
