/*	$NetBSD: pcap-dbus.h,v 1.2.14.1 2018/09/06 06:51:45 pgoyette Exp $	*/

pcap_t *dbus_create(const char *, char *, int *);
int dbus_findalldevs(pcap_if_list_t *devlistp, char *errbuf);
