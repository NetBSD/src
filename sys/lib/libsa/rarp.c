#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include <errno.h>
#include <string.h>

#include "stand.h"
#include "net.h"
#include "netif.h"

static int rarpsend(struct iodesc *, void *, int);
static int rarprecv(struct iodesc *, void *, int);

/*
 * Ethernet (Reverse) Address Resolution Protocol (see RFC 903, and 826).
 */
n_long
rarp_getipaddress(sock)
	int sock;
{
	struct iodesc *d;
	register struct ether_arp *ap;
	register void *pkt;
	struct {
		u_char header[HEADER_SIZE];
		struct ether_arp wrarp;
	} wbuf;
	union {
		u_char buffer[RECV_SIZE];
		struct {
			u_char header[HEADER_SIZE];
			struct ether_arp xrrarp;
		}xrbuf;
#define rrarp  xrbuf.xrrarp
	} rbuf;

#ifdef RARP_DEBUG
 	if (debug)
		printf("rarp: socket=%d\n", sock);
#endif
	if (!(d = socktodesc(sock))) {
		printf("rarp: bad socket. %d\n", sock);
		return(INADDR_ANY);
	}
#ifdef RARP_DEBUG
 	if (debug)
		printf("rarp: d=%x\n", (u_int)d);
#endif
	ap = &wbuf.wrarp;
	pkt = &rbuf.rrarp;
	pkt -= HEADER_SIZE;

	bzero(ap, sizeof(*ap));

	ap->arp_hrd = htons(ARPHRD_ETHER);
	ap->arp_pro = htons(ETHERTYPE_IP);
	ap->arp_hln = sizeof(ap->arp_sha); /* hardware address length */
	ap->arp_pln = sizeof(ap->arp_spa); /* protocol address length */
	ap->arp_op = htons(ARPOP_REQUEST);
	bcopy(d->myea, ap->arp_sha, 6);
	bcopy(d->myea, ap->arp_tha, 6);

	if (sendrecv(d,
		     rarpsend, ap, sizeof(*ap),
		     rarprecv, pkt, RECV_SIZE) < 0) {
		printf("No response for RARP request\n");
		return(INADDR_ANY);
	}

	return(myip);
}

/*
 * Broadcast a RARP request (i.e. who knows who I am)
 */
static int
rarpsend(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register int len;
{
#ifdef RARP_DEBUG
 	if (debug)
 	    printf("rarpsend: called\n");
#endif
	return (sendether(d, pkt, len, bcea, ETHERTYPE_REVARP));
}

/*
 * Called when packet containing RARP is received
 */
static int
rarprecv(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register int len;
{
	register struct ether_header *ep;
	register struct ether_arp *ap;

#ifdef RARP_DEBUG
 	if (debug)
 	    printf("rarprecv: called\n");
#endif
	if (len < sizeof(struct ether_header) + sizeof(struct ether_arp)) {
		errno = 0;
		return (-1);
	}

	ep = (struct ether_header *)pkt;
	if (ntohs(ep->ether_type) != ETHERTYPE_REVARP) {
		errno = 0;
		return (-1);
	}

	ap = (struct ether_arp *)(ep + 1);
	if (ntohs(ap->arp_op) != ARPOP_REPLY ||
	    ntohs(ap->arp_pro) != ETHERTYPE_IP)  {
		errno = 0;
		return (-1);
	}

	if (bcmp(ap->arp_tha, d->myea, 6)) {
		errno = 0;
		return (-1);
	}

	bcopy(ap->arp_tpa, (char *)&myip, sizeof(myip));
	bcopy(ap->arp_spa, (char *)&rootip, sizeof(rootip));

	return(0);
}
