/*	$NetBSD: pcap-snf.h,v 1.1.1.3 2013/12/31 16:57:20 christos Exp $	*/

pcap_t *snf_create(const char *, char *, int *);
int snf_findalldevs(pcap_if_t **devlistp, char *errbuf);
