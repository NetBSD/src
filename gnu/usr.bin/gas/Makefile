#	from: @(#)Makefile	6.1 (Berkeley) 3/3/91
#	$Id: Makefile,v 1.3 1993/10/02 20:57:03 pk Exp $

.if !defined (HOST)
HOST=$(MACHINE)
.endif
.if !defined (OBJFORMAT)
OBJFORMAT=aout
.endif

PROG=		as
SRCS=		app.c as.c atof-generic.c bignum-copy.c \
		cond.c expr.c flo-const.c flo-copy.c flonum-mult.c \
		frags.c hash.c hex-value.c input-file.c input-scrub.c \
		listing.c messages.c obstack.c output-file.c read.c subsegs.c \
		symbols.c version.c write.c xmalloc.c xrealloc.c \
		obj-$(OBJFORMAT).c
#	$(srcdir)/strerror.c
#	$(srcdir)/strstr.c
CFLAGS+=	-I$(.CURDIR) -I$(.CURDIR)/config \
		-DPIC -DOLD_GAS -DSIGTY=void -Derror=as_fatal
.PATH: $(.CURDIR)/config

$(PROG): config.status

config.status:
	ln -s $(.CURDIR)/config/tc-$(MACHINE).h $(.CURDIR)/targ-cpu.h
	ln -s $(.CURDIR)/config/obj-$(OBJFORMAT).h $(.CURDIR)/obj-format.h
	ln -s $(.CURDIR)/config/ho-$(HOST).h $(.CURDIR)/host.h
.if exists ($(.CURDIR)/config/te-$(MACHINE).h)
	ln -s $(.CURDIR)/config/te-$(MACHINE).h $(.CURDIR)/targ-env.h
.else
	ln -s $(.CURDIR)/config/te-generic.h $(.CURDIR)/targ-env.h
.endif
	echo "curdir=$(.CURDIR) machine=$(MACHINE) host=$(HOST)" > $(.TARGET)

CLEANFILES+= targ-cpu.h obj-format.h host.h targ-env.h config.status

.include "config/Makefile.$(MACHINE)"

.include <bsd.prog.mk>
