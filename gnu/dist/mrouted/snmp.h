/*	$NetBSD: snmp.h,v 1.1 2002/10/01 03:31:11 itojun Exp $	*/

extern int portlist[32], sdlen;
extern in_port_t dest_port;
extern int quantum;

extern int snmp_read_packet();

#define DEFAULT_PORT 161
