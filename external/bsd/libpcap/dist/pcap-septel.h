/*
 * pcap-septel.c: Packet capture interface for Intel Septel card
 *
 * The functionality of this code attempts to mimic that of pcap-linux as much
 * as possible.  This code is only needed when compiling in the Intel/Septel
 * card code at the same time as another type of device.
 *
 * Authors: Gilbert HOYEK (gil_hoyek@hotmail.com), Elias M. KHOURY
 * (+961 3 485343);
 *
 * @(#) $Header: /cvsroot/src/external/bsd/libpcap/dist/pcap-septel.h,v 1.1.1.1 2010/12/05 01:20:47 christos Exp $
 */

pcap_t *septel_create(const char *device, char *ebuf);

