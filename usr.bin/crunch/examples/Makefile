#	$NetBSD: Makefile,v 1.6 2003/07/10 11:21:51 lukem Exp $

NOMAN=	# defined

CRUNCHED=   fixit

# below is boiler-plate to make $(CRUNCHED) from $(CRUNCHED).conf
# I'd use PROG instead of CRUNCHED, but the system makefiles REALLY want
# to build things in the normal way if you use PROG.

CONF=	$(CRUNCHED).conf

OUTMK=	$(CRUNCHED).mk
OUTPUTS=  $(OUTMK) $(CRUNCHED).c $(CRUNCHED).cache

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
