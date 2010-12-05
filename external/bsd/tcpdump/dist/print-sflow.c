/*
 * Copyright (c) 1998-2007 The TCPDUMP project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code
 * distributions retain the above copyright notice and this paragraph
 * in its entirety, and (2) distributions including binary code include
 * the above copyright notice and this paragraph in its entirety in
 * the documentation or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * The SFLOW protocol as per http://www.sflow.org/developers/specifications.php
 *
 * Original code by Carles Kishimoto <carles.kishimoto@gmail.com>
 */

#ifndef lint
static const char rcsid[] _U_ =
"@(#) Header: /tcpdump/master/tcpdump/print-sflow.c,v 1.1 2007-08-08 17:20:58 hannes Exp";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tcpdump-stdinc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "extract.h"
#include "addrtoname.h"

/* 
 * sFlow datagram
 * 
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     Sflow version (2,4,5)                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |               IP version (1 for IPv4 | 2 for IPv6)            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     IP Address AGENT (4 or 16 bytes)          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          Sub agent ID                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      Datagram sequence number                 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      Switch uptime in ms                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    num samples in datagram                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 */

struct sflow_datagram_t {
    u_int8_t 	version[4];
    u_int8_t 	ip_version[4];
    u_int8_t 	agent[4];
    u_int8_t 	agent_id[4];
    u_int8_t 	seqnum[4];
    u_int8_t 	uptime[4];
    u_int8_t 	samples[4];
};

struct sflow_sample_header {
    u_int8_t	format[4];
    u_int8_t	len[4];
};

#define		SFLOW_FLOW_SAMPLE		1
#define		SFLOW_COUNTER_SAMPLE		2
#define		SFLOW_EXPANDED_FLOW_SAMPLE	3
#define		SFLOW_EXPANDED_COUNTER_SAMPLE	4

static const struct tok sflow_format_values[] = {
    { SFLOW_FLOW_SAMPLE, "flow sample" },
    { SFLOW_COUNTER_SAMPLE, "counter sample" },
    { SFLOW_EXPANDED_FLOW_SAMPLE, "expanded flow sample" },
    { SFLOW_EXPANDED_COUNTER_SAMPLE, "expanded counter sample" },
    { 0, NULL}
};

struct sflow_expanded_flow_sample_t {
    u_int8_t    seqnum[4];
    u_int8_t    type[4];
    u_int8_t    index[4];
    u_int8_t    rate[4];
    u_int8_t    pool[4];
    u_int8_t    drops[4];
    u_int8_t    in_interface_format[4];
    u_int8_t    in_interface_value[4];
    u_int8_t    out_interface_format[4];
    u_int8_t    out_interface_value[4];
    u_int8_t    records[4];
};

#define 	SFLOW_FLOW_RAW_PACKET			1
#define 	SFLOW_FLOW_ETHERNET_FRAME		2
#define 	SFLOW_FLOW_IPV4_DATA			3
#define 	SFLOW_FLOW_IPV6_DATA			4
#define 	SFLOW_FLOW_EXTENDED_SWITCH_DATA		1001
#define 	SFLOW_FLOW_EXTENDED_ROUTER_DATA		1002
#define 	SFLOW_FLOW_EXTENDED_GATEWAY_DATA 	1003
#define 	SFLOW_FLOW_EXTENDED_USER_DATA		1004
#define 	SFLOW_FLOW_EXTENDED_URL_DATA		1005
#define 	SFLOW_FLOW_EXTENDED_MPLS_DATA		1006
#define 	SFLOW_FLOW_EXTENDED_NAT_DATA		1007
#define 	SFLOW_FLOW_EXTENDED_MPLS_TUNNEL		1008
#define 	SFLOW_FLOW_EXTENDED_MPLS_VC		1009
#define 	SFLOW_FLOW_EXTENDED_MPLS_FEC		1010
#define 	SFLOW_FLOW_EXTENDED_MPLS_LVP_FEC	1011
#define 	SFLOW_FLOW_EXTENDED_VLAN_TUNNEL		1012

static const struct tok sflow_flow_type_values[] = {
    { SFLOW_FLOW_RAW_PACKET, "Raw packet"},
    { SFLOW_FLOW_ETHERNET_FRAME, "Ethernet frame"},
    { SFLOW_FLOW_IPV4_DATA, "IPv4 Data"},
    { SFLOW_FLOW_IPV6_DATA, "IPv6 Data"},
    { SFLOW_FLOW_EXTENDED_SWITCH_DATA, "Extended Switch data"},
    { SFLOW_FLOW_EXTENDED_ROUTER_DATA, "Extended Router data"},
    { SFLOW_FLOW_EXTENDED_GATEWAY_DATA, "Extended Gateway data"},
    { SFLOW_FLOW_EXTENDED_USER_DATA, "Extended User data"},
    { SFLOW_FLOW_EXTENDED_URL_DATA, "Extended URL data"},
    { SFLOW_FLOW_EXTENDED_MPLS_DATA, "Extended MPLS data"},
    { SFLOW_FLOW_EXTENDED_NAT_DATA, "Extended NAT data"},
    { SFLOW_FLOW_EXTENDED_MPLS_TUNNEL, "Extended MPLS tunnel"},
    { SFLOW_FLOW_EXTENDED_MPLS_VC, "Extended MPLS VC"},
    { SFLOW_FLOW_EXTENDED_MPLS_FEC, "Extended MPLS FEC"},
    { SFLOW_FLOW_EXTENDED_MPLS_LVP_FEC, "Extended MPLS LVP FEC"},
    { SFLOW_FLOW_EXTENDED_VLAN_TUNNEL, "Extended VLAN Tunnel"},
    { 0, NULL}
};

#define		SFLOW_HEADER_PROTOCOL_ETHERNET	1
#define		SFLOW_HEADER_PROTOCOL_IPV4	11
#define		SFLOW_HEADER_PROTOCOL_IPV6	12

static const struct tok sflow_flow_raw_protocol_values[] = {
    { SFLOW_HEADER_PROTOCOL_ETHERNET, "Ethernet"},
    { SFLOW_HEADER_PROTOCOL_IPV4, "IPv4"},
    { SFLOW_HEADER_PROTOCOL_IPV6, "IPv6"},
    { 0, NULL}
};
	
struct sflow_expanded_flow_raw_t {
    u_int8_t    protocol[4];
    u_int8_t    length[4];
    u_int8_t    stripped_bytes[4];
    u_int8_t    header_size[4];
};

struct sflow_expanded_counter_sample_t {
    u_int8_t    seqnum[4];
    u_int8_t    type[4];
    u_int8_t    index[4];
    u_int8_t    records[4];
};

#define         SFLOW_COUNTER_GENERIC           1
#define         SFLOW_COUNTER_ETHERNET          2
#define         SFLOW_COUNTER_TOKEN_RING        3
#define         SFLOW_COUNTER_BASEVG            4
#define         SFLOW_COUNTER_VLAN              5
#define         SFLOW_COUNTER_PROCESSOR         1001

static const struct tok sflow_counter_type_values[] = {
    { SFLOW_COUNTER_GENERIC, "Generic counter"},
    { SFLOW_COUNTER_ETHERNET, "Ethernet counter"},
    { SFLOW_COUNTER_TOKEN_RING, "Token ring counter"},
    { SFLOW_COUNTER_BASEVG, "100 BaseVG counter"},
    { SFLOW_COUNTER_VLAN, "Vlan counter"},
    { SFLOW_COUNTER_PROCESSOR, "Processor counter"},
    { 0, NULL}
};

#define		SFLOW_IFACE_DIRECTION_UNKNOWN		0
#define		SFLOW_IFACE_DIRECTION_FULLDUPLEX	1
#define		SFLOW_IFACE_DIRECTION_HALFDUPLEX	2
#define		SFLOW_IFACE_DIRECTION_IN		3
#define		SFLOW_IFACE_DIRECTION_OUT		4

static const struct tok sflow_iface_direction_values[] = {
    { SFLOW_IFACE_DIRECTION_UNKNOWN, "unknown"},
    { SFLOW_IFACE_DIRECTION_FULLDUPLEX, "full-duplex"},
    { SFLOW_IFACE_DIRECTION_HALFDUPLEX, "half-duplex"},
    { SFLOW_IFACE_DIRECTION_IN, "in"},
    { SFLOW_IFACE_DIRECTION_OUT, "out"},
    { 0, NULL}
};  

struct sflow_generic_counter_t {
    u_int8_t    ifindex[4];
    u_int8_t    iftype[4];
    u_int8_t    ifspeed[8];
    u_int8_t    ifdirection[4];	
    u_int8_t    ifstatus[4];
    u_int8_t    ifinoctets[8];	
    u_int8_t    ifinunicastpkts[4];	
    u_int8_t    ifinmulticastpkts[4];	
    u_int8_t    ifinbroadcastpkts[4];	
    u_int8_t    ifindiscards[4];	
    u_int8_t    ifinerrors[4];	
    u_int8_t    ifinunkownprotos[4];	
    u_int8_t    ifoutoctets[8];
    u_int8_t    ifoutunicastpkts[4];     
    u_int8_t    ifoutmulticastpkts[4];   
    u_int8_t    ifoutbroadcastpkts[4];         
    u_int8_t    ifoutdiscards[4];             
    u_int8_t    ifouterrors[4];        
    u_int8_t    ifpromiscmode[4];        
};

struct sflow_ethernet_counter_t {
    u_int8_t    alignerrors[4];
    u_int8_t    fcserrors[4];
    u_int8_t    single_collision_frames[4];
    u_int8_t    multiple_collision_frames[4];
    u_int8_t    test_errors[4];
    u_int8_t    deferred_transmissions[4];
    u_int8_t    late_collisions[4];
    u_int8_t    excessive_collisions[4];
    u_int8_t    mac_transmit_errors[4];
    u_int8_t    carrier_sense_errors[4];
    u_int8_t    frame_too_longs[4];
    u_int8_t    mac_receive_errors[4];
    u_int8_t    symbol_errors[4];
};

struct sflow_100basevg_counter_t {
    u_int8_t    in_highpriority_frames[4];
    u_int8_t    in_highpriority_octets[8];
    u_int8_t    in_normpriority_frames[4];
    u_int8_t    in_normpriority_octets[8];
    u_int8_t    in_ipmerrors[4]; 
    u_int8_t    in_oversized[4]; 
    u_int8_t    in_data_errors[4];
    u_int8_t    in_null_addressed_frames[4];
    u_int8_t    out_highpriority_frames[4];
    u_int8_t    out_highpriority_octets[8];
    u_int8_t    transitioninto_frames[4];
    u_int8_t    hc_in_highpriority_octets[8];
    u_int8_t    hc_in_normpriority_octets[8];
    u_int8_t    hc_out_highpriority_octets[8];
};

struct sflow_vlan_counter_t {
    u_int8_t    vlan_id[4];
    u_int8_t    octets[8];
    u_int8_t    unicast_pkt[4];
    u_int8_t    multicast_pkt[4];
    u_int8_t    broadcast_pkt[4];
    u_int8_t    discards[4];
};

void
sflow_print(const u_char *pptr, u_int len) {

    const struct sflow_datagram_t *sflow_datagram;
    const struct sflow_sample_header *sflow_sample;
    const struct sflow_expanded_flow_sample_t *sflow_expanded_flow_sample;
    const struct sflow_expanded_flow_raw_t *sflow_flow_raw;
    const struct sflow_expanded_counter_sample_t *sflow_expanded_counter_sample;
    const struct sflow_generic_counter_t *sflow_gen_counter;
    const struct sflow_ethernet_counter_t *sflow_eth_counter;
    const struct sflow_100basevg_counter_t *sflow_100basevg_counter;
    const struct sflow_vlan_counter_t *sflow_vlan_counter;
    const u_char *tptr;
    int tlen;
    u_int32_t sflow_sample_type, sflow_sample_len;
    int nsamples, nrecords, counter_len, counter_type, flow_len, flow_type;

    tptr=pptr;
    tlen = len;
    sflow_datagram = (const struct sflow_datagram_t *)pptr;
    TCHECK(*sflow_datagram);

    /*
     * Sanity checking of the header.
     */
    if (EXTRACT_32BITS(sflow_datagram->version) != 5) {
        printf("sFlow version %u packet not supported",
               EXTRACT_32BITS(sflow_datagram->version));
        return;
    }

    if (vflag < 1) {
        printf("sFlowv%u, %s agent %s, agent-id %u, length %u",
               EXTRACT_32BITS(sflow_datagram->version),
               EXTRACT_32BITS(sflow_datagram->ip_version) == 1 ? "IPv4" : "IPv6",
	       ipaddr_string(sflow_datagram->agent),
               EXTRACT_32BITS(sflow_datagram->samples),
               len);
        return;
    }

    /* ok they seem to want to know everything - lets fully decode it */
    nsamples=EXTRACT_32BITS(sflow_datagram->samples);
    printf("sFlowv%u, %s agent %s, agent-id %u, seqnum %u, uptime %u, samples %u, length %u",
           EXTRACT_32BITS(sflow_datagram->version),
           EXTRACT_32BITS(sflow_datagram->ip_version) == 1 ? "IPv4" : "IPv6",
           ipaddr_string(sflow_datagram->agent),
           EXTRACT_32BITS(sflow_datagram->agent_id),
           EXTRACT_32BITS(sflow_datagram->seqnum),
           EXTRACT_32BITS(sflow_datagram->uptime),
           nsamples,
           len);

    /* skip Common header */
    tptr+=sizeof(const struct sflow_datagram_t);
    tlen-=sizeof(const struct sflow_datagram_t);

    while (nsamples > 0 && tlen > 0) {
        sflow_sample = (const struct sflow_sample_header *)tptr;
        sflow_sample_type = (EXTRACT_32BITS(sflow_sample->format)&0x0FFF);
        sflow_sample_len = EXTRACT_32BITS(sflow_sample->len);

        tptr+=sizeof(struct sflow_sample_header);
        tlen-=sizeof(struct sflow_sample_header);
  
        printf("\n\t%s (%u), length %u,",
               tok2str(sflow_format_values, "Unknown", sflow_sample_type),
               sflow_sample_type,
               sflow_sample_len);

        /* basic sanity check */
        if (sflow_sample_type == 0 || sflow_sample_len ==0) {
            return;
        }

        /* did we capture enough for fully decoding the sample ? */
        if (!TTEST2(*tptr, sflow_sample_len)) 
            goto trunc;

	switch(sflow_sample_type) {
        case SFLOW_FLOW_SAMPLE:				/* XXX */
            break;

        case SFLOW_COUNTER_SAMPLE:			/* XXX */
            break;

        case SFLOW_EXPANDED_FLOW_SAMPLE:
            sflow_expanded_flow_sample = (const struct sflow_expanded_flow_sample_t *)tptr;
            nrecords = EXTRACT_32BITS(sflow_expanded_flow_sample->records);

            printf(" seqnum %u, type %u, idx %u, rate %u, pool %u, drops %u, records %u",
                   EXTRACT_32BITS(sflow_expanded_flow_sample->seqnum),
                   EXTRACT_32BITS(sflow_expanded_flow_sample->type),
                   EXTRACT_32BITS(sflow_expanded_flow_sample->index),
                   EXTRACT_32BITS(sflow_expanded_flow_sample->rate),
                   EXTRACT_32BITS(sflow_expanded_flow_sample->pool),
                   EXTRACT_32BITS(sflow_expanded_flow_sample->drops),
                   EXTRACT_32BITS(sflow_expanded_flow_sample->records));

            tptr+= sizeof(struct sflow_expanded_flow_sample_t);
            tlen-= sizeof(struct sflow_expanded_flow_sample_t);
	
            while ( nrecords > 0 && tlen > 0) {
				
                /* decode Flow record - 2 bytes */
                flow_type = EXTRACT_32BITS(tptr)&0x0FFF;
                flow_len = EXTRACT_32BITS(tptr+4);
                printf("\n\t  %s (%u) length %u",
                       tok2str(sflow_flow_type_values,"Unknown",flow_type),
                       flow_type,
                       flow_len);

                tptr += 8;
                tlen -= 8;

                /* did we capture enough for fully decoding the flow ? */
                if (!TTEST2(*tptr, flow_len))
                    goto trunc;

                switch(flow_type) {
                case SFLOW_FLOW_RAW_PACKET:
                    sflow_flow_raw = (const struct sflow_expanded_flow_raw_t *)tptr;
                    printf("\n\t    protocol %s (%u), length %u, stripped bytes %u, header_size %u",
                           tok2str(sflow_flow_raw_protocol_values,"Unknown",EXTRACT_32BITS(sflow_flow_raw->protocol)),
                           EXTRACT_32BITS(sflow_flow_raw->protocol),
                           EXTRACT_32BITS(sflow_flow_raw->length),
                           EXTRACT_32BITS(sflow_flow_raw->stripped_bytes),
                           EXTRACT_32BITS(sflow_flow_raw->header_size));
                    break;

                    /*
                     * FIXME those are the defined flow types that lack a decoder
                     */
                case SFLOW_FLOW_ETHERNET_FRAME:
                case SFLOW_FLOW_IPV4_DATA:
                case SFLOW_FLOW_IPV6_DATA:
                case SFLOW_FLOW_EXTENDED_SWITCH_DATA:
                case SFLOW_FLOW_EXTENDED_ROUTER_DATA:
                case SFLOW_FLOW_EXTENDED_GATEWAY_DATA:
                case SFLOW_FLOW_EXTENDED_USER_DATA:
                case SFLOW_FLOW_EXTENDED_URL_DATA:
                case SFLOW_FLOW_EXTENDED_MPLS_DATA:
                case SFLOW_FLOW_EXTENDED_NAT_DATA:
                case SFLOW_FLOW_EXTENDED_MPLS_TUNNEL:
                case SFLOW_FLOW_EXTENDED_MPLS_VC:
                case SFLOW_FLOW_EXTENDED_MPLS_FEC:
                case SFLOW_FLOW_EXTENDED_MPLS_LVP_FEC:
                case SFLOW_FLOW_EXTENDED_VLAN_TUNNEL:
                    break;
                default:
                    if (vflag <= 1)
                        print_unknown_data(tptr, "\n\t      ", flow_len);
                    break;

                }
                tptr += flow_len;
                tlen -= flow_len;
                nrecords--;
            }
            break;

        case SFLOW_EXPANDED_COUNTER_SAMPLE:
            sflow_expanded_counter_sample = (const struct sflow_expanded_counter_sample_t *)tptr;
            nrecords = EXTRACT_32BITS(sflow_expanded_counter_sample->records);

            printf(" seqnum %u, type %u, idx %u, records %u",
                   EXTRACT_32BITS(sflow_expanded_counter_sample->seqnum),
                   EXTRACT_32BITS(sflow_expanded_counter_sample->type),
                   EXTRACT_32BITS(sflow_expanded_counter_sample->index),
                   nrecords);

            tptr+= sizeof(struct sflow_expanded_counter_sample_t);
            tlen-= sizeof(struct sflow_expanded_counter_sample_t);

            while ( nrecords > 0 && tlen > 0) {

                /* decode counter record - 2 bytes */
                counter_type = EXTRACT_32BITS(tptr)&0x0FFF;			
                counter_len = EXTRACT_32BITS(tptr+4);
                printf("\n\t    %s (%u) length %u",
                       tok2str(sflow_counter_type_values,"Unknown",counter_type),
                       counter_type,
                       counter_len);

                tptr += 8;
                tlen -= 8;

                /* did we capture enough for fully decoding the counter ? */
                if (!TTEST2(*tptr, counter_len))
                    goto trunc;

                switch(counter_type) {
                case SFLOW_COUNTER_GENERIC:			
                    sflow_gen_counter = (const struct sflow_generic_counter_t *)tptr;
                    printf("\n\t      ifindex %u, iftype %u, ifspeed %u, ifdirection %u (%s)",
                           EXTRACT_32BITS(sflow_gen_counter->ifindex),
                           EXTRACT_32BITS(sflow_gen_counter->iftype),
                           EXTRACT_32BITS(sflow_gen_counter->ifspeed),
                           EXTRACT_32BITS(sflow_gen_counter->ifdirection),
                           tok2str(sflow_iface_direction_values, "Unknown",
                                   EXTRACT_32BITS(sflow_gen_counter->ifdirection)));
                    printf("\n\t      ifstatus %u, adminstatus: %s, operstatus: %s",
                           EXTRACT_32BITS(sflow_gen_counter->ifstatus),
                           EXTRACT_32BITS(sflow_gen_counter->ifstatus)&1 ? "up" : "down",
                           (EXTRACT_32BITS(sflow_gen_counter->ifstatus)>>1)&1 ? "up" : "down");
                    printf("\n\t      In octets %" PRIu64
                           ", unicast pkts %u, multicast pkts %u, broadcast pkts %u, discards %u",
                           EXTRACT_64BITS(sflow_gen_counter->ifinoctets),
                           EXTRACT_32BITS(sflow_gen_counter->ifinunicastpkts),
                           EXTRACT_32BITS(sflow_gen_counter->ifinmulticastpkts),
                           EXTRACT_32BITS(sflow_gen_counter->ifinbroadcastpkts),
                           EXTRACT_32BITS(sflow_gen_counter->ifindiscards));
                    printf("\n\t      In errors %u, unknown protos %u",
                           EXTRACT_32BITS(sflow_gen_counter->ifinerrors),
                           EXTRACT_32BITS(sflow_gen_counter->ifinunkownprotos));
                    printf("\n\t      Out octets %" PRIu64
                           ", unicast pkts %u, multicast pkts %u, broadcast pkts %u, discards %u",
                           EXTRACT_64BITS(sflow_gen_counter->ifoutoctets),
                           EXTRACT_32BITS(sflow_gen_counter->ifoutunicastpkts),
                           EXTRACT_32BITS(sflow_gen_counter->ifoutmulticastpkts),
                           EXTRACT_32BITS(sflow_gen_counter->ifoutbroadcastpkts),
                           EXTRACT_32BITS(sflow_gen_counter->ifoutdiscards));
                    printf("\n\t      Out errors %u, promisc mode %u", 
                           EXTRACT_32BITS(sflow_gen_counter->ifouterrors),
                           EXTRACT_32BITS(sflow_gen_counter->ifpromiscmode));
                    break;
                case SFLOW_COUNTER_ETHERNET:
                    sflow_eth_counter = (const struct sflow_ethernet_counter_t *)tptr;
                    printf("\n\t      align errors %u, fcs errors %u, single collision %u, multiple collision %u, test error %u",
                           EXTRACT_32BITS(sflow_eth_counter->alignerrors),
                           EXTRACT_32BITS(sflow_eth_counter->fcserrors),
                           EXTRACT_32BITS(sflow_eth_counter->single_collision_frames),
                           EXTRACT_32BITS(sflow_eth_counter->multiple_collision_frames),
                           EXTRACT_32BITS(sflow_eth_counter->test_errors));
                    printf("\n\t      deferred %u, late collision %u, excessive collision %u, mac trans error %u", 
                           EXTRACT_32BITS(sflow_eth_counter->deferred_transmissions),
                           EXTRACT_32BITS(sflow_eth_counter->late_collisions),
                           EXTRACT_32BITS(sflow_eth_counter->excessive_collisions),
                           EXTRACT_32BITS(sflow_eth_counter->mac_transmit_errors));
                    printf("\n\t      carrier error %u, frames too long %u, mac receive errors %u, symbol errors %u",	
                           EXTRACT_32BITS(sflow_eth_counter->carrier_sense_errors),
                           EXTRACT_32BITS(sflow_eth_counter->frame_too_longs),
                           EXTRACT_32BITS(sflow_eth_counter->mac_receive_errors),
                           EXTRACT_32BITS(sflow_eth_counter->symbol_errors));
                    break;
                case SFLOW_COUNTER_TOKEN_RING:			/* XXX */
                    break;
                case SFLOW_COUNTER_BASEVG:
                    sflow_100basevg_counter = (const struct sflow_100basevg_counter_t *)tptr;
                    printf("\n\t      in high prio frames %u, in high prio octets %" PRIu64,
                           EXTRACT_32BITS(sflow_100basevg_counter->in_highpriority_frames),
                           EXTRACT_64BITS(sflow_100basevg_counter->in_highpriority_octets));
                    printf("\n\t      in norm prio frames %u, in norm prio octets %" PRIu64,
                           EXTRACT_32BITS(sflow_100basevg_counter->in_normpriority_frames),
                           EXTRACT_64BITS(sflow_100basevg_counter->in_normpriority_octets));
                    printf("\n\t      in ipm errors %u, oversized %u, in data errors %u, null addressed frames %u",
                           EXTRACT_32BITS(sflow_100basevg_counter->in_ipmerrors),
                           EXTRACT_32BITS(sflow_100basevg_counter->in_oversized),
                           EXTRACT_32BITS(sflow_100basevg_counter->in_data_errors),
                           EXTRACT_32BITS(sflow_100basevg_counter->in_null_addressed_frames));
                    printf("\n\t      out high prio frames %u, out high prio octets %" PRIu64
                           ", trans into frames %u",
                           EXTRACT_32BITS(sflow_100basevg_counter->out_highpriority_frames),
                           EXTRACT_64BITS(sflow_100basevg_counter->out_highpriority_octets),
                           EXTRACT_32BITS(sflow_100basevg_counter->transitioninto_frames));
                    printf("\n\t      in hc high prio octets %" PRIu64
                           ", in hc norm prio octets %" PRIu64
                           ", out hc high prio octets %" PRIu64,
                           EXTRACT_64BITS(sflow_100basevg_counter->hc_in_highpriority_octets),
                           EXTRACT_64BITS(sflow_100basevg_counter->hc_in_normpriority_octets),
                           EXTRACT_64BITS(sflow_100basevg_counter->hc_out_highpriority_octets));
                    break;
                case SFLOW_COUNTER_VLAN:
                    sflow_vlan_counter = (const struct sflow_vlan_counter_t *)tptr;
                    printf("\n\t      vlan_id %u, octets %" PRIu64
                           ", unicast_pkt %u, multicast_pkt %u, broadcast_pkt %u, discards %u",
                           EXTRACT_32BITS(sflow_vlan_counter->vlan_id),
                           EXTRACT_64BITS(sflow_vlan_counter->octets),
                           EXTRACT_32BITS(sflow_vlan_counter->unicast_pkt),
                           EXTRACT_32BITS(sflow_vlan_counter->multicast_pkt),
                           EXTRACT_32BITS(sflow_vlan_counter->broadcast_pkt),
                           EXTRACT_32BITS(sflow_vlan_counter->discards));
                    break;
                case SFLOW_COUNTER_PROCESSOR:			/* XXX */
                    break;
                default:
                    if (vflag <= 1)
                        print_unknown_data(tptr, "\n\t\t", counter_len);
                    break;
                }
                tptr += counter_len;
                tlen -= counter_len;
                nrecords--;
            }
            break;
        default:
            if (vflag <= 1)
                print_unknown_data(tptr, "\n\t    ", sflow_sample_len);
            break;
        }
        tptr += sflow_sample_len;
        tlen -= sflow_sample_len;
        nsamples--;
    }
    return;

 trunc:
    printf("[|SFLOW]");
}

/*
 * Local Variables:
 * c-style: whitesmith
 * c-basic-offset: 4
 * End:
 */
