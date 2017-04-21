/*	$NetBSD: pcap-dag.h,v 1.2.4.1 2017/04/21 16:51:34 bouyer Exp $	*/

/*
 * pcap-dag.c: Packet capture interface for Endace DAG card.
 *
 * The functionality of this code attempts to mimic that of pcap-linux as much
 * as possible.  This code is only needed when compiling in the DAG card code
 * at the same time as another type of device.
 *
 * Author: Richard Littin, Sean Irvine ({richard,sean}@reeltwo.com)
 */

pcap_t *dag_create(const char *, char *, int *);
int dag_findalldevs(pcap_if_t **devlistp, char *errbuf);
