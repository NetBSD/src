/*	$NetBSD: pcap-snf.h,v 1.1.1.1.6.1 2014/05/22 15:48:20 yamt Exp $	*/

pcap_t *snf_create(const char *, char *, int *);
int snf_findalldevs(pcap_if_t **devlistp, char *errbuf);
