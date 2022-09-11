#	$NetBSD: Makefile.evbppc.inc,v 1.9 2022/09/11 09:14:56 rin Exp $

CPPFLAGS += -D__${BOARDTYPE}__

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISPPC:T}/ramdisk && ${PRINTOBJDIR}

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"

# XXX hack for clang:
# - integrated as does not recognize {l,st}swx.
# - it does not pass gas for appropriate -mfoo flag.
.if ${ACTIVE_CC} == "clang"
.  if !empty(AFLAGS:M-mcpu=403)
COPTS.trap.c+=	-fno-integrated-as -Wa,-m403
.  endif
.  if !empty(AFLAGS:M-mcpu=405)
COPTS.trap.c+=	-fno-integrated-as -Wa,-m405
.  endif
.endif
