#	from: @(#)Makefile	6.1 (Berkeley) 3/3/91
#	$Id: Makefile,v 1.8 1993/10/16 03:19:36 cgd Exp $

.include "config/Makefile.$(MACHINE)"

.if !defined (gas_hosttype)
gas_hosttype=$(MACHINE)
.endif
.if !defined (gas_target)
gas_target=$(MACHINE)
.endif
.if !defined (gas_objformat)
gas_objformat=aout
.endif

.if exists(${.CURDIR}/obj)
ADDINCLUDE=-I${.CURDIR}/obj
.endif

PROG=		as
SRCS+=		app.c as.c atof-generic.c bignum-copy.c \
		cond.c expr.c flo-const.c flo-copy.c flonum-mult.c \
		frags.c hash.c hex-value.c input-file.c input-scrub.c \
		listing.c messages.c obstack.c output-file.c read.c subsegs.c \
		symbols.c version.c write.c xmalloc.c xrealloc.c \
		obj-$(gas_objformat).c
CFLAGS+=	-I$(.CURDIR) ${ADDINCLUDE} -I$(.CURDIR)/config \
		-DPIC -DOLD_GAS -DSIGTY=void -Derror=as_fatal
#LDADD+=		-lgnumalloc
DPADD+=		/usr/lib/libgnumalloc.a

.PATH: $(.CURDIR)/config

beforedepend: config.status
$(PROG): config.status

config.status:
	/bin/rm -f targ-cpu.h ; ln -s $(.CURDIR)/config/tc-$(gas_target).h targ-cpu.h
	/bin/rm -f obj-format.h ; ln -s $(.CURDIR)/config/obj-$(gas_objformat).h obj-format.h
	/bin/rm -f host.h
.if exists ($(.CURDIR)/config/ho-$(gas_hosttype).h)
	ln -s $(.CURDIR)/config/ho-$(gas_hosttype).h host.h
.else
	ln -s $(.CURDIR)/config/ho-generic.h host.h
.endif
	/bin/rm -f targ-env.h
.if exists ($(.CURDIR)/config/te-$(MACHINE).h)
	ln -s $(.CURDIR)/config/te-$(MACHINE).h targ-env.h
.else
	ln -s $(.CURDIR)/config/te-generic.h targ-env.h
.endif
	echo "curdir=$(.CURDIR) machine=$(gas_target) host=$(gas_hosttype)" > $(.TARGET)

CLEANFILES+= targ-cpu.h obj-format.h host.h targ-env.h config.status

.include <bsd.prog.mk>
