#	$NetBSD: bsd.hostlib.mk,v 1.7 2003/08/01 17:04:01 lukem Exp $

.include <bsd.init.mk>
.include <bsd.sys.mk>

##### Basic targets
.PHONY:		cleanlib
clean:		cleanlib

##### Default values
CFLAGS+=	${COPTS}
HOST_MKDEP?=	CC=${HOST_CC:Q} mkdep

# Override these:
MKDEP:=		${HOST_MKDEP}

.if ${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN)
OBJHOSTMACHINE=	# set
.endif

##### Build rules
.if defined(HOSTLIB)
DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${YHEADER:D${SRCS:M*.y:.y=.h}}
.endif	# defined(HOSTLIB)

.if !empty(SRCS:N*.h:N*.sh)
OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.lo/g}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS} ${HOSTPROG} ${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}

${OBJS}: ${DPSRCS}

lib${HOSTLIB}.a: ${OBJS} ${DPADD}
	-rm -f ${.TARGET}
	${HOST_AR} cq ${.TARGET} ${OBJS}
	@${HOST_RANLIB} ${.TARGET}

.endif	# defined(OBJS) && !empty(OBJS)

realall: lib${HOSTLIB}.a

cleanlib:
	rm -f a.out [Ee]rrs mklog core *.core \
	    lib${HOSTLIB}.a ${OBJS} ${CLEANFILES}

beforedepend:
CFLAGS:=	${HOST_CFLAGS}
CPPFLAGS:=	${HOST_CPPFLAGS}

.if defined(SRCS)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; trap 'rm -f $$TMP ; exit 1' 1 2 3 13 15; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.lo:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

##### Pull in related .mk logic
.include <bsd.obj.mk>
.include <bsd.dep.mk>

${TARGETS}:	# ensure existence
