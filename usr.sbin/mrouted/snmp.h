/*	$NetBSD: snmp.h,v 1.4 2002/08/01 03:40:34 itojun Exp $	*/

extern int portlist[32], sdlen;
extern in_port_t dest_port;
extern int quantum;

extern int snmp_read_packet();

#define DEFAULT_PORT 161
