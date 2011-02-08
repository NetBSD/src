PROG=           dmctl
MAN=		dmctl.8
WARN= 		4
SRC=		dmctl.c

BINDIR=		/sbin

#LIBDM_INCLUDE= ${NETBSDSRCDIR}/lib/libdm/
#LIBDM_OBJDIR= ${NETBSDSRCDIR}/lib/libdm/
#CFLAGS+= -fno-inline -Wall -O0 -g
#CPPFLAGS+= -I. -I${LIBDM_INCLUDE}

LDADD+= -ldm

.ifdef RUMP_ACTION
CPPFLAGS+=  -D RUMP_ACTION
LDADD+= -lrumpclient
.endif

LDADD+= -lprop

#LDSTATIC= -static

.include <bsd.prog.mk>
