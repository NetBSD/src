/*	$NetBSD: snmp.h,v 1.1.2.2 2002/10/01 04:14:54 itojun Exp $	*/

extern int portlist[32], sdlen;
extern u_short dest_port;
extern int quantum;

extern int snmp_read_packet();

#define DEFAULT_PORT 161
