#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:48 kardel Exp $

NOGCCERROR=yes

.include <bsd.own.mk>


PROG=	ntpd
MAN=
#MAN=	named.8 lwresd.8 named.conf.5
BINDIR=	/usr/sbin

SRCS=	cmd_args.c				\
	ntp_config.c				\
	ntp_control.c				\
	ntp_crypto.c				\
	ntp_data_structures.c			\
	ntp_filegen.c				\
	ntp_intres.c				\
	ntp_io.c				\
	ntp_loopfilter.c			\
	ntp_monitor.c				\
	ntp_parser.c				\
	ntp_peer.c				\
	ntp_proto.c				\
	ntp_refclock.c				\
	ntp_request.c				\
	ntp_restrict.c				\
	ntp_scanner.c				\
	ntp_signd.c				\
	ntp_timer.c				\
	ntp_util.c				\
	ntpd-opts.c				\
	ntpd.c					\
	ntpsim.c				\
	refclock_acts.c				\
	refclock_arbiter.c			\
	refclock_arc.c				\
	refclock_as2201.c			\
	refclock_atom.c				\
	refclock_bancomm.c			\
	refclock_chronolog.c			\
	refclock_chu.c				\
	refclock_conf.c				\
	refclock_datum.c			\
	refclock_dumbclock.c			\
	refclock_fg.c				\
	refclock_gpsvme.c			\
	refclock_heath.c			\
	refclock_hopfpci.c			\
	refclock_hopfser.c			\
	refclock_hpgps.c			\
	refclock_irig.c				\
	refclock_jjy.c				\
	refclock_jupiter.c			\
	refclock_leitch.c			\
	refclock_local.c			\
	refclock_msfees.c			\
	refclock_mx4200.c			\
	refclock_neoclock4x.c			\
	refclock_nmea.c				\
	refclock_oncore.c			\
	refclock_palisade.c			\
	refclock_parse.c			\
	refclock_pcf.c				\
	refclock_pst.c				\
	refclock_ripencc.c			\
	refclock_shm.c				\
	refclock_tpro.c				\
	refclock_trak.c				\
	refclock_true.c				\
	refclock_tt560.c			\
	refclock_ulink.c			\
	refclock_wwv.c				\
	refclock_wwvb.c				\
	refclock_zyfer.c

LDADD+=		-L${LIBPARSE} -lparse -L${LIBISC} -liscntp -lm -lutil -ldns_sd
DPADD+=		${LIBPARSE}/libparse.a ${LIBISC}/libiscntp.a ${LIBOPTS}/libopts.a  ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpd

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
