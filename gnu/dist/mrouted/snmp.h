/*	$NetBSD: snmp.h,v 1.1.4.2 2002/12/07 00:35:24 he Exp $	*/

extern int portlist[32], sdlen;
extern in_port_t dest_port;
extern int quantum;

extern int snmp_read_packet();

#define DEFAULT_PORT 161
