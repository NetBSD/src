#	$NetBSD: Makefile,v 1.4 2000/08/30 23:51:55 jhawk Exp $

CRUNCHED=   fixit

# below is boiler-plate to make $(CRUNCHED) from $(CRUNCHED).conf
# I'd use PROG instead of CRUNCHED, but the system makefiles REALLY want
# to build things in the normal way if you use PROG.

CONF=	$(CRUNCHED).conf

OUTMK=	$(CRUNCHED).mk
OUTPUTS=  $(OUTMK) $(CRUNCHED).c $(CRUNCHED).cache

MKMAN=	no
CLEANFILES+=$(CRUNCHED) $(OUTPUTS) *.o *.lo *_stub.c

all: $(CRUNCHED)
exe: $(CRUNCHED)

$(OUTPUTS): $(CONF)
	crunchgen ${.CURDIR}/$(CONF)

$(CRUNCHED): $(OUTPUTS) submake

submake:
	${MAKE} -f $(OUTMK)
objs:
	${MAKE} -f $(OUTMK) objs

.include <bsd.prog.mk>
