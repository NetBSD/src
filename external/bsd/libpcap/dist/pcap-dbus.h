/*	$NetBSD: pcap-dbus.h,v 1.3 2018/09/03 15:26:43 christos Exp $	*/

pcap_t *dbus_create(const char *, char *, int *);
int dbus_findalldevs(pcap_if_list_t *devlistp, char *errbuf);
