/*	$NetBSD: snmp.h,v 1.6 2003/03/05 21:05:40 wiz Exp $	*/

extern int portlist[32], sdlen;
extern in_port_t dest_port;
extern int quantum;

extern int snmp_read_packet();

#define DEFAULT_PORT 161
