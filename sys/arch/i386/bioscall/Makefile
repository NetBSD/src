#	$NetBSD: Makefile,v 1.3 1996/09/08 15:36:51 jtk Exp $
#
# LP (Laptop Package)
#
# (C) 1994 by HOSOKAWA, Tatsumi <hosokawa@mt.cs.keio.ac.jp>
#
# This software may be used, modified, copied, and distributed in
# both source and binary form provided that the above copyright and
# these terms are retained. Under no circumstances is the author 
# responsible for the proper functioning of this software, nor does 
# the author assume any responsibility for damages incurred with its 
# use.
#
# Sep., 1994	Implemented on FreeBSD 1.1.5.1R (Toshiba AVS001WD)
# Oct., 1994	NetBSD port (1.0 BETA 10/2) by ukai
# Dec., 1995	NetBSD 1.1 kernel build retrofit, jtk@netbsd.org
#

DIR=${APMREL}${APMDIR}

.if exists (${DIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${DIR}/arch/${MACHINE_ARCH}
.include "${DIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

.PATH: ${DIR}

CC = ${APMCC} -I. -I$S/arch -I$S
LD = ${APMLD}
CFLAGS += -DINITIALIZER	-I${DIR} -DKERNEL \
	${APMCFLAGS:S@-I.@-I${KERNREL}.@g}

OBJS = biostramp.o

$(OBJS): ${I386}/include/apmvar.h

#.SUFFIXES: .c .S .o
#
#.c.o:
#	$(CC)  $(CFLAGS) $(OPTFLAGS) $(INC) -c $<
#
.S.o:
	$(CC)  $(CFLAGS) $(INC) -c $<

DEPS=${APMDEPS}

biostramp.inc: biostramp Makefile
	sh ${DIR}/bin2asm.sh biostramp > biostramp.inc

biostramp: $(OBJS) $(DEPS)
	$(LD) -Bstatic -N -T 0 -o biostramp $(OBJS)
	cp biostramp biostramp.sym
	@${APMSTRIP} biostramp
	@if [ `${LS} -l biostramp |awk '{print $$5}'` -gt 4096 ]; then \
		echo "biostramp too big"; \
		exit 1; \
	fi
	@sh ${DIR}/rmaouthdr biostramp biostramp.tmp
	@mv -f biostramp.tmp biostramp

#allclean: clean
#	@rm -f apm_init.inc
clean:
	rm -f *.o biostramp biostramp.sym biostramp.inc

#.include <bsd.prog.mk>
