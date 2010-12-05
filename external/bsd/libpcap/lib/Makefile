# $NetBSD: Makefile,v 1.1 2010/12/05 01:45:46 christos Exp $	

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

.SUFFIXES: .3 .3pcap

.3pcap.3:
	rm -f ${.TARGET}
	cp ${.IMPSRC} ${.TARGET}


SRCDIR=		${.CURDIR}/../dist/
.PATH:		${SRCDIR}

LIB=		pcap
MAN=		pcap.3 pcap_activate.3 pcap_breakloop.3 pcap_can_set_rfmon.3 \
		pcap_close.3 pcap_compile.3 pcap_create.3 pcap_datalink.3 \
		pcap_datalink_name_to_val.3 pcap_datalink_val_to_name.3 \
		pcap_dump.3 pcap_dump_close.3 pcap_dump_file.3 \
		pcap_dump_flush.3 pcap_dump_ftell.3 pcap_dump_open.3 \
		pcap_file.3 pcap_fileno.3 pcap_findalldevs.3 \
		pcap_free_datalinks.3 pcap_freealldevs.3 pcap_freecode.3 \
		pcap_get_selectable_fd.3 pcap_geterr.3 pcap_inject.3  \
		pcap_is_swapped.3 pcap_lib_version.3 pcap_list_datalinks.3 \
		pcap_lookupdev.3 pcap_lookupnet.3 pcap_loop.3 \
		pcap_major_version.3 pcap_next_ex.3 pcap_offline_filter.3 \
		pcap_open_dead.3 pcap_open_live.3 pcap_open_offline.3 \
		pcap_set_buffer_size.3 pcap_set_datalink.3 pcap_set_promisc.3 \
		pcap_set_rfmon.3 pcap_set_snaplen.3 pcap_set_timeout.3 \
		pcap_setdirection.3 pcap_setfilter.3 pcap_setnonblock.3 \
		pcap_snapshot.3 pcap_stats.3 pcap_statustostr.3 \
		pcap_strerror.3 pcap-savefile.5 pcap-filter.7 pcap-linktype.7


MAN3IN= pcap.3pcap.in pcap_compile.3pcap.in pcap_datalink.3pcap.in \
	pcap_dump_open.3pcap.in pcap_list_datalinks.3pcap.in \
	pcap_open_dead.3pcap.in pcap_open_offline.3pcap.in
MANMISCIN= pcap-filter.manmisc.in pcap-linktype.manmisc.in 
MANFILEIN= pcap-savefile.manfile.in 
CLEANFILES+= ${MAN}

WARNS?=		5

__sed: .USE
	@rm -f ${.TARGET}
	${TOOL_SED} -e 's,@MAN_FILE_FORMATS@,5,g' \
	    -e 's,@MAN_MISC_INFO@,7,g' < ${.ALLSRC} > ${.TARGET}

.for i in ${MAN3IN}
${i:S/pcap.in//}: ${i} __sed
.endfor

.for i in ${MANMISCIN}
${i:S/manmisc.in/7/}: ${i} __sed
.endfor

.for i in ${MANFILEIN}
${i:S/manfile.in/5/}: ${i} __sed
.endfor

CPPFLAGS+=	-I${.CURDIR}/../include -I${.CURDIR} -I${SRCDIR}
CPPFLAGS+=	-DYYBISON
LPREFIX=	pcap_
YPREFIX=	pcap_
YHEADER=	1

CPPFLAGS+=	-DHAVE_CONFIG_H

.if defined(HAVE_PCC)
CPPFLAGS+=	-D_U_=""
.elif defined(HAVE_GCC)
CPPFLAGS+=	-D_U_="__attribute__((__unused__))"
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
	inet.c grammar.y gencode.c fad-getad.c etherent.c bpf_image.c \
	bpf_dump.c version.c

.PATH:	${NETBSDSRCDIR}/sys/net
SRCS+=	bpf_filter.c

INCS=		pcap-namedb.h pcap.h
INCSDIR=	/usr/include

scanner.d scanner.o: tokdefs.h

tokdefs.h: grammar.h
	cp $? $@

CLEANFILES+=	tokdefs.h

.include <bsd.lib.mk>
