/*	$NetBSD: pcap-snf.h,v 1.1.1.1.12.1 2013/06/23 06:28:19 tls Exp $	*/

pcap_t *snf_create(const char *, char *);
int snf_platform_finddevs(pcap_if_t **devlistp, char *errbuf);
