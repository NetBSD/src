PROG=		macppc_installboot
CPPFLAGS+=	-I/sys/arch
CLEANFILES+=	mkmacppcboot macppcboot.raw
NOMAN=1

.include <bsd.prog.mk>

mkmacppcboot: mkmacppcboot.c
	${LINK.c} -o $@ $>

macppcboot.raw: mkmacppcboot bootxx.raw
	./mkmacppcboot <${.CURDIR}/bootxx.raw >$@

all: macppcboot.raw
