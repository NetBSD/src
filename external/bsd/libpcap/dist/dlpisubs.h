/*	$NetBSD: dlpisubs.h,v 1.1.1.2.6.1 2014/05/22 15:48:19 yamt Exp $	*/

/*
 * @(#) Header: /tcpdump/master/libpcap/dlpisubs.h,v 1.2 2008-04-04 19:37:45 guy Exp 
 */

#ifndef dlpisubs_h
#define	dlpisubs_h

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private data for capturing on DLPI devices.
 */
struct pcap_dlpi {
#ifdef HAVE_LIBDLPI
	dlpi_handle_t dlpi_hd;
#endif /* HAVE_LIBDLPI */
#ifdef DL_HP_RAWDLS
	int send_fd;
#endif /* DL_HP_RAWDLS */

	struct pcap_stat stat;
};

/*
 * Functions defined by dlpisubs.c.
 */
int pcap_stats_dlpi(pcap_t *, struct pcap_stat *);
int pcap_process_pkts(pcap_t *, pcap_handler, u_char *, int, u_char *, int);
int pcap_process_mactype(pcap_t *, u_int);
#ifdef HAVE_SYS_BUFMOD_H
int pcap_conf_bufmod(pcap_t *, int);
#endif
int pcap_alloc_databuf(pcap_t *);
int strioctl(int, int, int, char *);

#ifdef __cplusplus
}
#endif

#endif
