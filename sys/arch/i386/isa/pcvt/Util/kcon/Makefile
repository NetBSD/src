PROG=		kcon
DEVICE=		/dev/ttyv0
CFLAGS+= 	-I${.CURDIR}/../keycap -DKEYB_DEVICE=\"${DEVICE}\"

MKMAN=	no

KEYCAPOBJDIR!=	cd ${.CURDIR}/../keycap; make print-objdir

LDADD=		-L${KEYCAPOBJDIR} -lkeycap

.include <bsd.prog.mk>
