/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2003 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Set mDNS_InstantiateInlines to tell mDNSEmbeddedAPI.h to instantiate inline functions, if necessary
#define mDNS_InstantiateInlines 1
#include "DNSCommon.h"

// Disable certain benign warnings with Microsoft compilers
#if (defined(_MSC_VER))
	// Disable "conditional expression is constant" warning for debug macros.
	// Otherwise, this generates warnings for the perfectly natural construct "while(1)"
	// If someone knows a variant way of writing "while(1)" that doesn't generate warning messages, please let us know
	#pragma warning(disable:4127)
	// Disable "array is too small to include a terminating null character" warning
	// -- domain labels have an initial length byte, not a terminating null character
	#pragma warning(disable:4295)
#endif

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark - Program Constants
#endif

mDNSexport const mDNSInterfaceID mDNSInterface_Any       = 0;
mDNSexport const mDNSInterfaceID mDNSInterfaceMark       = (mDNSInterfaceID)-1;
mDNSexport const mDNSInterfaceID mDNSInterface_LocalOnly = (mDNSInterfaceID)-2;
mDNSexport const mDNSInterfaceID mDNSInterface_Unicast   = (mDNSInterfaceID)-3;
mDNSexport const mDNSInterfaceID mDNSInterface_P2P       = (mDNSInterfaceID)-4;

// Note: Microsoft's proposed "Link Local Multicast Name Resolution Protocol" (LLMNR) is essentially a limited version of
// Multicast DNS, using the same packet formats, naming syntax, and record types as Multicast DNS, but on a different UDP
// port and multicast address, which means it won't interoperate with the existing installed base of Multicast DNS responders.
// LLMNR uses IPv4 multicast address 224.0.0.252, IPv6 multicast address FF02::0001:0003, and UDP port 5355.
// Uncomment the appropriate lines below to build a special Multicast DNS responder for testing interoperability
// with Microsoft's LLMNR client code.

#define   DiscardPortAsNumber               9
#define   SSHPortAsNumber                  22
#define   UnicastDNSPortAsNumber           53
#define   SSDPPortAsNumber               1900
#define   IPSECPortAsNumber              4500
#define   NSIPCPortAsNumber              5030		// Port used for dnsextd to talk to local nameserver bound to loopback
#define   NATPMPAnnouncementPortAsNumber 5350
#define   NATPMPPortAsNumber             5351
#define   DNSEXTPortAsNumber             5352		// Port used for end-to-end DNS operations like LLQ, Updates with Leases, etc.
#define   MulticastDNSPortAsNumber       5353
#define   LoopbackIPCPortAsNumber        5354
//#define MulticastDNSPortAsNumber       5355		// LLMNR
#define   PrivateDNSPortAsNumber         5533

mDNSexport const mDNSIPPort DiscardPort            = { { DiscardPortAsNumber            >> 8, DiscardPortAsNumber            & 0xFF } };
mDNSexport const mDNSIPPort SSHPort                = { { SSHPortAsNumber                >> 8, SSHPortAsNumber                & 0xFF } };
mDNSexport const mDNSIPPort UnicastDNSPort         = { { UnicastDNSPortAsNumber         >> 8, UnicastDNSPortAsNumber         & 0xFF } };
mDNSexport const mDNSIPPort SSDPPort               = { { SSDPPortAsNumber               >> 8, SSDPPortAsNumber               & 0xFF } };
mDNSexport const mDNSIPPort IPSECPort              = { { IPSECPortAsNumber              >> 8, IPSECPortAsNumber              & 0xFF } };
mDNSexport const mDNSIPPort NSIPCPort              = { { NSIPCPortAsNumber              >> 8, NSIPCPortAsNumber              & 0xFF } };
mDNSexport const mDNSIPPort NATPMPAnnouncementPort = { { NATPMPAnnouncementPortAsNumber >> 8, NATPMPAnnouncementPortAsNumber & 0xFF } };
mDNSexport const mDNSIPPort NATPMPPort             = { { NATPMPPortAsNumber             >> 8, NATPMPPortAsNumber             & 0xFF } };
mDNSexport const mDNSIPPort DNSEXTPort             = { { DNSEXTPortAsNumber             >> 8, DNSEXTPortAsNumber             & 0xFF } };
mDNSexport const mDNSIPPort MulticastDNSPort       = { { MulticastDNSPortAsNumber       >> 8, MulticastDNSPortAsNumber       & 0xFF } };
mDNSexport const mDNSIPPort LoopbackIPCPort        = { { LoopbackIPCPortAsNumber        >> 8, LoopbackIPCPortAsNumber        & 0xFF } };
mDNSexport const mDNSIPPort PrivateDNSPort         = { { PrivateDNSPortAsNumber         >> 8, PrivateDNSPortAsNumber         & 0xFF } };

mDNSexport const OwnerOptData    zeroOwner         = { 0, 0, { { 0 } }, { { 0 } }, { { 0 } } };

mDNSexport const mDNSIPPort      zeroIPPort        = { { 0 } };
mDNSexport const mDNSv4Addr      zerov4Addr        = { { 0 } };
mDNSexport const mDNSv6Addr      zerov6Addr        = { { 0 } };
mDNSexport const mDNSEthAddr     zeroEthAddr       = { { 0 } };
mDNSexport const mDNSv4Addr      onesIPv4Addr      = { { 255, 255, 255, 255 } };
mDNSexport const mDNSv6Addr      onesIPv6Addr      = { { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 } };
mDNSexport const mDNSEthAddr     onesEthAddr       = { { 255, 255, 255, 255, 255, 255 } };
mDNSexport const mDNSAddr        zeroAddr          = { mDNSAddrType_None, {{{ 0 }}} };

mDNSexport const mDNSv4Addr  AllDNSAdminGroup   = { { 239, 255, 255, 251 } };
mDNSexport const mDNSv4Addr  AllHosts_v4        = { { 224,   0,   0,   1 } }; // For NAT-PMP Annoucements
mDNSexport const mDNSv6Addr  AllHosts_v6        = { { 0xFF,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01 } };
mDNSexport const mDNSv6Addr  NDP_prefix         = { { 0xFF,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, 0xFF,0x00,0x00,0xFB } }; // FF02:0:0:0:0:1:FF00::/104
mDNSexport const mDNSEthAddr AllHosts_v6_Eth    = { { 0x33, 0x33, 0x00, 0x00, 0x00, 0x01 } };
mDNSexport const mDNSAddr    AllDNSLinkGroup_v4 = { mDNSAddrType_IPv4, { { { 224,   0,   0, 251 } } } };
//mDNSexport const mDNSAddr  AllDNSLinkGroup_v4 = { mDNSAddrType_IPv4, { { { 224,   0,   0, 252 } } } }; // LLMNR
mDNSexport const mDNSAddr    AllDNSLinkGroup_v6 = { mDNSAddrType_IPv6, { { { 0xFF,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xFB } } } };
//mDNSexport const mDNSAddr  AllDNSLinkGroup_v6 = { mDNSAddrType_IPv6, { { { 0xFF,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x01,0x00,0x03 } } } }; // LLMNR

mDNSexport const mDNSOpaque16 zeroID          = { { 0, 0 } };
mDNSexport const mDNSOpaque16 onesID          = { { 255, 255 } };
mDNSexport const mDNSOpaque16 QueryFlags      = { { kDNSFlag0_QR_Query    | kDNSFlag0_OP_StdQuery,                0 } };
mDNSexport const mDNSOpaque16 uQueryFlags     = { { kDNSFlag0_QR_Query    | kDNSFlag0_OP_StdQuery | kDNSFlag0_RD, 0 } };
mDNSexport const mDNSOpaque16 ResponseFlags   = { { kDNSFlag0_QR_Response | kDNSFlag0_OP_StdQuery | kDNSFlag0_AA, 0 } };
mDNSexport const mDNSOpaque16 UpdateReqFlags  = { { kDNSFlag0_QR_Query    | kDNSFlag0_OP_Update,                  0 } };
mDNSexport const mDNSOpaque16 UpdateRespFlags = { { kDNSFlag0_QR_Response | kDNSFlag0_OP_Update,                  0 } };

mDNSexport const mDNSOpaque64 zeroOpaque64    = { { 0 } };

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - General Utility Functions
#endif

// return true for RFC1918 private addresses
mDNSexport mDNSBool mDNSv4AddrIsRFC1918(mDNSv4Addr *addr)
	{
	return ((addr->b[0] == 10) ||                                 // 10/8 prefix
			(addr->b[0] == 172 && (addr->b[1] & 0xF0) == 16) ||   // 172.16/12
			(addr->b[0] == 192 && addr->b[1] == 168));            // 192.168/16
	}

mDNSexport NetworkInterfaceInfo *GetFirstActiveInterface(NetworkInterfaceInfo *intf)
	{
	while (intf && !intf->InterfaceActive) intf = intf->next;
	return(intf);
	}

mDNSexport mDNSInterfaceID GetNextActiveInterfaceID(const NetworkInterfaceInfo *intf)
	{
	const NetworkInterfaceInfo *next = GetFirstActiveInterface(intf->next);
	if (next) return(next->InterfaceID); else return(mDNSNULL);
	}

mDNSexport mDNSu32 NumCacheRecordsForInterfaceID(const mDNS *const m, mDNSInterfaceID id)
	{
	mDNSu32 slot, used = 0;
	CacheGroup *cg;
	const CacheRecord *rr;
	FORALL_CACHERECORDS(slot, cg, rr)
		if (rr->resrec.InterfaceID == id) used++;
	return(used);
	}

mDNSexport char *DNSTypeName(mDNSu16 rrtype)
	{
	switch (rrtype)
		{
		case kDNSType_A:    return("Addr");
		case kDNSType_NS:   return("NS");
		case kDNSType_CNAME:return("CNAME");
		case kDNSType_SOA:  return("SOA");
		case kDNSType_NULL: return("NULL");
		case kDNSType_PTR:  return("PTR");
		case kDNSType_HINFO:return("HINFO");
		case kDNSType_TXT:  return("TXT");
		case kDNSType_AAAA: return("AAAA");
		case kDNSType_SRV:  return("SRV");
		case kDNSType_OPT:  return("OPT");
		case kDNSType_NSEC: return("NSEC");
		case kDNSType_TSIG: return("TSIG");
		case kDNSQType_ANY: return("ANY");
		default:			{
							static char buffer[16];
							mDNS_snprintf(buffer, sizeof(buffer), "(%d)", rrtype);
							return(buffer);
							}
		}
	}

// Note slight bug: this code uses the rdlength from the ResourceRecord object, to display
// the rdata from the RDataBody object. Sometimes this could be the wrong length -- but as
// long as this routine is only used for debugging messages, it probably isn't a big problem.
mDNSexport char *GetRRDisplayString_rdb(const ResourceRecord *const rr, const RDataBody *const rd1, char *const buffer)
	{
	const RDataBody2 *const rd = (RDataBody2 *)rd1;
	#define RemSpc (MaxMsg-1-length)
	char *ptr = buffer;
	mDNSu32 length = mDNS_snprintf(buffer, MaxMsg-1, "%4d %##s %s ", rr->rdlength, rr->name->c, DNSTypeName(rr->rrtype));
	if (rr->RecordType == kDNSRecordTypePacketNegative) return(buffer);
	if (!rr->rdlength) { mDNS_snprintf(buffer+length, RemSpc, "<< ZERO RDATA LENGTH >>"); return(buffer); }
	
	switch (rr->rrtype)
		{
		case kDNSType_A:	mDNS_snprintf(buffer+length, RemSpc, "%.4a", &rd->ipv4);          break;

		case kDNSType_NS:	// Same as PTR
		case kDNSType_CNAME:// Same as PTR
		case kDNSType_PTR:	mDNS_snprintf(buffer+length, RemSpc, "%##s", rd->name.c);       break;

		case kDNSType_SOA:  mDNS_snprintf(buffer+length, RemSpc, "%##s %##s %d %d %d %d %d",
								rd->soa.mname.c, rd->soa.rname.c,
								rd->soa.serial, rd->soa.refresh, rd->soa.retry, rd->soa.expire, rd->soa.min);
							break;

		case kDNSType_HINFO:// Display this the same as TXT (show all constituent strings)
		case kDNSType_TXT:  {
							const mDNSu8 *t = rd->txt.c;
							while (t < rd->txt.c + rr->rdlength)
								{
								length += mDNS_snprintf(buffer+length, RemSpc, "%s%#s", t > rd->txt.c ? "¦" : "", t);
								t += 1 + t[0];
								}
							} break;

		case kDNSType_AAAA:	mDNS_snprintf(buffer+length, RemSpc, "%.16a", &rd->ipv6);       break;
		case kDNSType_SRV:	mDNS_snprintf(buffer+length, RemSpc, "%u %u %u %##s",
								rd->srv.priority, rd->srv.weight, mDNSVal16(rd->srv.port), rd->srv.target.c); break;

		case kDNSType_OPT:  {
							const rdataOPT *opt;
							const rdataOPT *const end = (const rdataOPT *)&rd->data[rr->rdlength];
							length += mDNS_snprintf(buffer+length, RemSpc, "Max %d", rr->rrclass);
							for (opt = &rd->opt[0]; opt < end; opt++)
								{
								switch(opt->opt)
									{
									case kDNSOpt_LLQ:
										length += mDNS_snprintf(buffer+length, RemSpc, " Vers %d",     opt->u.llq.vers);
										length += mDNS_snprintf(buffer+length, RemSpc, " Op %d",       opt->u.llq.llqOp);
										length += mDNS_snprintf(buffer+length, RemSpc, " Err/Port %d", opt->u.llq.err);
										length += mDNS_snprintf(buffer+length, RemSpc, " ID %08X%08X", opt->u.llq.id.l[0], opt->u.llq.id.l[1]);
										length += mDNS_snprintf(buffer+length, RemSpc, " Lease %d",    opt->u.llq.llqlease);
										break;
									case kDNSOpt_Lease:
										length += mDNS_snprintf(buffer+length, RemSpc, " Lease %d",    opt->u.updatelease);
										break;
									case kDNSOpt_Owner:
										length += mDNS_snprintf(buffer+length, RemSpc, " Vers %d",     opt->u.owner.vers);
										length += mDNS_snprintf(buffer+length, RemSpc, " Seq %3d", (mDNSu8)opt->u.owner.seq);	// Display as unsigned
										length += mDNS_snprintf(buffer+length, RemSpc, " MAC %.6a",    opt->u.owner.HMAC.b);
										if (opt->optlen >= DNSOpt_OwnerData_ID_Wake_Space-4)
											{
											length += mDNS_snprintf(buffer+length, RemSpc, " I-MAC %.6a", opt->u.owner.IMAC.b);
											if (opt->optlen > DNSOpt_OwnerData_ID_Wake_Space-4)
												length += mDNS_snprintf(buffer+length, RemSpc, " Password %.6a", opt->u.owner.password.b);
											}
										break;
									default:
										length += mDNS_snprintf(buffer+length, RemSpc, " Unknown %d",  opt->opt);
										break;
									}
								}
							}
							break;

		case kDNSType_NSEC: {
							mDNSu16 i;
							for (i=0; i<255; i++)
								if (rd->nsec.bitmap[i>>3] & (128 >> (i&7)))
									length += mDNS_snprintf(buffer+length, RemSpc, "%s ", DNSTypeName(i));
							}
							break;

		default:			mDNS_snprintf(buffer+length, RemSpc, "RDLen %d: %s", rr->rdlength, rd->data);
							// Really should scan buffer to check if text is valid UTF-8 and only replace with dots if not
							for (ptr = buffer; *ptr; ptr++) if (*ptr < ' ') *ptr = '.';
							break;
		}
	return(buffer);
	}

// See comments in mDNSEmbeddedAPI.h
#if _PLATFORM_HAS_STRONG_PRNG_
#define mDNSRandomNumber mDNSPlatformRandomNumber
#else
mDNSlocal mDNSu32 mDNSRandomFromSeed(mDNSu32 seed)
	{
	return seed * 21 + 1;
	}

mDNSlocal mDNSu32 mDNSMixRandomSeed(mDNSu32 seed, mDNSu8 iteration)
	{
	return iteration ? mDNSMixRandomSeed(mDNSRandomFromSeed(seed), --iteration) : seed;
	}

mDNSlocal mDNSu32 mDNSRandomNumber()
	{
	static mDNSBool seeded = mDNSfalse;
	static mDNSu32 seed = 0;
	if (!seeded)
		{
		seed = mDNSMixRandomSeed(mDNSPlatformRandomSeed(), 100);
		seeded = mDNStrue;
		}
	return (seed = mDNSRandomFromSeed(seed));
	}
#endif // ! _PLATFORM_HAS_STRONG_PRNG_
	
mDNSexport mDNSu32 mDNSRandom(mDNSu32 max)		// Returns pseudo-random result from zero to max inclusive
	{
	mDNSu32 ret = 0;
	mDNSu32 mask = 1;

	while (mask < max) mask = (mask << 1) | 1;

	do ret = mDNSRandomNumber() & mask;
	while (ret > max);

	return ret;
	}

mDNSexport mDNSBool mDNSSameAddress(const mDNSAddr *ip1, const mDNSAddr *ip2)
	{
	if (ip1->type == ip2->type)
		{
		switch (ip1->type)
			{
			case mDNSAddrType_None : return(mDNStrue); // Empty addresses have no data and are therefore always equal
			case mDNSAddrType_IPv4 : return(mDNSBool)(mDNSSameIPv4Address(ip1->ip.v4, ip2->ip.v4));
			case mDNSAddrType_IPv6 : return(mDNSBool)(mDNSSameIPv6Address(ip1->ip.v6, ip2->ip.v6));
			}
		}
	return(mDNSfalse);
	}

mDNSexport mDNSBool mDNSAddrIsDNSMulticast(const mDNSAddr *ip)
	{
	switch(ip->type)
		{
		case mDNSAddrType_IPv4: return(mDNSBool)(mDNSSameIPv4Address(ip->ip.v4, AllDNSLinkGroup_v4.ip.v4));
		case mDNSAddrType_IPv6: return(mDNSBool)(mDNSSameIPv6Address(ip->ip.v6, AllDNSLinkGroup_v6.ip.v6));
		default: return(mDNSfalse);
		}
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Domain Name Utility Functions
#endif

mDNSexport mDNSBool SameDomainLabel(const mDNSu8 *a, const mDNSu8 *b)
	{
	int i;
	const int len = *a++;

	if (len > MAX_DOMAIN_LABEL)
		{ debugf("Malformed label (too long)"); return(mDNSfalse); }

	if (len != *b++) return(mDNSfalse);
	for (i=0; i<len; i++)
		{
		mDNSu8 ac = *a++;
		mDNSu8 bc = *b++;
		if (mDNSIsUpperCase(ac)) ac += 'a' - 'A';
		if (mDNSIsUpperCase(bc)) bc += 'a' - 'A';
		if (ac != bc) return(mDNSfalse);
		}
	return(mDNStrue);
	}

mDNSexport mDNSBool SameDomainName(const domainname *const d1, const domainname *const d2)
	{
	const mDNSu8 *      a   = d1->c;
	const mDNSu8 *      b   = d2->c;
	const mDNSu8 *const max = d1->c + MAX_DOMAIN_NAME;			// Maximum that's valid

	while (*a || *b)
		{
		if (a + 1 + *a >= max)
			{ debugf("Malformed domain name (more than 256 characters)"); return(mDNSfalse); }
		if (!SameDomainLabel(a, b)) return(mDNSfalse);
		a += 1 + *a;
		b += 1 + *b;
		}

	return(mDNStrue);
	}

mDNSexport mDNSBool SameDomainNameCS(const domainname *const d1, const domainname *const d2)
	{
	mDNSu16 l1 = DomainNameLength(d1);
	mDNSu16 l2 = DomainNameLength(d2);
	return(l1 <= MAX_DOMAIN_NAME && l1 == l2 && mDNSPlatformMemSame(d1, d2, l1));
	}

mDNSexport mDNSBool IsLocalDomain(const domainname *d)
	{
	// Domains that are defined to be resolved via link-local multicast are:
	// local., 254.169.in-addr.arpa., and {8,9,A,B}.E.F.ip6.arpa.
	static const domainname *nL = (const domainname*)"\x5" "local";
	static const domainname *nR = (const domainname*)"\x3" "254" "\x3" "169"         "\x7" "in-addr" "\x4" "arpa";
	static const domainname *n8 = (const domainname*)"\x1" "8"   "\x1" "e" "\x1" "f" "\x3" "ip6"     "\x4" "arpa";
	static const domainname *n9 = (const domainname*)"\x1" "9"   "\x1" "e" "\x1" "f" "\x3" "ip6"     "\x4" "arpa";
	static const domainname *nA = (const domainname*)"\x1" "a"   "\x1" "e" "\x1" "f" "\x3" "ip6"     "\x4" "arpa";
	static const domainname *nB = (const domainname*)"\x1" "b"   "\x1" "e" "\x1" "f" "\x3" "ip6"     "\x4" "arpa";

	const domainname *d1, *d2, *d3, *d4, *d5;	// Top-level domain, second-level domain, etc.
	d1 = d2 = d3 = d4 = d5 = mDNSNULL;
	while (d->c[0])
		{
		d5 = d4; d4 = d3; d3 = d2; d2 = d1; d1 = d;
		d = (const domainname*)(d->c + 1 + d->c[0]);
		}

	if (d1 && SameDomainName(d1, nL)) return(mDNStrue);
	if (d4 && SameDomainName(d4, nR)) return(mDNStrue);
	if (d5 && SameDomainName(d5, n8)) return(mDNStrue);
	if (d5 && SameDomainName(d5, n9)) return(mDNStrue);
	if (d5 && SameDomainName(d5, nA)) return(mDNStrue);
	if (d5 && SameDomainName(d5, nB)) return(mDNStrue);
	return(mDNSfalse);
	}

mDNSexport const mDNSu8 *LastLabel(const domainname *d)
	{
	const mDNSu8 *p = d->c;
	while (d->c[0])
		{
		p = d->c;
		d = (const domainname*)(d->c + 1 + d->c[0]);
		}
	return(p);
	}

// Returns length of a domain name INCLUDING the byte for the final null label
// e.g. for the root label "." it returns one
// For the FQDN "com." it returns 5 (length byte, three data bytes, final zero)
// Legal results are 1 (just root label) to 256 (MAX_DOMAIN_NAME)
// If the given domainname is invalid, result is 257 (MAX_DOMAIN_NAME+1)
mDNSexport mDNSu16 DomainNameLengthLimit(const domainname *const name, const mDNSu8 *limit)
	{
	const mDNSu8 *src = name->c;
	while (src < limit && *src <= MAX_DOMAIN_LABEL)
		{
		if (*src == 0) return((mDNSu16)(src - name->c + 1));
		src += 1 + *src;
		}
	return(MAX_DOMAIN_NAME+1);
	}

// CompressedDomainNameLength returns the length of a domain name INCLUDING the byte
// for the final null label, e.g. for the root label "." it returns one.
// E.g. for the FQDN "foo.com." it returns 9
// (length, three data bytes, length, three more data bytes, final zero).
// In the case where a parent domain name is provided, and the given name is a child
// of that parent, CompressedDomainNameLength returns the length of the prefix portion
// of the child name, plus TWO bytes for the compression pointer.
// E.g. for the name "foo.com." with parent "com.", it returns 6
// (length, three data bytes, two-byte compression pointer).
mDNSexport mDNSu16 CompressedDomainNameLength(const domainname *const name, const domainname *parent)
	{
	const mDNSu8 *src = name->c;
	if (parent && parent->c[0] == 0) parent = mDNSNULL;
	while (*src)
		{
		if (*src > MAX_DOMAIN_LABEL) return(MAX_DOMAIN_NAME+1);
		if (parent && SameDomainName((const domainname *)src, parent)) return((mDNSu16)(src - name->c + 2));
		src += 1 + *src;
		if (src - name->c >= MAX_DOMAIN_NAME) return(MAX_DOMAIN_NAME+1);
		}
	return((mDNSu16)(src - name->c + 1));
	}

// CountLabels() returns number of labels in name, excluding final root label
// (e.g. for "apple.com." CountLabels returns 2.)
mDNSexport int CountLabels(const domainname *d)
	{
	int count = 0;
	const mDNSu8 *ptr;
	for (ptr = d->c; *ptr; ptr = ptr + ptr[0] + 1) count++;
	return count;
	}

// SkipLeadingLabels skips over the first 'skip' labels in the domainname,
// returning a pointer to the suffix with 'skip' labels removed.
mDNSexport const domainname *SkipLeadingLabels(const domainname *d, int skip)
	{
	while (skip > 0 && d->c[0]) { d = (const domainname *)(d->c + 1 + d->c[0]); skip--; }
	return(d);
	}

// AppendLiteralLabelString appends a single label to an existing (possibly empty) domainname.
// The C string contains the label as-is, with no escaping, etc.
// Any dots in the name are literal dots, not label separators
// If successful, AppendLiteralLabelString returns a pointer to the next unused byte
// in the domainname bufer (i.e. the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 256 bytes)
// AppendLiteralLabelString returns mDNSNULL.
mDNSexport mDNSu8 *AppendLiteralLabelString(domainname *const name, const char *cstr)
	{
	mDNSu8       *      ptr  = name->c + DomainNameLength(name) - 1;	// Find end of current name
	const mDNSu8 *const lim1 = name->c + MAX_DOMAIN_NAME - 1;			// Limit of how much we can add (not counting final zero)
	const mDNSu8 *const lim2 = ptr + 1 + MAX_DOMAIN_LABEL;
	const mDNSu8 *const lim  = (lim1 < lim2) ? lim1 : lim2;
	mDNSu8       *lengthbyte = ptr++;									// Record where the length is going to go

	while (*cstr && ptr < lim) *ptr++ = (mDNSu8)*cstr++;	// Copy the data
	*lengthbyte = (mDNSu8)(ptr - lengthbyte - 1);			// Fill in the length byte
	*ptr++ = 0;												// Put the null root label on the end
	if (*cstr) return(mDNSNULL);							// Failure: We didn't successfully consume all input
	else return(ptr);										// Success: return new value of ptr
	}

// AppendDNSNameString appends zero or more labels to an existing (possibly empty) domainname.
// The C string is in conventional DNS syntax:
// Textual labels, escaped as necessary using the usual DNS '\' notation, separated by dots.
// If successful, AppendDNSNameString returns a pointer to the next unused byte
// in the domainname bufer (i.e. the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 256 bytes)
// AppendDNSNameString returns mDNSNULL.
mDNSexport mDNSu8 *AppendDNSNameString(domainname *const name, const char *cstring)
	{
	const char   *cstr      = cstring;
	mDNSu8       *      ptr = name->c + DomainNameLength(name) - 1;	// Find end of current name
	const mDNSu8 *const lim = name->c + MAX_DOMAIN_NAME - 1;		// Limit of how much we can add (not counting final zero)
	while (*cstr && ptr < lim)										// While more characters, and space to put them...
		{
		mDNSu8 *lengthbyte = ptr++;									// Record where the length is going to go
		if (*cstr == '.') { LogMsg("AppendDNSNameString: Illegal empty label in name \"%s\"", cstring); return(mDNSNULL); }
		while (*cstr && *cstr != '.' && ptr < lim)					// While we have characters in the label...
			{
			mDNSu8 c = (mDNSu8)*cstr++;								// Read the character
			if (c == '\\')											// If escape character, check next character
				{
				c = (mDNSu8)*cstr++;								// Assume we'll just take the next character
				if (mDNSIsDigit(cstr[-1]) && mDNSIsDigit(cstr[0]) && mDNSIsDigit(cstr[1]))
					{												// If three decimal digits,
					int v0 = cstr[-1] - '0';						// then interpret as three-digit decimal
					int v1 = cstr[ 0] - '0';
					int v2 = cstr[ 1] - '0';
					int val = v0 * 100 + v1 * 10 + v2;
					if (val <= 255) { c = (mDNSu8)val; cstr += 2; }	// If valid three-digit decimal value, use it
					}
				}
			*ptr++ = c;												// Write the character
			}
		if (*cstr) cstr++;											// Skip over the trailing dot (if present)
		if (ptr - lengthbyte - 1 > MAX_DOMAIN_LABEL)				// If illegal label, abort
			return(mDNSNULL);
		*lengthbyte = (mDNSu8)(ptr - lengthbyte - 1);				// Fill in the length byte
		}

	*ptr++ = 0;														// Put the null root label on the end
	if (*cstr) return(mDNSNULL);									// Failure: We didn't successfully consume all input
	else return(ptr);												// Success: return new value of ptr
	}

// AppendDomainLabel appends a single label to a name.
// If successful, AppendDomainLabel returns a pointer to the next unused byte
// in the domainname bufer (i.e. the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 256 bytes)
// AppendDomainLabel returns mDNSNULL.
mDNSexport mDNSu8 *AppendDomainLabel(domainname *const name, const domainlabel *const label)
	{
	int i;
	mDNSu8 *ptr = name->c + DomainNameLength(name) - 1;

	// Check label is legal
	if (label->c[0] > MAX_DOMAIN_LABEL) return(mDNSNULL);

	// Check that ptr + length byte + data bytes + final zero does not exceed our limit
	if (ptr + 1 + label->c[0] + 1 > name->c + MAX_DOMAIN_NAME) return(mDNSNULL);

	for (i=0; i<=label->c[0]; i++) *ptr++ = label->c[i];	// Copy the label data
	*ptr++ = 0;								// Put the null root label on the end
	return(ptr);
	}

mDNSexport mDNSu8 *AppendDomainName(domainname *const name, const domainname *const append)
	{
	mDNSu8       *      ptr = name->c + DomainNameLength(name) - 1;	// Find end of current name
	const mDNSu8 *const lim = name->c + MAX_DOMAIN_NAME - 1;		// Limit of how much we can add (not counting final zero)
	const mDNSu8 *      src = append->c;
	while (src[0])
		{
		int i;
		if (ptr + 1 + src[0] > lim) return(mDNSNULL);
		for (i=0; i<=src[0]; i++) *ptr++ = src[i];
		*ptr = 0;	// Put the null root label on the end
		src += i;
		}
	return(ptr);
	}

// MakeDomainLabelFromLiteralString makes a single domain label from a single literal C string (with no escaping).
// If successful, MakeDomainLabelFromLiteralString returns mDNStrue.
// If unable to convert the whole string to a legal domain label (i.e. because length is more than 63 bytes) then
// MakeDomainLabelFromLiteralString makes a legal domain label from the first 63 bytes of the string and returns mDNSfalse.
// In some cases silently truncated oversized names to 63 bytes is acceptable, so the return result may be ignored.
// In other cases silent truncation may not be acceptable, so in those cases the calling function needs to check the return result.
mDNSexport mDNSBool MakeDomainLabelFromLiteralString(domainlabel *const label, const char *cstr)
	{
	mDNSu8       *      ptr   = label->c + 1;						// Where we're putting it
	const mDNSu8 *const limit = label->c + 1 + MAX_DOMAIN_LABEL;	// The maximum we can put
	while (*cstr && ptr < limit) *ptr++ = (mDNSu8)*cstr++;			// Copy the label
	label->c[0] = (mDNSu8)(ptr - label->c - 1);						// Set the length byte
	return(*cstr == 0);												// Return mDNStrue if we successfully consumed all input
	}

// MakeDomainNameFromDNSNameString makes a native DNS-format domainname from a C string.
// The C string is in conventional DNS syntax:
// Textual labels, escaped as necessary using the usual DNS '\' notation, separated by dots.
// If successful, MakeDomainNameFromDNSNameString returns a pointer to the next unused byte
// in the domainname bufer (i.e. the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 256 bytes)
// MakeDomainNameFromDNSNameString returns mDNSNULL.
mDNSexport mDNSu8 *MakeDomainNameFromDNSNameString(domainname *const name, const char *cstr)
	{
	name->c[0] = 0;									// Make an empty domain name
	return(AppendDNSNameString(name, cstr));		// And then add this string to it
	}

mDNSexport char *ConvertDomainLabelToCString_withescape(const domainlabel *const label, char *ptr, char esc)
	{
	const mDNSu8 *      src = label->c;							// Domain label we're reading
	const mDNSu8        len = *src++;							// Read length of this (non-null) label
	const mDNSu8 *const end = src + len;						// Work out where the label ends
	if (len > MAX_DOMAIN_LABEL) return(mDNSNULL);				// If illegal label, abort
	while (src < end)											// While we have characters in the label
		{
		mDNSu8 c = *src++;
		if (esc)
			{
			if (c == '.' || c == esc)							// If character is a dot or the escape character
				*ptr++ = esc;									// Output escape character
			else if (c <= ' ')									// If non-printing ascii,
				{												// Output decimal escape sequence
				*ptr++ = esc;
				*ptr++ = (char)  ('0' + (c / 100)     );
				*ptr++ = (char)  ('0' + (c /  10) % 10);
				c      = (mDNSu8)('0' + (c      ) % 10);
				}
			}
		*ptr++ = (char)c;										// Copy the character
		}
	*ptr = 0;													// Null-terminate the string
	return(ptr);												// and return
	}

// Note: To guarantee that there will be no possible overrun, cstr must be at least MAX_ESCAPED_DOMAIN_NAME (1009 bytes)
mDNSexport char *ConvertDomainNameToCString_withescape(const domainname *const name, char *ptr, char esc)
	{
	const mDNSu8 *src         = name->c;							// Domain name we're reading
	const mDNSu8 *const max   = name->c + MAX_DOMAIN_NAME;			// Maximum that's valid

	if (*src == 0) *ptr++ = '.';									// Special case: For root, just write a dot

	while (*src)													// While more characters in the domain name
		{
		if (src + 1 + *src >= max) return(mDNSNULL);
		ptr = ConvertDomainLabelToCString_withescape((const domainlabel *)src, ptr, esc);
		if (!ptr) return(mDNSNULL);
		src += 1 + *src;
		*ptr++ = '.';												// Write the dot after the label
		}

	*ptr++ = 0;														// Null-terminate the string
	return(ptr);													// and return
	}

// RFC 1034 rules:
// Host names must start with a letter, end with a letter or digit,
// and have as interior characters only letters, digits, and hyphen.
// This was subsequently modified in RFC 1123 to allow the first character to be either a letter or a digit

mDNSexport void ConvertUTF8PstringToRFC1034HostLabel(const mDNSu8 UTF8Name[], domainlabel *const hostlabel)
	{
	const mDNSu8 *      src  = &UTF8Name[1];
	const mDNSu8 *const end  = &UTF8Name[1] + UTF8Name[0];
	      mDNSu8 *      ptr  = &hostlabel->c[1];
	const mDNSu8 *const lim  = &hostlabel->c[1] + MAX_DOMAIN_LABEL;
	while (src < end)
		{
		// Delete apostrophes from source name
		if (src[0] == '\'') { src++; continue; }		// Standard straight single quote
		if (src + 2 < end && src[0] == 0xE2 && src[1] == 0x80 && src[2] == 0x99)
			{ src += 3; continue; }	// Unicode curly apostrophe
		if (ptr < lim)
			{
			if (mDNSValidHostChar(*src, (ptr > &hostlabel->c[1]), (src < end-1))) *ptr++ = *src;
			else if (ptr > &hostlabel->c[1] && ptr[-1] != '-') *ptr++ = '-';
			}
		src++;
		}
	while (ptr > &hostlabel->c[1] && ptr[-1] == '-') ptr--;	// Truncate trailing '-' marks
	hostlabel->c[0] = (mDNSu8)(ptr - &hostlabel->c[1]);
	}

#define ValidTransportProtocol(X) ( (X)[0] == 4 && (X)[1] == '_' && \
	((((X)[2] | 0x20) == 'u' && ((X)[3] | 0x20) == 'd') || (((X)[2] | 0x20) == 't' && ((X)[3] | 0x20) == 'c')) && \
	((X)[4] | 0x20) == 'p')

mDNSexport mDNSu8 *ConstructServiceName(domainname *const fqdn,
	const domainlabel *name, const domainname *type, const domainname *const domain)
	{
	int i, len;
	mDNSu8 *dst = fqdn->c;
	const mDNSu8 *src;
	const char *errormsg;
#if APPLE_OSX_mDNSResponder
	mDNSBool	loggedUnderscore = mDNSfalse;
	static char typeBuf[MAX_ESCAPED_DOMAIN_NAME];
#endif

	// In the case where there is no name (and ONLY in that case),
	// a single-label subtype is allowed as the first label of a three-part "type"
	if (!name && type)
		{
		const mDNSu8 *s0 = type->c;
		if (s0[0] && s0[0] < 0x40)		// If legal first label (at least one character, and no more than 63)
			{
			const mDNSu8 * s1 = s0 + 1 + s0[0];
			if (s1[0] && s1[0] < 0x40)	// and legal second label (at least one character, and no more than 63)
				{
				const mDNSu8 *s2 = s1 + 1 + s1[0];
				if (s2[0] && s2[0] < 0x40 && s2[1+s2[0]] == 0)	// and we have three and only three labels
					{
					static const mDNSu8 SubTypeLabel[5] = "\x04_sub";
					src = s0;									// Copy the first label
					len = *src;
					for (i=0; i <= len;                      i++) *dst++ = *src++;
					for (i=0; i < (int)sizeof(SubTypeLabel); i++) *dst++ = SubTypeLabel[i];
					type = (const domainname *)s1;
					
					// Special support to enable the DNSServiceBrowse call made by Bonjour Browser
					// For these queries, we retract the "._sub" we just added between the subtype and the main type
					// Remove after Bonjour Browser is updated to use DNSServiceQueryRecord instead of DNSServiceBrowse
					if (SameDomainName((domainname*)s0, (const domainname*)"\x09_services\x07_dns-sd\x04_udp"))
						dst -= sizeof(SubTypeLabel);
					}
				}
			}
		}

	if (name && name->c[0])
		{
		src = name->c;									// Put the service name into the domain name
		len = *src;
		if (len >= 0x40) { errormsg = "Service instance name too long"; goto fail; }
		for (i=0; i<=len; i++) *dst++ = *src++;
		}
	else
		name = (domainlabel*)"";	// Set this up to be non-null, to avoid errors if we have to call LogMsg() below

	src = type->c;										// Put the service type into the domain name
	len = *src;
	if (len < 2 || len > 16)
		{
		LogMsg("Bad service type in %#s.%##s%##s Application protocol name must be underscore plus 1-15 characters. "
			"See <http://www.dns-sd.org/ServiceTypes.html>", name->c, type->c, domain->c);
#if APPLE_OSX_mDNSResponder
		ConvertDomainNameToCString(type, typeBuf);
		mDNSASLLog(mDNSNULL, "serviceType.nameTooLong", "noop", typeBuf, "");
#endif
		}
	if (len < 2 || len >= 0x40 || (len > 16 && !SameDomainName(domain, &localdomain))) return(mDNSNULL);
	if (src[1] != '_') { errormsg = "Application protocol name must begin with underscore"; goto fail; }
	for (i=2; i<=len; i++)
		{
		// Letters and digits are allowed anywhere
		if (mDNSIsLetter(src[i]) || mDNSIsDigit(src[i])) continue;
		// Hyphens are only allowed as interior characters
		// Underscores are not supposed to be allowed at all, but for backwards compatibility with some old products we do allow them,
		// with the same rule as hyphens
		if ((src[i] == '-' || src[i] == '_') && i > 2 && i < len) 
			{
#if APPLE_OSX_mDNSResponder
			if (src[i] == '_' && loggedUnderscore == mDNSfalse)
				{
				ConvertDomainNameToCString(type, typeBuf);
				mDNSASLLog(mDNSNULL, "serviceType.nameWithUnderscore", "noop", typeBuf, "");
				loggedUnderscore = mDNStrue;
				}
#endif
			continue;
			}
		errormsg = "Application protocol name must contain only letters, digits, and hyphens";
#if APPLE_OSX_mDNSResponder
		{
		ConvertDomainNameToCString(type, typeBuf);
		mDNSASLLog(mDNSNULL, "serviceType.nameWithIllegalCharacters", "noop", typeBuf, "");
		}
#endif
		 goto fail;
		}
	for (i=0; i<=len; i++) *dst++ = *src++;

	len = *src;
	if (!ValidTransportProtocol(src)) { errormsg = "Transport protocol name must be _udp or _tcp"; goto fail; }
	for (i=0; i<=len; i++) *dst++ = *src++;

	if (*src) { errormsg = "Service type must have only two labels"; goto fail; }

	*dst = 0;
	if (!domain->c[0]) { errormsg = "Service domain must be non-empty"; goto fail; }
	if (SameDomainName(domain, (const domainname*)"\x05" "local" "\x04" "arpa"))
		{ errormsg = "Illegal domain \"local.arpa.\" Use \"local.\" (or empty string)"; goto fail; }
	dst = AppendDomainName(fqdn, domain);
	if (!dst) { errormsg = "Service domain too long"; goto fail; }
	return(dst);

fail:
	LogMsg("ConstructServiceName: %s: %#s.%##s%##s", errormsg, name->c, type->c, domain->c);
	return(mDNSNULL);
	}

// A service name has the form: instance.application-protocol.transport-protocol.domain
// DeconstructServiceName is currently fairly forgiving: It doesn't try to enforce character
// set or length limits for the protocol names, and the final domain is allowed to be empty.
// However, if the given FQDN doesn't contain at least three labels,
// DeconstructServiceName will reject it and return mDNSfalse.
mDNSexport mDNSBool DeconstructServiceName(const domainname *const fqdn,
	domainlabel *const name, domainname *const type, domainname *const domain)
	{
	int i, len;
	const mDNSu8 *src = fqdn->c;
	const mDNSu8 *max = fqdn->c + MAX_DOMAIN_NAME;
	mDNSu8 *dst;

	dst = name->c;										// Extract the service name
	len = *src;
	if (!len)         { debugf("DeconstructServiceName: FQDN empty!");                             return(mDNSfalse); }
	if (len >= 0x40)  { debugf("DeconstructServiceName: Instance name too long");                  return(mDNSfalse); }
	for (i=0; i<=len; i++) *dst++ = *src++;

	dst = type->c;										// Extract the service type
	len = *src;
	if (!len)         { debugf("DeconstructServiceName: FQDN contains only one label!");           return(mDNSfalse); }
	if (len >= 0x40)  { debugf("DeconstructServiceName: Application protocol name too long");      return(mDNSfalse); }
	if (src[1] != '_'){ debugf("DeconstructServiceName: No _ at start of application protocol");   return(mDNSfalse); }
	for (i=0; i<=len; i++) *dst++ = *src++;

	len = *src;
	if (!len)         { debugf("DeconstructServiceName: FQDN contains only two labels!");          return(mDNSfalse); }
	if (!ValidTransportProtocol(src))
	                  { debugf("DeconstructServiceName: Transport protocol must be _udp or _tcp"); return(mDNSfalse); }
	for (i=0; i<=len; i++) *dst++ = *src++;
	*dst++ = 0;											// Put terminator on the end of service type

	dst = domain->c;									// Extract the service domain
	while (*src)
		{
		len = *src;
		if (len >= 0x40)
			{ debugf("DeconstructServiceName: Label in service domain too long"); return(mDNSfalse); }
		if (src + 1 + len + 1 >= max)
			{ debugf("DeconstructServiceName: Total service domain too long"); return(mDNSfalse); }
		for (i=0; i<=len; i++) *dst++ = *src++;
		}
	*dst++ = 0;		// Put the null root label on the end

	return(mDNStrue);
	}

// Notes on UTF-8:
// 0xxxxxxx represents a 7-bit ASCII value from 0x00 to 0x7F
// 10xxxxxx is a continuation byte of a multi-byte character
// 110xxxxx is the first byte of a 2-byte character (11 effective bits; values 0x     80 - 0x     800-1)
// 1110xxxx is the first byte of a 3-byte character (16 effective bits; values 0x    800 - 0x   10000-1)
// 11110xxx is the first byte of a 4-byte character (21 effective bits; values 0x  10000 - 0x  200000-1)
// 111110xx is the first byte of a 5-byte character (26 effective bits; values 0x 200000 - 0x 4000000-1)
// 1111110x is the first byte of a 6-byte character (31 effective bits; values 0x4000000 - 0x80000000-1)
//
// UTF-16 surrogate pairs are used in UTF-16 to encode values larger than 0xFFFF.
// Although UTF-16 surrogate pairs are not supposed to appear in legal UTF-8, we want to be defensive
// about that too. (See <http://www.unicode.org/faq/utf_bom.html#34>, "What are surrogates?")
// The first of pair is a UTF-16 value in the range 0xD800-0xDBFF (11101101 1010xxxx 10xxxxxx in UTF-8),
// and the second    is a UTF-16 value in the range 0xDC00-0xDFFF (11101101 1011xxxx 10xxxxxx in UTF-8).

mDNSexport mDNSu32 TruncateUTF8ToLength(mDNSu8 *string, mDNSu32 length, mDNSu32 max)
	{
	if (length > max)
		{
		mDNSu8 c1 = string[max];										// First byte after cut point
		mDNSu8 c2 = (max+1 < length) ? string[max+1] : (mDNSu8)0xB0;	// Second byte after cut point
		length = max;	// Trim length down
		while (length > 0)
			{
			// Check if the byte right after the chop point is a UTF-8 continuation byte,
			// or if the character right after the chop point is the second of a UTF-16 surrogate pair.
			// If so, then we continue to chop more bytes until we get to a legal chop point.
			mDNSBool continuation    = ((c1 & 0xC0) == 0x80);
			mDNSBool secondsurrogate = (c1 == 0xED && (c2 & 0xF0) == 0xB0);
			if (!continuation && !secondsurrogate) break;
			c2 = c1;
			c1 = string[--length];
			}
		// Having truncated characters off the end of our string, also cut off any residual white space
		while (length > 0 && string[length-1] <= ' ') length--;
		}
	return(length);
	}

// Returns true if a rich text label ends in " (nnn)", or if an RFC 1034
// name ends in "-nnn", where n is some decimal number.
mDNSexport mDNSBool LabelContainsSuffix(const domainlabel *const name, const mDNSBool RichText)
	{
	mDNSu16 l = name->c[0];

	if (RichText)
		{
		if (l < 4) return mDNSfalse;							// Need at least " (2)"
		if (name->c[l--] != ')') return mDNSfalse;				// Last char must be ')'
		if (!mDNSIsDigit(name->c[l])) return mDNSfalse;			// Preceeded by a digit
		l--;
		while (l > 2 && mDNSIsDigit(name->c[l])) l--;			// Strip off digits
		return (name->c[l] == '(' && name->c[l - 1] == ' ');
		}
	else
		{
		if (l < 2) return mDNSfalse;							// Need at least "-2"
		if (!mDNSIsDigit(name->c[l])) return mDNSfalse;			// Last char must be a digit
		l--;
		while (l > 2 && mDNSIsDigit(name->c[l])) l--;			// Strip off digits
		return (name->c[l] == '-');
		}
	}

// removes an auto-generated suffix (appended on a name collision) from a label.  caller is
// responsible for ensuring that the label does indeed contain a suffix.  returns the number
// from the suffix that was removed.
mDNSexport mDNSu32 RemoveLabelSuffix(domainlabel *name, mDNSBool RichText)
	{
	mDNSu32 val = 0, multiplier = 1;

	// Chop closing parentheses from RichText suffix
	if (RichText && name->c[0] >= 1 && name->c[name->c[0]] == ')') name->c[0]--;

	// Get any existing numerical suffix off the name
	while (mDNSIsDigit(name->c[name->c[0]]))
		{ val += (name->c[name->c[0]] - '0') * multiplier; multiplier *= 10; name->c[0]--; }

	// Chop opening parentheses or dash from suffix
	if (RichText)
		{
		if (name->c[0] >= 2 && name->c[name->c[0]] == '(' && name->c[name->c[0]-1] == ' ') name->c[0] -= 2;
		}
	else
		{
		if (name->c[0] >= 1 && name->c[name->c[0]] == '-') name->c[0] -= 1;
		}

	return(val);
	}

// appends a numerical suffix to a label, with the number following a whitespace and enclosed
// in parentheses (rich text) or following two consecutive hyphens (RFC 1034 domain label).
mDNSexport void AppendLabelSuffix(domainlabel *const name, mDNSu32 val, const mDNSBool RichText)
	{
	mDNSu32 divisor = 1, chars = 2;	// Shortest possible RFC1034 name suffix is 2 characters ("-2")
	if (RichText) chars = 4;		// Shortest possible RichText suffix is 4 characters (" (2)")

	// Truncate trailing spaces from RichText names
	if (RichText) while (name->c[name->c[0]] == ' ') name->c[0]--;

	while (divisor < 0xFFFFFFFFUL/10 && val >= divisor * 10) { divisor *= 10; chars++; }

	name->c[0] = (mDNSu8) TruncateUTF8ToLength(name->c+1, name->c[0], MAX_DOMAIN_LABEL - chars);

	if (RichText) { name->c[++name->c[0]] = ' '; name->c[++name->c[0]] = '('; }
	else          { name->c[++name->c[0]] = '-'; }

	while (divisor)
		{
		name->c[++name->c[0]] = (mDNSu8)('0' + val / divisor);
		val     %= divisor;
		divisor /= 10;
		}

	if (RichText) name->c[++name->c[0]] = ')';
	}

mDNSexport void IncrementLabelSuffix(domainlabel *name, mDNSBool RichText)
	{
	mDNSu32 val = 0;

	if (LabelContainsSuffix(name, RichText))
		val = RemoveLabelSuffix(name, RichText);

	// If no existing suffix, start by renaming "Foo" as "Foo (2)" or "Foo-2" as appropriate.
	// If existing suffix in the range 2-9, increment it.
	// If we've had ten conflicts already, there are probably too many hosts trying to use the same name,
	// so add a random increment to improve the chances of finding an available name next time.
	if      (val == 0) val = 2;
	else if (val < 10) val++;
	else               val += 1 + mDNSRandom(99);

	AppendLabelSuffix(name, val, RichText);
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Resource Record Utility Functions
#endif

// Set up a AuthRecord with sensible default values.
// These defaults may be overwritten with new values before mDNS_Register is called
mDNSexport void mDNS_SetupResourceRecord(AuthRecord *rr, RData *RDataStorage, mDNSInterfaceID InterfaceID,
	mDNSu16 rrtype, mDNSu32 ttl, mDNSu8 RecordType, AuthRecType artype, mDNSRecordCallback Callback, void *Context)
	{
	//
	// LocalOnly auth record can be created with LocalOnly InterfaceID or a valid InterfaceID.
	// Most of the applications normally create with LocalOnly InterfaceID and we store them as
	// such, so that we can deliver the response to questions that specify LocalOnly InterfaceID.
	// LocalOnly resource records can also be created with valid InterfaceID which happens today
	// when we create LocalOnly records for /etc/hosts.

	if (InterfaceID == mDNSInterface_LocalOnly && artype != AuthRecordLocalOnly)
		{
		LogMsg("mDNS_SetupResourceRecord: ERROR!! Mismatch LocalOnly record InterfaceID %p called with artype %d", InterfaceID, artype);
		return;
		}
	else if (InterfaceID == mDNSInterface_P2P && artype != AuthRecordP2P)
		{
		LogMsg("mDNS_SetupResourceRecord: ERROR!! Mismatch P2P record InterfaceID %p called with artype %d", InterfaceID, artype);
		return;
		}
	else if (!InterfaceID && (artype == AuthRecordP2P || artype == AuthRecordLocalOnly))
		{
		LogMsg("mDNS_SetupResourceRecord: ERROR!! Mismatch InterfaceAny record InterfaceID %p called with artype %d", InterfaceID, artype);
		return;
		}

	// Don't try to store a TTL bigger than we can represent in platform time units
	if (ttl > 0x7FFFFFFFUL / mDNSPlatformOneSecond)
		ttl = 0x7FFFFFFFUL / mDNSPlatformOneSecond;
	else if (ttl == 0)		// And Zero TTL is illegal
		ttl = DefaultTTLforRRType(rrtype);

	// Field Group 1: The actual information pertaining to this resource record
	rr->resrec.RecordType        = RecordType;
	rr->resrec.InterfaceID       = InterfaceID;
	rr->resrec.name              = &rr->namestorage;
	rr->resrec.rrtype            = rrtype;
	rr->resrec.rrclass           = kDNSClass_IN;
	rr->resrec.rroriginalttl     = ttl;
	rr->resrec.rDNSServer		 = mDNSNULL;
//	rr->resrec.rdlength          = MUST set by client and/or in mDNS_Register_internal
//	rr->resrec.rdestimate        = set in mDNS_Register_internal
//	rr->resrec.rdata             = MUST be set by client

	if (RDataStorage)
		rr->resrec.rdata = RDataStorage;
	else
		{
		rr->resrec.rdata = &rr->rdatastorage;
		rr->resrec.rdata->MaxRDLength = sizeof(RDataBody);
		}

	// Field Group 2: Persistent metadata for Authoritative Records
	rr->Additional1       = mDNSNULL;
	rr->Additional2       = mDNSNULL;
	rr->DependentOn       = mDNSNULL;
	rr->RRSet             = mDNSNULL;
	rr->RecordCallback    = Callback;
	rr->RecordContext     = Context;

	rr->AutoTarget        = Target_Manual;
	rr->AllowRemoteQuery  = mDNSfalse;
	rr->ForceMCast        = mDNSfalse;

	rr->WakeUp            = zeroOwner;
	rr->AddressProxy      = zeroAddr;
	rr->TimeRcvd          = 0;
	rr->TimeExpire        = 0;
	rr->ARType            = artype;

	// Field Group 3: Transient state for Authoritative Records (set in mDNS_Register_internal)
	// Field Group 4: Transient uDNS state for Authoritative Records (set in mDNS_Register_internal)

	// For now, until the uDNS code is fully integrated, it's helpful to zero the uDNS state fields here too, just in case
	// (e.g. uDNS_RegisterService short-circuits the usual mDNS_Register_internal record registration calls, so a bunch
	// of fields don't get set up properly. In particular, if we don't zero rr->QueuedRData then the uDNS code crashes.)
	rr->state             = regState_Zero;
	rr->uselease          = 0;
	rr->expire            = 0;
	rr->Private           = 0;
	rr->updateid          = zeroID;
	rr->zone              = rr->resrec.name;
	rr->nta               = mDNSNULL;
	rr->tcp               = mDNSNULL;
	rr->OrigRData         = 0;
	rr->OrigRDLen         = 0;
	rr->InFlightRData     = 0;
	rr->InFlightRDLen     = 0;
	rr->QueuedRData       = 0;
	rr->QueuedRDLen       = 0;	
	mDNSPlatformMemZero(&rr->NATinfo, sizeof(rr->NATinfo));
	rr->SRVChanged = mDNSfalse;
	rr->mState = mergeState_Zero;

	rr->namestorage.c[0]  = 0;		// MUST be set by client before calling mDNS_Register()
	}

mDNSexport void mDNS_SetupQuestion(DNSQuestion *const q, const mDNSInterfaceID InterfaceID, const domainname *const name,
               const mDNSu16 qtype, mDNSQuestionCallback *const callback, void *const context)
	{
	q->InterfaceID         = InterfaceID;
	q->Target              = zeroAddr;
	AssignDomainName(&q->qname, name);
	q->qtype               = qtype;
	q->qclass              = kDNSClass_IN;
	q->LongLived           = (qtype == kDNSType_PTR);
	q->ExpectUnique        = (qtype != kDNSType_PTR);
	q->ForceMCast          = mDNSfalse;
	q->ReturnIntermed      = mDNSfalse;
	q->SuppressUnusable    = mDNSfalse;
	q->SearchListIndex     = 0;
	q->AppendSearchDomains = 0;
	q->RetryWithSearchDomains = mDNSfalse;
	q->TimeoutQuestion     = 0;
	q->WakeOnResolve       = 0;
	q->qnameOrig           = mDNSNULL;
	q->QuestionCallback    = callback;
	q->QuestionContext     = context;
	}

mDNSexport mDNSu32 RDataHashValue(const ResourceRecord *const rr)
	{
	int len = rr->rdlength;
	const RDataBody2 *const rdb = (RDataBody2 *)rr->rdata->u.data;
	switch(rr->rrtype)
		{
		case kDNSType_NS:
		case kDNSType_CNAME:
		case kDNSType_PTR:
		case kDNSType_DNAME: return DomainNameHashValue(&rdb->name);

		case kDNSType_SOA:   return rdb->soa.serial  +
									rdb->soa.refresh +
									rdb->soa.retry   +
									rdb->soa.expire  +
									rdb->soa.min     +
									DomainNameHashValue(&rdb->soa.mname) +
									DomainNameHashValue(&rdb->soa.rname);

		case kDNSType_MX:
		case kDNSType_AFSDB:
		case kDNSType_RT:
		case kDNSType_KX:	 return DomainNameHashValue(&rdb->mx.exchange);

		case kDNSType_RP:	 return DomainNameHashValue(&rdb->rp.mbox)   + DomainNameHashValue(&rdb->rp.txt);

		case kDNSType_PX:	 return DomainNameHashValue(&rdb->px.map822) + DomainNameHashValue(&rdb->px.mapx400);

		case kDNSType_SRV:	 return DomainNameHashValue(&rdb->srv.target);

		case kDNSType_OPT:	 return 0;	// OPT is a pseudo-RR container structure; makes no sense to compare

		case kDNSType_NSEC:	 len = sizeof(rdataNSEC);	// Use in-memory length of 32, and fall through default checksum computation below

		default:
			{
			mDNSu32 sum = 0;
			int i;
			for (i=0; i+1 < len; i+=2)
				{
				sum += (((mDNSu32)(rdb->data[i])) << 8) | rdb->data[i+1];
				sum = (sum<<3) | (sum>>29);
				}
			if (i < len)
				{
				sum += ((mDNSu32)(rdb->data[i])) << 8;
				}
			return(sum);
			}
		}
	}

// r1 has to be a full ResourceRecord including rrtype and rdlength
// r2 is just a bare RDataBody, which MUST be the same rrtype and rdlength as r1
mDNSexport mDNSBool SameRDataBody(const ResourceRecord *const r1, const RDataBody *const r2, DomainNameComparisonFn *samename)
	{
	const RDataBody2 *const b1 = (RDataBody2 *)r1->rdata->u.data;
	const RDataBody2 *const b2 = (RDataBody2 *)r2;
	switch(r1->rrtype)
		{
		case kDNSType_NS:
		case kDNSType_CNAME:
		case kDNSType_PTR:
		case kDNSType_DNAME:return(SameDomainName(&b1->name, &b2->name));

		case kDNSType_SOA:	return(mDNSBool)(  	b1->soa.serial   == b2->soa.serial             &&
												b1->soa.refresh  == b2->soa.refresh            &&
												b1->soa.retry    == b2->soa.retry              &&
												b1->soa.expire   == b2->soa.expire             &&
												b1->soa.min      == b2->soa.min                &&
												samename(&b1->soa.mname, &b2->soa.mname) &&
												samename(&b1->soa.rname, &b2->soa.rname));

		case kDNSType_MX:
		case kDNSType_AFSDB:
		case kDNSType_RT:
		case kDNSType_KX:	return(mDNSBool)(  	b1->mx.preference == b2->mx.preference &&
												samename(&b1->mx.exchange, &b2->mx.exchange));

		case kDNSType_RP:	return(mDNSBool)(  	samename(&b1->rp.mbox, &b2->rp.mbox) &&
												samename(&b1->rp.txt,  &b2->rp.txt));

		case kDNSType_PX:	return(mDNSBool)(  	b1->px.preference == b2->px.preference          &&
												samename(&b1->px.map822,  &b2->px.map822) &&
												samename(&b1->px.mapx400, &b2->px.mapx400));

		case kDNSType_SRV:	return(mDNSBool)(  	b1->srv.priority == b2->srv.priority       &&
												b1->srv.weight   == b2->srv.weight         &&
												mDNSSameIPPort(b1->srv.port, b2->srv.port) &&
												samename(&b1->srv.target, &b2->srv.target));

		case kDNSType_OPT:	return mDNSfalse;	// OPT is a pseudo-RR container structure; makes no sense to compare

		case kDNSType_NSEC: return(mDNSPlatformMemSame(b1->data, b2->data, sizeof(rdataNSEC)));

		default:			return(mDNSPlatformMemSame(b1->data, b2->data, r1->rdlength));
		}
	}

// ResourceRecordAnswersQuestion returns mDNStrue if the given resource record is a valid answer to the given question.
// SameNameRecordAnswersQuestion is the same, except it skips the expensive SameDomainName() call.
// SameDomainName() is generally cheap when the names don't match, but expensive when they do match,
// because it has to check all the way to the end of the names to be sure.
// In cases where we know in advance that the names match it's especially advantageous to skip the
// SameDomainName() call because that's precisely the time when it's most expensive and least useful.

mDNSexport mDNSBool SameNameRecordAnswersQuestion(const ResourceRecord *const rr, const DNSQuestion *const q)
	{
	// LocalOnly/P2P questions can be answered with AuthRecordAny in this function. LocalOnly/P2P records
	// are handled in LocalOnlyRecordAnswersQuestion
	if ((rr->InterfaceID == mDNSInterface_LocalOnly) || (rr->InterfaceID == mDNSInterface_P2P))
		{
		LogMsg("SameNameRecordAnswersQuestion: ERROR!! called with LocalOnly ResourceRecord %p, Question %p", rr->InterfaceID, q->InterfaceID);
		return mDNSfalse;
		}
	if (rr->InterfaceID &&
		q ->InterfaceID && q->InterfaceID != mDNSInterface_LocalOnly &&
		rr->InterfaceID != q->InterfaceID) return(mDNSfalse);

	// Resource record received via unicast, the DNSServer entries should match ?
	if (!rr->InterfaceID && rr->rDNSServer != q->qDNSServer) return(mDNSfalse);

	// If ResourceRecord received via multicast, but question was unicast, then shouldn't use record to answer this question
	if (rr->InterfaceID && !mDNSOpaque16IsZero(q->TargetQID)) return(mDNSfalse);

	// RR type CNAME matches any query type. QTYPE ANY matches any RR type. QCLASS ANY matches any RR class.
	if (!RRTypeAnswersQuestionType(rr,q->qtype)) return(mDNSfalse);
	if (rr->rrclass != q->qclass && q->qclass != kDNSQClass_ANY) return(mDNSfalse);

	return(mDNStrue);
	}

mDNSexport mDNSBool ResourceRecordAnswersQuestion(const ResourceRecord *const rr, const DNSQuestion *const q)
	{
	// LocalOnly/P2P questions can be answered with AuthRecordAny in this function. LocalOnly/P2P records
	// are handled in LocalOnlyRecordAnswersQuestion
	if ((rr->InterfaceID == mDNSInterface_LocalOnly) || (rr->InterfaceID == mDNSInterface_P2P))
		{
		LogMsg("ResourceRecordAnswersQuestion: ERROR!! called with LocalOnly/P2P ResourceRecord %p, Question %p", rr->InterfaceID, q->InterfaceID);
		return mDNSfalse;
		}

	if (rr->InterfaceID &&
		q ->InterfaceID && q->InterfaceID != mDNSInterface_LocalOnly &&
		rr->InterfaceID != q->InterfaceID) return(mDNSfalse);

	// Resource record received via unicast, the DNSServer entries should match ?
	if (!rr->InterfaceID && rr->rDNSServer != q->qDNSServer) return(mDNSfalse);

	// If ResourceRecord received via multicast, but question was unicast, then shouldn't use record to answer this question.
	if (rr->InterfaceID && !mDNSOpaque16IsZero(q->TargetQID)) return(mDNSfalse);

	// RR type CNAME matches any query type. QTYPE ANY matches any RR type. QCLASS ANY matches any RR class.
	if (!RRTypeAnswersQuestionType(rr,q->qtype)) return(mDNSfalse);
	if (rr->rrclass != q->qclass && q->qclass != kDNSQClass_ANY) return(mDNSfalse);

	return(rr->namehash == q->qnamehash && SameDomainName(rr->name, &q->qname));
	}

// We have a separate function to handle LocalOnly AuthRecords because they can be created with
// a valid InterfaceID (e.g., scoped /etc/hosts) and can be used to answer unicast questions unlike
// multicast resource records (which has a valid InterfaceID) which can't be used to answer
// unicast questions. ResourceRecordAnswersQuestion/SameNameRecordAnswersQuestion can't tell whether
// a resource record is multicast or LocalOnly by just looking at the ResourceRecord because
// LocalOnly records are truly identified by ARType in the AuthRecord.  As P2P and LocalOnly record
// are kept in the same hash table, we use the same function to make it easy for the callers when
// they walk the hash table to answer LocalOnly/P2P questions
//
mDNSexport mDNSBool LocalOnlyRecordAnswersQuestion(AuthRecord *const ar, const DNSQuestion *const q)
	{
	ResourceRecord *rr = &ar->resrec;
	
	// mDNSInterface_Any questions can be answered with LocalOnly/P2P records in this function. AuthRecord_Any
	// records are handled in ResourceRecordAnswersQuestion/SameNameRecordAnswersQuestion
	if (RRAny(ar))
		{
		LogMsg("LocalOnlyRecordAnswersQuestion: ERROR!! called with regular AuthRecordAny %##s", rr->name->c);
		return mDNSfalse;
		}
		
	// Questions with mDNSInterface_LocalOnly InterfaceID should be answered with all resource records that are
	// *local* to the machine. These include resource records that have InterfaceID set to mDNSInterface_LocalOnly,
	// mDNSInterface_Any and any other real InterfaceID. Hence, LocalOnly questions should not be checked against
	// the InterfaceID in the resource record.
	//
	// mDNSInterface_Unicast does not indicate any scope and hence treat them like mDNSInterface_Any.

	if (rr->InterfaceID && 
		q->InterfaceID && q->InterfaceID != mDNSInterface_LocalOnly && q->InterfaceID != mDNSInterface_Unicast &&
		rr->InterfaceID != q->InterfaceID) return(mDNSfalse);

	// Entries in /etc/hosts are added as LocalOnly resource records. The LocalOnly resource records
	// may have a scope e.g., fe80::1%en0. The question may be scoped or not: the InterfaceID may be set
	// to mDNSInterface_Any, mDNSInterface_LocalOnly or a real InterfaceID (scoped). 
	//
	// 1) Question: Any, LocalOnly Record: no scope. This question should be answered with this record.
	//
	// 2) Question: Any, LocalOnly Record: scoped.  This question should be answered with the record because
	//    traditionally applications never specify scope e.g., getaddrinfo, but need to be able
	//    to get to /etc/hosts entries.
	//
	// 3) Question: Scoped (LocalOnly or InterfaceID), LocalOnly Record: no scope. This is the inverse of (2).
	//    If we register a LocalOnly record, we need to answer a LocalOnly question. If the /etc/hosts has a
	//    non scoped entry, it may not make sense to answer a scoped question. But we can't tell these two
	//    cases apart. As we currently answer LocalOnly question with LocalOnly record, we continue to do so.
	//
	// 4) Question: Scoped (LocalOnly or InterfaceID), LocalOnly Record: scoped. LocalOnly questions should be
	//    answered with any resource record where as if it has a valid InterfaceID, the scope should match.
	//
	// (1) and (2) is bypassed because we check for a non-NULL InterfaceID above. For (3), the InterfaceID is NULL
	// and hence bypassed above. For (4) we bypassed LocalOnly questions and checked the scope of the record
	// against the question.
	//
	// For P2P, InterfaceIDs of the question and the record should match.

	// If ResourceRecord received via multicast, but question was unicast, then shouldn't use record to answer this question.
	// LocalOnly authoritative answers are exempt. LocalOnly authoritative answers are used for /etc/host entries.
	// We don't want a local process to be able to create a fake LocalOnly address record for "www.bigbank.com" which would then
	// cause other applications (e.g. Safari) to connect to the wrong address. The rpc to register records filters out records
	// with names that don't end in local and have mDNSInterface_LocalOnly set.
	//
	// Note: The check is bypassed for LocalOnly and for P2P it is not needed as only .local records are registered and for
	// a question to match its names, it also has to end in .local and that question can't be a unicast question (See
	// Question_uDNS macro and its usage). As P2P does not enforce .local only registrations we still make this check
	// and also makes it future proof.

	if (ar->ARType != AuthRecordLocalOnly && rr->InterfaceID && !mDNSOpaque16IsZero(q->TargetQID)) return(mDNSfalse);

	// RR type CNAME matches any query type. QTYPE ANY matches any RR type. QCLASS ANY matches any RR class.
	if (!RRTypeAnswersQuestionType(rr,q->qtype)) return(mDNSfalse);
	if (rr->rrclass != q->qclass && q->qclass != kDNSQClass_ANY) return(mDNSfalse);

	return(rr->namehash == q->qnamehash && SameDomainName(rr->name, &q->qname));
	}

mDNSexport mDNSBool AnyTypeRecordAnswersQuestion(const ResourceRecord *const rr, const DNSQuestion *const q)
	{
	// LocalOnly/P2P questions can be answered with AuthRecordAny in this function. LocalOnly/P2P records
	// are handled in LocalOnlyRecordAnswersQuestion
	if ((rr->InterfaceID == mDNSInterface_LocalOnly) || (rr->InterfaceID == mDNSInterface_P2P))
		{
		LogMsg("AnyTypeRecordAnswersQuestion: ERROR!! called with LocalOnly ResourceRecord %p, Question %p", rr->InterfaceID, q->InterfaceID);
		return mDNSfalse;
		}
	if (rr->InterfaceID &&
		q ->InterfaceID && q->InterfaceID != mDNSInterface_LocalOnly &&
		rr->InterfaceID != q->InterfaceID) return(mDNSfalse);

	// Resource record received via unicast, the DNSServer entries should match ?
	// Note that Auth Records are normally setup with NULL InterfaceID and
	// both the DNSServers are assumed to be NULL in that case
	if (!rr->InterfaceID && rr->rDNSServer != q->qDNSServer) return(mDNSfalse);

	// If ResourceRecord received via multicast, but question was unicast, then shouldn't use record to answer this question
	if (rr->InterfaceID && !mDNSOpaque16IsZero(q->TargetQID)) return(mDNSfalse);

	if (rr->rrclass != q->qclass && q->qclass != kDNSQClass_ANY) return(mDNSfalse);

	return(rr->namehash == q->qnamehash && SameDomainName(rr->name, &q->qname));
	}

// This is called with both unicast resource record and multicast resource record. The question that
// received the unicast response could be the regular unicast response from a DNS server or a response
// to a mDNS QU query. The main reason we need this function is that we can't compare DNSServers between the
// question and the resource record because the resource record is not completely initialized in
// mDNSCoreReceiveResponse when this function is called.
mDNSexport mDNSBool ResourceRecordAnswersUnicastResponse(const ResourceRecord *const rr, const DNSQuestion *const q)
	{
	// For resource records created using multicast, the InterfaceIDs have to match
	if (rr->InterfaceID &&
		q->InterfaceID && rr->InterfaceID != q->InterfaceID) return(mDNSfalse);

	// If ResourceRecord received via multicast, but question was unicast, then shouldn't use record to answer this question.
	if (rr->InterfaceID && !mDNSOpaque16IsZero(q->TargetQID)) return(mDNSfalse);

	// RR type CNAME matches any query type. QTYPE ANY matches any RR type. QCLASS ANY matches any RR class.
	if (!RRTypeAnswersQuestionType(rr,q->qtype)) return(mDNSfalse);

	if (rr->rrclass != q->qclass && q->qclass != kDNSQClass_ANY) return(mDNSfalse);

	return(rr->namehash == q->qnamehash && SameDomainName(rr->name, &q->qname));
	}

mDNSexport mDNSu16 GetRDLength(const ResourceRecord *const rr, mDNSBool estimate)
	{
	const RDataBody2 *const rd = (RDataBody2 *)rr->rdata->u.data;
	const domainname *const name = estimate ? rr->name : mDNSNULL;
	if (rr->rrclass == kDNSQClass_ANY) return(rr->rdlength);	// Used in update packets to mean "Delete An RRset" (RFC 2136)
	else switch (rr->rrtype)
		{
		case kDNSType_A:	return(sizeof(rd->ipv4));

		case kDNSType_NS:
		case kDNSType_CNAME:
		case kDNSType_PTR:
		case kDNSType_DNAME:return(CompressedDomainNameLength(&rd->name, name));

		case kDNSType_SOA:  return(mDNSu16)(CompressedDomainNameLength(&rd->soa.mname, name) +
											CompressedDomainNameLength(&rd->soa.rname, name) +
											5 * sizeof(mDNSOpaque32));

		case kDNSType_NULL:
		case kDNSType_TSIG:
		case kDNSType_TXT:
		case kDNSType_X25:
		case kDNSType_ISDN:
		case kDNSType_LOC:
		case kDNSType_DHCID:return(rr->rdlength); // Not self-describing, so have to just trust rdlength

		case kDNSType_HINFO:return(mDNSu16)(2 + (int)rd->data[0] + (int)rd->data[1 + (int)rd->data[0]]);

		case kDNSType_MX:
		case kDNSType_AFSDB:
		case kDNSType_RT:
		case kDNSType_KX:	return(mDNSu16)(2 + CompressedDomainNameLength(&rd->mx.exchange, name));

		case kDNSType_RP:	return(mDNSu16)(CompressedDomainNameLength(&rd->rp.mbox, name) +
											CompressedDomainNameLength(&rd->rp.txt, name));

		case kDNSType_PX:	return(mDNSu16)(2 + CompressedDomainNameLength(&rd->px.map822, name) +
												CompressedDomainNameLength(&rd->px.mapx400, name));

		case kDNSType_AAAA:	return(sizeof(rd->ipv6));

		case kDNSType_SRV:	return(mDNSu16)(6 + CompressedDomainNameLength(&rd->srv.target, name));

		case kDNSType_OPT:  return(rr->rdlength);

		case kDNSType_NSEC: {
							int i;
							for (i=sizeof(rdataNSEC); i>0; i--) if (rd->nsec.bitmap[i-1]) break;
							// For our simplified use of NSEC synthetic records:
							// nextname is always the record's own name,
							// and if we have at least one record type that exists,
							//  - the block number is always 0,
							//  - the count byte is a value in the range 1-32,
							//  - followed by the 1-32 data bytes
							return(mDNSu16)((estimate ? 2 : DomainNameLength(rr->name)) + (i ? (2 + i) : 0));
							}

		default:			debugf("Warning! Don't know how to get length of resource type %d", rr->rrtype);
							return(rr->rdlength);
		}
	}

// When a local client registers (or updates) a record, we use this routine to do some simple validation checks
// to help reduce the risk of bogus malformed data on the network
mDNSexport mDNSBool ValidateRData(const mDNSu16 rrtype, const mDNSu16 rdlength, const RData *const rd)
	{
	mDNSu16 len;

	switch(rrtype)
		{
		case kDNSType_A:	return(rdlength == sizeof(mDNSv4Addr));

		case kDNSType_NS:	// Same as PTR
		case kDNSType_MD:	// Same as PTR
		case kDNSType_MF:	// Same as PTR
		case kDNSType_CNAME:// Same as PTR
		//case kDNSType_SOA not checked
		case kDNSType_MB:	// Same as PTR
		case kDNSType_MG:	// Same as PTR
		case kDNSType_MR:	// Same as PTR
		//case kDNSType_NULL not checked (no specified format, so always valid)
		//case kDNSType_WKS not checked
		case kDNSType_PTR:	len = DomainNameLengthLimit(&rd->u.name, rd->u.data + rdlength);
							return(len <= MAX_DOMAIN_NAME && rdlength == len);

		case kDNSType_HINFO:// Same as TXT (roughly)
		case kDNSType_MINFO:// Same as TXT (roughly)
		case kDNSType_TXT:  if (!rdlength) return(mDNSfalse); // TXT record has to be at least one byte (RFC 1035)
							{
							const mDNSu8 *ptr = rd->u.txt.c;
							const mDNSu8 *end = rd->u.txt.c + rdlength;
							while (ptr < end) ptr += 1 + ptr[0];
							return (ptr == end);
							}

		case kDNSType_AAAA:	return(rdlength == sizeof(mDNSv6Addr));

		case kDNSType_MX:   // Must be at least two-byte preference, plus domainname
							// Call to DomainNameLengthLimit() implicitly enforces both requirements for us
							len = DomainNameLengthLimit(&rd->u.mx.exchange, rd->u.data + rdlength);
							return(len <= MAX_DOMAIN_NAME && rdlength == 2+len);

		case kDNSType_SRV:	// Must be at least priority+weight+port, plus domainname
							// Call to DomainNameLengthLimit() implicitly enforces both requirements for us
							len = DomainNameLengthLimit(&rd->u.srv.target, rd->u.data + rdlength);
							return(len <= MAX_DOMAIN_NAME && rdlength == 6+len);

		//case kDNSType_NSEC not checked

		default:			return(mDNStrue);	// Allow all other types without checking
		}
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - DNS Message Creation Functions
#endif

mDNSexport void InitializeDNSMessage(DNSMessageHeader *h, mDNSOpaque16 id, mDNSOpaque16 flags)
	{
	h->id             = id;
	h->flags          = flags;
	h->numQuestions   = 0;
	h->numAnswers     = 0;
	h->numAuthorities = 0;
	h->numAdditionals = 0;
	}

mDNSexport const mDNSu8 *FindCompressionPointer(const mDNSu8 *const base, const mDNSu8 *const end, const mDNSu8 *const domname)
	{
	const mDNSu8 *result = end - *domname - 1;

	if (*domname == 0) return(mDNSNULL);	// There's no point trying to match just the root label

	// This loop examines each possible starting position in packet, starting end of the packet and working backwards
	while (result >= base)
		{
		// If the length byte and first character of the label match, then check further to see
		// if this location in the packet will yield a useful name compression pointer.
		if (result[0] == domname[0] && result[1] == domname[1])
			{
			const mDNSu8 *name = domname;
			const mDNSu8 *targ = result;
			while (targ + *name < end)
				{
				// First see if this label matches
				int i;
				const mDNSu8 *pointertarget;
				for (i=0; i <= *name; i++) if (targ[i] != name[i]) break;
				if (i <= *name) break;							// If label did not match, bail out
				targ += 1 + *name;								// Else, did match, so advance target pointer
				name += 1 + *name;								// and proceed to check next label
				if (*name == 0 && *targ == 0) return(result);	// If no more labels, we found a match!
				if (*name == 0) break;							// If no more labels to match, we failed, so bail out

				// The label matched, so now follow the pointer (if appropriate) and then see if the next label matches
				if (targ[0] < 0x40) continue;					// If length value, continue to check next label
				if (targ[0] < 0xC0) break;						// If 40-BF, not valid
				if (targ+1 >= end) break;						// Second byte not present!
				pointertarget = base + (((mDNSu16)(targ[0] & 0x3F)) << 8) + targ[1];
				if (targ < pointertarget) break;				// Pointertarget must point *backwards* in the packet
				if (pointertarget[0] >= 0x40) break;			// Pointertarget must point to a valid length byte
				targ = pointertarget;
				}
			}
		result--;	// We failed to match at this search position, so back up the tentative result pointer and try again
		}
	return(mDNSNULL);
	}

// Put a string of dot-separated labels as length-prefixed labels
// domainname is a fully-qualified name (i.e. assumed to be ending in a dot, even if it doesn't)
// msg points to the message we're building (pass mDNSNULL if we don't want to use compression pointers)
// end points to the end of the message so far
// ptr points to where we want to put the name
// limit points to one byte past the end of the buffer that we must not overrun
// domainname is the name to put
mDNSexport mDNSu8 *putDomainNameAsLabels(const DNSMessage *const msg,
	mDNSu8 *ptr, const mDNSu8 *const limit, const domainname *const name)
	{
	const mDNSu8 *const base        = (const mDNSu8 *)msg;
	const mDNSu8 *      np          = name->c;
	const mDNSu8 *const max         = name->c + MAX_DOMAIN_NAME;	// Maximum that's valid
	const mDNSu8 *      pointer     = mDNSNULL;
	const mDNSu8 *const searchlimit = ptr;

	if (!ptr) { LogMsg("putDomainNameAsLabels %##s ptr is null", name->c); return(mDNSNULL); }

	if (!*np)		// If just writing one-byte root label, make sure we have space for that
		{
		if (ptr >= limit) return(mDNSNULL);
		}
	else			// else, loop through writing labels and/or a compression offset
		{
		do	{
			if (*np > MAX_DOMAIN_LABEL)
				{ LogMsg("Malformed domain name %##s (label more than 63 bytes)", name->c); return(mDNSNULL); }
	
			// This check correctly allows for the final trailing root label:
			// e.g.
			// Suppose our domain name is exactly 256 bytes long, including the final trailing root label.
			// Suppose np is now at name->c[249], and we're about to write our last non-null label ("local").
			// We know that max will be at name->c[256]
			// That means that np + 1 + 5 == max - 1, so we (just) pass the "if" test below, write our
			// six bytes, then exit the loop, write the final terminating root label, and the domain
			// name we've written is exactly 256 bytes long, exactly at the correct legal limit.
			// If the name is one byte longer, then we fail the "if" test below, and correctly bail out.
			if (np + 1 + *np >= max)
				{ LogMsg("Malformed domain name %##s (more than 256 bytes)", name->c); return(mDNSNULL); }
	
			if (base) pointer = FindCompressionPointer(base, searchlimit, np);
			if (pointer)					// Use a compression pointer if we can
				{
				const mDNSu16 offset = (mDNSu16)(pointer - base);
				if (ptr+2 > limit) return(mDNSNULL);	// If we don't have two bytes of space left, give up
				*ptr++ = (mDNSu8)(0xC0 | (offset >> 8));
				*ptr++ = (mDNSu8)(        offset &  0xFF);
				return(ptr);
				}
			else							// Else copy one label and try again
				{
				int i;
				mDNSu8 len = *np++;
				// If we don't at least have enough space for this label *plus* a terminating zero on the end, give up
				if (ptr + 1 + len >= limit) return(mDNSNULL);
				*ptr++ = len;
				for (i=0; i<len; i++) *ptr++ = *np++;
				}
			} while (*np);					// While we've got characters remaining in the name, continue
		}

	*ptr++ = 0;		// Put the final root label
	return(ptr);
	}

mDNSlocal mDNSu8 *putVal16(mDNSu8 *ptr, mDNSu16 val)
	{
	ptr[0] = (mDNSu8)((val >> 8 ) & 0xFF);
	ptr[1] = (mDNSu8)((val      ) & 0xFF);
	return ptr + sizeof(mDNSOpaque16);
	}

mDNSlocal mDNSu8 *putVal32(mDNSu8 *ptr, mDNSu32 val)
	{
	ptr[0] = (mDNSu8)((val >> 24) & 0xFF);
	ptr[1] = (mDNSu8)((val >> 16) & 0xFF);
	ptr[2] = (mDNSu8)((val >>  8) & 0xFF);
	ptr[3] = (mDNSu8)((val      ) & 0xFF);
	return ptr + sizeof(mDNSu32);
	}

// msg points to the message we're building (pass mDNSNULL if we don't want to use compression pointers)
mDNSexport mDNSu8 *putRData(const DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, const ResourceRecord *const rr)
	{
	const RDataBody2 *const rdb = (RDataBody2 *)rr->rdata->u.data;
	switch (rr->rrtype)
		{
		case kDNSType_A:	if (rr->rdlength != 4)
								{ debugf("putRData: Illegal length %d for kDNSType_A", rr->rdlength); return(mDNSNULL); }
							if (ptr + 4 > limit) return(mDNSNULL);
							*ptr++ = rdb->ipv4.b[0];
							*ptr++ = rdb->ipv4.b[1];
							*ptr++ = rdb->ipv4.b[2];
							*ptr++ = rdb->ipv4.b[3];
							return(ptr);

		case kDNSType_NS:
		case kDNSType_CNAME:
		case kDNSType_PTR:
		case kDNSType_DNAME:return(putDomainNameAsLabels(msg, ptr, limit, &rdb->name));

		case kDNSType_SOA:  ptr = putDomainNameAsLabels(msg, ptr, limit, &rdb->soa.mname);
							if (!ptr) return(mDNSNULL);
							ptr = putDomainNameAsLabels(msg, ptr, limit, &rdb->soa.rname);
							if (!ptr || ptr + 20 > limit) return(mDNSNULL);
							ptr = putVal32(ptr, rdb->soa.serial);
							ptr = putVal32(ptr, rdb->soa.refresh);
							ptr = putVal32(ptr, rdb->soa.retry);
							ptr = putVal32(ptr, rdb->soa.expire);
							ptr = putVal32(ptr, rdb->soa.min);
			                return(ptr);

		case kDNSType_NULL:
		case kDNSType_HINFO:
		case kDNSType_TSIG:
		case kDNSType_TXT:
		case kDNSType_X25:
		case kDNSType_ISDN:
		case kDNSType_LOC:
		case kDNSType_DHCID:if (ptr + rr->rdlength > limit) return(mDNSNULL);
							mDNSPlatformMemCopy(ptr, rdb->data, rr->rdlength);
							return(ptr + rr->rdlength);

		case kDNSType_MX:
		case kDNSType_AFSDB:
		case kDNSType_RT:
		case kDNSType_KX:	if (ptr + 3 > limit) return(mDNSNULL);
							ptr = putVal16(ptr, rdb->mx.preference);
							return(putDomainNameAsLabels(msg, ptr, limit, &rdb->mx.exchange));

		case kDNSType_RP:	ptr = putDomainNameAsLabels(msg, ptr, limit, &rdb->rp.mbox);
							if (!ptr) return(mDNSNULL);
							ptr = putDomainNameAsLabels(msg, ptr, limit, &rdb->rp.txt);
			                return(ptr);

		case kDNSType_PX:	if (ptr + 5 > limit) return(mDNSNULL);
							ptr = putVal16(ptr, rdb->px.preference);
							ptr = putDomainNameAsLabels(msg, ptr, limit, &rdb->px.map822);
							if (!ptr) return(mDNSNULL);
							ptr = putDomainNameAsLabels(msg, ptr, limit, &rdb->px.mapx400);
			                return(ptr);

		case kDNSType_AAAA:	if (rr->rdlength != sizeof(rdb->ipv6))
								{ debugf("putRData: Illegal length %d for kDNSType_AAAA", rr->rdlength); return(mDNSNULL); }
							if (ptr + sizeof(rdb->ipv6) > limit) return(mDNSNULL);
							mDNSPlatformMemCopy(ptr, &rdb->ipv6, sizeof(rdb->ipv6));
							return(ptr + sizeof(rdb->ipv6));

		case kDNSType_SRV:	if (ptr + 7 > limit) return(mDNSNULL);
							*ptr++ = (mDNSu8)(rdb->srv.priority >> 8);
							*ptr++ = (mDNSu8)(rdb->srv.priority &  0xFF);
							*ptr++ = (mDNSu8)(rdb->srv.weight   >> 8);
							*ptr++ = (mDNSu8)(rdb->srv.weight   &  0xFF);
							*ptr++ = rdb->srv.port.b[0];
							*ptr++ = rdb->srv.port.b[1];
							return(putDomainNameAsLabels(msg, ptr, limit, &rdb->srv.target));

		case kDNSType_OPT:	{
							int len = 0;
							const rdataOPT *opt;
							const rdataOPT *const end = (const rdataOPT *)&rr->rdata->u.data[rr->rdlength];
							for (opt = &rr->rdata->u.opt[0]; opt < end; opt++) len += DNSOpt_Data_Space(opt);
							if (ptr + len > limit) { LogMsg("ERROR: putOptRData - out of space"); return mDNSNULL; }
						
							for (opt = &rr->rdata->u.opt[0]; opt < end; opt++)
								{
								const int space = DNSOpt_Data_Space(opt);
								ptr = putVal16(ptr, opt->opt);
								ptr = putVal16(ptr, (mDNSu16)space - 4);
								switch (opt->opt)
									{
									case kDNSOpt_LLQ:
										ptr = putVal16(ptr, opt->u.llq.vers);
										ptr = putVal16(ptr, opt->u.llq.llqOp);
										ptr = putVal16(ptr, opt->u.llq.err);
										mDNSPlatformMemCopy(ptr, opt->u.llq.id.b, 8);  // 8-byte id
										ptr += 8;
										ptr = putVal32(ptr, opt->u.llq.llqlease);
										break;
									case kDNSOpt_Lease:
										ptr = putVal32(ptr, opt->u.updatelease);
										break;
									case kDNSOpt_Owner:
										*ptr++ = opt->u.owner.vers;
										*ptr++ = opt->u.owner.seq;
										mDNSPlatformMemCopy(ptr, opt->u.owner.HMAC.b, 6);  // 6-byte Host identifier
										ptr += 6;
										if (space >= DNSOpt_OwnerData_ID_Wake_Space)
											{
											mDNSPlatformMemCopy(ptr, opt->u.owner.IMAC.b, 6);	// 6-byte interface MAC
											ptr += 6;
											if (space > DNSOpt_OwnerData_ID_Wake_Space)
												{
												mDNSPlatformMemCopy(ptr, opt->u.owner.password.b, space - DNSOpt_OwnerData_ID_Wake_Space);
												ptr += space - DNSOpt_OwnerData_ID_Wake_Space;
												}
											}
										break;
									}
								}
							return ptr;
							}

		case kDNSType_NSEC: {
							// For our simplified use of NSEC synthetic records:
							// nextname is always the record's own name,
							// the block number is always 0,
							// the count byte is a value in the range 1-32,
							// followed by the 1-32 data bytes
							int i, j;
							for (i=sizeof(rdataNSEC); i>0; i--) if (rdb->nsec.bitmap[i-1]) break;
							ptr = putDomainNameAsLabels(msg, ptr, limit, rr->name);
							if (!ptr) return(mDNSNULL);
							if (i)		// Only put a block if at least one type exists for this name
								{
								if (ptr + 2 + i > limit) return(mDNSNULL);
								*ptr++ = 0;
								*ptr++ = (mDNSu8)i;
								for (j=0; j<i; j++) *ptr++ = rdb->nsec.bitmap[j];
								}
							return ptr;
							}

		default:			debugf("putRData: Warning! Writing unknown resource type %d as raw data", rr->rrtype);
							if (ptr + rr->rdlength > limit) return(mDNSNULL);
							mDNSPlatformMemCopy(ptr, rdb->data, rr->rdlength);
							return(ptr + rr->rdlength);
		}
	}

#define IsUnicastUpdate(X) (!mDNSOpaque16IsZero((X)->h.id) && ((X)->h.flags.b[0] & kDNSFlag0_OP_Mask) == kDNSFlag0_OP_Update)

mDNSexport mDNSu8 *PutResourceRecordTTLWithLimit(DNSMessage *const msg, mDNSu8 *ptr, mDNSu16 *count, ResourceRecord *rr, mDNSu32 ttl, const mDNSu8 *limit)
	{
	mDNSu8 *endofrdata;
	mDNSu16 actualLength;
	// When sending SRV to conventional DNS server (i.e. in DNS update requests) we should not do name compression on the rdata (RFC 2782)
	const DNSMessage *const rdatacompressionbase = (IsUnicastUpdate(msg) && rr->rrtype == kDNSType_SRV) ? mDNSNULL : msg;

	if (rr->RecordType == kDNSRecordTypeUnregistered)
		{
		LogMsg("PutResourceRecord ERROR! Attempt to put kDNSRecordTypeUnregistered %##s (%s)", rr->name->c, DNSTypeName(rr->rrtype));
		return(ptr);
		}

	if (!ptr) { LogMsg("PutResourceRecordTTLWithLimit ptr is null"); return(mDNSNULL); }

	ptr = putDomainNameAsLabels(msg, ptr, limit, rr->name);
	if (!ptr || ptr + 10 >= limit) return(mDNSNULL);	// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rr->rrtype  >> 8);
	ptr[1] = (mDNSu8)(rr->rrtype  &  0xFF);
	ptr[2] = (mDNSu8)(rr->rrclass >> 8);
	ptr[3] = (mDNSu8)(rr->rrclass &  0xFF);
	ptr[4] = (mDNSu8)((ttl >> 24) &  0xFF);
	ptr[5] = (mDNSu8)((ttl >> 16) &  0xFF);
	ptr[6] = (mDNSu8)((ttl >>  8) &  0xFF);
	ptr[7] = (mDNSu8)( ttl        &  0xFF);
	// ptr[8] and ptr[9] filled in *after* we find out how much space the rdata takes
	
	endofrdata = putRData(rdatacompressionbase, ptr+10, limit, rr);
	if (!endofrdata) { verbosedebugf("Ran out of space in PutResourceRecord for %##s (%s)", rr->name->c, DNSTypeName(rr->rrtype)); return(mDNSNULL); }

	// Go back and fill in the actual number of data bytes we wrote
	// (actualLength can be less than rdlength when domain name compression is used)
	actualLength = (mDNSu16)(endofrdata - ptr - 10);
	ptr[8] = (mDNSu8)(actualLength >> 8);
	ptr[9] = (mDNSu8)(actualLength &  0xFF);

	if (count) (*count)++;
	else LogMsg("PutResourceRecordTTL: ERROR: No target count to update for %##s (%s)", rr->name->c, DNSTypeName(rr->rrtype));
	return(endofrdata);
	}

mDNSlocal mDNSu8 *putEmptyResourceRecord(DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, mDNSu16 *count, const AuthRecord *rr)
	{
	ptr = putDomainNameAsLabels(msg, ptr, limit, rr->resrec.name);
	if (!ptr || ptr + 10 > limit) return(mDNSNULL);		// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rr->resrec.rrtype  >> 8);				// Put type
	ptr[1] = (mDNSu8)(rr->resrec.rrtype  &  0xFF);
	ptr[2] = (mDNSu8)(rr->resrec.rrclass >> 8);				// Put class
	ptr[3] = (mDNSu8)(rr->resrec.rrclass &  0xFF);
	ptr[4] = ptr[5] = ptr[6] = ptr[7] = 0;				// TTL is zero
	ptr[8] = ptr[9] = 0;								// RDATA length is zero
	(*count)++;
	return(ptr + 10);
	}

mDNSexport mDNSu8 *putQuestion(DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, const domainname *const name, mDNSu16 rrtype, mDNSu16 rrclass)
	{
	ptr = putDomainNameAsLabels(msg, ptr, limit, name);
	if (!ptr || ptr+4 >= limit) return(mDNSNULL);			// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rrtype  >> 8);
	ptr[1] = (mDNSu8)(rrtype  &  0xFF);
	ptr[2] = (mDNSu8)(rrclass >> 8);
	ptr[3] = (mDNSu8)(rrclass &  0xFF);
	msg->h.numQuestions++;
	return(ptr+4);
	}

// for dynamic updates
mDNSexport mDNSu8 *putZone(DNSMessage *const msg, mDNSu8 *ptr, mDNSu8 *limit, const domainname *zone, mDNSOpaque16 zoneClass)
	{
	ptr = putDomainNameAsLabels(msg, ptr, limit, zone);
	if (!ptr || ptr + 4 > limit) return mDNSNULL;		// If we're out-of-space, return NULL
	*ptr++ = (mDNSu8)(kDNSType_SOA  >> 8);
	*ptr++ = (mDNSu8)(kDNSType_SOA  &  0xFF);
	*ptr++ = zoneClass.b[0];
	*ptr++ = zoneClass.b[1];
	msg->h.mDNS_numZones++;
	return ptr;
	}

// for dynamic updates
mDNSexport mDNSu8 *putPrereqNameNotInUse(const domainname *const name, DNSMessage *const msg, mDNSu8 *const ptr, mDNSu8 *const end)
	{
	AuthRecord prereq;
	mDNS_SetupResourceRecord(&prereq, mDNSNULL, mDNSInterface_Any, kDNSQType_ANY, kStandardTTL, 0, AuthRecordAny, mDNSNULL, mDNSNULL);
	AssignDomainName(&prereq.namestorage, name);
	prereq.resrec.rrtype = kDNSQType_ANY;
	prereq.resrec.rrclass = kDNSClass_NONE;
	return putEmptyResourceRecord(msg, ptr, end, &msg->h.mDNS_numPrereqs, &prereq);
	}

// for dynamic updates
mDNSexport mDNSu8 *putDeletionRecord(DNSMessage *msg, mDNSu8 *ptr, ResourceRecord *rr)
	{
	// deletion: specify record w/ TTL 0, class NONE
	const mDNSu16 origclass = rr->rrclass;
	rr->rrclass = kDNSClass_NONE;
	ptr = PutResourceRecordTTLJumbo(msg, ptr, &msg->h.mDNS_numUpdates, rr, 0);
	rr->rrclass = origclass;
	return ptr;
	}

// for dynamic updates
mDNSexport mDNSu8 *putDeletionRecordWithLimit(DNSMessage *msg, mDNSu8 *ptr, ResourceRecord *rr, mDNSu8 *limit)
	{
	// deletion: specify record w/ TTL 0, class NONE
	const mDNSu16 origclass = rr->rrclass;
	rr->rrclass = kDNSClass_NONE;
	ptr = PutResourceRecordTTLWithLimit(msg, ptr, &msg->h.mDNS_numUpdates, rr, 0, limit);
	rr->rrclass = origclass;
	return ptr;
	}

mDNSexport mDNSu8 *putDeleteRRSetWithLimit(DNSMessage *msg, mDNSu8 *ptr, const domainname *name, mDNSu16 rrtype, mDNSu8 *limit)
	{
	mDNSu16 class = kDNSQClass_ANY;
	
	ptr = putDomainNameAsLabels(msg, ptr, limit, name);
	if (!ptr || ptr + 10 >= limit) return mDNSNULL;	// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rrtype  >> 8);
	ptr[1] = (mDNSu8)(rrtype  &  0xFF);
	ptr[2] = (mDNSu8)(class >> 8);
	ptr[3] = (mDNSu8)(class &  0xFF);
	ptr[4] = ptr[5] = ptr[6] = ptr[7] = 0; // zero ttl
	ptr[8] = ptr[9] = 0; // zero rdlength/rdata

	msg->h.mDNS_numUpdates++;
	return ptr + 10;
	}

// for dynamic updates
mDNSexport mDNSu8 *putDeleteAllRRSets(DNSMessage *msg, mDNSu8 *ptr, const domainname *name)
	{
	const mDNSu8 *limit = msg->data + AbsoluteMaxDNSMessageData;
	mDNSu16 class = kDNSQClass_ANY;
	mDNSu16 rrtype = kDNSQType_ANY;
	
	ptr = putDomainNameAsLabels(msg, ptr, limit, name);
	if (!ptr || ptr + 10 >= limit) return mDNSNULL;	// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rrtype >> 8);
	ptr[1] = (mDNSu8)(rrtype &  0xFF);
	ptr[2] = (mDNSu8)(class  >> 8);
	ptr[3] = (mDNSu8)(class  &  0xFF);
	ptr[4] = ptr[5] = ptr[6] = ptr[7] = 0; // zero ttl
	ptr[8] = ptr[9] = 0; // zero rdlength/rdata

	msg->h.mDNS_numUpdates++;
	return ptr + 10;
	}

// for dynamic updates
mDNSexport mDNSu8 *putUpdateLease(DNSMessage *msg, mDNSu8 *end, mDNSu32 lease)
	{
	AuthRecord rr;
	mDNS_SetupResourceRecord(&rr, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
	rr.resrec.rrclass    = NormalMaxDNSMessageData;
	rr.resrec.rdlength   = sizeof(rdataOPT);	// One option in this OPT record
	rr.resrec.rdestimate = sizeof(rdataOPT);
	rr.resrec.rdata->u.opt[0].opt           = kDNSOpt_Lease;
	rr.resrec.rdata->u.opt[0].u.updatelease = lease;
	end = PutResourceRecordTTLJumbo(msg, end, &msg->h.numAdditionals, &rr.resrec, 0);
	if (!end) { LogMsg("ERROR: putUpdateLease - PutResourceRecordTTL"); return mDNSNULL; }
	return end;
	}

// for dynamic updates
mDNSexport mDNSu8 *putUpdateLeaseWithLimit(DNSMessage *msg, mDNSu8 *end, mDNSu32 lease, mDNSu8 *limit)
	{
	AuthRecord rr;
	mDNS_SetupResourceRecord(&rr, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
	rr.resrec.rrclass    = NormalMaxDNSMessageData;
	rr.resrec.rdlength   = sizeof(rdataOPT);	// One option in this OPT record
	rr.resrec.rdestimate = sizeof(rdataOPT);
	rr.resrec.rdata->u.opt[0].opt           = kDNSOpt_Lease;
	rr.resrec.rdata->u.opt[0].u.updatelease = lease;
	end = PutResourceRecordTTLWithLimit(msg, end, &msg->h.numAdditionals, &rr.resrec, 0, limit);
	if (!end) { LogMsg("ERROR: putUpdateLease - PutResourceRecordTTLWithLimit"); return mDNSNULL; }
	return end;
	}

mDNSexport mDNSu8 *putHINFO(const mDNS *const m, DNSMessage *const msg, mDNSu8 *end, DomainAuthInfo *authInfo, mDNSu8 *limit)
	{
	if (authInfo && authInfo->AutoTunnel)
		{
		AuthRecord hinfo;
		mDNSu8 *h = hinfo.rdatastorage.u.data;
		mDNSu16 len = 2 + m->HIHardware.c[0] + m->HISoftware.c[0];
		mDNSu8 *newptr;
		mDNS_SetupResourceRecord(&hinfo, mDNSNULL, mDNSInterface_Any, kDNSType_HINFO, 0, kDNSRecordTypeUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
		AppendDomainLabel(&hinfo.namestorage, &m->hostlabel);
		AppendDomainName (&hinfo.namestorage, &authInfo->domain);
		hinfo.resrec.rroriginalttl = 0;
		mDNSPlatformMemCopy(h, &m->HIHardware, 1 + (mDNSu32)m->HIHardware.c[0]);
		h += 1 + (int)h[0];
		mDNSPlatformMemCopy(h, &m->HISoftware, 1 + (mDNSu32)m->HISoftware.c[0]);
		hinfo.resrec.rdlength   = len;
		hinfo.resrec.rdestimate = len;
		newptr = PutResourceRecordTTLWithLimit(msg, end, &msg->h.numAdditionals, &hinfo.resrec, 0, limit);
		return newptr;
		}
	else
		return end;
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - DNS Message Parsing Functions
#endif

mDNSexport mDNSu32 DomainNameHashValue(const domainname *const name)
	{
	mDNSu32 sum = 0;
	const mDNSu8 *c;

	for (c = name->c; c[0] != 0 && c[1] != 0; c += 2)
		{
		sum += ((mDNSIsUpperCase(c[0]) ? c[0] + 'a' - 'A' : c[0]) << 8) |
				(mDNSIsUpperCase(c[1]) ? c[1] + 'a' - 'A' : c[1]);
		sum = (sum<<3) | (sum>>29);
		}
	if (c[0]) sum += ((mDNSIsUpperCase(c[0]) ? c[0] + 'a' - 'A' : c[0]) << 8);
	return(sum);
	}

mDNSexport void SetNewRData(ResourceRecord *const rr, RData *NewRData, mDNSu16 rdlength)
	{
	domainname *target;
	if (NewRData)
		{
		rr->rdata    = NewRData;
		rr->rdlength = rdlength;
		}
	// Must not try to get target pointer until after updating rr->rdata
	target = GetRRDomainNameTarget(rr);
	rr->rdlength   = GetRDLength(rr, mDNSfalse);
	rr->rdestimate = GetRDLength(rr, mDNStrue);
	rr->rdatahash  = target ? DomainNameHashValue(target) : RDataHashValue(rr);
	}

mDNSexport const mDNSu8 *skipDomainName(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end)
	{
	mDNSu16 total = 0;

	if (ptr < (mDNSu8*)msg || ptr >= end)
		{ debugf("skipDomainName: Illegal ptr not within packet boundaries"); return(mDNSNULL); }

	while (1)						// Read sequence of labels
		{
		const mDNSu8 len = *ptr++;	// Read length of this label
		if (len == 0) return(ptr);	// If length is zero, that means this name is complete
		switch (len & 0xC0)
			{
			case 0x00:	if (ptr + len >= end)					// Remember: expect at least one more byte for the root label
							{ debugf("skipDomainName: Malformed domain name (overruns packet end)"); return(mDNSNULL); }
						if (total + 1 + len >= MAX_DOMAIN_NAME)	// Remember: expect at least one more byte for the root label
							{ debugf("skipDomainName: Malformed domain name (more than 256 characters)"); return(mDNSNULL); }
						ptr += len;
						total += 1 + len;
						break;

			case 0x40:	debugf("skipDomainName: Extended EDNS0 label types 0x%X not supported", len); return(mDNSNULL);
			case 0x80:	debugf("skipDomainName: Illegal label length 0x%X", len); return(mDNSNULL);
			case 0xC0:	return(ptr+1);
			}
		}
	}

// Routine to fetch an FQDN from the DNS message, following compression pointers if necessary.
mDNSexport const mDNSu8 *getDomainName(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end,
	domainname *const name)
	{
	const mDNSu8 *nextbyte = mDNSNULL;					// Record where we got to before we started following pointers
	mDNSu8       *np = name->c;							// Name pointer
	const mDNSu8 *const limit = np + MAX_DOMAIN_NAME;	// Limit so we don't overrun buffer

	if (ptr < (mDNSu8*)msg || ptr >= end)
		{ debugf("getDomainName: Illegal ptr not within packet boundaries"); return(mDNSNULL); }

	*np = 0;						// Tentatively place the root label here (may be overwritten if we have more labels)

	while (1)						// Read sequence of labels
		{
		const mDNSu8 len = *ptr++;	// Read length of this label
		if (len == 0) break;		// If length is zero, that means this name is complete
		switch (len & 0xC0)
			{
			int i;
			mDNSu16 offset;

			case 0x00:	if (ptr + len >= end)		// Remember: expect at least one more byte for the root label
							{ debugf("getDomainName: Malformed domain name (overruns packet end)"); return(mDNSNULL); }
						if (np + 1 + len >= limit)	// Remember: expect at least one more byte for the root label
							{ debugf("getDomainName: Malformed domain name (more than 256 characters)"); return(mDNSNULL); }
						*np++ = len;
						for (i=0; i<len; i++) *np++ = *ptr++;
						*np = 0;	// Tentatively place the root label here (may be overwritten if we have more labels)
						break;

			case 0x40:	debugf("getDomainName: Extended EDNS0 label types 0x%X not supported in name %##s", len, name->c);
						return(mDNSNULL);

			case 0x80:	debugf("getDomainName: Illegal label length 0x%X in domain name %##s", len, name->c); return(mDNSNULL);

			case 0xC0:	offset = (mDNSu16)((((mDNSu16)(len & 0x3F)) << 8) | *ptr++);
						if (!nextbyte) nextbyte = ptr;	// Record where we got to before we started following pointers
						ptr = (mDNSu8 *)msg + offset;
						if (ptr < (mDNSu8*)msg || ptr >= end)
							{ debugf("getDomainName: Illegal compression pointer not within packet boundaries"); return(mDNSNULL); }
						if (*ptr & 0xC0)
							{ debugf("getDomainName: Compression pointer must point to real label"); return(mDNSNULL); }
						break;
			}
		}

	if (nextbyte) return(nextbyte);
	else return(ptr);
	}

mDNSexport const mDNSu8 *skipResourceRecord(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end)
	{
	mDNSu16 pktrdlength;

	ptr = skipDomainName(msg, ptr, end);
	if (!ptr) { debugf("skipResourceRecord: Malformed RR name"); return(mDNSNULL); }

	if (ptr + 10 > end) { debugf("skipResourceRecord: Malformed RR -- no type/class/ttl/len!"); return(mDNSNULL); }
	pktrdlength = (mDNSu16)((mDNSu16)ptr[8] <<  8 | ptr[9]);
	ptr += 10;
	if (ptr + pktrdlength > end) { debugf("skipResourceRecord: RDATA exceeds end of packet"); return(mDNSNULL); }

	return(ptr + pktrdlength);
	}

mDNSexport const mDNSu8 *GetLargeResourceRecord(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *ptr,
    const mDNSu8 *end, const mDNSInterfaceID InterfaceID, mDNSu8 RecordType, LargeCacheRecord *const largecr)
	{
	CacheRecord *const rr = &largecr->r;
	RDataBody2 *const rdb = (RDataBody2 *)rr->smallrdatastorage.data;
	mDNSu16 pktrdlength;
	
	if (largecr == &m->rec && m->rec.r.resrec.RecordType)
		{
		LogMsg("GetLargeResourceRecord: m->rec appears to be already in use for %s", CRDisplayString(m, &m->rec.r));
#if ForceAlerts
		*(long*)0 = 0;
#endif
		}

	rr->next              = mDNSNULL;
	rr->resrec.name       = &largecr->namestorage;

	rr->NextInKAList      = mDNSNULL;
	rr->TimeRcvd          = m ? m->timenow : 0;
	rr->DelayDelivery     = 0;
	rr->NextRequiredQuery = m ? m->timenow : 0;		// Will be updated to the real value when we call SetNextCacheCheckTimeForRecord()
	rr->LastUsed          = m ? m->timenow : 0;
	rr->CRActiveQuestion  = mDNSNULL;
	rr->UnansweredQueries = 0;
	rr->LastUnansweredTime= 0;
#if ENABLE_MULTI_PACKET_QUERY_SNOOPING
	rr->MPUnansweredQ     = 0;
	rr->MPLastUnansweredQT= 0;
	rr->MPUnansweredKA    = 0;
	rr->MPExpectingKA     = mDNSfalse;
#endif
	rr->NextInCFList      = mDNSNULL;

	rr->resrec.InterfaceID       = InterfaceID;
	rr->resrec.rDNSServer = mDNSNULL;

	ptr = getDomainName(msg, ptr, end, &largecr->namestorage);		// Will bail out correctly if ptr is NULL
	if (!ptr) { debugf("GetLargeResourceRecord: Malformed RR name"); return(mDNSNULL); }
	rr->resrec.namehash = DomainNameHashValue(rr->resrec.name);

	if (ptr + 10 > end) { debugf("GetLargeResourceRecord: Malformed RR -- no type/class/ttl/len!"); return(mDNSNULL); }

	rr->resrec.rrtype            = (mDNSu16) ((mDNSu16)ptr[0] <<  8 | ptr[1]);
	rr->resrec.rrclass           = (mDNSu16)(((mDNSu16)ptr[2] <<  8 | ptr[3]) & kDNSClass_Mask);
	rr->resrec.rroriginalttl     = (mDNSu32) ((mDNSu32)ptr[4] << 24 | (mDNSu32)ptr[5] << 16 | (mDNSu32)ptr[6] << 8 | ptr[7]);
	if (rr->resrec.rroriginalttl > 0x70000000UL / mDNSPlatformOneSecond && (mDNSs32)rr->resrec.rroriginalttl != -1)
		rr->resrec.rroriginalttl = 0x70000000UL / mDNSPlatformOneSecond;
	// Note: We don't have to adjust m->NextCacheCheck here -- this is just getting a record into memory for
	// us to look at. If we decide to copy it into the cache, then we'll update m->NextCacheCheck accordingly.
	pktrdlength           = (mDNSu16)((mDNSu16)ptr[8] <<  8 | ptr[9]);

	// If mDNS record has cache-flush bit set, we mark it unique
	// For uDNS records, all are implicitly deemed unique (a single DNS server is always
	// authoritative for the entire RRSet), unless this is a truncated response
	if (ptr[2] & (kDNSClass_UniqueRRSet >> 8) || (!InterfaceID && !(msg->h.flags.b[0] & kDNSFlag0_TC)))
		RecordType |= kDNSRecordTypePacketUniqueMask;
	ptr += 10;
	if (ptr + pktrdlength > end) { debugf("GetLargeResourceRecord: RDATA exceeds end of packet"); return(mDNSNULL); }
	end = ptr + pktrdlength;		// Adjust end to indicate the end of the rdata for this resource record

	rr->resrec.rdata = (RData*)&rr->smallrdatastorage;
	rr->resrec.rdata->MaxRDLength = MaximumRDSize;

	if (!RecordType) LogMsg("GetLargeResourceRecord: No RecordType for %##s", rr->resrec.name->c);

	// IMPORTANT: Any record type we understand and unpack into a structure containing domainnames needs to have corresponding
	// cases in SameRDataBody() and RDataHashValue() to do a semantic comparison (or checksum) of the structure instead of a blind
	// bitwise memory compare (or sum). This is because a domainname is a fixed size structure holding variable-length data.
	// Any bytes past the logical end of the name are undefined, and a blind bitwise memory compare may indicate that
	// two domainnames are different when semantically they are the same name and it's only the unused bytes that differ.
	if (rr->resrec.rrclass == kDNSQClass_ANY && pktrdlength == 0)	// Used in update packets to mean "Delete An RRset" (RFC 2136)
		rr->resrec.rdlength = 0;
	else switch (rr->resrec.rrtype)
		{
		case kDNSType_A:	if (pktrdlength != sizeof(mDNSv4Addr)) goto fail;
							rdb->ipv4.b[0] = ptr[0];
							rdb->ipv4.b[1] = ptr[1];
							rdb->ipv4.b[2] = ptr[2];
							rdb->ipv4.b[3] = ptr[3];
							break;

		case kDNSType_NS:
		case kDNSType_CNAME:
		case kDNSType_PTR:
		case kDNSType_DNAME:ptr = getDomainName(msg, ptr, end, &rdb->name);
							if (ptr != end) { debugf("GetLargeResourceRecord: Malformed CNAME/PTR RDATA name"); goto fail; }
							//debugf("%##s PTR %##s rdlen %d", rr->resrec.name.c, rdb->name.c, pktrdlength);
							break;

		case kDNSType_SOA:  ptr = getDomainName(msg, ptr, end, &rdb->soa.mname);
							if (!ptr)              { debugf("GetLargeResourceRecord: Malformed SOA RDATA mname"); goto fail; }
							ptr = getDomainName(msg, ptr, end, &rdb->soa.rname);
							if (!ptr)              { debugf("GetLargeResourceRecord: Malformed SOA RDATA rname"); goto fail; }
							if (ptr + 0x14 != end) { debugf("GetLargeResourceRecord: Malformed SOA RDATA");       goto fail; }
							rdb->soa.serial  = (mDNSs32) ((mDNSs32)ptr[0x00] << 24 | (mDNSs32)ptr[0x01] << 16 | (mDNSs32)ptr[0x02] << 8 | ptr[0x03]);
							rdb->soa.refresh = (mDNSu32) ((mDNSu32)ptr[0x04] << 24 | (mDNSu32)ptr[0x05] << 16 | (mDNSu32)ptr[0x06] << 8 | ptr[0x07]);
							rdb->soa.retry   = (mDNSu32) ((mDNSu32)ptr[0x08] << 24 | (mDNSu32)ptr[0x09] << 16 | (mDNSu32)ptr[0x0A] << 8 | ptr[0x0B]);
							rdb->soa.expire  = (mDNSu32) ((mDNSu32)ptr[0x0C] << 24 | (mDNSu32)ptr[0x0D] << 16 | (mDNSu32)ptr[0x0E] << 8 | ptr[0x0F]);
							rdb->soa.min     = (mDNSu32) ((mDNSu32)ptr[0x10] << 24 | (mDNSu32)ptr[0x11] << 16 | (mDNSu32)ptr[0x12] << 8 | ptr[0x13]);
							break;

		case kDNSType_NULL:
		case kDNSType_HINFO:
		case kDNSType_TSIG:
		case kDNSType_TXT:
		case kDNSType_X25:
		case kDNSType_ISDN:
		case kDNSType_LOC:
		case kDNSType_DHCID:if (pktrdlength > rr->resrec.rdata->MaxRDLength)
								{
								debugf("GetLargeResourceRecord: %s rdata size (%d) exceeds storage (%d)",
									DNSTypeName(rr->resrec.rrtype), pktrdlength, rr->resrec.rdata->MaxRDLength);
								goto fail;
								}
							rr->resrec.rdlength = pktrdlength;
							mDNSPlatformMemCopy(rdb->data, ptr, pktrdlength);
							break;

		case kDNSType_MX:
		case kDNSType_AFSDB:
		case kDNSType_RT:
		case kDNSType_KX:	if (pktrdlength < 3) goto fail;	// Preference + domainname
							rdb->mx.preference = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
							ptr = getDomainName(msg, ptr+2, end, &rdb->mx.exchange);
							if (ptr != end) { debugf("GetLargeResourceRecord: Malformed MX name"); goto fail; }
							//debugf("%##s SRV %##s rdlen %d", rr->resrec.name.c, rdb->srv.target.c, pktrdlength);
							break;

		case kDNSType_RP:	ptr = getDomainName(msg, ptr, end, &rdb->rp.mbox);	// Domainname + domainname
							if (!ptr)       { debugf("GetLargeResourceRecord: Malformed RP mbox"); goto fail; }
							ptr = getDomainName(msg, ptr, end, &rdb->rp.txt);
							if (ptr != end) { debugf("GetLargeResourceRecord: Malformed RP txt"); goto fail; }
							break;

		case kDNSType_PX:	if (pktrdlength < 4) goto fail;	// Preference + domainname + domainname
							rdb->px.preference = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
							ptr = getDomainName(msg, ptr, end, &rdb->px.map822);
							if (!ptr)       { debugf("GetLargeResourceRecord: Malformed PX map822"); goto fail; }
							ptr = getDomainName(msg, ptr, end, &rdb->px.mapx400);
							if (ptr != end) { debugf("GetLargeResourceRecord: Malformed PX mapx400"); goto fail; }
							break;

		case kDNSType_AAAA:	if (pktrdlength != sizeof(mDNSv6Addr)) goto fail;
							mDNSPlatformMemCopy(&rdb->ipv6, ptr, sizeof(rdb->ipv6));
							break;

		case kDNSType_SRV:	if (pktrdlength < 7) goto fail;	// Priority + weight + port + domainname
							rdb->srv.priority = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
							rdb->srv.weight   = (mDNSu16)((mDNSu16)ptr[2] <<  8 | ptr[3]);
							rdb->srv.port.b[0] = ptr[4];
							rdb->srv.port.b[1] = ptr[5];
							ptr = getDomainName(msg, ptr+6, end, &rdb->srv.target);
							if (ptr != end) { debugf("GetLargeResourceRecord: Malformed SRV RDATA name"); goto fail; }
							//debugf("%##s SRV %##s rdlen %d", rr->resrec.name.c, rdb->srv.target.c, pktrdlength);
							break;

		case kDNSType_OPT:	{
							rdataOPT *opt = rr->resrec.rdata->u.opt;
							rr->resrec.rdlength = 0;
							while (ptr < end && (mDNSu8 *)(opt+1) < rr->resrec.rdata->u.data + MaximumRDSize)
								{
								const rdataOPT *const currentopt = opt;
								if (ptr + 4 > end) { LogInfo("GetLargeResourceRecord: OPT RDATA ptr + 4 > end"); goto fail; }
								opt->opt    = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
								opt->optlen = (mDNSu16)((mDNSu16)ptr[2] <<  8 | ptr[3]);
								ptr += 4;
								if (ptr + opt->optlen > end) { LogInfo("GetLargeResourceRecord: ptr + opt->optlen > end"); goto fail; }
								switch (opt->opt)
									{
									case kDNSOpt_LLQ:
										if (opt->optlen == DNSOpt_LLQData_Space - 4)
											{
											opt->u.llq.vers  = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
											opt->u.llq.llqOp = (mDNSu16)((mDNSu16)ptr[2] <<  8 | ptr[3]);
											opt->u.llq.err   = (mDNSu16)((mDNSu16)ptr[4] <<  8 | ptr[5]);
											mDNSPlatformMemCopy(opt->u.llq.id.b, ptr+6, 8);
											opt->u.llq.llqlease = (mDNSu32) ((mDNSu32)ptr[14] << 24 | (mDNSu32)ptr[15] << 16 | (mDNSu32)ptr[16] << 8 | ptr[17]);
											if (opt->u.llq.llqlease > 0x70000000UL / mDNSPlatformOneSecond)
												opt->u.llq.llqlease = 0x70000000UL / mDNSPlatformOneSecond;
											opt++;
											}
										break;
									case kDNSOpt_Lease:
										if (opt->optlen == DNSOpt_LeaseData_Space - 4)
											{
											opt->u.updatelease = (mDNSu32) ((mDNSu32)ptr[0] << 24 | (mDNSu32)ptr[1] << 16 | (mDNSu32)ptr[2] << 8 | ptr[3]);
											if (opt->u.updatelease > 0x70000000UL / mDNSPlatformOneSecond)
												opt->u.updatelease = 0x70000000UL / mDNSPlatformOneSecond;
											opt++;
											}
										break;
									case kDNSOpt_Owner:
										if (ValidOwnerLength(opt->optlen))
											{
											opt->u.owner.vers = ptr[0];
											opt->u.owner.seq  = ptr[1];
											mDNSPlatformMemCopy(opt->u.owner.HMAC.b, ptr+2, 6);		// 6-byte MAC address
											mDNSPlatformMemCopy(opt->u.owner.IMAC.b, ptr+2, 6);		// 6-byte MAC address
											opt->u.owner.password = zeroEthAddr;
											if (opt->optlen >= DNSOpt_OwnerData_ID_Wake_Space-4)
												{
												mDNSPlatformMemCopy(opt->u.owner.IMAC.b, ptr+8, 6);	// 6-byte MAC address
												// This mDNSPlatformMemCopy is safe because the ValidOwnerLength(opt->optlen) check above
												// ensures that opt->optlen is no more than DNSOpt_OwnerData_ID_Wake_PW6_Space - 4
												if (opt->optlen > DNSOpt_OwnerData_ID_Wake_Space-4)
													mDNSPlatformMemCopy(opt->u.owner.password.b, ptr+14, opt->optlen - (DNSOpt_OwnerData_ID_Wake_Space-4));
												}
											opt++;
											}
										break;
									}
								ptr += currentopt->optlen;
								}
							rr->resrec.rdlength = (mDNSu16)((mDNSu8*)opt - rr->resrec.rdata->u.data);
							if (ptr != end) { LogInfo("GetLargeResourceRecord: Malformed OptRdata"); goto fail; }
							break;
							}

		case kDNSType_NSEC: {
							unsigned int i, j;
							domainname d;
							ptr = getDomainName(msg, ptr, end, &d);		// Ignored for our simplified use of NSEC synthetic records
							if (!ptr) { LogInfo("GetLargeResourceRecord: Malformed NSEC nextname"); goto fail; }
							mDNSPlatformMemZero(rdb->nsec.bitmap, sizeof(rdb->nsec.bitmap));
							if (ptr < end)
								{
								if (*ptr++ != 0) { debugf("GetLargeResourceRecord: We only handle block zero NSECs"); goto fail; }
								i = *ptr++;
								if (i > sizeof(rdataNSEC)) { debugf("GetLargeResourceRecord: invalid block length %d", i); goto fail; }
								for (j=0; j<i; j++) rdb->nsec.bitmap[j] = *ptr++;
								}
							if (ptr != end) { debugf("GetLargeResourceRecord: Malformed NSEC"); goto fail; }
							break;
							}

		default:			if (pktrdlength > rr->resrec.rdata->MaxRDLength)
								{
								debugf("GetLargeResourceRecord: rdata %d (%s) size (%d) exceeds storage (%d)",
									rr->resrec.rrtype, DNSTypeName(rr->resrec.rrtype), pktrdlength, rr->resrec.rdata->MaxRDLength);
								goto fail;
								}
							debugf("GetLargeResourceRecord: Warning! Reading resource type %d (%s) as opaque data",
								rr->resrec.rrtype, DNSTypeName(rr->resrec.rrtype));
							// Note: Just because we don't understand the record type, that doesn't
							// mean we fail. The DNS protocol specifies rdlength, so we can
							// safely skip over unknown records and ignore them.
							// We also grab a binary copy of the rdata anyway, since the caller
							// might know how to interpret it even if we don't.
							rr->resrec.rdlength = pktrdlength;
							mDNSPlatformMemCopy(rdb->data, ptr, pktrdlength);
							break;
		}

	SetNewRData(&rr->resrec, mDNSNULL, 0);		// Sets rdlength, rdestimate, rdatahash for us

	// Success! Now fill in RecordType to show this record contains valid data
	rr->resrec.RecordType = RecordType;
	return(end);

fail:
	// If we were unable to parse the rdata in this record, we indicate that by
	// returing a 'kDNSRecordTypePacketNegative' record with rdlength set to zero
	rr->resrec.RecordType = kDNSRecordTypePacketNegative;
	rr->resrec.rdlength   = 0;
	rr->resrec.rdestimate = 0;
	rr->resrec.rdatahash  = 0;
	return(end);
	}

mDNSexport const mDNSu8 *skipQuestion(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end)
	{
	ptr = skipDomainName(msg, ptr, end);
	if (!ptr) { debugf("skipQuestion: Malformed domain name in DNS question section"); return(mDNSNULL); }
	if (ptr+4 > end) { debugf("skipQuestion: Malformed DNS question section -- no query type and class!"); return(mDNSNULL); }
	return(ptr+4);
	}

mDNSexport const mDNSu8 *getQuestion(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end, const mDNSInterfaceID InterfaceID,
	DNSQuestion *question)
	{
	mDNSPlatformMemZero(question, sizeof(*question));
	question->InterfaceID = InterfaceID;
	if (!InterfaceID) question->TargetQID = onesID;	// In DNSQuestions we use TargetQID as the indicator of whether it's unicast or multicast
	ptr = getDomainName(msg, ptr, end, &question->qname);
	if (!ptr) { debugf("Malformed domain name in DNS question section"); return(mDNSNULL); }
	if (ptr+4 > end) { debugf("Malformed DNS question section -- no query type and class!"); return(mDNSNULL); }

	question->qnamehash = DomainNameHashValue(&question->qname);
	question->qtype  = (mDNSu16)((mDNSu16)ptr[0] << 8 | ptr[1]);			// Get type
	question->qclass = (mDNSu16)((mDNSu16)ptr[2] << 8 | ptr[3]);			// and class
	return(ptr+4);
	}

mDNSexport const mDNSu8 *LocateAnswers(const DNSMessage *const msg, const mDNSu8 *const end)
	{
	int i;
	const mDNSu8 *ptr = msg->data;
	for (i = 0; i < msg->h.numQuestions && ptr; i++) ptr = skipQuestion(msg, ptr, end);
	return(ptr);
	}

mDNSexport const mDNSu8 *LocateAuthorities(const DNSMessage *const msg, const mDNSu8 *const end)
	{
	int i;
	const mDNSu8 *ptr = LocateAnswers(msg, end);
	for (i = 0; i < msg->h.numAnswers && ptr; i++) ptr = skipResourceRecord(msg, ptr, end);
	return(ptr);
	}

mDNSexport const mDNSu8 *LocateAdditionals(const DNSMessage *const msg, const mDNSu8 *const end)
	{
	int i;
	const mDNSu8 *ptr = LocateAuthorities(msg, end);
	for (i = 0; i < msg->h.numAuthorities; i++) ptr = skipResourceRecord(msg, ptr, end);
	return (ptr);
	}

mDNSexport const mDNSu8 *LocateOptRR(const DNSMessage *const msg, const mDNSu8 *const end, int minsize)
	{
	int i;
	const mDNSu8 *ptr = LocateAdditionals(msg, end);

	// Locate the OPT record.
	// According to RFC 2671, "One OPT pseudo-RR can be added to the additional data section of either a request or a response."
	// This implies that there may be *at most* one OPT record per DNS message, in the Additional Section,
	// but not necessarily the *last* entry in the Additional Section.
	for (i = 0; ptr && i < msg->h.numAdditionals; i++)
		{
		if (ptr + DNSOpt_Header_Space + minsize <= end &&	// Make sure we have 11+minsize bytes of data
			ptr[0] == 0                                &&	// Name must be root label
			ptr[1] == (kDNSType_OPT >> 8  )            &&	// rrtype OPT
			ptr[2] == (kDNSType_OPT & 0xFF)            &&
			((mDNSu16)ptr[9] << 8 | (mDNSu16)ptr[10]) >= (mDNSu16)minsize)
			return(ptr);
		else
			ptr = skipResourceRecord(msg, ptr, end);
		}
	return(mDNSNULL);
	}

// On success, GetLLQOptData returns pointer to storage within shared "m->rec";
// it is caller's responsibilty to clear m->rec.r.resrec.RecordType after use
// Note: An OPT RDataBody actually contains one or more variable-length rdataOPT objects packed together
// The code that currently calls this assumes there's only one, instead of iterating through the set
mDNSexport const rdataOPT *GetLLQOptData(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *const end)
	{
	const mDNSu8 *ptr = LocateOptRR(msg, end, DNSOpt_LLQData_Space);
	if (ptr)
		{
		ptr = GetLargeResourceRecord(m, msg, ptr, end, 0, kDNSRecordTypePacketAdd, &m->rec);
		if (ptr && m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative) return(&m->rec.r.resrec.rdata->u.opt[0]);
		}
	return(mDNSNULL);
	}

// Get the lease life of records in a dynamic update
// returns 0 on error or if no lease present
mDNSexport mDNSu32 GetPktLease(mDNS *m, DNSMessage *msg, const mDNSu8 *end)
	{
	mDNSu32 result = 0;
	const mDNSu8 *ptr = LocateOptRR(msg, end, DNSOpt_LeaseData_Space);
	if (ptr) ptr = GetLargeResourceRecord(m, msg, ptr, end, 0, kDNSRecordTypePacketAdd, &m->rec);
	if (ptr && m->rec.r.resrec.rdlength >= DNSOpt_LeaseData_Space && m->rec.r.resrec.rdata->u.opt[0].opt == kDNSOpt_Lease)
		result = m->rec.r.resrec.rdata->u.opt[0].u.updatelease;
	m->rec.r.resrec.RecordType = 0;		// Clear RecordType to show we're not still using it
	return(result);
	}

mDNSlocal const mDNSu8 *DumpRecords(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end, int count, char *label)
	{
	int i;
	LogMsg("%2d %s", count, label);
	for (i = 0; i < count && ptr; i++)
		{
		// This puts a LargeCacheRecord on the stack instead of using the shared m->rec storage,
		// but since it's only used for debugging (and probably only on OS X, not on
		// embedded systems) putting a 9kB object on the stack isn't a big problem.
		LargeCacheRecord largecr;
		ptr = GetLargeResourceRecord(m, msg, ptr, end, mDNSInterface_Any, kDNSRecordTypePacketAns, &largecr);
		if (ptr) LogMsg("%2d TTL%8d %s", i, largecr.r.resrec.rroriginalttl, CRDisplayString(m, &largecr.r));
		}
	if (!ptr) LogMsg("ERROR: Premature end of packet data");
	return(ptr);
	}

#define DNS_OP_Name(X) (                              \
	(X) == kDNSFlag0_OP_StdQuery ? ""         :       \
	(X) == kDNSFlag0_OP_Iquery   ? "Iquery "  :       \
	(X) == kDNSFlag0_OP_Status   ? "Status "  :       \
	(X) == kDNSFlag0_OP_Unused3  ? "Unused3 " :       \
	(X) == kDNSFlag0_OP_Notify   ? "Notify "  :       \
	(X) == kDNSFlag0_OP_Update   ? "Update "  : "?? " )

#define DNS_RC_Name(X) (                             \
	(X) == kDNSFlag1_RC_NoErr    ? "NoErr"    :      \
	(X) == kDNSFlag1_RC_FormErr  ? "FormErr"  :      \
	(X) == kDNSFlag1_RC_ServFail ? "ServFail" :      \
	(X) == kDNSFlag1_RC_NXDomain ? "NXDomain" :      \
	(X) == kDNSFlag1_RC_NotImpl  ? "NotImpl"  :      \
	(X) == kDNSFlag1_RC_Refused  ? "Refused"  :      \
	(X) == kDNSFlag1_RC_YXDomain ? "YXDomain" :      \
	(X) == kDNSFlag1_RC_YXRRSet  ? "YXRRSet"  :      \
	(X) == kDNSFlag1_RC_NXRRSet  ? "NXRRSet"  :      \
	(X) == kDNSFlag1_RC_NotAuth  ? "NotAuth"  :      \
	(X) == kDNSFlag1_RC_NotZone  ? "NotZone"  : "??" )

// Note: DumpPacket expects the packet header fields in host byte order, not network byte order
mDNSexport void DumpPacket(mDNS *const m, mStatus status, mDNSBool sent, char *transport,
	const mDNSAddr *srcaddr, mDNSIPPort srcport,
	const mDNSAddr *dstaddr, mDNSIPPort dstport, const DNSMessage *const msg, const mDNSu8 *const end)
	{
	mDNSBool IsUpdate = ((msg->h.flags.b[0] & kDNSFlag0_OP_Mask) == kDNSFlag0_OP_Update);
	const mDNSu8 *ptr = msg->data;
	int i;
	DNSQuestion q;
	char tbuffer[64], sbuffer[64], dbuffer[64] = "";
	if (!status) tbuffer[mDNS_snprintf(tbuffer, sizeof(tbuffer), sent ? "Sent" : "Received"                        )] = 0;
	else         tbuffer[mDNS_snprintf(tbuffer, sizeof(tbuffer), "ERROR %d %sing", status, sent ? "Send" : "Receiv")] = 0;
	if (sent) sbuffer[mDNS_snprintf(sbuffer, sizeof(sbuffer), "port "        )] = 0;
	else      sbuffer[mDNS_snprintf(sbuffer, sizeof(sbuffer), "%#a:", srcaddr)] = 0;
	if (dstaddr || !mDNSIPPortIsZero(dstport))
		dbuffer[mDNS_snprintf(dbuffer, sizeof(dbuffer), " to %#a:%d", dstaddr, mDNSVal16(dstport))] = 0;

	LogMsg("-- %s %s DNS %s%s (flags %02X%02X) RCODE: %s (%d) %s%s%s%s%s%sID: %d %d bytes from %s%d%s%s --",
		tbuffer, transport,
		DNS_OP_Name(msg->h.flags.b[0] & kDNSFlag0_OP_Mask),
		msg->h.flags.b[0] & kDNSFlag0_QR_Response ? "Response" : "Query",
		msg->h.flags.b[0], msg->h.flags.b[1],
		DNS_RC_Name(msg->h.flags.b[1] & kDNSFlag1_RC_Mask),
		msg->h.flags.b[1] & kDNSFlag1_RC_Mask,
		msg->h.flags.b[0] & kDNSFlag0_AA ? "AA " : "",
		msg->h.flags.b[0] & kDNSFlag0_TC ? "TC " : "",
		msg->h.flags.b[0] & kDNSFlag0_RD ? "RD " : "",
		msg->h.flags.b[1] & kDNSFlag1_RA ? "RA " : "",
		msg->h.flags.b[1] & kDNSFlag1_AD ? "AD " : "",
		msg->h.flags.b[1] & kDNSFlag1_CD ? "CD " : "",
		mDNSVal16(msg->h.id),
		end - msg->data,
		sbuffer, mDNSVal16(srcport), dbuffer,
		(msg->h.flags.b[0] & kDNSFlag0_TC) ? " (truncated)" : ""
		);

	LogMsg("%2d %s", msg->h.numQuestions, IsUpdate ? "Zone" : "Questions");
	for (i = 0; i < msg->h.numQuestions && ptr; i++)
		{
		ptr = getQuestion(msg, ptr, end, mDNSInterface_Any, &q);
		if (ptr) LogMsg("%2d %##s %s", i, q.qname.c, DNSTypeName(q.qtype));
		}
	ptr = DumpRecords(m, msg, ptr, end, msg->h.numAnswers,     IsUpdate ? "Prerequisites" : "Answers");
	ptr = DumpRecords(m, msg, ptr, end, msg->h.numAuthorities, IsUpdate ? "Updates"       : "Authorities");
	ptr = DumpRecords(m, msg, ptr, end, msg->h.numAdditionals, "Additionals");
	LogMsg("--------------");
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Packet Sending Functions
#endif

// Stub definition of TCPSocket_struct so we can access flags field. (Rest of TCPSocket_struct is platform-dependent.)
struct TCPSocket_struct { TCPSocketFlags flags; /* ... */ };

struct UDPSocket_struct
	{
	mDNSIPPort port; // MUST BE FIRST FIELD -- mDNSCoreReceive expects every UDPSocket_struct to begin with mDNSIPPort port
	};

// Note: When we sign a DNS message using DNSDigest_SignMessage(), the current real-time clock value is used, which
// is why we generally defer signing until we send the message, to ensure the signature is as fresh as possible.
mDNSexport mStatus mDNSSendDNSMessage(mDNS *const m, DNSMessage *const msg, mDNSu8 *end,
    mDNSInterfaceID InterfaceID, UDPSocket *src, const mDNSAddr *dst, mDNSIPPort dstport, TCPSocket *sock, DomainAuthInfo *authInfo)
	{
	mStatus status = mStatus_NoError;
	const mDNSu16 numAdditionals = msg->h.numAdditionals;
	mDNSu8 *newend;
	mDNSu8 *limit = msg->data + AbsoluteMaxDNSMessageData;

	// Zero-length message data is okay (e.g. for a DNS Update ack, where all we need is an ID and an error code
	if (end < msg->data || end - msg->data > AbsoluteMaxDNSMessageData)
		{
		LogMsg("mDNSSendDNSMessage: invalid message %p %p %d", msg->data, end, end - msg->data);
		return mStatus_BadParamErr;
		}

	newend = putHINFO(m, msg, end, authInfo, limit);
	if (!newend) LogMsg("mDNSSendDNSMessage: putHINFO failed msg %p end %p, limit %p", msg->data, end, limit); // Not fatal
	else end = newend;
	
	// Put all the integer values in IETF byte-order (MSB first, LSB second)
	SwapDNSHeaderBytes(msg);
	
	if (authInfo) DNSDigest_SignMessage(msg, &end, authInfo, 0);	// DNSDigest_SignMessage operates on message in network byte order
	if (!end) { LogMsg("mDNSSendDNSMessage: DNSDigest_SignMessage failed"); status = mStatus_NoMemoryErr; }
	else
		{
		// Send the packet on the wire
		if (!sock)
			status = mDNSPlatformSendUDP(m, msg, end, InterfaceID, src, dst, dstport);
		else
			{
			mDNSu16 msglen = (mDNSu16)(end - (mDNSu8 *)msg);
			mDNSu8 lenbuf[2] = { (mDNSu8)(msglen >> 8), (mDNSu8)(msglen & 0xFF) };
			long nsent = mDNSPlatformWriteTCP(sock, (char*)lenbuf, 2);		// Should do scatter/gather here -- this is probably going out as two packets
			if (nsent != 2) { LogMsg("mDNSSendDNSMessage: write msg length failed %d/%d", nsent, 2); status = mStatus_ConnFailed; }
			else
				{
				nsent = mDNSPlatformWriteTCP(sock, (char *)msg, msglen);
				if (nsent != msglen) { LogMsg("mDNSSendDNSMessage: write msg body failed %d/%d", nsent, msglen); status = mStatus_ConnFailed; }
				}
			}
		}

	// Swap the integer values back the way they were (remember that numAdditionals may have been changed by putHINFO and/or SignMessage)
	SwapDNSHeaderBytes(msg);

	// Dump the packet with the HINFO and TSIG
	if (mDNS_PacketLoggingEnabled && !mDNSOpaque16IsZero(msg->h.id))
		DumpPacket(m, status, mDNStrue, sock && (sock->flags & kTCPSocketFlags_UseTLS) ? "TLS" : sock ? "TCP" : "UDP", mDNSNULL, src ? src->port : MulticastDNSPort, dst, dstport, msg, end);

	// put the number of additionals back the way it was
	msg->h.numAdditionals = numAdditionals;

	return(status);
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - RR List Management & Task Management
#endif

mDNSexport void mDNS_Lock_(mDNS *const m, const char * const functionname)
	{
	// MUST grab the platform lock FIRST!
	mDNSPlatformLock(m);

	// Normally, mDNS_reentrancy is zero and so is mDNS_busy
	// However, when we call a client callback mDNS_busy is one, and we increment mDNS_reentrancy too
	// If that client callback does mDNS API calls, mDNS_reentrancy and mDNS_busy will both be one
	// If mDNS_busy != mDNS_reentrancy that's a bad sign
	if (m->mDNS_busy != m->mDNS_reentrancy)
		{
		LogMsg("%s: mDNS_Lock: Locking failure! mDNS_busy (%ld) != mDNS_reentrancy (%ld)", functionname, m->mDNS_busy, m->mDNS_reentrancy);
#if ForceAlerts
		*(long*)0 = 0;
#endif
		}

	// If this is an initial entry into the mDNSCore code, set m->timenow
	// else, if this is a re-entrant entry into the mDNSCore code, m->timenow should already be set
	if (m->mDNS_busy == 0)
		{
		if (m->timenow)
			LogMsg("%s: mDNS_Lock: m->timenow already set (%ld/%ld)", functionname, m->timenow, mDNS_TimeNow_NoLock(m));
		m->timenow = mDNS_TimeNow_NoLock(m);
		if (m->timenow == 0) m->timenow = 1;
		}
	else if (m->timenow == 0)
		{
		LogMsg("%s: mDNS_Lock: m->mDNS_busy is %ld but m->timenow not set", functionname, m->mDNS_busy);
		m->timenow = mDNS_TimeNow_NoLock(m);
		if (m->timenow == 0) m->timenow = 1;
		}

	if (m->timenow_last - m->timenow > 0)
		{
		m->timenow_adjust += m->timenow_last - m->timenow;
		debugf("%s: mDNSPlatformRawTime went backwards by %ld ticks; setting correction factor to %ld", functionname, m->timenow_last - m->timenow, m->timenow_adjust);
		m->timenow = m->timenow_last;
		}
	m->timenow_last = m->timenow;

	// Increment mDNS_busy so we'll recognise re-entrant calls
	m->mDNS_busy++;
	}

mDNSlocal AuthRecord *AnyLocalRecordReady(const mDNS *const m)
	{
	AuthRecord *rr;
	for (rr = m->NewLocalRecords; rr; rr = rr->next)
		if (LocalRecordReady(rr)) return rr;
	return mDNSNULL;
	}

mDNSlocal mDNSs32 GetNextScheduledEvent(const mDNS *const m)
	{
	mDNSs32 e = m->timenow + 0x78000000;
	if (m->mDNSPlatformStatus != mStatus_NoError) return(e);
	if (m->NewQuestions)
		{
		if (m->NewQuestions->DelayAnswering) e = m->NewQuestions->DelayAnswering;
		else return(m->timenow);
		}
	if (m->NewLocalOnlyQuestions)                     return(m->timenow);
	if (m->NewLocalRecords && AnyLocalRecordReady(m)) return(m->timenow);
	if (m->NewLocalOnlyRecords)                       return(m->timenow);
	if (m->SPSProxyListChanged)                       return(m->timenow);
	if (m->LocalRemoveEvents)                         return(m->timenow);

#ifndef UNICAST_DISABLED
	if (e - m->NextuDNSEvent         > 0) e = m->NextuDNSEvent;
	if (e - m->NextScheduledNATOp    > 0) e = m->NextScheduledNATOp;
	if (m->NextSRVUpdate && e - m->NextSRVUpdate > 0) e = m->NextSRVUpdate;
#endif

	if (e - m->NextCacheCheck        > 0) e = m->NextCacheCheck;
	if (e - m->NextScheduledSPS      > 0) e = m->NextScheduledSPS;
	// NextScheduledSPRetry only valid when DelaySleep not set
	if (!m->DelaySleep && m->SleepLimit && e - m->NextScheduledSPRetry > 0) e = m->NextScheduledSPRetry;
	if (m->DelaySleep && e - m->DelaySleep > 0) e = m->DelaySleep;

	if (m->SuppressSending)
		{
		if (e - m->SuppressSending       > 0) e = m->SuppressSending;
		}
	else
		{
		if (e - m->NextScheduledQuery    > 0) e = m->NextScheduledQuery;
		if (e - m->NextScheduledProbe    > 0) e = m->NextScheduledProbe;
		if (e - m->NextScheduledResponse > 0) e = m->NextScheduledResponse;
		}
	if (e - m->NextScheduledStopTime > 0) e = m->NextScheduledStopTime;
	return(e);
	}

mDNSexport void ShowTaskSchedulingError(mDNS *const m)
	{
	AuthRecord *rr;
	mDNS_Lock(m);

	LogMsg("Task Scheduling Error: Continuously busy for more than a second");
	
	// Note: To accurately diagnose *why* we're busy, the debugging code here needs to mirror the logic in GetNextScheduledEvent above

	if (m->NewQuestions && (!m->NewQuestions->DelayAnswering || m->timenow - m->NewQuestions->DelayAnswering >= 0))
		LogMsg("Task Scheduling Error: NewQuestion %##s (%s)",
			m->NewQuestions->qname.c, DNSTypeName(m->NewQuestions->qtype));

	if (m->NewLocalOnlyQuestions)
		LogMsg("Task Scheduling Error: NewLocalOnlyQuestions %##s (%s)",
			m->NewLocalOnlyQuestions->qname.c, DNSTypeName(m->NewLocalOnlyQuestions->qtype));

	if (m->NewLocalRecords)
		{
		rr = AnyLocalRecordReady(m);
		if (rr) LogMsg("Task Scheduling Error: NewLocalRecords %s", ARDisplayString(m, rr));
		}
	
	if (m->NewLocalOnlyRecords) LogMsg("Task Scheduling Error: NewLocalOnlyRecords");

	if (m->SPSProxyListChanged) LogMsg("Task Scheduling Error: SPSProxyListChanged");
	if (m->LocalRemoveEvents)   LogMsg("Task Scheduling Error: LocalRemoveEvents");

	if (m->timenow - m->NextScheduledEvent    >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledEvent %d",    m->timenow - m->NextScheduledEvent);

#ifndef UNICAST_DISABLED
	if (m->timenow - m->NextuDNSEvent         >= 0)
		LogMsg("Task Scheduling Error: m->NextuDNSEvent %d",         m->timenow - m->NextuDNSEvent);
	if (m->timenow - m->NextScheduledNATOp    >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledNATOp %d",    m->timenow - m->NextScheduledNATOp);
	if (m->NextSRVUpdate && m->timenow - m->NextSRVUpdate >= 0)
		LogMsg("Task Scheduling Error: m->NextSRVUpdate %d",         m->timenow - m->NextSRVUpdate);
#endif

	if (m->timenow - m->NextCacheCheck        >= 0)
		LogMsg("Task Scheduling Error: m->NextCacheCheck %d",        m->timenow - m->NextCacheCheck);
	if (m->timenow - m->NextScheduledSPS      >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledSPS %d",      m->timenow - m->NextScheduledSPS);
	if (!m->DelaySleep && m->SleepLimit && m->timenow - m->NextScheduledSPRetry >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledSPRetry %d",  m->timenow - m->NextScheduledSPRetry);
	if (m->DelaySleep && m->timenow - m->DelaySleep >= 0)
		LogMsg("Task Scheduling Error: m->DelaySleep %d",            m->timenow - m->DelaySleep);

	if (m->SuppressSending && m->timenow - m->SuppressSending >= 0)
		LogMsg("Task Scheduling Error: m->SuppressSending %d",       m->timenow - m->SuppressSending);
	if (m->timenow - m->NextScheduledQuery    >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledQuery %d",    m->timenow - m->NextScheduledQuery);
	if (m->timenow - m->NextScheduledProbe    >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledProbe %d",    m->timenow - m->NextScheduledProbe);
	if (m->timenow - m->NextScheduledResponse >= 0)
		LogMsg("Task Scheduling Error: m->NextScheduledResponse %d", m->timenow - m->NextScheduledResponse);

	mDNS_Unlock(m);
	}

mDNSexport void mDNS_Unlock_(mDNS *const m, const char * const functionname)
	{
	// Decrement mDNS_busy
	m->mDNS_busy--;
	
	// Check for locking failures
	if (m->mDNS_busy != m->mDNS_reentrancy)
		{
		LogMsg("%s: mDNS_Unlock: Locking failure! mDNS_busy (%ld) != mDNS_reentrancy (%ld)", functionname, m->mDNS_busy, m->mDNS_reentrancy);
#if ForceAlerts
		*(long*)0 = 0;
#endif
		}

	// If this is a final exit from the mDNSCore code, set m->NextScheduledEvent and clear m->timenow
	if (m->mDNS_busy == 0)
		{
		m->NextScheduledEvent = GetNextScheduledEvent(m);
		if (m->timenow == 0) LogMsg("%s: mDNS_Unlock: ERROR! m->timenow aready zero", functionname);
		m->timenow = 0;
		}

	// MUST release the platform lock LAST!
	mDNSPlatformUnlock(m);
	}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Specialized mDNS version of vsnprintf
#endif

static const struct mDNSprintf_format
	{
	unsigned      leftJustify : 1;
	unsigned      forceSign : 1;
	unsigned      zeroPad : 1;
	unsigned      havePrecision : 1;
	unsigned      hSize : 1;
	unsigned      lSize : 1;
	char          altForm;
	char          sign;		// +, - or space
	unsigned int  fieldWidth;
	unsigned int  precision;
	} mDNSprintf_format_default = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

mDNSexport mDNSu32 mDNS_vsnprintf(char *sbuffer, mDNSu32 buflen, const char *fmt, va_list arg)
	{
	mDNSu32 nwritten = 0;
	int c;
	if (buflen == 0) return(0);
	buflen--;		// Pre-reserve one space in the buffer for the terminating null
	if (buflen == 0) goto exit;

	for (c = *fmt; c != 0; c = *++fmt)
		{
		if (c != '%')
			{
			*sbuffer++ = (char)c;
			if (++nwritten >= buflen) goto exit;
			}
		else
			{
			unsigned int i=0, j;
			// The mDNS Vsprintf Argument Conversion Buffer is used as a temporary holding area for
			// generating decimal numbers, hexdecimal numbers, IP addresses, domain name strings, etc.
			// The size needs to be enough for a 256-byte domain name plus some error text.
			#define mDNS_VACB_Size 300
			char mDNS_VACB[mDNS_VACB_Size];
			#define mDNS_VACB_Lim (&mDNS_VACB[mDNS_VACB_Size])
			#define mDNS_VACB_Remain(s) ((mDNSu32)(mDNS_VACB_Lim - s))
			char *s = mDNS_VACB_Lim, *digits;
			struct mDNSprintf_format F = mDNSprintf_format_default;
	
			while (1)	//  decode flags
				{
				c = *++fmt;
				if      (c == '-') F.leftJustify = 1;
				else if (c == '+') F.forceSign = 1;
				else if (c == ' ') F.sign = ' ';
				else if (c == '#') F.altForm++;
				else if (c == '0') F.zeroPad = 1;
				else break;
				}
	
			if (c == '*')	//  decode field width
				{
				int f = va_arg(arg, int);
				if (f < 0) { f = -f; F.leftJustify = 1; }
				F.fieldWidth = (unsigned int)f;
				c = *++fmt;
				}
			else
				{
				for (; c >= '0' && c <= '9'; c = *++fmt)
					F.fieldWidth = (10 * F.fieldWidth) + (c - '0');
				}
	
			if (c == '.')	//  decode precision
				{
				if ((c = *++fmt) == '*')
					{ F.precision = va_arg(arg, unsigned int); c = *++fmt; }
				else for (; c >= '0' && c <= '9'; c = *++fmt)
						F.precision = (10 * F.precision) + (c - '0');
				F.havePrecision = 1;
				}
	
			if (F.leftJustify) F.zeroPad = 0;
	
			conv:
			switch (c)	//  perform appropriate conversion
				{
				unsigned long n;
				case 'h' :	F.hSize = 1; c = *++fmt; goto conv;
				case 'l' :	// fall through
				case 'L' :	F.lSize = 1; c = *++fmt; goto conv;
				case 'd' :
				case 'i' :	if (F.lSize) n = (unsigned long)va_arg(arg, long);
							else n = (unsigned long)va_arg(arg, int);
							if (F.hSize) n = (short) n;
							if ((long) n < 0) { n = (unsigned long)-(long)n; F.sign = '-'; }
							else if (F.forceSign) F.sign = '+';
							goto decimal;
				case 'u' :	if (F.lSize) n = va_arg(arg, unsigned long);
							else n = va_arg(arg, unsigned int);
							if (F.hSize) n = (unsigned short) n;
							F.sign = 0;
							goto decimal;
				decimal:	if (!F.havePrecision)
								{
								if (F.zeroPad)
									{
									F.precision = F.fieldWidth;
									if (F.sign) --F.precision;
									}
								if (F.precision < 1) F.precision = 1;
								}
							if (F.precision > mDNS_VACB_Size - 1)
								F.precision = mDNS_VACB_Size - 1;
							for (i = 0; n; n /= 10, i++) *--s = (char)(n % 10 + '0');
							for (; i < F.precision; i++) *--s = '0';
							if (F.sign) { *--s = F.sign; i++; }
							break;
	
				case 'o' :	if (F.lSize) n = va_arg(arg, unsigned long);
							else n = va_arg(arg, unsigned int);
							if (F.hSize) n = (unsigned short) n;
							if (!F.havePrecision)
								{
								if (F.zeroPad) F.precision = F.fieldWidth;
								if (F.precision < 1) F.precision = 1;
								}
							if (F.precision > mDNS_VACB_Size - 1)
								F.precision = mDNS_VACB_Size - 1;
							for (i = 0; n; n /= 8, i++) *--s = (char)(n % 8 + '0');
							if (F.altForm && i && *s != '0') { *--s = '0'; i++; }
							for (; i < F.precision; i++) *--s = '0';
							break;
	
				case 'a' :	{
							unsigned char *a = va_arg(arg, unsigned char *);
							if (!a) { static char emsg[] = "<<NULL>>"; s = emsg; i = sizeof(emsg)-1; }
							else
								{
								s = mDNS_VACB;	// Adjust s to point to the start of the buffer, not the end
								if (F.altForm)
									{
									mDNSAddr *ip = (mDNSAddr*)a;
									switch (ip->type)
										{
										case mDNSAddrType_IPv4: F.precision =  4; a = (unsigned char *)&ip->ip.v4; break;
										case mDNSAddrType_IPv6: F.precision = 16; a = (unsigned char *)&ip->ip.v6; break;
										default:                F.precision =  0; break;
										}
									}
								if (F.altForm && !F.precision)
									i = mDNS_snprintf(mDNS_VACB, sizeof(mDNS_VACB), "«ZERO ADDRESS»");
								else switch (F.precision)
									{
									case  4: i = mDNS_snprintf(mDNS_VACB, sizeof(mDNS_VACB), "%d.%d.%d.%d",
														a[0], a[1], a[2], a[3]); break;
									case  6: i = mDNS_snprintf(mDNS_VACB, sizeof(mDNS_VACB), "%02X:%02X:%02X:%02X:%02X:%02X",
														a[0], a[1], a[2], a[3], a[4], a[5]); break;
									case 16: i = mDNS_snprintf(mDNS_VACB, sizeof(mDNS_VACB),
														"%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
														a[0x0], a[0x1], a[0x2], a[0x3], a[0x4], a[0x5], a[0x6], a[0x7],
														a[0x8], a[0x9], a[0xA], a[0xB], a[0xC], a[0xD], a[0xE], a[0xF]); break;
									default: i = mDNS_snprintf(mDNS_VACB, sizeof(mDNS_VACB), "%s", "<< ERROR: Must specify"
														" address size (i.e. %.4a=IPv4, %.6a=Ethernet, %.16a=IPv6) >>"); break;
									}
								}
							}
							break;
	
				case 'p' :	F.havePrecision = F.lSize = 1;
							F.precision = sizeof(void*) * 2;	// 8 characters on 32-bit; 16 characters on 64-bit
				case 'X' :	digits = "0123456789ABCDEF";
							goto hexadecimal;
				case 'x' :	digits = "0123456789abcdef";
				hexadecimal:if (F.lSize) n = va_arg(arg, unsigned long);
							else n = va_arg(arg, unsigned int);
							if (F.hSize) n = (unsigned short) n;
							if (!F.havePrecision)
								{
								if (F.zeroPad)
									{
									F.precision = F.fieldWidth;
									if (F.altForm) F.precision -= 2;
									}
								if (F.precision < 1) F.precision = 1;
								}
							if (F.precision > mDNS_VACB_Size - 1)
								F.precision = mDNS_VACB_Size - 1;
							for (i = 0; n; n /= 16, i++) *--s = digits[n % 16];
							for (; i < F.precision; i++) *--s = '0';
							if (F.altForm) { *--s = (char)c; *--s = '0'; i += 2; }
							break;
	
				case 'c' :	*--s = (char)va_arg(arg, int); i = 1; break;
	
				case 's' :	s = va_arg(arg, char *);
							if (!s) { static char emsg[] = "<<NULL>>"; s = emsg; i = sizeof(emsg)-1; }
							else switch (F.altForm)
								{
								case 0: i=0;
										if (!F.havePrecision)				// C string
											while (s[i]) i++;
										else
											{
											while ((i < F.precision) && s[i]) i++;
											// Make sure we don't truncate in the middle of a UTF-8 character
											// If last character we got was any kind of UTF-8 multi-byte character,
											// then see if we have to back up.
											// This is not as easy as the similar checks below, because
											// here we can't assume it's safe to examine the *next* byte, so we
											// have to confine ourselves to working only backwards in the string.
											j = i;		// Record where we got to
											// Now, back up until we find first non-continuation-char
											while (i>0 && (s[i-1] & 0xC0) == 0x80) i--;
											// Now s[i-1] is the first non-continuation-char
											// and (j-i) is the number of continuation-chars we found
											if (i>0 && (s[i-1] & 0xC0) == 0xC0)	// If we found a start-char
												{
												i--;		// Tentatively eliminate this start-char as well
												// Now (j-i) is the number of characters we're considering eliminating.
												// To be legal UTF-8, the start-char must contain (j-i) one-bits,
												// followed by a zero bit. If we shift it right by (7-(j-i)) bits
												// (with sign extension) then the result has to be 0xFE.
												// If this is right, then we reinstate the tentatively eliminated bytes.
												if (((j-i) < 7) && (((s[i] >> (7-(j-i))) & 0xFF) == 0xFE)) i = j;
												}
											}
										break;
								case 1: i = (unsigned char) *s++; break;	// Pascal string
								case 2: {									// DNS label-sequence name
										unsigned char *a = (unsigned char *)s;
										s = mDNS_VACB;	// Adjust s to point to the start of the buffer, not the end
										if (*a == 0) *s++ = '.';	// Special case for root DNS name
										while (*a)
											{
											char buf[63*4+1];
											if (*a > 63)
												{ s += mDNS_snprintf(s, mDNS_VACB_Remain(s), "<<INVALID LABEL LENGTH %u>>", *a); break; }
											if (s + *a >= &mDNS_VACB[254])
												{ s += mDNS_snprintf(s, mDNS_VACB_Remain(s), "<<NAME TOO LONG>>"); break; }
											// Need to use ConvertDomainLabelToCString to do proper escaping here,
											// so it's clear what's a literal dot and what's a label separator
											ConvertDomainLabelToCString((domainlabel*)a, buf);
											s += mDNS_snprintf(s, mDNS_VACB_Remain(s), "%s.", buf);
											a += 1 + *a;
											}
										i = (mDNSu32)(s - mDNS_VACB);
										s = mDNS_VACB;	// Reset s back to the start of the buffer
										break;
										}
								}
							// Make sure we don't truncate in the middle of a UTF-8 character (see similar comment below)
							if (F.havePrecision && i > F.precision)
								{ i = F.precision; while (i>0 && (s[i] & 0xC0) == 0x80) i--; }
							break;
	
				case 'n' :	s = va_arg(arg, char *);
							if      (F.hSize) * (short *) s = (short)nwritten;
							else if (F.lSize) * (long  *) s = (long)nwritten;
							else              * (int   *) s = (int)nwritten;
							continue;
	
				default:	s = mDNS_VACB;
							i = mDNS_snprintf(mDNS_VACB, sizeof(mDNS_VACB), "<<UNKNOWN FORMAT CONVERSION CODE %%%c>>", c);

				case '%' :	*sbuffer++ = (char)c;
							if (++nwritten >= buflen) goto exit;
							break;
				}
	
			if (i < F.fieldWidth && !F.leftJustify)			// Pad on the left
				do	{
					*sbuffer++ = ' ';
					if (++nwritten >= buflen) goto exit;
					} while (i < --F.fieldWidth);
	
			// Make sure we don't truncate in the middle of a UTF-8 character.
			// Note: s[i] is the first eliminated character; i.e. the next character *after* the last character of the
			// allowed output. If s[i] is a UTF-8 continuation character, then we've cut a unicode character in half,
			// so back up 'i' until s[i] is no longer a UTF-8 continuation character. (if the input was proprly
			// formed, s[i] will now be the UTF-8 start character of the multi-byte character we just eliminated).
			if (i > buflen - nwritten)
				{ i = buflen - nwritten; while (i>0 && (s[i] & 0xC0) == 0x80) i--; }
			for (j=0; j<i; j++) *sbuffer++ = *s++;			// Write the converted result
			nwritten += i;
			if (nwritten >= buflen) goto exit;
	
			for (; i < F.fieldWidth; i++)					// Pad on the right
				{
				*sbuffer++ = ' ';
				if (++nwritten >= buflen) goto exit;
				}
			}
		}
	exit:
	*sbuffer++ = 0;
	return(nwritten);
	}

mDNSexport mDNSu32 mDNS_snprintf(char *sbuffer, mDNSu32 buflen, const char *fmt, ...)
	{
	mDNSu32 length;
	
	va_list ptr;
	va_start(ptr,fmt);
	length = mDNS_vsnprintf(sbuffer, buflen, fmt, ptr);
	va_end(ptr);
	
	return(length);
	}
