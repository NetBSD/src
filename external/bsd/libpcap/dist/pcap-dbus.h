/*	$NetBSD: pcap-dbus.h,v 1.1.1.1 2013/12/31 16:57:20 christos Exp $	*/

pcap_t *dbus_create(const char *, char *, int *);
int dbus_findalldevs(pcap_if_t **devlistp, char *errbuf);
