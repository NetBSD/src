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
TOP=..
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
DFLAGS=$(IPFLKM) $(DEF)
IPF=ipf.o common.o parse.o opt.o inet_addr.o facpri.o
IPT=ipt.o common.o parse.o fil.o opt.o ipft_sn.o ipft_ef.o ipft_td.o \
    ipft_pc.o ipft_tx.o misc.o ip_nat_u.o ip_frag_u.o ip_state_u.o ip_log_u.o \
    inet_addr.o ipft_hx.o ip_fil_u.o ip_proxy_u.o ip_auth_u.o natparse.o \
    facpri.o printnat.o printstate.o
IPNAT=ipnat.o kmem.o common.o natparse.o inet_addr.o printnat.o
FILS=fils.o parse.o common.o kmem.o opt.o inet_addr.o facpri.o printstate.o

all:
	(cd $(TOP) $(MAKE) $(MFLAGS) sunos4; )

sunos4 solaris1 build: ipf ipfstat ipftest ipmon ipnat ipfs if_ipl.o

ipfstat: $(FILS)
	$(CC) $(DEBUG) $(CFLAGS) $(STATETOP_CFLAGS) $(STATETOP_INC) $(FILS) \
	-o $@ $(LIBS) $(STATETOP_LIB) -lkvm

ipf: $(IPF)
	$(CC) $(DEBUG) $(CFLAGS) $(IPF) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipf
	ln -s `pwd`/ipf $(TOP)

ipfs: ipfs.o
	$(CC) $(DEBUG) $(CFLAGS) ipfs.o -o $@ $(LIBS)

ipftest: $(IPT)
	$(CC) $(DEBUG) $(CFLAGS) $(IPT) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipftest
	ln -s `pwd`/ipftest $(TOP)

ipnat: $(IPNAT)
	$(CC) $(DEBUG) $(CFLAGS) $(IPNAT) -o $@ $(LIBS) -lkvm

tests:
	(cd test; make )

ipfs.o: $(TOP)/ipfs.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_state.h \
	$(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipfs.c -o $@

fils.o: $(TOP)/fils.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) $(STATETOP_CFLAGS) $(STATETOP_INC) \
	-c $(TOP)/fils.c -o $@

fil.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/fil.c -o $@

fil_k.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) $(POLICY) $(DFLAGS) -c $(TOP)/fil.c -o $@

ipf.o: $(TOP)/ipf.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipf.c -o $@

ipt.o: $(TOP)/ipt.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipt.c -o $@

misc.o: $(TOP)/misc.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/misc.c -o $@

inet_addr.o: $(TOP)/inet_addr.c
	$(CC) $(ATON) $(DEBUG) $(CFLAGS) -c $(TOP)/inet_addr.c -o $@

opt.o: $(TOP)/opt.c
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/opt.c -o $@

ipnat.o: $(TOP)/ipnat.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipnat.c -o $@

natparse.o: $(TOP)/natparse.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h \
	$(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/natparse.c -o $@

printnat.o: $(TOP)/printnat.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h \
	$(TOP)/ip_compat.h $(TOP)/ip_proxy.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/printnat.c -o $@

printstate.o: $(TOP)/printstate.c $(TOP)/ip_fil.h $(TOP)/ipf.h \
	$(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/printstate.c -o $@

ipft_sn.o: $(TOP)/ipft_sn.c $(TOP)/ipt.h $(TOP)/ipf.h $(TOP)/ip_fil.h \
	$(TOP)/snoop.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_sn.c -o $@

ipft_ef.o: $(TOP)/ipft_ef.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_ef.c -o $@

ipft_td.o: $(TOP)/ipft_td.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_td.c -o $@

ipft_pc.o: $(TOP)/ipft_pc.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_pc.c -o $@

ipft_tx.o: $(TOP)/ipft_tx.c $(TOP)/ipf.h $(TOP)/ip_compat.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_tx.c -o $@

ipft_hx.o: $(TOP)/ipft_hx.c $(TOP)/ipf.h $(TOP)/ip_compat.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_hx.c -o $@

if_ipl.o: $(MODOBJS)
	ld -r $(MODOBJS) -o $(LKM)
	${RM} -f if_ipl

ip_nat.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_nat.c -o $@

ip_frag.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_frag.c -o $@

ip_state.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_state.c -o $@

ip_proxy.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	$(TOP)/ip_ftp_pxy.c $(TOP)/ip_rcmd_pxy.c $(TOP)/ip_raudio_pxy.c \
	$(TOP)/ip_ipsec_pxy.c $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_proxy.c -o $@

ip_auth.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_auth.c -o $@

ip_nat_u.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_nat.c -o $@

ip_frag_u.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_frag.c -o $@

ip_state_u.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_state.c -o $@

ip_proxy_u.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	$(TOP)/ip_ftp_pxy.c $(TOP)/ip_rcmd_pxy.c $(TOP)/ip_raudio_pxy.c \
	$(TOP)/ip_ipsec_pxy.c $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_proxy.c -o $@

ip_auth_u.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_auth.c -o $@

ip_log_u.o: $(TOP)/ip_log.c $(TOP)/ip_compat.h $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_log.c -o $@

ip_fil.o: $(TOP)/$(IPFILC) $(TOP)/ip_fil.h $(TOP)/ip_compat.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/$(IPFILC) -o $@

ip_log.o: $(TOP)/ip_log.c $(TOP)/ip_fil.h $(TOP)/ip_compat.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_log.c -o $@

ip_fil_u.o: $(TOP)/$(IPFILC) $(TOP)/ip_fil.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_fil.c -o $@

ip_input.o: $(TOP)/ip_input.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(IPDEF) -c $(TOP)/ip_input.c -o $@

ip_output.o: $(TOP)/ip_output.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(IPDEF) -c $(TOP)/ip_output.c -o $@

mls_ipl.o: $(TOP)/mls_ipl.c $(TOP)/ipl.h
	-/bin/rm -f $(TOP)/vnode_if.c
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/mls_ipl.c -o $@

kmem.o: $(TOP)/kmem.c
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/kmem.c -o $@

parse.o: $(TOP)/parse.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/parse.c -o $@

common.o: $(TOP)/common.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/common.c -o $@

facpri.o: $(TOP)/facpri.c $(TOP)/facpri.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/facpri.c -o $@

ipmon: $(TOP)/ipmon.c
	$(CC) $(DEBUG) $(CFLAGS) $(LOGFAC) $(TOP)/ipmon.c -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipmon
	ln -s `pwd`/ipmon $(TOP)

clean:
	${RM} -f core *.o ipt fils ipf ipfstat ipftest ipmon if_ipl ipnat \
		ipfs $(LKM) y.tab.? lex.yy.c
	${MAKE} -f Makefile.ipsend clean


install: all $(TOP)/ip_fil.h
	-$(CP) $(TOP)/ip_fil.h /usr/include/netinet/ip_fil.h
	-$(CHMOD) 444 /usr/include/netinet/ip_fil.h
	-$(INSTALL) -cs -g wheel -m 755 -o root ipf $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipfs $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipnat $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipftest $(BINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipfstat $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipmon $(BINDEST)
	(cd $(TOP)/man; make INSTALL=$(INSTALL) MANDIR=$(MANDIR) install; cd ..)
