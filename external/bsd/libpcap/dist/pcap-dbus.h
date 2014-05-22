/*	$NetBSD: pcap-dbus.h,v 1.1.1.1.4.2 2014/05/22 15:48:20 yamt Exp $	*/

pcap_t *dbus_create(const char *, char *, int *);
int dbus_findalldevs(pcap_if_t **devlistp, char *errbuf);
