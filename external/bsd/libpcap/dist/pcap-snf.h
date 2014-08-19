/*	$NetBSD: pcap-snf.h,v 1.1.1.1.12.2 2014/08/19 23:47:16 tls Exp $	*/

pcap_t *snf_create(const char *, char *, int *);
int snf_findalldevs(pcap_if_t **devlistp, char *errbuf);
