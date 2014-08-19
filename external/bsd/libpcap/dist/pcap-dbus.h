/*	$NetBSD: pcap-dbus.h,v 1.1.1.1.8.2 2014/08/19 23:47:16 tls Exp $	*/

pcap_t *dbus_create(const char *, char *, int *);
int dbus_findalldevs(pcap_if_t **devlistp, char *errbuf);
