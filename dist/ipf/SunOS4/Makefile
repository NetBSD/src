#
# Copyright (C) 1993-1998 by Darren Reed.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and due credit is given
# to the original author and the contributors.
#
BINDEST=/usr/local/bin
SBINDEST=/sbin
MANDIR=/usr/share/man
CC=/usr/5bin/cc
CFLAGS=-I..
#
# For SunOS 4.1.x
#
DCPU:sh=uname -m
DEF=-D$(DCPU) -D__$(DCPU)__ -DINET -DKERNEL -D_KERNEL -Dsun $(CPU)
IPDEF=$(DEF) -DGATEWAY -DDIRECTED_BROADCAST
IPFILC=ip_fil.c
ATON=-DNEED_INET_ATON
DEBUG=-g
# For the LKM:
LKM=if_ipl.o
LOGFAC=-DLOGFAC=LOG_LOCAL0
#
MFLAGS="BINDEST=$(BINDEST)" "SBINDEST=$(SBINDEST)" "MANDIR=$(MANDIR)" \
        'CFLAGS=$(CFLAGS) $(SOLARIS2)' "IPFLKM=$(IPFLKM)" \
        "IPFLOG=$(IPFLOG)" "LOGFAC=$(LOGFAC)" "POLICY=$(POLICY)" \
        "SOLARIS2=$(SOLARIS2)" "DEBUG=$(DEBUG)" "DCPU=$(CPU)" \
        "CPUDIR=$(CPUDIR)"
#
########## ########## ########## ########## ########## ########## ##########
#
CP=/bin/cp
RM=/bin/rm
CHMOD=/bin/chmod
INSTALL=install
#
MODOBJS=ip_fil.o fil_k.o mls_ipl.o ip_nat.o ip_state.o ip_frag.o ip_proxy.o \
	ip_auth.o ip_log.o
DFLAGS=$(IPFLKM) $(IPFLOG) $(DEF)
IPF=ipf.o parse.o opt.o inet_addr.o facpri.o
IPT=ipt.o parse.o fil.o ipft_sn.o ipft_ef.o ipft_td.o ipft_pc.o opt.o \
    ipft_tx.o misc.o ip_nat_u.o ip_frag_u.o ip_state_u.o inet_addr.o \
    ipft_hx.o ip_fil_u.o ip_proxy_u.o ip_auth_u.o natparse.o facpri.o
FILS=fils.o parse.o kmem.o opt.o inet_addr.o facpri.o

all:
	(cd ..; $(MAKE) $(MFLAGS) sunos4; )

sunos4 solaris1 build: ipf ipfstat ipftest ipmon ipnat if_ipl.o

ipfstat: $(FILS)
	$(CC) $(DEBUG) $(CFLAGS) $(FILS) -o $@ $(LIBS)

ipf: $(IPF)
	$(CC) $(DEBUG) $(CFLAGS) $(IPF) -o $@ $(LIBS)
	/bin/rm -f ../ipf
	ln -s `pwd`/ipf ..

ipftest: $(IPT)
	$(CC) $(DEBUG) $(CFLAGS) $(IPT) -o $@ $(LIBS)
	/bin/rm -f ../ipftest
	ln -s `pwd`/ipftest ..

ipnat: ipnat.o kmem.o natparse.o
	$(CC) $(DEBUG) $(CFLAGS) ipnat.o kmem.o natparse.o -o $@ $(LIBS)

tests:
	(cd test; make )

fils.o: ../fils.c ../ip_fil.h ../ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../fils.c -o $@

fil.o: ../fil.c ../ip_fil.h ../ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../fil.c -o $@

fil_k.o: ../fil.c ../ip_fil.h ../ipf.h
	$(CC) $(DEBUG) $(CFLAGS) $(POLICY) $(DFLAGS) -c ../fil.c -o $@

ipf.o: ../ipf.c ../ip_fil.h ../ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipf.c -o $@

ipt.o: ../ipt.c ../ip_fil.h ../ipt.h ../ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipt.c -o $@

misc.o: ../misc.c ../ip_fil.h ../ipt.h ../ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../misc.c -o $@

inet_addr.o: ../inet_addr.c
	$(CC) $(ATON) $(DEBUG) $(CFLAGS) -c ../inet_addr.c -o $@

opt.o: ../opt.c
	$(CC) $(DEBUG) $(CFLAGS) -c ../opt.c -o $@

ipnat.o: ../ipnat.c ../ip_fil.h ../ipf.h ../ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipnat.c -o $@

natparse.o: ../natparse.c ../ip_fil.h ../ipf.h ../ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../natparse.c -o $@

ipft_sn.o: ../ipft_sn.c ../ipt.h ../ipf.h ../ip_fil.h ../snoop.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipft_sn.c -o $@

ipft_ef.o: ../ipft_ef.c ../ipf.h ../ip_fil.h ../ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipft_ef.c -o $@

ipft_td.o: ../ipft_td.c ../ipf.h ../ip_fil.h ../ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipft_td.c -o $@

ipft_pc.o: ../ipft_pc.c ../ipf.h ../ip_fil.h ../ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipft_pc.c -o $@

ipft_tx.o: ../ipft_tx.c ../ipf.h ../ip_compat.h ../ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipft_tx.c -o $@

ipft_hx.o: ../ipft_hx.c ../ipf.h ../ip_compat.h ../ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ipft_hx.c -o $@

if_ipl.o: $(MODOBJS)
	ld -r $(MODOBJS) -o $(LKM)
	${RM} -f if_ipl

ip_nat.o: ../ip_nat.c ../ip_nat.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../ip_nat.c -o $@

ip_frag.o: ../ip_frag.c ../ip_frag.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../ip_frag.c -o $@

ip_state.o: ../ip_state.c ../ip_state.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../ip_state.c -o $@

ip_proxy.o: ../ip_proxy.c ../ip_proxy.h ../ip_compat.h ../ip_ftp_pxy.c \
	../ip_rcmd_pxy.c ../ip_raudio_pxy.c $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../ip_proxy.c -o $@

ip_auth.o: ../ip_auth.c ../ip_auth.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../ip_auth.c -o $@

ip_nat_u.o: ../ip_nat.c ../ip_nat.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ip_nat.c -o $@

ip_frag_u.o: ../ip_frag.c ../ip_frag.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ip_frag.c -o $@

ip_state_u.o: ../ip_state.c ../ip_state.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ip_state.c -o $@

ip_proxy_u.o: ../ip_proxy.c ../ip_proxy.h ../ip_compat.h ../ip_ftp_pxy.c \
	../ip_rcmd_pxy.c ../ip_raudio_pxy.c $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ip_proxy.c -o $@

ip_auth_u.o: ../ip_auth.c ../ip_auth.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ip_auth.c -o $@

ip_fil.o: ../$(IPFILC) ../ip_fil.h ../ip_compat.h ../ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../$(IPFILC) -o $@

ip_log.o: ../ip_log.c ../ip_fil.h ../ip_compat.h ../ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../ip_log.c -o $@

ip_fil_u.o: ../$(IPFILC) ../ip_fil.h ../ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../ip_fil.c -o $@

ip_input.o: ../ip_input.c ../ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(IPDEF) -c ../ip_input.c -o $@

ip_output.o: ../ip_output.c ../ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(IPDEF) -c ../ip_output.c -o $@

mls_ipl.o: ../mls_ipl.c ../ipl.h
	-/bin/rm -f ../vnode_if.c
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c ../mls_ipl.c -o $@

kmem.o: ../kmem.c
	$(CC) $(DEBUG) $(CFLAGS) -c ../kmem.c -o $@

parse.o: ../parse.c ../ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../parse.c -o $@

facpri.o: ../facpri.c ../facpri.h
	$(CC) $(DEBUG) $(CFLAGS) -c ../facpri.c -o $@

ipmon: ../ipmon.c
	$(CC) $(DEBUG) $(CFLAGS) $(LOGFAC) ../ipmon.c -o $@ $(LIBS)

clean:
	${RM} -f core *.o ipt fils ipf ipfstat ipftest ipmon if_ipl ipnat \
		$(LKM)
	${MAKE} -f Makefile.ipsend clean


install: all ../ip_fil.h
	-$(CP) ../ip_fil.h /usr/include/netinet/ip_fil.h
	-$(CHMOD) 444 /usr/include/netinet/ip_fil.h
	-$(INSTALL) -cs -g wheel -m 755 -o root ipfstat ipf ipnat $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipmon ipftest $(BINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipftest ipftest $(BINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipf ipftest $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipnat ipftest $(SBINDEST)
	(cd ../man; make INSTALL=$(INSTALL) MANDIR=$(MANDIR) install; cd ..)
