#include "DNSCommon.h"                  // Defines general DNS utility routines

mDNSexport mStatus mDNS_InitStorage_ut(mDNS *const m, mDNS_PlatformSupport *const p,
									   CacheEntity *rrcachestorage, mDNSu32 rrcachesize,
									   mDNSBool AdvertiseLocalAddresses, mDNSCallback *Callback, void *Context)
{
	return mDNS_InitStorage(m, p, rrcachestorage, rrcachesize, AdvertiseLocalAddresses, Callback, Context);
}

mDNSexport mStatus ArpLogMsgTest(mDNS *const m, const ARP_EthIP *const arp, const mDNSInterfaceID InterfaceID)
{
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, InterfaceID);
    static const char msg[] = "ARP Req message";
    LogMsg("Arp %-7s %s %.6a %.4a for %.4a",
           intf->ifname, msg, arp->sha.b, arp->spa.b, arp->tpa.b);
    return mStatus_NoError;
}
