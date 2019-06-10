/*	$NetBSD: pcap-snf.h,v 1.2.16.1 2019/06/10 21:44:59 christos Exp $	*/

pcap_t *snf_create(const char *, char *, int *);
int snf_findalldevs(pcap_if_list_t *devlistp, char *errbuf);
