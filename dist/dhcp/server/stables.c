/* stables.c

   Tables of information only used by server... */

/*
 * Copyright (c) 1995-2003 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: stables.c,v 1.3 2003/02/18 17:08:45 drochner Exp $ Copyright (c) 1995-2003 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

#if defined (FAILOVER_PROTOCOL)

/* This is used to indicate some kind of failure when generating a
   failover option. */
failover_option_t null_failover_option = { 0, 0 };
failover_option_t skip_failover_option = { 0, 0 };

/* Information about failover options, for printing, encoding
   and decoding. */
struct failover_option_info ft_options [] =
{
	{ 0, "unused", FT_UNDEF, 0, 0, 0 },
	{ FTO_BINDING_STATUS, "binding-status",
	  FT_UINT8, 1, FM_OFFSET (binding_status), FTB_BINDING_STATUS },
	{ FTO_ASSIGNED_IP_ADDRESS, "assigned-IP-address",
	  FT_IPADDR, 1, FM_OFFSET (assigned_addr), FTB_ASSIGNED_IP_ADDRESS },
	{ FTO_SERVER_ADDR, "sending-server-IP-address",
	  FT_IPADDR, 1, FM_OFFSET (server_addr), FTB_SERVER_ADDR },
	{ FTO_ADDRESSES_TRANSFERRED, "addresses-transferred",
	  FT_UINT32, 1, FM_OFFSET (addresses_transferred),
	  FTB_ADDRESSES_TRANSFERRED },
	{ FTO_CLIENT_IDENTIFIER, "client-identifier",
	  FT_BYTES, 0, FM_OFFSET (client_identifier), FTB_CLIENT_IDENTIFIER },
	{ FTO_CHADDR, "client-hardware-address",
	  FT_BYTES, 0, FM_OFFSET (chaddr), FTB_CHADDR },
	{ FTO_DDNS, "DDNS",
	  FT_DDNS, 1, FM_OFFSET (ddns), FTB_DDNS },
	{ FTO_REJECT_REASON, "reject-reason",
	  FT_UINT8, 1, FM_OFFSET (reject_reason), FTB_REJECT_REASON },
	{ FTO_MESSAGE, "message",
	  FT_TEXT, 0, FM_OFFSET (message), FTB_MESSAGE },
	{ FTO_MCLT, "MCLT",
	  FT_UINT32, 1, FM_OFFSET (mclt), FTB_MCLT },
	{ FTO_VENDOR_CLASS, "vendor-class-identifier",
	  FT_TEXT_OR_BYTES, 0, FM_OFFSET (vendor_class), FTB_VENDOR_CLASS },
	{ 12, "undefined", FT_UNDEF, 0, 0, 0 },
	{ FTO_LEASE_EXPIRY, "lease-expiration-time",
	  FT_UINT32, 1, FM_OFFSET (expiry), FTB_LEASE_EXPIRY },
	{ FTO_POTENTIAL_EXPIRY, "potential-expiration-time",
	  FT_UINT32, 1, FM_OFFSET (potential_expiry), FTB_POTENTIAL_EXPIRY },
	{ FTO_GRACE_EXPIRY, "grace-expiration-time",
	  FT_UINT32, 1, FM_OFFSET (grace_expiry), FTB_GRACE_EXPIRY },
	{ FTO_CLTT, "client-last-transaction-time",
	  FT_UINT32, 1, FM_OFFSET (client_ltt), FTB_CLTT },
	{ FTO_STOS, "start-time-of-state",
	  FT_UINT32, 1, FM_OFFSET (stos), FTB_STOS },
	{ FTO_SERVER_STATE, "server-state",
	  FT_UINT8, 1, FM_OFFSET (server_state), FTB_SERVER_STATE },
	{ FTO_SERVER_FLAGS, "server-flags",
	  FT_UINT8, 1, FM_OFFSET (server_flags), FTB_SERVER_FLAGS },
	{ FTO_VENDOR_OPTIONS, "vendor-specific-options",
	  FT_BYTES, 0, FM_OFFSET (vendor_options), FTB_VENDOR_OPTIONS },
	{ FTO_MAX_UNACKED, "max-unacked-bndupd",
	  FT_UINT32, 1, FM_OFFSET (max_unacked), FTB_MAX_UNACKED },
	{ 22, "undefined", FT_UNDEF, 0, 0 },
	{ FTO_RECEIVE_TIMER, "receive-timer",
	  FT_UINT32, 1, FM_OFFSET (receive_timer), FTB_RECEIVE_TIMER },
	{ FTO_HBA, "hash-bucket-assignment",
	  FT_BYTES, 0, FM_OFFSET (hba), FTB_HBA },
	{ FTO_MESSAGE_DIGEST, "message-digest",
	  FT_DIGEST, 0, 0, FTB_MESSAGE_DIGEST },
	{ FTO_PROTOCOL_VERSION, "protocol-version", 
	  FT_UINT8, 1, FM_OFFSET (protocol_version), FTB_PROTOCOL_VERSION },
	{ FTO_TLS_REQUEST, "TLS-request",
	  FT_UINT8, 2, FM_OFFSET (tls_request), FTB_TLS_REQUEST },
	{ FTO_TLS_REPLY, "TLS-reply",
	  FT_BYTES, 1, FM_OFFSET (tls_reply ), FTB_TLS_REPLY },
	{ FTO_REQUEST_OPTIONS, "client-request-options",
	  FT_BYTES, 0, FM_OFFSET (request_options), FTB_REQUEST_OPTIONS },
	{ FTO_REPLY_OPTIONS, "client-reply-options",
	  FT_BYTES, 0, FM_OFFSET (reply_options), FTB_REPLY_OPTIONS }
};

/* These are really options that make sense for a particular request - if
   some other option comes in, we're not going to use it, so we can just
   discard it.  Note that the message-digest option is allowed for all
   message types, but is not saved - it's just used to validate the message
   and then discarded - so it's not mentioned here. */

u_int32_t fto_allowed [] = {
	0,	/* 0 unused */
	0,	/* 1 POOLREQ */
	FTB_ADDRESSES_TRANSFERRED, /* 2 POOLRESP */
	(FTB_ASSIGNED_IP_ADDRESS | FTB_BINDING_STATUS | FTB_CLIENT_IDENTIFIER |
	 FTB_CHADDR | FTB_LEASE_EXPIRY | FTB_POTENTIAL_EXPIRY | FTB_STOS |
	 FTB_CLTT | FTB_REQUEST_OPTIONS | FTB_REPLY_OPTIONS), /* 3 BNDUPD */
	(FTB_ASSIGNED_IP_ADDRESS | FTB_BINDING_STATUS | FTB_CLIENT_IDENTIFIER |
	 FTB_CHADDR | FTB_LEASE_EXPIRY | FTB_POTENTIAL_EXPIRY | FTB_STOS |
	 FTB_CLTT | FTB_REQUEST_OPTIONS | FTB_REPLY_OPTIONS |
	 FTB_REJECT_REASON | FTB_MESSAGE), /* 4 BNDACK */
	(FTB_SERVER_ADDR | FTB_MAX_UNACKED | FTB_RECEIVE_TIMER |
	 FTB_VENDOR_CLASS | FTB_PROTOCOL_VERSION | FTB_TLS_REQUEST |
	 FTB_MCLT | FTB_HBA), /* 5 CONNECT */
	(FTB_SERVER_ADDR | FTB_MAX_UNACKED | FTB_RECEIVE_TIMER |
	 FTB_VENDOR_CLASS | FTB_PROTOCOL_VERSION | FTB_TLS_REPLY |
	 FTB_REJECT_REASON | FTB_MESSAGE), /* CONNECTACK */
	0, /* 7 UPDREQ */
	0, /* 8 UPDDONE */
	0, /* 9 UPDREQALL */
	(FTB_SERVER_STATE | FTB_SERVER_FLAGS | FTB_STOS), /* 10 STATE */
	0,	/* 11 CONTACT */
	(FTB_REJECT_REASON | FTB_MESSAGE) /* 12 DISCONNECT */
};

/* Sizes of the various types. */
int ft_sizes [] = {
	1, /* FT_UINT8 */
	4, /* FT_IPADDR */
	4, /* FT_UINT32 */
	1, /* FT_BYTES */
	1, /* FT_TEXT_OR_BYTES */
	0, /* FT_DDNS */
	0, /* FT_DDNS1 */
	2, /* FT_UINT16 */
	1, /* FT_TEXT */
	0, /* FT_UNDEF */
	0, /* FT_DIGEST */
};

/* Names of the various failover link states. */
const char *dhcp_flink_state_names [] = {
	"invalid state 0",
	"startup",
	"message length wait",
	"message wait",
	"disconnected"
};
#endif /* FAILOVER_PROTOCOL */

/* Failover binding state names.   These are used even if there is no
   failover protocol support. */
const char *binding_state_names [] = {
	"free", "active", "expired", "released", "abandoned",
	"reset", "backup", "reserved", "bootp" };

struct universe agent_universe;
struct option agent_options [256] = {
	{ "pad", "",					&agent_universe, 0 },
	{ "circuit-id", "X",				&agent_universe, 1 },
	{ "remote-id", "X",				&agent_universe, 2 },
	{ "agent-id", "I",				&agent_universe, 3 },
	{ "DOCSIS-device-class", "L",			&agent_universe, 4 },
	{ "unknown-5", "X",				&agent_universe, 5 },
	{ "unknown-6", "X",				&agent_universe, 6 },
	{ "unknown-7", "X",				&agent_universe, 7 },
	{ "unknown-8", "X",				&agent_universe, 8 },
	{ "unknown-9", "X",				&agent_universe, 9 },
	{ "unknown-10", "X",				&agent_universe, 10 },
	{ "unknown-11", "X",				&agent_universe, 11 },
	{ "unknown-12", "X",				&agent_universe, 12 },
	{ "unknown-13", "X",				&agent_universe, 13 },
	{ "unknown-14", "X",				&agent_universe, 14 },
	{ "unknown-15", "X",				&agent_universe, 15 },
	{ "unknown-16", "X",				&agent_universe, 16 },
	{ "unknown-17", "X",				&agent_universe, 17 },
	{ "unknown-18", "X",				&agent_universe, 18 },
	{ "unknown-19", "X",				&agent_universe, 19 },
	{ "unknown-20", "X",				&agent_universe, 20 },
	{ "unknown-21", "X",				&agent_universe, 21 },
	{ "unknown-22", "X",				&agent_universe, 22 },
	{ "unknown-23", "X",				&agent_universe, 23 },
	{ "unknown-24", "X",				&agent_universe, 24 },
	{ "unknown-25", "X",				&agent_universe, 25 },
	{ "unknown-26", "X",				&agent_universe, 26 },
	{ "unknown-27", "X",				&agent_universe, 27 },
	{ "unknown-28", "X",				&agent_universe, 28 },
	{ "unknown-29", "X",				&agent_universe, 29 },
	{ "unknown-30", "X",				&agent_universe, 30 },
	{ "unknown-31", "X",				&agent_universe, 31 },
	{ "unknown-32", "X",				&agent_universe, 32 },
	{ "unknown-33", "X",				&agent_universe, 33 },
	{ "unknown-34", "X",				&agent_universe, 34 },
	{ "unknown-35", "X",				&agent_universe, 35 },
	{ "unknown-36", "X",				&agent_universe, 36 },
	{ "unknown-37", "X",				&agent_universe, 37 },
	{ "unknown-38", "X",				&agent_universe, 38 },
	{ "unknown-39", "X",				&agent_universe, 39 },
	{ "unknown-40", "X",				&agent_universe, 40 },
	{ "unknown-41", "X",				&agent_universe, 41 },
	{ "unknown-42", "X",				&agent_universe, 42 },
	{ "unknown-43", "X",				&agent_universe, 43 },
	{ "unknown-44", "X",				&agent_universe, 44 },
	{ "unknown-45", "X",				&agent_universe, 45 },
	{ "unknown-46", "X",				&agent_universe, 46 },
	{ "unknown-47", "X",				&agent_universe, 47 },
	{ "unknown-48", "X",				&agent_universe, 48 },
	{ "unknown-49", "X",				&agent_universe, 49 },
	{ "unknown-50", "X",				&agent_universe, 50 },
	{ "unknown-51", "X",				&agent_universe, 51 },
	{ "unknown-52", "X",				&agent_universe, 52 },
	{ "unknown-53", "X",				&agent_universe, 53 },
	{ "unknown-54", "X",				&agent_universe, 54 },
	{ "unknown-55", "X",				&agent_universe, 55 },
	{ "unknown-56", "X",				&agent_universe, 56 },
	{ "unknown-57", "X",				&agent_universe, 57 },
	{ "unknown-58", "X",				&agent_universe, 58 },
	{ "unknown-59", "X",				&agent_universe, 59 },
	{ "unknown-60", "X",				&agent_universe, 60 },
	{ "unknown-61", "X",				&agent_universe, 61 },
	{ "unknown-62", "X",				&agent_universe, 62 },
	{ "unknown-63", "X",				&agent_universe, 63 },
	{ "unknown-64", "X",				&agent_universe, 64 },
	{ "unknown-65", "X",				&agent_universe, 65 },
	{ "unknown-66", "X",				&agent_universe, 66 },
	{ "unknown-67", "X",				&agent_universe, 67 },
	{ "unknown-68", "X",				&agent_universe, 68 },
	{ "unknown-69", "X",				&agent_universe, 69 },
	{ "unknown-70", "X",				&agent_universe, 70 },
	{ "unknown-71", "X",				&agent_universe, 71 },
	{ "unknown-72", "X",				&agent_universe, 72 },
	{ "unknown-73", "X",				&agent_universe, 73 },
	{ "unknown-74", "X",				&agent_universe, 74 },
	{ "unknown-75", "X",				&agent_universe, 75 },
	{ "unknown-76", "X",				&agent_universe, 76 },
	{ "unknown-77", "X",				&agent_universe, 77 },
	{ "unknown-78", "X",				&agent_universe, 78 },
	{ "unknown-79", "X",				&agent_universe, 79 },
	{ "unknown-80", "X",				&agent_universe, 80 },
	{ "unknown-81", "X",				&agent_universe, 81 },
	{ "unknown-82", "X",				&agent_universe, 82 },
	{ "unknown-83", "X",				&agent_universe, 83 },
	{ "unknown-84", "X",				&agent_universe, 84 },
	{ "unknown-85", "X",				&agent_universe, 85 },
	{ "unknown-86", "X",				&agent_universe, 86 },
	{ "unknown-87", "X",				&agent_universe, 87 },
	{ "unknown-88", "X",				&agent_universe, 88 },
	{ "unknown-89", "X",				&agent_universe, 89 },
	{ "unknown-90", "X",				&agent_universe, 90 },
	{ "unknown-91", "X",				&agent_universe, 91 },
	{ "unknown-92", "X",				&agent_universe, 92 },
	{ "unknown-93", "X",				&agent_universe, 93 },
	{ "unknown-94", "X",				&agent_universe, 94 },
	{ "unknown-95", "X",				&agent_universe, 95 },
	{ "unknown-96", "X",				&agent_universe, 96 },
	{ "unknown-97", "X",				&agent_universe, 97 },
	{ "unknown-98", "X",				&agent_universe, 98 },
	{ "unknown-99", "X",				&agent_universe, 99 },
	{ "unknown-100", "X",				&agent_universe, 100 },
	{ "unknown-101", "X",				&agent_universe, 101 },
	{ "unknown-102", "X",				&agent_universe, 102 },
	{ "unknown-103", "X",				&agent_universe, 103 },
	{ "unknown-104", "X",				&agent_universe, 104 },
	{ "unknown-105", "X",				&agent_universe, 105 },
	{ "unknown-106", "X",				&agent_universe, 106 },
	{ "unknown-107", "X",				&agent_universe, 107 },
	{ "unknown-108", "X",				&agent_universe, 108 },
	{ "unknown-109", "X",				&agent_universe, 109 },
	{ "unknown-110", "X",				&agent_universe, 110 },
	{ "unknown-111", "X",				&agent_universe, 111 },
	{ "unknown-112", "X",				&agent_universe, 112 },
	{ "unknown-113", "X",				&agent_universe, 113 },
	{ "unknown-114", "X",				&agent_universe, 114 },
	{ "unknown-115", "X",				&agent_universe, 115 },
	{ "unknown-116", "X",				&agent_universe, 116 },
	{ "unknown-117", "X",				&agent_universe, 117 },
	{ "unknown-118", "X",				&agent_universe, 118 },
	{ "unknown-119", "X",				&agent_universe, 119 },
	{ "unknown-120", "X",				&agent_universe, 120 },
	{ "unknown-121", "X",				&agent_universe, 121 },
	{ "unknown-122", "X",				&agent_universe, 122 },
	{ "unknown-123", "X",				&agent_universe, 123 },
	{ "unknown-124", "X",				&agent_universe, 124 },
	{ "unknown-125", "X",				&agent_universe, 125 },
	{ "unknown-126", "X",				&agent_universe, 126 },
	{ "unknown-127", "X",				&agent_universe, 127 },
	{ "unknown-128", "X",				&agent_universe, 128 },
	{ "unknown-129", "X",				&agent_universe, 129 },
	{ "unknown-130", "X",				&agent_universe, 130 },
	{ "unknown-131", "X",				&agent_universe, 131 },
	{ "unknown-132", "X",				&agent_universe, 132 },
	{ "unknown-133", "X",				&agent_universe, 133 },
	{ "unknown-134", "X",				&agent_universe, 134 },
	{ "unknown-135", "X",				&agent_universe, 135 },
	{ "unknown-136", "X",				&agent_universe, 136 },
	{ "unknown-137", "X",				&agent_universe, 137 },
	{ "unknown-138", "X",				&agent_universe, 138 },
	{ "unknown-139", "X",				&agent_universe, 139 },
	{ "unknown-140", "X",				&agent_universe, 140 },
	{ "unknown-141", "X",				&agent_universe, 141 },
	{ "unknown-142", "X",				&agent_universe, 142 },
	{ "unknown-143", "X",				&agent_universe, 143 },
	{ "unknown-144", "X",				&agent_universe, 144 },
	{ "unknown-145", "X",				&agent_universe, 145 },
	{ "unknown-146", "X",				&agent_universe, 146 },
	{ "unknown-147", "X",				&agent_universe, 147 },
	{ "unknown-148", "X",				&agent_universe, 148 },
	{ "unknown-149", "X",				&agent_universe, 149 },
	{ "unknown-150", "X",				&agent_universe, 150 },
	{ "unknown-151", "X",				&agent_universe, 151 },
	{ "unknown-152", "X",				&agent_universe, 152 },
	{ "unknown-153", "X",				&agent_universe, 153 },
	{ "unknown-154", "X",				&agent_universe, 154 },
	{ "unknown-155", "X",				&agent_universe, 155 },
	{ "unknown-156", "X",				&agent_universe, 156 },
	{ "unknown-157", "X",				&agent_universe, 157 },
	{ "unknown-158", "X",				&agent_universe, 158 },
	{ "unknown-159", "X",				&agent_universe, 159 },
	{ "unknown-160", "X",				&agent_universe, 160 },
	{ "unknown-161", "X",				&agent_universe, 161 },
	{ "unknown-162", "X",				&agent_universe, 162 },
	{ "unknown-163", "X",				&agent_universe, 163 },
	{ "unknown-164", "X",				&agent_universe, 164 },
	{ "unknown-165", "X",				&agent_universe, 165 },
	{ "unknown-166", "X",				&agent_universe, 166 },
	{ "unknown-167", "X",				&agent_universe, 167 },
	{ "unknown-168", "X",				&agent_universe, 168 },
	{ "unknown-169", "X",				&agent_universe, 169 },
	{ "unknown-170", "X",				&agent_universe, 170 },
	{ "unknown-171", "X",				&agent_universe, 171 },
	{ "unknown-172", "X",				&agent_universe, 172 },
	{ "unknown-173", "X",				&agent_universe, 173 },
	{ "unknown-174", "X",				&agent_universe, 174 },
	{ "unknown-175", "X",				&agent_universe, 175 },
	{ "unknown-176", "X",				&agent_universe, 176 },
	{ "unknown-177", "X",				&agent_universe, 177 },
	{ "unknown-178", "X",				&agent_universe, 178 },
	{ "unknown-179", "X",				&agent_universe, 179 },
	{ "unknown-180", "X",				&agent_universe, 180 },
	{ "unknown-181", "X",				&agent_universe, 181 },
	{ "unknown-182", "X",				&agent_universe, 182 },
	{ "unknown-183", "X",				&agent_universe, 183 },
	{ "unknown-184", "X",				&agent_universe, 184 },
	{ "unknown-185", "X",				&agent_universe, 185 },
	{ "unknown-186", "X",				&agent_universe, 186 },
	{ "unknown-187", "X",				&agent_universe, 187 },
	{ "unknown-188", "X",				&agent_universe, 188 },
	{ "unknown-189", "X",				&agent_universe, 189 },
	{ "unknown-190", "X",				&agent_universe, 190 },
	{ "unknown-191", "X",				&agent_universe, 191 },
	{ "unknown-192", "X",				&agent_universe, 192 },
	{ "unknown-193", "X",				&agent_universe, 193 },
	{ "unknown-194", "X",				&agent_universe, 194 },
	{ "unknown-195", "X",				&agent_universe, 195 },
	{ "unknown-196", "X",				&agent_universe, 196 },
	{ "unknown-197", "X",				&agent_universe, 197 },
	{ "unknown-198", "X",				&agent_universe, 198 },
	{ "unknown-199", "X",				&agent_universe, 199 },
	{ "unknown-200", "X",				&agent_universe, 200 },
	{ "unknown-201", "X",				&agent_universe, 201 },
	{ "unknown-202", "X",				&agent_universe, 202 },
	{ "unknown-203", "X",				&agent_universe, 203 },
	{ "unknown-204", "X",				&agent_universe, 204 },
	{ "unknown-205", "X",				&agent_universe, 205 },
	{ "unknown-206", "X",				&agent_universe, 206 },
	{ "unknown-207", "X",				&agent_universe, 207 },
	{ "unknown-208", "X",				&agent_universe, 208 },
	{ "unknown-209", "X",				&agent_universe, 209 },
	{ "unknown-210", "X",				&agent_universe, 210 },
	{ "unknown-211", "X",				&agent_universe, 211 },
	{ "unknown-212", "X",				&agent_universe, 212 },
	{ "unknown-213", "X",				&agent_universe, 213 },
	{ "unknown-214", "X",				&agent_universe, 214 },
	{ "unknown-215", "X",				&agent_universe, 215 },
	{ "unknown-216", "X",				&agent_universe, 216 },
	{ "unknown-217", "X",				&agent_universe, 217 },
	{ "unknown-218", "X",				&agent_universe, 218 },
	{ "unknown-219", "X",				&agent_universe, 219 },
	{ "unknown-220", "X",				&agent_universe, 220 },
	{ "unknown-221", "X",				&agent_universe, 221 },
	{ "unknown-222", "X",				&agent_universe, 222 },
	{ "unknown-223", "X",				&agent_universe, 223 },
	{ "unknown-224", "X",				&agent_universe, 224 },
	{ "unknown-225", "X",				&agent_universe, 225 },
	{ "unknown-226", "X",				&agent_universe, 226 },
	{ "unknown-227", "X",				&agent_universe, 227 },
	{ "unknown-228", "X",				&agent_universe, 228 },
	{ "unknown-229", "X",				&agent_universe, 229 },
	{ "unknown-230", "X",				&agent_universe, 230 },
	{ "unknown-231", "X",				&agent_universe, 231 },
	{ "unknown-232", "X",				&agent_universe, 232 },
	{ "unknown-233", "X",				&agent_universe, 233 },
	{ "unknown-234", "X",				&agent_universe, 234 },
	{ "unknown-235", "X",				&agent_universe, 235 },
	{ "unknown-236", "X",				&agent_universe, 236 },
	{ "unknown-237", "X",				&agent_universe, 237 },
	{ "unknown-238", "X",				&agent_universe, 238 },
	{ "unknown-239", "X",				&agent_universe, 239 },
	{ "unknown-240", "X",				&agent_universe, 240 },
	{ "unknown-241", "X",				&agent_universe, 241 },
	{ "unknown-242", "X",				&agent_universe, 242 },
	{ "unknown-243", "X",				&agent_universe, 243 },
	{ "unknown-244", "X",				&agent_universe, 244 },
	{ "unknown-245", "X",				&agent_universe, 245 },
	{ "unknown-246", "X",				&agent_universe, 246 },
	{ "unknown-247", "X",				&agent_universe, 247 },
	{ "unknown-248", "X",				&agent_universe, 248 },
	{ "unknown-249", "X",				&agent_universe, 249 },
	{ "unknown-250", "X",				&agent_universe, 250 },
	{ "unknown-251", "X",				&agent_universe, 251 },
	{ "unknown-252", "X",				&agent_universe, 252 },
	{ "unknown-253", "X",				&agent_universe, 253 },
	{ "unknown-254", "X",				&agent_universe, 254 },
	{ "option-end", "e",				&agent_universe, 255 },
};

struct universe server_universe;
struct option server_options [256] = {
	{ "pad", "",					&server_universe, 0 },
	{ "default-lease-time", "T",			&server_universe, 1 },
	{ "max-lease-time", "T",			&server_universe, 2 },
	{ "min-lease-time", "T",			&server_universe, 3 },
	{ "dynamic-bootp-lease-cutoff", "T",		&server_universe, 4 },
	{ "dynamic-bootp-lease-length", "L",		&server_universe, 5 },
	{ "boot-unknown-clients", "f",			&server_universe, 6 },
	{ "dynamic-bootp", "f",				&server_universe, 7 },
	{ "allow-bootp", "f",				&server_universe, 8 },
	{ "allow-booting", "f",				&server_universe, 9 },
	{ "one-lease-per-client", "f",			&server_universe, 10 },
	{ "get-lease-hostnames", "f",			&server_universe, 11 },
	{ "use-host-decl-names", "f",			&server_universe, 12 },
	{ "use-lease-addr-for-default-route", "f",	&server_universe, 13 },
	{ "min-secs", "B",				&server_universe, 14 },
	{ "filename", "t",				&server_universe, 15 },
	{ "server-name", "t",				&server_universe, 16 },
	{ "next-server", "I",				&server_universe, 17 },
	{ "authoritative", "f",				&server_universe, 18 },
	{ "vendor-option-space", "U",			&server_universe, 19 },
	{ "always-reply-rfc1048", "f",			&server_universe, 20 },
	{ "site-option-space", "X",			&server_universe, 21 },
	{ "always-broadcast", "f",			&server_universe, 22 },
	{ "ddns-domainname", "t",			&server_universe, 23 },
	{ "ddns-hostname", "t",				&server_universe, 24 },
	{ "ddns-rev-domainname", "t",			&server_universe, 25 },
	{ "lease-file-name", "t",			&server_universe, 26 },
	{ "pid-file-name", "t",				&server_universe, 27 },
	{ "duplicates", "f",				&server_universe, 28 },
	{ "declines", "f",				&server_universe, 29 },
	{ "ddns-updates", "f",				&server_universe, 30 },
	{ "omapi-port", "S",				&server_universe, 31 },
	{ "local-port", "S",				&server_universe, 32 },
	{ "limited-broadcast-address", "I",		&server_universe, 33 },
	{ "remote-port", "S",				&server_universe, 34 },
	{ "local-address", "I",				&server_universe, 35 },
	{ "omapi-key", "d",				&server_universe, 36 },
	{ "stash-agent-options", "f",			&server_universe, 37 },
	{ "ddns-ttl", "T",				&server_universe, 38 },
	{ "ddns-update-style", "Nddns-styles.",		&server_universe, 39 },
	{ "client-updates", "f",			&server_universe, 40 },
	{ "update-optimization", "f",			&server_universe, 41 },
	{ "ping-check", "f",				&server_universe, 42 },
	{ "update-static-leases", "f",			&server_universe, 43 },
	{ "log-facility", "Nsyslog-facilities.",	&server_universe, 44 },
	{ "do-forward-updates", "f",			&server_universe, 45 },
	{ "unknown-46", "X",				&server_universe, 46 },
	{ "unknown-47", "X",				&server_universe, 47 },
	{ "unknown-48", "X",				&server_universe, 48 },
	{ "unknown-49", "X",				&server_universe, 49 },
	{ "unknown-50", "X",				&server_universe, 50 },
	{ "unknown-51", "X",				&server_universe, 51 },
	{ "unknown-52", "X",				&server_universe, 52 },
	{ "unknown-53", "X",				&server_universe, 53 },
	{ "unknown-54", "X",				&server_universe, 54 },
	{ "unknown-55", "X",				&server_universe, 55 },
	{ "unknown-56", "X",				&server_universe, 56 },
	{ "unknown-57", "X",				&server_universe, 57 },
	{ "unknown-58", "X",				&server_universe, 58 },
	{ "unknown-59", "X",				&server_universe, 59 },
	{ "unknown-60", "X",				&server_universe, 60 },
	{ "unknown-61", "X",				&server_universe, 61 },
	{ "unknown-62", "X",				&server_universe, 62 },
	{ "unknown-63", "X",				&server_universe, 63 },
	{ "unknown-64", "X",				&server_universe, 64 },
	{ "unknown-65", "X",				&server_universe, 65 },
	{ "unknown-66", "X",				&server_universe, 66 },
	{ "unknown-67", "X",				&server_universe, 67 },
	{ "unknown-68", "X",				&server_universe, 68 },
	{ "unknown-69", "X",				&server_universe, 69 },
	{ "unknown-70", "X",				&server_universe, 70 },
	{ "unknown-71", "X",				&server_universe, 71 },
	{ "unknown-72", "X",				&server_universe, 72 },
	{ "unknown-73", "X",				&server_universe, 73 },
	{ "unknown-74", "X",				&server_universe, 74 },
	{ "unknown-75", "X",				&server_universe, 75 },
	{ "unknown-76", "X",				&server_universe, 76 },
	{ "unknown-77", "X",				&server_universe, 77 },
	{ "unknown-78", "X",				&server_universe, 78 },
	{ "unknown-79", "X",				&server_universe, 79 },
	{ "unknown-80", "X",				&server_universe, 80 },
	{ "unknown-81", "X",				&server_universe, 81 },
	{ "unknown-82", "X",				&server_universe, 82 },
	{ "unknown-83", "X",				&server_universe, 83 },
	{ "unknown-84", "X",				&server_universe, 84 },
	{ "unknown-85", "X",				&server_universe, 85 },
	{ "unknown-86", "X",				&server_universe, 86 },
	{ "unknown-87", "X",				&server_universe, 87 },
	{ "unknown-88", "X",				&server_universe, 88 },
	{ "unknown-89", "X",				&server_universe, 89 },
	{ "unknown-90", "X",				&server_universe, 90 },
	{ "unknown-91", "X",				&server_universe, 91 },
	{ "unknown-92", "X",				&server_universe, 92 },
	{ "unknown-93", "X",				&server_universe, 93 },
	{ "unknown-94", "X",				&server_universe, 94 },
	{ "unknown-95", "X",				&server_universe, 95 },
	{ "unknown-96", "X",				&server_universe, 96 },
	{ "unknown-97", "X",				&server_universe, 97 },
	{ "unknown-98", "X",				&server_universe, 98 },
	{ "unknown-99", "X",				&server_universe, 99 },
	{ "unknown-100", "X",				&server_universe, 100 },
	{ "unknown-101", "X",				&server_universe, 101 },
	{ "unknown-102", "X",				&server_universe, 102 },
	{ "unknown-103", "X",				&server_universe, 103 },
	{ "unknown-104", "X",				&server_universe, 104 },
	{ "unknown-105", "X",				&server_universe, 105 },
	{ "unknown-106", "X",				&server_universe, 106 },
	{ "unknown-107", "X",				&server_universe, 107 },
	{ "unknown-108", "X",				&server_universe, 108 },
	{ "unknown-109", "X",				&server_universe, 109 },
	{ "unknown-110", "X",				&server_universe, 110 },
	{ "unknown-111", "X",				&server_universe, 111 },
	{ "unknown-112", "X",				&server_universe, 112 },
	{ "unknown-113", "X",				&server_universe, 113 },
	{ "unknown-114", "X",				&server_universe, 114 },
	{ "unknown-115", "X",				&server_universe, 115 },
	{ "unknown-116", "X",				&server_universe, 116 },
	{ "unknown-117", "X",				&server_universe, 117 },
	{ "unknown-118", "X",				&server_universe, 118 },
	{ "unknown-119", "X",				&server_universe, 119 },
	{ "unknown-120", "X",				&server_universe, 120 },
	{ "unknown-121", "X",				&server_universe, 121 },
	{ "unknown-122", "X",				&server_universe, 122 },
	{ "unknown-123", "X",				&server_universe, 123 },
	{ "unknown-124", "X",				&server_universe, 124 },
	{ "unknown-125", "X",				&server_universe, 125 },
	{ "unknown-126", "X",				&server_universe, 126 },
	{ "unknown-127", "X",				&server_universe, 127 },
	{ "unknown-128", "X",				&server_universe, 128 },
	{ "unknown-129", "X",				&server_universe, 129 },
	{ "unknown-130", "X",				&server_universe, 130 },
	{ "unknown-131", "X",				&server_universe, 131 },
	{ "unknown-132", "X",				&server_universe, 132 },
	{ "unknown-133", "X",				&server_universe, 133 },
	{ "unknown-134", "X",				&server_universe, 134 },
	{ "unknown-135", "X",				&server_universe, 135 },
	{ "unknown-136", "X",				&server_universe, 136 },
	{ "unknown-137", "X",				&server_universe, 137 },
	{ "unknown-138", "X",				&server_universe, 138 },
	{ "unknown-139", "X",				&server_universe, 139 },
	{ "unknown-140", "X",				&server_universe, 140 },
	{ "unknown-141", "X",				&server_universe, 141 },
	{ "unknown-142", "X",				&server_universe, 142 },
	{ "unknown-143", "X",				&server_universe, 143 },
	{ "unknown-144", "X",				&server_universe, 144 },
	{ "unknown-145", "X",				&server_universe, 145 },
	{ "unknown-146", "X",				&server_universe, 146 },
	{ "unknown-147", "X",				&server_universe, 147 },
	{ "unknown-148", "X",				&server_universe, 148 },
	{ "unknown-149", "X",				&server_universe, 149 },
	{ "unknown-150", "X",				&server_universe, 150 },
	{ "unknown-151", "X",				&server_universe, 151 },
	{ "unknown-152", "X",				&server_universe, 152 },
	{ "unknown-153", "X",				&server_universe, 153 },
	{ "unknown-154", "X",				&server_universe, 154 },
	{ "unknown-155", "X",				&server_universe, 155 },
	{ "unknown-156", "X",				&server_universe, 156 },
	{ "unknown-157", "X",				&server_universe, 157 },
	{ "unknown-158", "X",				&server_universe, 158 },
	{ "unknown-159", "X",				&server_universe, 159 },
	{ "unknown-160", "X",				&server_universe, 160 },
	{ "unknown-161", "X",				&server_universe, 161 },
	{ "unknown-162", "X",				&server_universe, 162 },
	{ "unknown-163", "X",				&server_universe, 163 },
	{ "unknown-164", "X",				&server_universe, 164 },
	{ "unknown-165", "X",				&server_universe, 165 },
	{ "unknown-166", "X",				&server_universe, 166 },
	{ "unknown-167", "X",				&server_universe, 167 },
	{ "unknown-168", "X",				&server_universe, 168 },
	{ "unknown-169", "X",				&server_universe, 169 },
	{ "unknown-170", "X",				&server_universe, 170 },
	{ "unknown-171", "X",				&server_universe, 171 },
	{ "unknown-172", "X",				&server_universe, 172 },
	{ "unknown-173", "X",				&server_universe, 173 },
	{ "unknown-174", "X",				&server_universe, 174 },
	{ "unknown-175", "X",				&server_universe, 175 },
	{ "unknown-176", "X",				&server_universe, 176 },
	{ "unknown-177", "X",				&server_universe, 177 },
	{ "unknown-178", "X",				&server_universe, 178 },
	{ "unknown-179", "X",				&server_universe, 179 },
	{ "unknown-180", "X",				&server_universe, 180 },
	{ "unknown-181", "X",				&server_universe, 181 },
	{ "unknown-182", "X",				&server_universe, 182 },
	{ "unknown-183", "X",				&server_universe, 183 },
	{ "unknown-184", "X",				&server_universe, 184 },
	{ "unknown-185", "X",				&server_universe, 185 },
	{ "unknown-186", "X",				&server_universe, 186 },
	{ "unknown-187", "X",				&server_universe, 187 },
	{ "unknown-188", "X",				&server_universe, 188 },
	{ "unknown-189", "X",				&server_universe, 189 },
	{ "unknown-190", "X",				&server_universe, 190 },
	{ "unknown-191", "X",				&server_universe, 191 },
	{ "unknown-192", "X",				&server_universe, 192 },
	{ "unknown-193", "X",				&server_universe, 193 },
	{ "unknown-194", "X",				&server_universe, 194 },
	{ "unknown-195", "X",				&server_universe, 195 },
	{ "unknown-196", "X",				&server_universe, 196 },
	{ "unknown-197", "X",				&server_universe, 197 },
	{ "unknown-198", "X",				&server_universe, 198 },
	{ "unknown-199", "X",				&server_universe, 199 },
	{ "unknown-200", "X",				&server_universe, 200 },
	{ "unknown-201", "X",				&server_universe, 201 },
	{ "unknown-202", "X",				&server_universe, 202 },
	{ "unknown-203", "X",				&server_universe, 203 },
	{ "unknown-204", "X",				&server_universe, 204 },
	{ "unknown-205", "X",				&server_universe, 205 },
	{ "unknown-206", "X",				&server_universe, 206 },
	{ "unknown-207", "X",				&server_universe, 207 },
	{ "unknown-208", "X",				&server_universe, 208 },
	{ "unknown-209", "X",				&server_universe, 209 },
	{ "unknown-210", "X",				&server_universe, 210 },
	{ "unknown-211", "X",				&server_universe, 211 },
	{ "unknown-212", "X",				&server_universe, 212 },
	{ "unknown-213", "X",				&server_universe, 213 },
	{ "unknown-214", "X",				&server_universe, 214 },
	{ "unknown-215", "X",				&server_universe, 215 },
	{ "unknown-216", "X",				&server_universe, 216 },
	{ "unknown-217", "X",				&server_universe, 217 },
	{ "unknown-218", "X",				&server_universe, 218 },
	{ "unknown-219", "X",				&server_universe, 219 },
	{ "unknown-220", "X",				&server_universe, 220 },
	{ "unknown-221", "X",				&server_universe, 221 },
	{ "unknown-222", "X",				&server_universe, 222 },
	{ "unknown-223", "X",				&server_universe, 223 },
	{ "unknown-224", "X",				&server_universe, 224 },
	{ "unknown-225", "X",				&server_universe, 225 },
	{ "unknown-226", "X",				&server_universe, 226 },
	{ "unknown-227", "X",				&server_universe, 227 },
	{ "unknown-228", "X",				&server_universe, 228 },
	{ "unknown-229", "X",				&server_universe, 229 },
	{ "unknown-230", "X",				&server_universe, 230 },
	{ "unknown-231", "X",				&server_universe, 231 },
	{ "unknown-232", "X",				&server_universe, 232 },
	{ "unknown-233", "X",				&server_universe, 233 },
	{ "unknown-234", "X",				&server_universe, 234 },
	{ "unknown-235", "X",				&server_universe, 235 },
	{ "unknown-236", "X",				&server_universe, 236 },
	{ "unknown-237", "X",				&server_universe, 237 },
	{ "unknown-238", "X",				&server_universe, 238 },
	{ "unknown-239", "X",				&server_universe, 239 },
	{ "unknown-240", "X",				&server_universe, 240 },
	{ "unknown-241", "X",				&server_universe, 241 },
	{ "unknown-242", "X",				&server_universe, 242 },
	{ "unknown-243", "X",				&server_universe, 243 },
	{ "unknown-244", "X",				&server_universe, 244 },
	{ "unknown-245", "X",				&server_universe, 245 },
	{ "unknown-246", "X",				&server_universe, 246 },
	{ "unknown-247", "X",				&server_universe, 247 },
	{ "unknown-248", "X",				&server_universe, 248 },
	{ "unknown-249", "X",				&server_universe, 249 },
	{ "unknown-250", "X",				&server_universe, 250 },
	{ "unknown-251", "X",				&server_universe, 251 },
	{ "unknown-252", "X",				&server_universe, 252 },
	{ "unknown-253", "X",				&server_universe, 253 },
	{ "unknown-254", "X",				&server_universe, 254 },
	{ "option-end", "e",				&server_universe, 255 },
};

struct enumeration_value ddns_styles_values [] = {
	{ "none", 0 },
	{ "ad-hoc", 1 },
	{ "interim", 2 },
	{ (char *)0, 0 }
};

struct enumeration ddns_styles = {
	(struct enumeration *)0,
	"ddns-styles",
	ddns_styles_values
};

struct enumeration_value syslog_values [] = {
#if defined (LOG_KERN)
	{ "kern", LOG_KERN },
#endif
#if defined (LOG_USER)
	{ "user", LOG_USER },
#endif
#if defined (LOG_MAIL)
	{ "mail", LOG_MAIL },
#endif
#if defined (LOG_DAEMON)
	{ "daemon", LOG_DAEMON },
#endif
#if defined (LOG_AUTH)
	{ "auth", LOG_AUTH },
#endif
#if defined (LOG_SYSLOG)
	{ "syslog", LOG_SYSLOG },
#endif
#if defined (LOG_LPR)
	{ "lpr", LOG_LPR },
#endif
#if defined (LOG_NEWS)
	{ "news", LOG_NEWS },
#endif
#if defined (LOG_UUCP)
	{ "uucp", LOG_UUCP },
#endif
#if defined (LOG_CRON)
	{ "cron", LOG_CRON },
#endif
#if defined (LOG_AUTHPRIV)
	{ "authpriv", LOG_AUTHPRIV },
#endif
#if defined (LOG_FTP)
	{ "ftp", LOG_FTP },
#endif
#if defined (LOG_LOCAL0)
	{ "local0", LOG_LOCAL0 },
#endif
#if defined (LOG_LOCAL1)
	{ "local1", LOG_LOCAL1 },
#endif
#if defined (LOG_LOCAL2)
	{ "local2", LOG_LOCAL2 },
#endif
#if defined (LOG_LOCAL3)
	{ "local3", LOG_LOCAL3 },
#endif
#if defined (LOG_LOCAL4)
	{ "local4", LOG_LOCAL4 },
#endif
#if defined (LOG_LOCAL5)
	{ "local5", LOG_LOCAL5 },
#endif
#if defined (LOG_LOCAL6)
	{ "local6", LOG_LOCAL6 },
#endif
#if defined (LOG_LOCAL7)
	{ "local7", LOG_LOCAL7 },
#endif
	{ (char *)0, 0 }
};

struct enumeration syslog_enum = {
	(struct enumeration *)0,
	"syslog-facilities",
	syslog_values
};

void initialize_server_option_spaces()
{
	int i;

	/* Set up the Relay Agent Information Option suboption space... */
	agent_universe.name = "agent";
	agent_universe.option_state_dereference =
		linked_option_state_dereference;
	agent_universe.lookup_func = lookup_linked_option;
	agent_universe.save_func = save_linked_option;
	agent_universe.delete_func = delete_linked_option;
	agent_universe.encapsulate = linked_option_space_encapsulate;
	agent_universe.foreach = linked_option_space_foreach;
	agent_universe.decode = parse_option_buffer;
	agent_universe.index = universe_count++;
	agent_universe.length_size = 1;
	agent_universe.tag_size = 1;
	agent_universe.store_tag = putUChar;
	agent_universe.store_length = putUChar;
	universes [agent_universe.index] = &agent_universe;
	option_new_hash (&agent_universe.hash, 1, MDL);
	if (!agent_universe.hash)
		log_fatal ("Can't allocate agent option hash table.");
	for (i = 0; i < 256; i++) {
		agent_universe.options [i] = &agent_options [i];
		option_hash_add (agent_universe.hash,
				 agent_options [i].name, 0,
				 &agent_options [i], MDL);
	}
	agent_universe.enc_opt = &dhcp_options [DHO_DHCP_AGENT_OPTIONS];


	/* Set up the server option universe... */
	server_universe.name = "server";
	server_universe.lookup_func = lookup_hashed_option;
	server_universe.option_state_dereference =
		hashed_option_state_dereference;
	server_universe.save_func = save_hashed_option;
	server_universe.delete_func = delete_hashed_option;
	server_universe.encapsulate = hashed_option_space_encapsulate;
	server_universe.foreach = hashed_option_space_foreach;
	server_universe.length_size = 1;
	server_universe.tag_size = 1;
	server_universe.store_tag = putUChar;
	server_universe.store_length = putUChar;
	server_universe.index = universe_count++;
	universes [server_universe.index] = &server_universe;
	option_new_hash (&server_universe.hash, 1, MDL);
	if (!server_universe.hash)
		log_fatal ("Can't allocate server option hash table.");
	for (i = 0; i < 256; i++) {
		server_universe.options [i] = &server_options [i];
		option_hash_add (server_universe.hash,
				 server_options [i].name, 0,
				 &server_options [i], MDL);
	}

	/* Add the server and agent option spaces to the option space hash. */
	universe_hash_add (universe_hash,
			   agent_universe.name, 0, &agent_universe, MDL);
	universe_hash_add (universe_hash,
			   server_universe.name, 0, &server_universe, MDL);

	/* Make the server universe the configuration option universe. */
	config_universe = &server_universe;
	vendor_cfg_option = &server_options [SV_VENDOR_OPTION_SPACE];
}
