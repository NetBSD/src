/*	$NetBSD: pcap-snf.h,v 1.1.1.2 2013/04/06 15:57:45 christos Exp $	*/

pcap_t *snf_create(const char *, char *);
int snf_platform_finddevs(pcap_if_t **devlistp, char *errbuf);
