/* $NetBSD: nif_fxp.c,v 1.1.2.1 2007/06/02 08:44:38 nisimura Exp $ */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

struct iodesc sockets[2]; /* SOPEN_MAX */

struct iodesc *socktodesc(int);
void netif_init(void);
int netif_open(void *);
int netif_close(int);
ssize_t netif_put(struct iodesc *, void *, size_t);
ssize_t netif_get(struct iodesc *, void *, size_t, time_t);

void *fxp_init(void *);
int fxp_send(void *, char *, unsigned);
int fxp_recv(void *, char *, unsigned, unsigned);

struct iodesc *
socktodesc(sock)
	int sock;
{
	if (sock < 0 || sock >= 2)
		return NULL;
	return &sockets[sock];
}

void
netif_init()
{
	struct iodesc *s;
	extern uint8_t en[];

	s = &sockets[0];
	s->io_netif = fxp_init(s->myea); /* determine MAC address */

	memcpy(en, s->myea, sizeof(s->myea));
}

int
netif_open(cookie)
	void *cookie;
{

	return 0;
}

int
netif_close(sock)
	int sock;
{

	/* nothing to do for the HW */
	return 0;
}

/*
 * Send a packet.  The ether header is already there.
 * Return the length sent (or -1 on error).
 */
ssize_t
netif_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	size_t len;
{
	ssize_t rv;
	size_t sendlen;

	sendlen = len;
	if (sendlen < 60)
		sendlen = 60;

	rv = fxp_send(desc->io_netif, pkt, sendlen);

	return rv;
}

/*
 * Receive a packet, including the ether header.
 * Return the total length received (or -1 on error).
 */
ssize_t
netif_get(desc, pkt, maxlen, timo)
	struct iodesc *desc;
	void *pkt;
	size_t maxlen;
	time_t timo;
{
	int len;

	len = fxp_recv(desc->io_netif, pkt, maxlen, timo);
	if (len == -1) {
		printf("timeout\n");
		/* XXX errno = ... */
	}

	if (len < 12)
		return -1;
	return len;
}
