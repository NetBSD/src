/*	$NetBSD: main.c,v 1.1.1.1 2009/12/13 16:57:10 kardel Exp $	*/

#include <l_stdlib.h>
#include <ntp_fp.h>
#include <ntp.h>
#include <ntp_stdlib.h>
#include <ntp_unixtime.h>
#include <isc/result.h>
#include <isc/net.h>
#include <stdio.h>

#include <sntp-opts.h>

#include "crypto.h"
#include "kod_management.h"
#include "networking.h"
#include "utilities.h"
#include "log.h"

char *progname = "sntp";	/* for msyslog */

int ai_fam_pref = AF_UNSPEC;
volatile int debug;

struct key *keys = NULL;

void set_li_vn_mode (struct pkt *spkt, char leap, char version, char mode); 
int sntp_main (int argc, char **argv);
int on_wire (struct addrinfo *host);
int set_time (double offset);


int 
main (
		int argc,
		char **argv
		) 
{
	return sntp_main(argc, argv);
}

/*
 * The actual main function.
 */
int  
sntp_main (
		int argc, 
		char **argv
		) 
{
	register int c;
	struct kod_entry *reason = NULL;
	int optct;
	int sync_data_suc = 0;
	struct addrinfo **resh = NULL;
	struct addrinfo *ai;
	int resc;
	int kodc;
	int ow_ret;
	char *hostname;

	/* IPv6 available? */
	if (isc_net_probeipv6() != ISC_R_SUCCESS) {
		ai_fam_pref = AF_INET;
#ifdef DEBUG
		printf("No ipv6 support available, forcing ipv4\n");
#endif
	}
	else {
		/* Check for options -4 and -6 */
		if (HAVE_OPT(IPV4))
			ai_fam_pref = AF_INET;
		else if (HAVE_OPT(IPV6))
			ai_fam_pref = AF_INET6;
	}

	log_msg("Started sntp", 0);

	optct = optionProcess(&sntpOptions, argc, argv);
	argc -= optct;
	argv += optct; 

	/* Parse config file if declared TODO */

	/* Initialize logging system */
	if (HAVE_OPT(FILELOG))
		init_log(OPT_ARG(FILELOG));

	/* 
	 * If there's a specified KOD file init KOD system.  If not use
	 * default file.  For embedded systems with no writable
	 * filesystem, -K /dev/null can be used to disable KoD storage.
	 */
	if (HAVE_OPT(KOD))
		kod_init_kod_db(OPT_ARG(KOD));
	else
		kod_init_kod_db("/var/db/ntp-kod");

	if (HAVE_OPT(KEYFILE))
		auth_init(OPT_ARG(KEYFILE), &keys);

#ifdef EXERCISE_KOD_DB
	add_entry("192.168.169.170", "DENY");
	add_entry("192.168.169.171", "DENY");
	add_entry("192.168.169.172", "DENY");
	add_entry("192.168.169.173", "DENY");
	add_entry("192.168.169.174", "DENY");
	delete_entry("192.168.169.174", "DENY");
	delete_entry("192.168.169.172", "DENY");
	delete_entry("192.168.169.170", "DENY");
	if ((kodc = search_entry("192.168.169.173", &reason)) == 0)
		printf("entry for 192.168.169.173 not found but should have been!\n");
	else
		free(reason);
#endif

	/* Considering employing a variable that prevents functions of doing anything until 
	 * everything is initialized properly 
	 */
	resc = resolve_hosts(argv, argc, &resh, ai_fam_pref);

	if (resc < 1) {
		printf("Unable to resolve hostname(s)\n");
		return -1;
	}

	/* Select a certain ntp server according to simple criteria? For now
	 * let's just pay attention to previous KoDs.
	 */
	for (c = 0; c < resc && !sync_data_suc; c++) {
		ai = resh[c];
		do {
			hostname = addrinfo_to_str(ai);

			if ((kodc = search_entry(hostname, &reason)) == 0) {
				if (is_reachable(ai)) {
					ow_ret = on_wire(ai);
					if (ow_ret < 0)
						printf("on_wire failed for server %s!\n", hostname);
					else
						sync_data_suc = 1;
				}
			} else {
				printf("%d prior KoD%s for %s, skipping.\n", 
					kodc, (kodc > 1) ? "s" : "", hostname);
				free(reason);
			}
			free(hostname);
			ai = ai->ai_next;
		} while (NULL != ai);
		freeaddrinfo(resh[c]);
	}
	free(resh);

	return 0;
}

/* The heart of (S)NTP, exchange NTP packets and compute values to correct the local clock */
int
on_wire (
		struct addrinfo *host
					)
{
	char logmsg[32 + INET6_ADDRSTRLEN];
	char addr_buf[INET6_ADDRSTRLEN];
	register int try;
	SOCKET sock;
	struct pkt x_pkt;
	struct pkt r_pkt;
	char *ref;

	for(try=0; try<5; try++) {
		struct timeval tv_xmt, tv_dst;
		double t21, t34, delta, offset, precision, root_dispersion;
		int digits, error, rpktl, sw_case;
		char *hostname = NULL, *ts_str = NULL;
		char *log_str;
		u_fp p_rdly, p_rdsp;
		l_fp p_rec, p_xmt, p_ref, p_org, xmt, tmp, dst;

		memset(&r_pkt, 0, sizeof(r_pkt));
		memset(&x_pkt, 0, sizeof(x_pkt));

		error = GETTIMEOFDAY(&tv_xmt, (struct timezone *)NULL);

		tv_xmt.tv_sec += JAN_1970;

#ifdef DEBUG
		printf("sntp on_wire: Current time sec: %i msec: %i\n", (unsigned int) tv_xmt.tv_sec, 
				(unsigned int) tv_xmt.tv_usec);
#endif

		TVTOTS(&tv_xmt, &xmt);
		HTONL_FP(&xmt, &(x_pkt.xmt));

		x_pkt.stratum = STRATUM_TO_PKT(STRATUM_UNSPEC);
		x_pkt.ppoll = 8;
		/* FIXME! Modus broadcast + adr. check -> bdr. pkt */
		set_li_vn_mode(&x_pkt, LEAP_NOTINSYNC, 4, 3);

		create_socket(&sock, (sockaddr_u *)host->ai_addr);

		sendpkt(sock, (sockaddr_u *)host->ai_addr, &x_pkt, LEN_PKT_NOMAC);
		rpktl = recvpkt(sock, &r_pkt, &x_pkt);

		closesocket(sock);

		if(rpktl > 0)
			sw_case = 1;
		else
			sw_case = rpktl;

		switch(sw_case) {
			case SERVER_UNUSEABLE:
				return -1;
				break;

			case PACKET_UNUSEABLE:
				break;

			case SERVER_AUTH_FAIL:
				break;

			case KOD_DEMOBILIZE:
				/* Received a DENY or RESTR KOD packet */
				hostname = addrinfo_to_str(host);
				ref = (char *)&r_pkt.refid;
				add_entry(hostname, ref);

				if (ENABLED_OPT(NORMALVERBOSE))
					printf("sntp on_wire: Received KOD packet with code: %c%c%c%c from %s, demobilizing all connections\n", 
					       ref[0], ref[1], ref[2], ref[3],
					       hostname);

				log_str = emalloc(INET6_ADDRSTRLEN + 72);
				snprintf(log_str, INET6_ADDRSTRLEN + 72, 
					 "Received a KOD packet with code %c%c%c%c from %s, demobilizing all connections", 
					 ref[0], ref[1], ref[2], ref[3],
					 hostname);
				log_msg(log_str, 2);
				free(log_str);
				break;

			case KOD_RATE:
				/* Hmm... probably we should sleep a bit here */
				break;

			case 1:

			/* Convert timestamps from network to host byte order */
			p_rdly = NTOHS_FP(r_pkt.rootdelay);
			p_rdsp = NTOHS_FP(r_pkt.rootdisp);
			NTOHL_FP(&r_pkt.reftime, &p_ref);
			NTOHL_FP(&r_pkt.org, &p_org);
			NTOHL_FP(&r_pkt.rec, &p_rec);
			NTOHL_FP(&r_pkt.xmt, &p_xmt);

			if (ENABLED_OPT(NORMALVERBOSE)) {
				getnameinfo(host->ai_addr, host->ai_addrlen, addr_buf, 
						sizeof(addr_buf), NULL, 0, NI_NUMERICHOST);

				printf("sntp on_wire: Received %i bytes from %s\n", rpktl, addr_buf);
			}

			precision = LOGTOD(r_pkt.precision);
#ifdef DEBUG
			printf("sntp precision: %f\n", precision);
#endif /* DEBUG */
			for (digits = 0; (precision *= 10.) < 1.; ++digits)
				/* empty */ ;
			if (digits > 6)
				digits = 6;

			root_dispersion = FPTOD(p_rdsp);

#ifdef DEBUG
			printf("sntp rootdelay: %f\n", FPTOD(p_rdly));
			printf("sntp rootdisp: %f\n", root_dispersion);

			pkt_output(&r_pkt, rpktl, stdout);

			printf("sntp on_wire: r_pkt.reftime:\n");
			l_fp_output(&(r_pkt.reftime), stdout);
			printf("sntp on_wire: r_pkt.org:\n");
			l_fp_output(&(r_pkt.org), stdout);
			printf("sntp on_wire: r_pkt.rec:\n");
			l_fp_output(&(r_pkt.rec), stdout);
			printf("sntp on_wire: r_pkt.rec:\n");
			l_fp_output_bin(&(r_pkt.rec), stdout);
			printf("sntp on_wire: r_pkt.rec:\n");
			l_fp_output_dec(&(r_pkt.rec), stdout);
			printf("sntp on_wire: r_pkt.xmt:\n");
			l_fp_output(&(r_pkt.xmt), stdout);
#endif

			/* Compute offset etc. */
			GETTIMEOFDAY(&tv_dst, (struct timezone *)NULL);

			tv_dst.tv_sec += JAN_1970;

			tmp = p_rec;
			L_SUB(&tmp, &p_org);

			LFPTOD(&tmp, t21);

			TVTOTS(&tv_dst, &dst);

			tmp = dst;
			L_SUB(&tmp, &p_xmt);

			LFPTOD(&tmp, t34);

			offset = (t21 + t34) / 2.;
			delta = t21 - t34;

			if(ENABLED_OPT(NORMALVERBOSE))
				printf("sntp on_wire:\tt21: %.6f\t\t t34: %.6f\n\t\tdelta: %.6f\t offset: %.6f\n", 
					t21, t34, delta, offset);

			ts_str = tv_to_str(&tv_dst);

			printf("%s ", ts_str);

			if(offset > 0)
				printf("+");

			printf("%.*f", digits, offset);

			if (root_dispersion > 0.)
				printf(" +/- %f secs", root_dispersion);

			printf("\n");

			free(ts_str);

			if(ENABLED_OPT(SETTOD) || ENABLED_OPT(ADJTIME))
				return set_time(offset); 

			return 0;
		}
	}

	getnameinfo(host->ai_addr, host->ai_addrlen, addr_buf, sizeof(addr_buf), NULL, 0, NI_NUMERICHOST);

	snprintf(logmsg, sizeof(logmsg), "Received no useable packet from %s!", addr_buf);
	log_msg(logmsg, 1);

	if (ENABLED_OPT(NORMALVERBOSE))
		printf("sntp on_wire: Received no useable packet from %s!\n", addr_buf);

	return -1;
}

/* Compute the 8 bits for li_vn_mode */
void
set_li_vn_mode (
		struct pkt *spkt,
		char leap,
		char version,
		char mode
	       ) 
{

	if(leap > 3) {
		debug_msg("set_li_vn_mode: leap > 3 using max. 3");
		leap = 3;
	}

	if(mode > 7) {
		debug_msg("set_li_vn_mode: mode > 7, using client mode 3");
		mode = 3;
	}

	spkt->li_vn_mode  = leap << 6;
	spkt->li_vn_mode |= version << 3;
	spkt->li_vn_mode |= mode;
}

/* set_time corrects the local clock by offset with either settimeofday() or by default 
 * with adjtime()/adjusttimeofday().
 */
int
set_time (
		double offset
	 )
{
	struct timeval tp;

	if(ENABLED_OPT(SETTOD)) {
		GETTIMEOFDAY(&tp, (struct timezone *)NULL);

		tp.tv_sec += (int) offset;
		tp.tv_usec += offset - (double)((int)offset);

		if(SETTIMEOFDAY(&tp, (struct timezone *)NULL) < 0) {
			printf("set_time: settimeofday(): Time not set: %s\n",
				strerror(errno));
			return -1;
		}
		else {
			return 0;
		}
	}
	else {
		tp.tv_sec = (int) offset;
		tp.tv_usec = offset - (double)((int)offset);

		if(ADJTIMEOFDAY(&tp, NULL) < 0) {
			printf("set_time: adjtime(): Time not set: %s\n",
				strerror(errno));
			return -1;
		}
		else {
			return 0;
		}
	}
}
