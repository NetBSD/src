/*	$NetBSD: pcap-dbus.h,v 1.2.16.1 2019/06/10 21:44:59 christos Exp $	*/

pcap_t *dbus_create(const char *, char *, int *);
int dbus_findalldevs(pcap_if_list_t *devlistp, char *errbuf);
