#	$NetBSD: bsd.klinks.mk,v 1.14 2017/12/06 02:06:45 christos Exp $
#

.include <bsd.own.mk>

KLINK_MACHINE?=	${MACHINE}

##### Default values
.if !defined(S)
.   if defined(NETBSDSRCDIR)
S=	${NETBSDSRCDIR}/sys
.   elif defined(BSDSRCDIR)
S=	${BSDSRCDIR}/sys
.   else
S=	/sys
.   endif
.endif

KLINKFILES+=	${MACHINE_CPU} ${KLINK_MACHINE}

.if ${KLINK_MACHINE} == "sun2" || ${KLINK_MACHINE} == "sun3"
KLINKFILES+=	sun68k
.elif ${KLINK_MACHINE} == "sparc64"
KLINKFILES+=	sparc
.elif ${KLINK_MACHINE} == "i386"
KLINKFILES+=	x86
.elif ${KLINK_MACHINE} == "amd64"
KLINKFILES+=	x86 i386
.elif ${KLINK_MACHINE} == "evbmips"
KLINKFILES+=	algor sbmips
.elif ${MACHINE_CPU} == "aarch64"
KLINKFILES+=	arm
.elif defined(XEN_BUILD) || ${KLINK_MACHINE} == "xen"
KLINKFILES+=	xen
CLEANFILES+=	xen-ma/machine # xen-ma
CPPFLAGS+=	-I${.OBJDIR}/xen-ma
.endif

CLEANFILES+= machine ${KLINKFILES}

# XXX.  This should be done a better way.  It's @'d to reduce visual spew.
# XXX   .BEGIN is used to make sure the links are done before anything else.
.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	-@rm -f machine && \
	    ln -s $S/arch/${KLINK_MACHINE}/include machine
.   for kl in ${KLINKFILES}
	-@if [ -d $S/arch/${kl}/include ]; then \
	    rm -f ${kl} && ln -s $S/arch/${kl}/include ${kl}; \
	fi
.   endfor
.   if defined(XEN_BUILD) || ${KLINK_MACHINE} == "xen"
	-@rm -rf xen-ma && mkdir xen-ma && \
	    ln -s ../${XEN_BUILD:U${MACHINE_ARCH}} xen-ma/machine
.   endif
.endif
