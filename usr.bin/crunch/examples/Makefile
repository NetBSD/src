#	$NetBSD: Makefile,v 1.5 2001/12/12 01:48:44 tv Exp $

CRUNCHED=   fixit

# below is boiler-plate to make $(CRUNCHED) from $(CRUNCHED).conf
# I'd use PROG instead of CRUNCHED, but the system makefiles REALLY want
# to build things in the normal way if you use PROG.

CONF=	$(CRUNCHED).conf

OUTMK=	$(CRUNCHED).mk
OUTPUTS=  $(OUTMK) $(CRUNCHED).c $(CRUNCHED).cache

NOMAN=	# defined
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
