/*	$NetBSD: ntp_proto.c,v 1.1.1.2 2000/04/22 14:53:19 simonb Exp $	*/

/*
 * ntp_proto.c - NTP version 4 protocol machinery
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

#include "ntpd.h"
#include "ntp_stdlib.h"
#include "ntp_unixtime.h"
#include "ntp_control.h"
#include "ntp_string.h"
#include "ntp_crypto.h"

#if defined(VMS) && defined(VMS_LOCALUNIT)	/*wjm*/
#include "ntp_refclock.h"
#endif

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/sysctl.h>
#endif

/*
 * System variables are declared here. See Section 3.2 of the
 * specification.
 */
u_char	sys_leap;		/* system leap indicator */
u_char	sys_stratum;		/* stratum of system */
s_char	sys_precision;		/* local clock precision */
double	sys_rootdelay;		/* distance to current sync source */
double	sys_rootdispersion;	/* dispersion of system clock */
u_int32 sys_refid;		/* reference source for local clock */
static	double sys_offset;	/* current local clock offset */
l_fp	sys_reftime;		/* time we were last updated */
struct	peer *sys_peer; 	/* our current peer */
#ifdef AUTOKEY
u_long	sys_automax;		/* maximum session key lifetime */
#endif /* AUTOKEY */

/*
 * Nonspecified system state variables.
 */
int	sys_bclient;		/* we set our time to broadcasts */
double	sys_bdelay; 		/* broadcast client default delay */
int	sys_authenticate;	/* requre authentication for config */
l_fp	sys_authdelay;		/* authentication delay */
static	u_long sys_authdly[2]; 	/* authentication delay shift reg */
static	u_char leap_consensus;	/* consensus of survivor leap bits */
static	double sys_maxd; 	/* select error (squares) */
static	double sys_epsil;	/* system error (squares) */
keyid_t	sys_private;		/* private value for session seed */
int	sys_manycastserver;	/* 1 => respond to manycast client pkts */
#ifdef AUTOKEY
char	*sys_hostname;		/* gethostname() name */
u_int	sys_hostnamelen;	/* name length (round to word) */
#endif /* AUTOKEY */

/*
 * Statistics counters
 */
u_long	sys_stattime;		/* time when we started recording */
u_long	sys_badstratum; 	/* packets with invalid stratum */
u_long	sys_oldversionpkt;	/* old version packets received */
u_long	sys_newversionpkt;	/* new version packets received */
u_long	sys_unknownversion;	/* don't know version packets */
u_long	sys_badlength;		/* packets with bad length */
u_long	sys_processed;		/* packets processed */
u_long	sys_badauth;		/* packets dropped because of auth */
u_long	sys_limitrejected;	/* pkts rejected due to client count per net */

static	double	root_distance	P((struct peer *));
static	double	clock_combine	P((struct peer **, int));
static	void	peer_xmit	P((struct peer *));
static	void	fast_xmit	P((struct recvbuf *, int, keyid_t));
static	void	clock_update	P((void));
int	default_get_precision	P((void));


/*
 * transmit - Transmit Procedure. See Section 3.4.2 of the
 *	specification.
 */
void
transmit(
	struct peer *peer	/* peer structure pointer */
	)
{
	int hpoll;

	hpoll = peer->hpoll;
	if (peer->burst == 0) {
		u_char oreach;

		/*
		 * Determine reachability and diddle things if we
		 * haven't heard from the host for a while. If the peer
		 * is not configured and not likely to stay around,
		 * we exhaust it.
		 */
		oreach = peer->reach;
		if (oreach & 0x01)
			peer->valid++;
		if (oreach & 0x80)
			peer->valid--;
		peer->reach <<= 1;
		if (peer->reach == 0) {

			/*
			 * If this association has become unreachable,
			 * clear it and raise a trap.
			 */
			if (oreach != 0) {
				report_event(EVNT_UNREACH, peer);
				peer->timereachable = current_time;
				peer_clear(peer);
			}

			/*
			 * If this association is unreachable and not
			 * configured, we give it a little while before
			 * pulling the plug. This is to allow semi-
			 * persistent things like cryptographic
			 * authentication to complete the dance. There
			 * is a denial-of-service hazard here.
			 */
			if (!(peer->flags & FLAG_CONFIG)) {
				peer->tailcnt++;
				if (peer->tailcnt > NTP_TAILMAX) {
					unpeer(peer);
					return;
				}
			}

			/*
			 * We would like to respond quickly when the
			 * peer comes back to life. If the probes since
			 * becoming unreachable are less than
			 * NTP_UNREACH, clamp the poll interval to the
			 * minimum. In order to minimize the network
			 * traffic, the interval gradually ramps up the
			 * the maximum after that.
			 */
			peer->ppoll = peer->maxpoll;
			if (peer->unreach < NTP_UNREACH) {
				if (peer->hmode == MODE_CLIENT ||
				    peer->hmode == MODE_ACTIVE)
					peer->unreach++;
				hpoll = peer->minpoll;
			} else {
				hpoll++;
			}
			if (peer->flags & FLAG_BURST) {
				if (peer->flags & FLAG_MCAST2)
					peer->burst = NTP_SHIFT;
				else
					peer->burst = 2;
			}

		} else {

			/*
			 * Here the peer is reachable. If there is no
			 * system peer or if the stratum of the system
			 * peer is greater than this peer, clamp the
			 * poll interval to the minimum. If less than
			 * two samples are in the reachability register,
			 * reduce the interval; if more than six samples
			 * are in the register, increase the interval.
			 */
			peer->unreach = 0;
			if (sys_peer == 0)
				hpoll = peer->minpoll;
			else if (sys_peer->stratum > peer->stratum)
				hpoll = peer->minpoll;
			if ((peer->reach & 0x03) == 0) {
				clock_filter(peer, 0., 0., MAXDISPERSE);
				clock_select();
			}
			if (peer->valid <= 2 && hpoll > peer->minpoll)
				hpoll--;
			else if (peer->valid >= NTP_SHIFT - 2)
				hpoll++;
			if (peer->flags & FLAG_BURST)
				peer->burst = NTP_SHIFT;
		}
	} else {
		peer->burst--;
		if (peer->burst == 0) {

			/*
			 * If a broadcast client at this point, the
			 * burst has concluded, so we turn off the
			 * burst, switch to client mode and purge the
			 * keylist, since no further transmissions will
			 * be made.
			 */
			if (peer->flags & FLAG_MCAST2) {
				peer->flags &= ~FLAG_BURST;
				peer->hmode = MODE_BCLIENT;
#ifdef AUTOKEY
				key_expire(peer);
#endif /* AUTOKEY */
			}
			clock_select();
			poll_update(peer, hpoll);
			return;
		}
	}

	/*
	 * We need to be very careful about honking uncivilized time. If
	 * not operating in broadcast mode, honk in all except broadcast
	 * client mode. If operating in broadcast mode and synchronized
	 * to a real source, honk except when the peer is the local-
	 * clock driver and the prefer flag is not set. In other words,
	 * in broadcast mode we never honk unless known to be
	 * synchronized to real time.
	 */
	if (peer->hmode != MODE_BROADCAST) {
		if (peer->hmode != MODE_BCLIENT)
			peer_xmit(peer);
	} else if (sys_peer != 0 && sys_leap != LEAP_NOTINSYNC) {
		if (!(sys_peer->refclktype == REFCLK_LOCALCLOCK &&
		    !(sys_peer->flags & FLAG_PREFER)))
			peer_xmit(peer);
	}
	peer->outdate = current_time;
	poll_update(peer, hpoll);
}

/*
 * receive - Receive Procedure.  See section 3.4.3 in the specification.
 */
void
receive(
	struct recvbuf *rbufp
	)
{
	register struct peer *peer;
	register struct pkt *pkt;
	int hismode;
	int oflags;
	int restrict_mask;
	int has_mac;			/* length of MAC field */
	int authlen;			/* offset of MAC field */
	int is_authentic;		/* cryptosum ok */
	int is_error;			/* parse error */
	keyid_t pkeyid, skeyid, tkeyid;	/* cryptographic keys */
	struct peer *peer2;
	int retcode = AM_NOMATCH;

	/*
	 * Monitor the packet and get restrictions. Note that the packet
	 * length for control and private mode packets must be checked
	 * by the service routines. Note that no statistics counters are
	 * recorded for restrict violations, since these counters are in
	 * the restriction routine.
	 */
	ntp_monitor(rbufp);
	restrict_mask = restrictions(&rbufp->recv_srcadr);
#ifdef DEBUG
	if (debug > 2)
		printf("receive: at %ld %s restrict %02x\n",
		    current_time, ntoa(&rbufp->recv_srcadr),
		    restrict_mask);
#endif
	if (restrict_mask & RES_IGNORE)
		return;				/* no amything */
	pkt = &rbufp->recv_pkt;
	if (PKT_VERSION(pkt->li_vn_mode) >= NTP_VERSION)
		sys_newversionpkt++;
	else if (PKT_VERSION(pkt->li_vn_mode) >= NTP_OLDVERSION)
		sys_oldversionpkt++;
	else {
		sys_unknownversion++;		/* unknown version */
		return;
	}
	if (PKT_MODE(pkt->li_vn_mode) == MODE_PRIVATE) {
		if (restrict_mask & RES_NOQUERY)
		    return;			/* no query private */
		process_private(rbufp, ((restrict_mask &
		    RES_NOMODIFY) == 0));
		return;
	}
	if (PKT_MODE(pkt->li_vn_mode) == MODE_CONTROL) {
		if (restrict_mask & RES_NOQUERY)
		    return;			/* no query control */
		process_control(rbufp, restrict_mask);
		return;
	}
	if (restrict_mask & RES_DONTSERVE)
		return;				/* no time service */
	if (restrict_mask & RES_LIMITED) {
		sys_limitrejected++;
                return;				/* too many clients */
        }
	if (rbufp->recv_length < LEN_PKT_NOMAC) {
		sys_badlength++;
		return;				/* no runt packets */
	}

	/*
	 * Figure out his mode and validate the packet. This has some
	 * legacy raunch that probably should be removed. If from NTPv1
	 * mode zero, The NTPv4 mode is determined from the source port.
	 * If the port number is zero, it is from a symmetric active
	 * association; otherwise, it is from a client association. From
	 * NTPv2 on, all we do is toss out mode zero packets, since
	 * control and private mode packets have already been handled.
	 * In either case, toss out broadcast packets if we are not a
	 * broadcast client. 
	 */
	hismode = (int)PKT_MODE(pkt->li_vn_mode);
	if (PKT_VERSION(pkt->li_vn_mode) == NTP_OLDVERSION && hismode ==
	    0) {
		if (SRCPORT(&rbufp->recv_srcadr) == NTP_PORT)
			hismode = MODE_ACTIVE;
		else
			hismode = MODE_CLIENT;
	} else {
		if (hismode == MODE_UNSPEC) {
			sys_badlength++;
			return;			/* invalid mode */
		}
	}
	if ((PKT_MODE(pkt->li_vn_mode) == MODE_BROADCAST &&
	    !sys_bclient))
		return;

	/*
	 * Parse the extension field if present. We figure out whether
	 * an extension field is present by measuring the MAC size. If
	 * the number of words following the packet header is 0 or 1, no
	 * MAC is present and the packet is not authenticated. If 1, the
	 * packet is a reply to a previous request that failed to
	 * authenticate. If 3, the packet is authenticated with DES; if
	 * 5, the packet is authenticated with MD5. If greater than 5,
	 * an extension field is present. If 2 or 4, the packet is a
	 * runt and thus discarded.
	 */
	pkeyid = skeyid = tkeyid = 0;
	authlen = LEN_PKT_NOMAC;
	while ((has_mac = rbufp->recv_length - authlen) > 0) {
		int temp;

		if (has_mac % 4 != 0 || has_mac < 0) {
			sys_badlength++;
			return;
		}
		if (has_mac == 1 * 4 || has_mac == 3 * 4 || has_mac ==
		    MAX_MAC_LEN) {
			skeyid = ntohl(((u_int32 *)pkt)[authlen / 4]);
			break;

		} else if (has_mac > MAX_MAC_LEN) {
			temp = ntohl(((u_int32 *)pkt)[authlen / 4]) &
			    0xffff;
			if (temp < 4 || temp % 4 != 0) {
				sys_badlength++;
				return;
			}
			authlen += temp;
		} else {
			sys_badlength++;
			return;
		}
	}

	/*
	 * We have tossed out as many buggy packets as possible early in
	 * the game to reduce the exposure to a clogging attack. Now we
	 * have to burn some cycles to find the association and
	 * authenticate the packet if required. Note that we burn only
	 * MD5 or DES cycles, again to reduce exposure. There may be no
	 * matching association and that's okay.
	 */
	peer = findpeer(&rbufp->recv_srcadr, rbufp->dstadr, rbufp->fd,
	    hismode, &retcode);
	is_authentic = 0;
	if (has_mac == 0) {
#ifdef DEBUG
		if (debug)
			printf("receive: at %ld %s mode %d code %d\n",
			    current_time, ntoa(&rbufp->recv_srcadr),
			    hismode, retcode);
#endif
	} else {
#ifdef AUTOKEY
		/*
		 * For autokey modes, generate the session key
		 * and install in the key cache. Use the socket
		 * multicast or unicast address as appropriate.
		 * Remember, we don't know these addresses until
		 * the first packet has been received. Bummer.
		 */
		if (skeyid > NTP_MAXKEY) {
		
			/*
			 * More on the autokey dance (AKD). A cookie is
			 * constructed from public and private values.
			 * For broadcast packets, the cookie is public
			 * (zero). For packets that match no
			 * association, the cookie is hashed from the
			 * addresses and private value. For server
			 * packets, the cookie was previously obtained
			 * from the server. For symmetric modes, the
			 * cookie was previously constructed using an
			 * agreement protocol; however, should PKI be
			 * unavailable, we construct a fake agreement as
			 * the EXOR of the peer and host cookies.
			 */
			if (hismode == MODE_BROADCAST) {
				pkeyid = 0;
			} else if (peer == 0) {
				pkeyid = session_key(
				    &rbufp->recv_srcadr,
				    &rbufp->dstadr->sin, 0, sys_private,
				    0);
			} else if (hismode == MODE_CLIENT) {
				pkeyid = peer->hcookie;
			} else {
#ifdef PUBKEY
				if (crypto_enable)
					pkeyid = peer->pcookie.key;
				else
					pkeyid = peer->pcookie.key;
					
#else
				if (hismode == MODE_SERVER)
					pkeyid = peer->pcookie.key;
				else
					pkeyid = peer->hcookie ^
					    peer->pcookie.key;
#endif /* PUBKEY */
			}

			/*
			 * The session key includes both the public
			 * values and cookie. We have to be careful to
			 * use the right socket addresses for broadcast
			 * and unicast packets. In case of an extension
			 * field, the cookie used for authentication
			 * purposes is zero. Note the hash is saved for
			 * use later in the autokey mambo.
			 */
			if (hismode == MODE_BROADCAST) {
				tkeyid = session_key(
				    &rbufp->recv_srcadr,
				    &rbufp->dstadr->bcast, skeyid,
				    pkeyid, 2);
			} else if (authlen > LEN_PKT_NOMAC) {
				session_key(&rbufp->recv_srcadr,
				    &rbufp->dstadr->sin, skeyid, 0, 2);
				tkeyid = session_key(
				    &rbufp->recv_srcadr,
				    &rbufp->dstadr->sin, skeyid, pkeyid,
				    0);
			} else {
				tkeyid = session_key(
				    &rbufp->recv_srcadr,
				    &rbufp->dstadr->sin, skeyid, pkeyid,
				    2);
			}

		}
#endif /* AUTOKEY */

		/*
		 * Compute the cryptosum. Note a clogging attack may
		 * succeed in bloating the key cache. If an autokey,
		 * purge it immediately, since we won't be needing it
		 * again.
		 */
		if (authdecrypt(skeyid, (u_int32 *)pkt, authlen,
		    has_mac))
			is_authentic = 1;
		else
			sys_badauth++;
#ifdef AUTOKEY
		if (skeyid > NTP_MAXKEY)
			authtrust(skeyid, 0);
#endif /* AUTOKEY */
#ifdef DEBUG
		if (debug)
			printf(
			    "receive: at %ld %s mode %d code %d keyid %08x len %d mac %d auth %d\n",
			    current_time, ntoa(&rbufp->recv_srcadr),
			    hismode, retcode, skeyid, authlen, has_mac,
			    is_authentic);
#endif
	}

	/*
	 * The new association matching rules are driven by a table
	 * specified in ntp.h. We have replaced the *default* behaviour
	 * of replying to bogus packets in server mode in this version.
	 * A packet must now match an association in order to be
	 * processed. In the event that no association exists, then an
	 * association is mobilized if need be. Two different
	 * associations can be mobilized a) passive associations b)
	 * client associations due to broadcasts or manycasts.
	 */
	is_error = 0;
	switch (retcode) {
	case AM_FXMIT:

		/*
		 * If the client is configured purely as a broadcast
		 * client and not as an manycast server, it has no
		 * business being a server. Simply go home. Otherwise,
		 * send a MODE_SERVER response and go home. Note that we
		 * don't do a authentication check here, since we can't
		 * set the system clock; but, we do set the key ID to
		 * zero to tell the caller about this.
		 */
		if (!sys_bclient || sys_manycastserver) {
			if (is_authentic)
				fast_xmit(rbufp, MODE_SERVER, skeyid);
			else
				fast_xmit(rbufp, MODE_SERVER, 0);
		}
		return;

	case AM_MANYCAST:

		/*
		 * This could be in response to a multicast packet sent
		 * by the "manycast" mode association. Find peer based
		 * on the originate timestamp in the packet. Note that
		 * we don't mobilize a new association, unless the
		 * packet is properly authenticated. The response must
		 * be properly authenticated and it's darn funny of the
		 * manycaster isn't around now. 
		 */
		if ((sys_authenticate && !is_authentic)) {
			is_error = 1;
			break;
		}
		peer2 = (struct peer *)findmanycastpeer(&pkt->org);
		if (peer2 == 0) {
			is_error = 1;
			break;
		}

		/*
		 * Create a new association and copy the peer variables
		 * to it. If something goes wrong, carefully pry the new
		 * association away and return its marbles to the candy
		 * store.
		 */
		peer = newpeer(&rbufp->recv_srcadr, rbufp->dstadr,
		    MODE_CLIENT, PKT_VERSION(pkt->li_vn_mode),
		    NTP_MINDPOLL, NTP_MAXDPOLL, 0, skeyid);
		if (peer == 0) {
			is_error = 1;
			break;
		}
		peer_config_manycast(peer2, peer);
#ifdef PUBKEY
		if (crypto_enable)
			ntp_res_name(peer->srcadr.sin_addr.s_addr,
				     peer->associd);
#endif /* PUBKEY */
		break;

	case AM_ERR:

		/*
		 * Something bad happened. Dirty floor will be mopped by
		 * the code at the end of this adventure.
		 */
		is_error = 1;
		break;

	case AM_NEWPASS:

		/*
		 * Okay, we're going to keep him around.  Allocate him
		 * some memory. But, don't do that unless the packet is
		 * properly authenticated.
		 */
		if ((sys_authenticate && !is_authentic)) {
			fast_xmit(rbufp, MODE_PASSIVE, 0);
			return;
		}
		peer = newpeer(&rbufp->recv_srcadr, rbufp->dstadr,
		    MODE_PASSIVE, PKT_VERSION(pkt->li_vn_mode),
	 	    NTP_MINDPOLL, NTP_MAXDPOLL, 0, skeyid);
#ifdef PUBKEY
		if (crypto_enable)
			ntp_res_name(peer->srcadr.sin_addr.s_addr,
				     peer->associd);
#endif /* PUBKEY */
		break;

	case AM_NEWBCL:

		/*
		 * Broadcast client being set up now. Do this only if
		 * the packet is properly authenticated.
		 */
		if ((restrict_mask & RES_NOPEER) || !sys_bclient ||
		    (sys_authenticate && !is_authentic)) {
			is_error = 1;
			break;
		}
		peer = newpeer(&rbufp->recv_srcadr, rbufp->dstadr,
		    MODE_MCLIENT, PKT_VERSION(pkt->li_vn_mode),
		    NTP_MINDPOLL, NTP_MAXDPOLL, 0, skeyid);
		if (peer == 0)
			break;
		peer->flags |= FLAG_MCAST1 | FLAG_MCAST2 | FLAG_BURST;
		peer->hmode = MODE_CLIENT;
#ifdef PUBKEY
		if (crypto_enable)
			ntp_res_name(peer->srcadr.sin_addr.s_addr,
				     peer->associd);
#endif /* PUBKEY */
		break;

	case AM_POSSBCL:
	case AM_PROCPKT:

		/*
		 * It seems like it is okay to process the packet now
		 */
		break;

	default:

		/*
		 * shouldn't be getting here, but simply return anyway!
		 */
		is_error = 1;
	}
	if (is_error) {

		/*
		 * Error stub. If we get here, something broke. We
		 * scuttle the autokey if necessary and sink the ship.
		 * This can occur only upon mobilization, so we can
		 * throw the structure away without fear of breaking
		 * anything.
		 */
		if (peer != 0)
			if (!(peer->flags & FLAG_CONFIG))
				unpeer(peer);
#ifdef DEBUG
		if (debug)
			printf("receive: bad protocol %d\n", retcode);
#endif
		return;
	}

	/*
	 * If the peer isn't configured, set his authenable and autokey
	 * status based on the packet. Once the status is set, it can't
	 * be unset. It seems like a silly idea to do this here, rather
	 * in the configuration routine, but in some goofy cases the
	 * first packet sent cannot be authenticated and we need a way
	 * for the dude to change his mind.
	 */
	oflags = peer->flags;
	peer->timereceived = current_time;
	peer->received++;
	if (!(peer->flags & FLAG_CONFIG) && has_mac) {
		peer->flags |= FLAG_AUTHENABLE;
#ifdef AUTOKEY
		if (skeyid > NTP_MAXKEY)
			peer->flags |= FLAG_SKEY;
#endif /* AUTOKEY */
	}

	/*
	 * A valid packet must be from an authentic and allowed source.
	 * All packets must pass the authentication allowed tests.
	 * Autokey authenticated packets must pass additional tests and
	 * public-key authenticated packets must have the credentials
	 * verified. If all tests are passed, the packet is forwarded
	 * for processing. If not, the packet is discarded and the
	 * association demobilized if appropriate.
	 */
	peer->flash = 0;
	if (is_authentic) {
		peer->flags |= FLAG_AUTHENTIC;
		peer->tailcnt = 0;
	} else {
		peer->flags &= ~FLAG_AUTHENTIC;
	}
	if (peer->hmode == MODE_BROADCAST &&
	    (restrict_mask & RES_DONTTRUST))	/* test 9 */
		peer->flash |= TEST9;		/* access denied */
	if (peer->flags & FLAG_AUTHENABLE) {

		/*
		 * Here we have a little bit of nastyness. Should
		 * authentication fail in client mode, it could either
		 * be a hacker attempting to jam the protocol, or it
		 * could be the server has just refreshed its keys. On
		 * the premiss the later is more likely than the former
		 * and that even the former can't do real evil, we
		 * simply ask for the cookie again.
		 */
		if (!(peer->flags & FLAG_AUTHENTIC)) { /* test 5 */
			peer->flash |= TEST5;	/* auth failed */
#ifdef AUTOKEY
			if (hismode == MODE_SERVER)
				peer->pcookie.tstamp = 0;
#endif /* AUTOKEY */
		} else if (!(oflags & FLAG_AUTHENABLE)) {
			report_event(EVNT_PEERAUTH, peer);
		}
	}
	if (peer->flash) {
#ifdef DEBUG
		if (debug)
			printf("receive: bad packet %03x\n",
			    peer->flash);
#endif
		return;
	}

#ifdef AUTOKEY
	/*
	 * More autokey dance. The rules of the cha-cha are as follows:
	 *
	 * 1. If there is no key or the key is not auto, do nothing.
	 *
	 * 2. If an extension field contains a verified signature, it is
	 *    self-authenticated and we sit the dance.
	 *
	 * 3. If this is a server reply, check only to see that the
	 *    transmitted key ID matches the received key ID.
	 *
	 * 4. Check to see that one or more hashes of the current key ID
	 *    matches the previous key ID or ultimate original key ID
	 *    obtained from the broadcaster or symmetric peer. If no
	 *    match, arm for an autokey values update.
	 */
	if (peer->flags & FLAG_SKEY) {
		peer->flash |= TEST10;
		crypto_recv(peer, rbufp);
		if (!peer->flash & TEST10) {
			peer->pkeyid = skeyid;
		} else if (hismode == MODE_SERVER) {
			if (skeyid == peer->keyid)
				peer->flash &= ~TEST10;
		} else {
			int i = 0;

			for (i = 0;; i++) {
				if (tkeyid == peer->pkeyid ||
				    tkeyid == peer->recauto.key) {
					peer->flash &= ~TEST10;
					peer->pkeyid = skeyid;
					break;
				}
				if (i > peer->recauto.seq) {
					peer->recauto.tstamp = 0;
					break;
				}
				if (hismode == MODE_BROADCAST)
					tkeyid = session_key(
					    &rbufp->recv_srcadr,
					    &rbufp->dstadr->bcast,
					    tkeyid, pkeyid, 0);
				else
					tkeyid = session_key(
					    &rbufp->recv_srcadr,
					    &rbufp->dstadr->sin,
					    tkeyid, pkeyid, 0);
			}
		}
#ifdef PUBKEY
		/*
		 * If the autokey boogie fails, the server may be bogus
		 * or worse. Raise an alarm and rekey this thing.
		 */
		if (peer->flash & TEST10)
			peer->flags &= ~FLAG_AUTOKEY;
		if (!(peer->flags & FLAG_AUTOKEY))
			peer->flash |= TEST11;
#endif /* PUBKEY */
	}
#endif /* AUTOKEY */

	/*
	 * We have survived the gaunt. Forward to the packet routine. If
	 * a symmetric passive association has been mobilized and the
	 * association doesn't deserve to live, it will die in the
	 * transmit routine if not reachable after timeout.
	 */
	process_packet(peer, pkt, &rbufp->recv_time);
}


/*
 * process_packet - Packet Procedure, a la Section 3.4.4 of the
 *	specification. Or almost, at least. If we're in here we have a
 *	reasonable expectation that we will be having a long term
 *	relationship with this host.
 */
void
process_packet(
	register struct peer *peer,
	register struct pkt *pkt,
	l_fp *recv_ts
	)
{
	l_fp t10, t23;
	double p_offset, p_del, p_disp;
	double dtemp;
	l_fp p_rec, p_xmt, p_org, p_reftime;
	l_fp ci;
	int pmode;

	/*
	 * Swap header fields and keep the books. The books amount to
	 * the receive timestamp and poll interval in the header. We
	 * need these even if there are other problems in order to crank
	 * up the state machine.
	 */
	sys_processed++;
	peer->processed++;
	p_del = FPTOD(NTOHS_FP(pkt->rootdelay));
	p_disp = FPTOD(NTOHS_FP(pkt->rootdispersion));
	NTOHL_FP(&pkt->reftime, &p_reftime);
	NTOHL_FP(&pkt->rec, &p_rec);
	NTOHL_FP(&pkt->xmt, &p_xmt);
	if (PKT_MODE(pkt->li_vn_mode) != MODE_BROADCAST)
		NTOHL_FP(&pkt->org, &p_org);
	else
		p_org = peer->rec;
	peer->rec = *recv_ts;
	peer->ppoll = pkt->ppoll;
	pmode = PKT_MODE(pkt->li_vn_mode);

	/*
	 * Test for old or duplicate packets (tests 1 through 3).
	 */
	if (L_ISHIS(&peer->org, &p_xmt))	/* count old packets */
		peer->oldpkt++;
	if (L_ISEQU(&peer->org, &p_xmt))	/* test 1 */
		peer->flash |= TEST1;		/* duplicate packet */
	if (PKT_MODE(pkt->li_vn_mode) != MODE_BROADCAST) {
		if (!L_ISEQU(&peer->xmt, &p_org)) /* test 2 */
			peer->flash |= TEST2;	/* bogus packet */
		if (L_ISZERO(&p_rec) || L_ISZERO(&p_org)) /* test 3 */
			peer->flash |= TEST3;	/* unsynchronized */
	}
	if (L_ISZERO(&p_xmt))			/* test 3 */
		peer->flash |= TEST3;		/* unsynchronized */
	peer->org = p_xmt;

	/*
	 * Test for valid packet header (tests 6 through 8)
	 */
	ci = p_xmt;
	L_SUB(&ci, &p_reftime);
	LFPTOD(&ci, dtemp);
	if (PKT_LEAP(pkt->li_vn_mode) == LEAP_NOTINSYNC || /* test 6 */
	    PKT_TO_STRATUM(pkt->stratum) >= NTP_MAXSTRATUM ||
	    dtemp < 0)
		peer->flash |= TEST6;	/* peer clock unsynchronized */
	if (!(peer->flags & FLAG_CONFIG) && sys_peer != 0) { /* test 7 */
		if (PKT_TO_STRATUM(pkt->stratum) > sys_stratum) {
			peer->flash |= TEST7;	/* peer stratum too high */
			sys_badstratum++;
		}
	}
	if (fabs(p_del) >= MAXDISPERSE		/* test 8 */
	    || p_disp >= MAXDISPERSE)
		peer->flash |= TEST8;		/* delay/dispersion too high */

	/*
	 * If the packet header is invalid, abandon ship. Otherwise
	 * capture the header data.
	 */
	if (peer->flash & ~(u_int)(TEST1 | TEST2 | TEST3 | TEST4)) {
#ifdef DEBUG
		if (debug)
			printf("packet: bad header %03x\n",
			    peer->flash);
#endif
		return;
	}

	/*
	 * The header is valid. Capture the remaining header values and
	 * mark as reachable.
	 */
	record_raw_stats(&peer->srcadr, &peer->dstadr->sin,
	    &p_org, &p_rec, &p_xmt, &peer->rec);
	peer->leap = PKT_LEAP(pkt->li_vn_mode);
	peer->pmode = pmode;		/* unspec */
	peer->stratum = PKT_TO_STRATUM(pkt->stratum);
	peer->precision = pkt->precision;
	peer->rootdelay = p_del;
	peer->rootdispersion = p_disp;
	peer->refid = pkt->refid;
	peer->reftime = p_reftime;
	if (peer->reach == 0) {
		report_event(EVNT_REACH, peer);
		peer->timereachable = current_time;
	}
	peer->reach |= 1;
	poll_update(peer, peer->hpoll);

	/*
	 * If running in a client/server association, calculate the
	 * clock offset c, roundtrip delay d and dispersion e. We use
	 * the equations (reordered from those in the spec). Note that,
	 * in a broadcast association, org has been set to the time of
	 * last reception. Note the computation of dispersion includes
	 * the system precision plus that due to the frequency error
	 * since the originate time.
	 *
	 * c = ((t2 - t3) + (t1 - t0)) / 2
	 * d = (t2 - t3) - (t1 - t0)
	 * e = (org - rec) (seconds only)
	 */
	t10 = p_xmt;			/* compute t1 - t0 */
	L_SUB(&t10, &peer->rec);
	t23 = p_rec;			/* compute t2 - t3 */
	L_SUB(&t23, &p_org);
	ci = t10;
	p_disp = CLOCK_PHI * (peer->rec.l_ui - p_org.l_ui);

	/*
	 * If running in a broadcast association, the clock offset is
	 * (t1 - t0) corrected by the one-way delay, but we can't
	 * measure that directly; therefore, we start up in
	 * client/server mode, calculate the clock offset, using the
	 * engineered refinement algorithms, while also receiving
	 * broadcasts. When a broadcast is received in client/server
	 * mode, we calculate a correction factor to use after switching
	 * back to broadcast mode. We know NTP_SKEWFACTOR == 16, which
	 * accounts for the simplified ei calculation.
	 *
	 * If FLAG_MCAST2 is set, we are a broadcast/multicast client.
	 * If FLAG_MCAST1 is set, we haven't calculated the propagation
	 * delay. If hmode is MODE_CLIENT, we haven't set the local
	 * clock in client/server mode. Initially, we come up
	 * MODE_CLIENT. When the clock is first updated and FLAG_MCAST2
	 * is set, we switch from MODE_CLIENT to MODE_BCLIENT.
	 */
	if (pmode == MODE_BROADCAST) {
		if (peer->flags & FLAG_MCAST1) {
			LFPTOD(&ci, p_offset);
			peer->estbdelay = peer->offset - p_offset;
			if (peer->hmode != MODE_BCLIENT) {
#ifdef DEBUG
				if (debug)
					printf("packet: bclient %03x\n",
					    peer->flash);
#endif
				return;
			}
			peer->flags &= ~FLAG_MCAST1;
		}
		DTOLFP(peer->estbdelay, &t10);
		L_ADD(&ci, &t10);
		p_del = peer->delay;
	} else {
		L_ADD(&ci, &t23);
		L_RSHIFT(&ci);
		L_SUB(&t23, &t10);
		LFPTOD(&t23, p_del);
	}
	LFPTOD(&ci, p_offset);
	if (fabs(p_del) >= MAXDISPERSE || p_disp >= MAXDISPERSE) /* test 4 */
		peer->flash |= TEST4;	/* delay/dispersion too big */

	/*
	 * If the packet data are invalid (tests 1 through 4), abandon
	 * ship. Otherwise, forward to the clock filter.
	 */
	if (peer->flash) {
#ifdef DEBUG
		if (debug)
			printf("packet: bad data %03x\n",
			    peer->flash);
#endif
		return;
	}
	clock_filter(peer, p_offset, p_del, fabs(p_disp));
	clock_select();
	record_peer_stats(&peer->srcadr, ctlpeerstatus(peer),
	    peer->offset, peer->delay, peer->disp,
	    SQRT(peer->variance));
}


/*
 * clock_update - Called at system process update intervals.
 */
static void
clock_update(void)
{
	u_char oleap;
	u_char ostratum;

	/*
	 * Reset/adjust the system clock. Do this only if there is a
	 * system peer and we haven't seen that peer lately. Watch for
	 * timewarps here.
	 */
	if (sys_peer == 0)
		return;
	if (sys_peer->pollsw == FALSE || sys_peer->burst > 0)
		return;
	sys_peer->pollsw = FALSE;
#ifdef DEBUG
	if (debug)
		printf("clock_update: at %ld assoc %d \n", current_time,
		    peer_associations);
#endif
	oleap = sys_leap;
	ostratum = sys_stratum;
	switch (local_clock(sys_peer, sys_offset, sys_epsil)) {

	/*
	 * Clock is too screwed up. Just exit for now.
	 */
	case -1:
		report_event(EVNT_SYSFAULT, (struct peer *)0);
		exit(1);
		/*NOTREACHED*/

	/*
	 * Clock was stepped. Flush all time values of all peers.
	 */
	case 1:
		clear_all();
		NLOG(NLOG_SYNCSTATUS)
			msyslog(LOG_INFO, "synchronisation lost");
		sys_peer = 0;
		sys_stratum = STRATUM_UNSPEC;
		report_event(EVNT_CLOCKRESET, (struct peer *)0);
		break;

	/*
	 * Update the system stratum, leap bits, root delay, root
	 * dispersion, reference ID and reference time. We also update
	 * select dispersion and max frequency error. If the leap
	 * changes, we gotta reroll the keys.
	 */
	default:
		sys_stratum = sys_peer->stratum + 1;
		if (sys_stratum == 1)
			sys_refid = sys_peer->refid;
		else
			sys_refid = sys_peer->srcadr.sin_addr.s_addr;
		sys_reftime = sys_peer->rec;
		sys_rootdelay = sys_peer->rootdelay +
		    fabs(sys_peer->delay);
		sys_leap = leap_consensus;
	}
	if (oleap != sys_leap) {
		report_event(EVNT_SYNCCHG, (struct peer *)0);
		expire_all();
	}
	if (ostratum != sys_stratum)
		report_event(EVNT_PEERSTCHG, (struct peer *)0);
}


/*
 * poll_update - update peer poll interval. See Section 3.4.9 of the
 *	   spec.
 */
void
poll_update(
	struct peer *peer,
	int hpoll
	)
{
	long update, oldpoll;

	/*
	 * The wiggle-the-poll-interval dance. Broadcasters dance only
	 * the minpoll beat. Reference clock partners sit this one out.
	 * Dancers surviving the clustering algorithm beat to the system
	 * clock. Broadcast clients are usually lead by their broadcast
	 * partner, but faster in the initial mating dance.
	 */
	oldpoll = peer->hpoll;
	if (peer->hmode == MODE_BROADCAST) {
		peer->hpoll = peer->minpoll;
	} else if (peer->flags & FLAG_SYSPEER) {
		peer->hpoll = sys_poll;
	} else {
		if (hpoll > peer->maxpoll)
			peer->hpoll = peer->maxpoll;
		else if (hpoll < peer->minpoll)
			peer->hpoll = peer->minpoll;
		else
			peer->hpoll = hpoll;
	}
	if (peer->burst > 0) {
		if (peer->nextdate != current_time)
			return;
		if (peer->flags & FLAG_REFCLOCK)
			peer->nextdate++;
		else if (peer->reach & 0x1)
			peer->nextdate += RANDPOLL(BURST_INTERVAL2);
		else
			peer->nextdate += RANDPOLL(BURST_INTERVAL1);
	} else {
		update = max(min(peer->ppoll, peer->hpoll),
		    peer->minpoll);
		peer->nextdate = peer->outdate + RANDPOLL(update);
	}

	/*
	 * Bit of crass arrogance at this point. If the poll interval
	 * has changed and we have a keylist, the lifetimes in the
	 * keylist are probably bogus. In this case purge the keylist
	 * and regenerate it later.
	 */
#ifdef AUTOKEY
	if (peer->hpoll != oldpoll)
		key_expire(peer);
#endif /* AUTOKEY */
#ifdef DEBUG
	if (debug > 1)
		printf("poll_update: at %lu %s flags %04x poll %d burst %d last %lu next %lu\n",
		    current_time, ntoa(&peer->srcadr), peer->flags,
		    hpoll, peer->burst, peer->outdate, peer->nextdate);
#endif
}


/*
 * clear - clear peer filter registers.  See Section 3.4.8 of the spec.
 */
void
peer_clear(
	register struct peer *peer
	)
{
	register int i;

	/*
	 * If cryptographic credentials have been acquired, toss them to
	 * Valhalla. Note that autokeys are ephemeral, in that they are
	 * tossed immediately upon use. Therefore, the keylist can be
	 * purged anytime without needing to preserve random keys. Note
	 * that, if the peer is purged, the cryptographic variables are
	 * purged, too. This makes it much harder to sneak in some
	 * unauthenticated data in the clock filter.
	 */
#ifdef DEBUG
	if (debug)
		printf("peer_clear: at %ld\n", current_time);
#endif
#ifdef AUTOKEY
	key_expire(peer);
#endif /* AUTOKEY */

	/*
	 * If he dies as a multicast client, he comes back to life as
	 * a multicast client in client mode in order to recover the
	 * initial autokey values.
	 */
	peer->flags &= ~FLAG_AUTOKEY;
	if (peer->flags & FLAG_MCAST2) {
		peer->flags |= FLAG_MCAST1 | FLAG_BURST;
		peer->hmode = MODE_CLIENT;
	}
	memset(CLEAR_TO_ZERO(peer), 0, LEN_CLEAR_TO_ZERO);
	peer->estbdelay = sys_bdelay;
	peer->hpoll = peer->minpoll;
	peer->pollsw = FALSE;
	peer->variance = MAXDISPERSE;
	peer->epoch = current_time;
	for (i = 0; i < NTP_SHIFT; i++) {
		peer->filter_order[i] = i;
		peer->filter_disp[i] = MAXDISPERSE;
		peer->filter_epoch[i] = current_time;
	}
	poll_update(peer, peer->minpoll);
}


/*
 * clock_filter - add incoming clock sample to filter register and run
 *		  the filter procedure to find the best sample.
 */
void
clock_filter(
	register struct peer *peer,
	double sample_offset,
	double sample_delay,
	double sample_disp
	)
{
	register int i, j, k, n;
	register u_char *ord;
	double distance[NTP_SHIFT];
	double off, dly, var, dsp, dtemp, etemp;

	/*
	 * Update error bounds and calculate distances. The distance for
	 * each sample is equal to the sample dispersion plus one-half
	 * the sample delay. Also initialize the sort index vector.
	 */
	dtemp = CLOCK_PHI * (current_time - peer->update);
	peer->update = current_time;
	ord = peer->filter_order;
	j = peer->filter_nextpt;
	for (i = 0; i < NTP_SHIFT; i++) {
		peer->filter_disp[j] += dtemp;
		if (peer->filter_disp[j] > MAXDISPERSE)
			peer->filter_disp[j] = MAXDISPERSE;
		distance[i] = fabs(peer->filter_delay[j]) / 2 +
		    peer->filter_disp[j];
		ord[i] = j;
		if (--j < 0)
			j += NTP_SHIFT;
	}

	/*
	 * Shift the new sample into the register and discard the oldest
	 * one. The new offset and delay come directly from the caller.
	 * The dispersion from the caller is increased by the sum of the
	 * precision in the the packet header and the precision of this
	 * machine.
	 */
	peer->filter_offset[peer->filter_nextpt] = sample_offset;
	peer->filter_delay[peer->filter_nextpt] = sample_delay;
	dsp = LOGTOD(peer->precision) + LOGTOD(sys_precision) +
	    sample_disp;
	peer->filter_disp[peer->filter_nextpt] = min(dsp, MAXDISPERSE);
	peer->filter_epoch[peer->filter_nextpt] = current_time;
	distance[0] = min(dsp + fabs(sample_delay) / 2, MAXDISTANCE);
	peer->filter_nextpt++;
	if (peer->filter_nextpt >= NTP_SHIFT)
		peer->filter_nextpt = 0;

	/*
	 * Sort the samples in the register by distance. The winning
	 * sample will be in ord[0]. Sort the samples only if they
	 * are younger than the Allen intercept; however, keep a minimum
	 * of two samples so that we can compute jitter.
	 */
	dtemp = min(allan_xpt, NTP_SHIFT * ULOGTOD(sys_poll));
	for (n = 0; n < NTP_SHIFT; n++) {
		if (n > 1 && current_time - peer->filter_epoch[ord[n]] >
		    dtemp)
			break;
		for (j = 0; j < n; j++) {
			if (distance[j] > distance[n]) {
				etemp = distance[j];
				k = ord[j];
				distance[j] = distance[n];
				ord[j] = ord[n];
				distance[n] = etemp;
				ord[n] = k;
			}
		}
	} 
	
	/*
	 * Compute the offset, delay, variance (squares) and error
	 * bound. The offset, delay and variance are weighted by the
	 * reciprocal of distance and normalized. The error bound is
	 * weighted exponentially. When no acceptable samples remain in
	 * the shift register, quietly tiptoe home.
	 */
	off = dly = var = dsp = dtemp = 0;
	for (i = NTP_SHIFT - 1; i >= 0; i--) {
		dsp = NTP_FWEIGHT * (dsp + peer->filter_disp[ord[i]]);
		if (i < n && distance[i] < MAXDISTANCE) {
			dtemp += 1. / distance[i];
			off += peer->filter_offset[ord[i]] /
			    distance[i];
			dly += peer->filter_delay[ord[i]] /
			    distance[i];
			var += DIFF(peer->filter_offset[ord[i]],
			    peer->filter_offset[ord[0]]) /
			    SQUARE(distance[i]);
		}
	}
	if (dtemp == 0)
		return;
	peer->delay = dly / dtemp;
	peer->variance = min(var / SQUARE(dtemp), MAXDISPERSE);
	peer->disp = min(dsp, MAXDISPERSE);
	peer->epoch = current_time;
	etemp = peer->offset;
	peer->offset = off / dtemp;

	/*
	 * A new sample is useful only if it is younger than the last
	 * one used.
	 */
	if (peer->filter_epoch[ord[0]] > peer->epoch) {
#ifdef DEBUG
		if (debug)
			printf("clock_filter: discard %lu\n",
			    peer->filter_epoch[ord[0]] - peer->epoch);
#endif
		return;
	}

	/*
	 * If the offset exceeds the dispersion by CLOCK_SGATE and the
	 * interval since the last update is less than twice the system
	 * poll interval, consider the update a popcorn spike and ignore
	 * it.
	 */
	if (fabs(etemp - peer->offset) > CLOCK_SGATE &&
	    peer->filter_epoch[ord[0]] - peer->epoch < (1 <<
	    (sys_poll + 1))) {
#ifdef DEBUG
		if (debug)
			printf("clock_filter: popcorn spike %.6f\n",
			    off);
#endif
		return;
	}

	/*
	 * The mitigated sample statistics are saved for later
	 * processing, but can be processed only once.
	 */
	peer->epoch = peer->filter_epoch[ord[0]];
	peer->pollsw = TRUE;
#ifdef DEBUG
	if (debug)
		printf(
		    "clock_filter: offset %.6f delay %.6f disp %.6f std %.6f, age %lu\n",
		    peer->offset, peer->delay, peer->disp,
		    SQRT(peer->variance), current_time - peer->epoch);
#endif
}


/*
 * clock_select - find the pick-of-the-litter clock
 */
void
clock_select(void)
{
	register struct peer *peer;
	int i;
	int nlist, nl3;
	double d, e, f;
	int j;
	int n;
	int allow, found, k;
	double high, low;
	double synch[NTP_MAXCLOCK], error[NTP_MAXCLOCK];
	struct peer *osys_peer;
	struct peer *typeacts = 0;
	struct peer *typelocal = 0;
	struct peer *typepps = 0;
	struct peer *typeprefer = 0;
	struct peer *typesystem = 0;

	static int list_alloc = 0;
	static struct endpoint *endpoint = NULL;
	static int *indx = NULL;
	static struct peer **peer_list = NULL;
	static u_int endpoint_size = 0;
	static u_int indx_size = 0;
	static u_int peer_list_size = 0;

	/*
	 * Initialize. If a prefer peer does not survive this thing,
	 * the pps_update switch will remain zero.
	 */
	pps_update = 0;
	nlist = 0;
	low = 1e9;
	high = -1e9;
	for (n = 0; n < HASH_SIZE; n++)
		nlist += peer_hash_count[n];
	if (nlist > list_alloc) {
		if (list_alloc > 0) {
			free(endpoint);
			free(indx);
			free(peer_list);
		}
		while (list_alloc < nlist) {
			list_alloc += 5;
			endpoint_size += 5 * 3 * sizeof *endpoint;
			indx_size += 5 * 3 * sizeof *indx;
			peer_list_size += 5 * sizeof *peer_list;
		}
		endpoint = (struct endpoint *)emalloc(endpoint_size);
		indx = (int *)emalloc(indx_size);
		peer_list = (struct peer **)emalloc(peer_list_size);
	}

	/*
	 * This first chunk of code is supposed to go through all
	 * peers we know about to find the peers which are most likely
	 * to succeed. We run through the list doing the sanity checks
	 * and trying to insert anyone who looks okay.
	 */
	nlist = nl3 = 0;	/* none yet */
	for (n = 0; n < HASH_SIZE; n++) {
		for (peer = peer_hash[n]; peer != 0; peer = peer->next) {
			peer->flags &= ~FLAG_SYSPEER;
			peer->status = CTL_PST_SEL_REJECT;
			if (peer->flags & FLAG_NOSELECT)
				continue;	/* noselect (survey) */
			if (peer->reach == 0)
				continue;	/* unreachable */
			if (peer->stratum > 1 && peer->refid ==
			    peer->dstadr->sin.sin_addr.s_addr)
				continue;	/* sync loop */
			if (root_distance(peer) >= MAXDISTANCE + 2 *
			    CLOCK_PHI * ULOGTOD(sys_poll)) {
				peer->seldisptoolarge++;
				continue;	/* noisy or broken */
			}

			/*
			 * Don't allow the local-clock or acts drivers
			 * in the kitchen at this point, unless the
			 * prefer peer. Do that later, but only if
			 * nobody else is around.
			 */
			if (peer->refclktype == REFCLK_LOCALCLOCK
#if defined(VMS) && defined(VMS_LOCALUNIT)
				/* wjm: local unit VMS_LOCALUNIT taken seriously */
				&& REFCLOCKUNIT(&peer->srcadr) != VMS_LOCALUNIT
#endif	/* VMS && VMS_LOCALUNIT */
				) {
				typelocal = peer;
				if (!(peer->flags & FLAG_PREFER))
					continue; /* no local clock */
			}
			if (peer->sstclktype == CTL_SST_TS_TELEPHONE) {
				typeacts = peer;
				if (!(peer->flags & FLAG_PREFER))
					continue; /* no acts */
			}

			/*
			 * If we get this far, we assume the peer is
			 * acceptable.
			 */
			peer->status = CTL_PST_SEL_SANE;
			peer_list[nlist++] = peer;

			/*
			 * Insert each interval endpoint on the sorted
			 * list.
			 */
			e = peer->offset;	 /* Upper end */
			f = root_distance(peer);
			e = e + f;
			for (i = nl3 - 1; i >= 0; i--) {
				if (e >= endpoint[indx[i]].val)
					break;
				indx[i + 3] = indx[i];
			}
			indx[i + 3] = nl3;
			endpoint[nl3].type = 1;
			endpoint[nl3++].val = e;

			e = e - f;		/* Center point */
			for ( ; i >= 0; i--) {
				if (e >= endpoint[indx[i]].val)
					break;
				indx[i + 2] = indx[i];
			}
			indx[i + 2] = nl3;
			endpoint[nl3].type = 0;
			endpoint[nl3++].val = e;

			e = e - f;		/* Lower end */
			for ( ; i >= 0; i--) {
				if (e >= endpoint[indx[i]].val)
					break;
				indx[i + 1] = indx[i];
			}
			indx[i + 1] = nl3;
			endpoint[nl3].type = -1;
			endpoint[nl3++].val = e;
		}
	}
#ifdef DEBUG
	if (debug > 2)
		for (i = 0; i < nl3; i++)
			printf("select: endpoint %2d %.6f\n",
			   endpoint[indx[i]].type,
			   endpoint[indx[i]].val);
#endif
	i = 0;
	j = nl3 - 1;
	allow = nlist;		/* falsetickers assumed */
	found = 0;
	while (allow > 0) {
		allow--;
		for (n = 0; i <= j; i++) {
			n += endpoint[indx[i]].type;
			if (n < 0)
				break;
			if (endpoint[indx[i]].type == 0)
				found++;
		}
		for (n = 0; i <= j; j--) {
			n += endpoint[indx[j]].type;
			if (n > 0)
				break;
			if (endpoint[indx[j]].type == 0)
				found++;
		}
		if (found > allow)
			break;
		low = endpoint[indx[i++]].val;
		high = endpoint[indx[j--]].val;
	}

	/*
	 * If no survivors remain at this point, check if the acts or
	 * local clock drivers have been found. If so, nominate one of
	 * them as the only survivor. Otherwise, give up and declare us
	 * unsynchronized.
	 */
	if ((allow << 1) >= nlist) {
		if (typeacts != 0) {
			typeacts->status = CTL_PST_SEL_SANE;
			peer_list[0] = typeacts;
			nlist = 1;
		} else if (typelocal != 0) {
			typelocal->status = CTL_PST_SEL_SANE;
			peer_list[0] = typelocal;
			nlist = 1;
		} else {
			if (sys_peer != 0) {
				report_event(EVNT_PEERSTCHG,
				    (struct peer *)0);
				NLOG(NLOG_SYNCSTATUS)
				msyslog(LOG_INFO,
				    "synchronisation lost");
			}
			sys_peer = 0;
			return;
		}
	}
#ifdef DEBUG
	if (debug > 2)
		printf("select: low %.6f high %.6f\n", low, high);
#endif

	/*
	 * Clustering algorithm. Process intersection list to discard
	 * outlyers. Construct candidate list in cluster order
	 * determined by the sum of peer synchronization distance plus
	 * scaled stratum. We must find at least one peer.
	 */
	j = 0;
	for (i = 0; i < nlist; i++) {
		peer = peer_list[i];
		if (nlist > 1 && (low >= peer->offset ||
			peer->offset >= high))
			continue;
		peer->status = CTL_PST_SEL_CORRECT;
		d = root_distance(peer) + peer->stratum * MAXDISPERSE;
		if (j >= NTP_MAXCLOCK) {
			if (d >= synch[j - 1])
				continue;
			else
				j--;
		}
		for (k = j; k > 0; k--) {
			if (d >= synch[k - 1])
				break;
			synch[k] = synch[k - 1];
			peer_list[k] = peer_list[k - 1];
		}
		peer_list[k] = peer;
		synch[k] = d;
		j++;
	}
	nlist = j;

#ifdef DEBUG
	if (debug > 2)
		for (i = 0; i < nlist; i++)
			printf("select: %s distance %.6f\n",
			    ntoa(&peer_list[i]->srcadr), synch[i]);
#endif

	/*
	 * Now, prune outlyers by root dispersion. Continue as long as
	 * there are more than NTP_MINCLOCK survivors and the minimum
	 * select dispersion is greater than the maximum peer
	 * dispersion. Stop if we are about to discard a prefer peer.
	 */
	for (i = 0; i < nlist; i++) {
		peer = peer_list[i];
		error[i] = peer->variance;
		if (i < NTP_CANCLOCK)
			peer->status = CTL_PST_SEL_SELCAND;
		else
			peer->status = CTL_PST_SEL_DISTSYSPEER;
	}
	while (1) {
		sys_maxd = 0;
		d = error[0];
		for (k = i = nlist - 1; i >= 0; i--) {
			double sdisp = 0;

			for (j = nlist - 1; j > 0; j--) {
				sdisp = NTP_SWEIGHT * (sdisp +
					DIFF(peer_list[i]->offset,
					peer_list[j]->offset));
			}
			if (sdisp > sys_maxd) {
				sys_maxd = sdisp;
				k = i;
			}
			if (error[i] < d)
				d = error[i];
		}

#ifdef DEBUG
		if (debug > 2)
			printf(
			    "select: survivors %d select %.6f peer %.6f\n",
			    nlist, SQRT(sys_maxd), SQRT(d));
#endif
		if (nlist <= NTP_MINCLOCK || sys_maxd <= d ||
			peer_list[k]->flags & FLAG_PREFER)
			break;
		for (j = k + 1; j < nlist; j++) {
			peer_list[j - 1] = peer_list[j];
			error[j - 1] = error[j];
		}
		nlist--;
	}
#ifdef DEBUG
	if (debug > 2) {
		for (i = 0; i < nlist; i++)
			printf(
			    "select: %s offset %.6f, distance %.6f poll %d\n",
			    ntoa(&peer_list[i]->srcadr), peer_list[i]->offset,
			    synch[i], peer_list[i]->pollsw);
	}
#endif

	/*
	 * What remains is a list of not greater than NTP_MINCLOCK
	 * peers. We want only a peer at the lowest stratum to become
	 * the system peer, although all survivors are eligible for the
	 * combining algorithm. First record their order, diddle the
	 * flags and clamp the poll intervals. Then, consider the peers
	 * at the lowest stratum. Of these, OR the leap bits on the
	 * assumption that, if some of them honk nonzero bits, they must
	 * know what they are doing. Also, check for prefer and pps
	 * peers. If a prefer peer is found within clock_max, update the
	 * pps switch. Of the other peers not at the lowest stratum,
	 * check if the system peer is among them and, if found, zap
	 * him. We note that the head of the list is at the lowest
	 * stratum and that unsynchronized peers cannot survive this
	 * far.
	 */
	leap_consensus = 0;
	for (i = nlist - 1; i >= 0; i--) {
		peer_list[i]->status = CTL_PST_SEL_SYNCCAND;
		peer_list[i]->flags |= FLAG_SYSPEER;
		poll_update(peer_list[i], peer_list[i]->hpoll);
		if (peer_list[i]->stratum == peer_list[0]->stratum) {
			leap_consensus |= peer_list[i]->leap;
			if (peer_list[i]->refclktype == REFCLK_ATOM_PPS)
				typepps = peer_list[i];
			if (peer_list[i] == sys_peer)
				typesystem = peer_list[i];
			if (peer_list[i]->flags & FLAG_PREFER) {
				typeprefer = peer_list[i];
				if (fabs(typeprefer->offset) <
				    clock_max)
					pps_update = 1;
			}
		} else {
			if (peer_list[i] == sys_peer)
				sys_peer = 0;
		}
	}

	/*
	 * Mitigation rules of the game. There are several types of
	 * peers that make a difference here: (1) prefer local peers
	 * (type REFCLK_LOCALCLOCK with FLAG_PREFER) or prefer modem
	 * peers (type REFCLK_NIST_ATOM etc with FLAG_PREFER), (2) pps
	 * peers (type REFCLK_ATOM_PPS), (3) remaining prefer peers
	 * (flag FLAG_PREFER), (4) the existing system peer, if any, (5)
	 * the head of the survivor list. Note that only one peer can be
	 * declared prefer. The order of preference is in the order
	 * stated. Note that all of these must be at the lowest stratum,
	 * i.e., the stratum of the head of the survivor list.
	 */
	osys_peer = sys_peer;
	if (typeprefer && (typeprefer->refclktype == REFCLK_LOCALCLOCK
	    || typeprefer->sstclktype == CTL_SST_TS_TELEPHONE ||
		!typepps)) {
		sys_peer = typeprefer;
		sys_peer->status = CTL_PST_SEL_SYSPEER;
		sys_offset = sys_peer->offset;
		sys_epsil = sys_peer->variance;
#ifdef DEBUG
		if (debug > 2)
			printf("select: prefer offset %.6f\n",
			    sys_offset);
#endif
	} else if (typepps && pps_update) {
		sys_peer = typepps;
		sys_peer->status = CTL_PST_SEL_PPS;
		sys_offset = sys_peer->offset;
		sys_epsil = sys_peer->variance;
		if (!pps_control)
			NLOG(NLOG_SYSEVENT) /* conditional syslog */
				msyslog(LOG_INFO, "pps sync enabled");
		pps_control = current_time;
#ifdef DEBUG
		if (debug > 2)
			printf("select: pps offset %.6f\n", sys_offset);
#endif
	} else {
		if (!typesystem)
			sys_peer = peer_list[0];
		sys_peer->status = CTL_PST_SEL_SYSPEER;
		sys_offset = clock_combine(peer_list, nlist);
		sys_epsil = sys_peer->variance + sys_maxd;
#ifdef DEBUG
		if (debug > 2)
			printf("select: combine offset %.6f\n",
			   sys_offset);
#endif
	}
	if (osys_peer != sys_peer)
		report_event(EVNT_PEERSTCHG, (struct peer *)0);
	clock_update();
}

/*
 * clock_combine - combine offsets from selected peers
 */
static double
clock_combine(
	struct peer **peers,
	int npeers
	)
{
	int i;
	double x, y, z;
	y = z = 0;
	for (i = 0; i < npeers; i++) {
		x = root_distance(peers[i]);
		y += 1. / x;
		z += peers[i]->offset / x;
	}
	return (z / y);
}

/*
 * root_distance - compute synchronization distance from peer to root
 */
static double
root_distance(
	struct peer *peer
	)
{
	return ((fabs(peer->delay) + peer->rootdelay) / 2 +
		peer->rootdispersion + peer->disp +
		    SQRT(peer->variance) + CLOCK_PHI * (current_time -
		    peer->update));
}

/*
 * peer_xmit - send packet for persistent association.
 */
static void
peer_xmit(
	struct peer *peer	/* peer structure pointer */
	)
{
	struct pkt xpkt;	/* transmit packet */
	int find_rtt = (peer->cast_flags & MDF_MCAST) &&
	    peer->hmode != MODE_BROADCAST;
	int sendlen, pktlen;
	keyid_t xkeyid;		/* transmit key ID */
	l_fp xmt_tx;

	/*
	 * Initialize transmit packet header fields.
	 */
	xpkt.li_vn_mode = PKT_LI_VN_MODE(sys_leap, peer->version,
	    peer->hmode);
	xpkt.stratum = STRATUM_TO_PKT(sys_stratum);
	xpkt.ppoll = peer->hpoll;
	xpkt.precision = sys_precision;
	xpkt.rootdelay = HTONS_FP(DTOFP(sys_rootdelay));
	xpkt.rootdispersion = HTONS_FP(DTOUFP(sys_rootdispersion +
	    LOGTOD(sys_precision)));
	xpkt.refid = sys_refid;
	HTONL_FP(&sys_reftime, &xpkt.reftime);
	HTONL_FP(&peer->org, &xpkt.org);
	HTONL_FP(&peer->rec, &xpkt.rec);

	/*
	 * If the received packet contains a MAC, the transmitted packet
	 * is authenticated and contains a MAC. If not, the transmitted
	 * packet is not authenticated.
	 *
	 * In the current I/O semantics we can't find the local
	 * interface address to generate a session key until after
	 * receiving a packet. So, the first packet goes out
	 * unauthenticated. That's why the really icky test next is
	 * here.
	 */
	sendlen = LEN_PKT_NOMAC;
	if (!(peer->flags & FLAG_AUTHENABLE) ||
	    (peer->dstadr->sin.sin_addr.s_addr == 0 &&
	    peer->dstadr->bcast.sin_addr.s_addr == 0)) {
		get_systime(&peer->xmt);
		HTONL_FP(&peer->xmt, &xpkt.xmt);
		sendpkt(&peer->srcadr, find_rtt ? any_interface :
		    peer->dstadr, ((peer->cast_flags & MDF_MCAST) &&
		    !find_rtt) ? ((peer->cast_flags & MDF_ACAST) ? -7 :
		    peer->ttl) : -8, &xpkt, sendlen);
		peer->sent++;
#ifdef DEBUG
		if (debug)
			printf("transmit: at %ld %s mode %d\n",
			    current_time, ntoa(&peer->srcadr),
			    peer->hmode);
#endif
		return;
	}

	/*
	 * The received packet contains a MAC, so the transmitted packet
	 * must be authenticated. If autokey is enabled, fuss with the
	 * various modes; otherwise, private key cryptography is used.
	 */
#ifdef AUTOKEY
	if ((peer->flags & FLAG_SKEY)) {
		u_int cmmd;

		/*
		 * The Public Key Dance (PKD): Cryptographic credentials
		 * are contained in extension fields, each including a
		 * 4-octet length/code word followed by a 4-octet
		 * association ID and optional additional data. Optional
		 * data includes a 4-octet data length field followed by
		 * the data itself. Request messages are sent from a
		 * configured association; response messages can be sent
		 * from a configured association or can take the fast
		 * path without ever matching an association. Response
		 * messages have the same code as the request, but have
		 * a response bit and possibly an error bit set. In this
		 * implementation, a message may contain no more than
		 * one command and no more than one response.
		 *
		 * Cryptographic session keys include both a public and
		 * a private componet. Request and response messages
		 * using extension fields are always sent with the
		 * private component set to zero. Packets without
		 * extension fields indlude the private component when
		 * the session key is generated.
		 */
		while (1) {
		
			/*
			 * Allocate and initialize a keylist if not
			 * already done. Then, use the list in inverse
			 * order, discarding keys once used. Keep the
			 * latest key around until the next one, so
			 * clients can use client/server packets to
			 * compute propagation delay.
			 *
			 * Note that once a key is used from the list,
			 * it is retained in the key cache until the
			 * next key is used. This is to allow a client
			 * to retrieve the encrypted session key
			 * identifier to verify authenticity.
			 *
			 * If for some reason a key is no longer in the
			 * key cache, a birthday has happened and the
			 * pseudo-random sequence is probably broken. In
			 * that case, purge the keylist and regenerate
			 * it.
			 */
			if (peer->keynumber == 0)
				make_keylist(peer);
			else
				peer->keynumber--;
			xkeyid = peer->keylist[peer->keynumber];
			if (authistrusted(xkeyid))
				break;
			else
				key_expire(peer);
		}
		peer->keyid = xkeyid;
		switch (peer->hmode) {

		/*
		 * In broadcast mode and a new keylist; otherwise, send
		 * the association ID so the client can request the
		 * values at other times.
		 */
		case MODE_BROADCAST:
			if (peer->keynumber == peer->sndauto.tstamp)
				cmmd = CRYPTO_AUTO | CRYPTO_RESP;
			else
				cmmd = CRYPTO_ASSOC | CRYPTO_RESP;
			sendlen += crypto_xmit((u_int32 *)&xpkt,
			    sendlen, cmmd, peer->hcookie,
			    peer->associd);
			break;

		/*
		 * In symmetric modes the public key, Diffie-Hellman
		 * values and autokey values are required. In principle,
		 * these values can be provided in any order; however,
		 * the protocol is most efficient if the values are
		 * provided in the order listed. This happens with the
		 * following rules:
		 *
		 * 1. Don't send anything except a public-key request or
		 *    a public-key response until the public key has
		 *    been stored. 
		 *
		 * 2. If a public-key response is pending, always send
		 *    it first before any other command or response.
		 *
		 * 3. Once the public key has been stored, don't send
		 *    anything except Diffie-Hellman commands or
		 *    responses until the agreed key has been stored.
		 *
		 * 4. If a Diffie-Hellman response is pending, always
		 *    send it last after any other command or response.
		 *
		 * 5. When the agreed key has been stored and the key
		 *    list is regenerated, send the autokey values
		 *    gratis.
		 */
		case MODE_ACTIVE:
		case MODE_PASSIVE:
#ifdef PUBKEY
			if (crypto_enable && peer->cmmd != 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, (peer->cmmd >> 16) |
				    CRYPTO_RESP, peer->hcookie,
				    peer->associd);
				peer->cmmd = 0;
			}
			if (crypto_enable && crypto_flags &
			    CRYPTO_FLAG_PUBL && peer->pubkey == 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_NAME, peer->hcookie,
				    peer->assoc);
			} else if (peer->pcookie.tstamp == 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_DH, peer->hcookie,
				    peer->assoc);
#else
			if (peer->cmmd != 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, (peer->cmmd >> 16) |
				    CRYPTO_RESP, peer->hcookie,
				    peer->associd);
				peer->cmmd = 0;
			}
			if (peer->pcookie.tstamp == 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_PRIV, peer->hcookie,
				    peer->assoc);
#endif /* PUBKEY */
			} else if (peer->recauto.tstamp == 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_AUTO, peer->hcookie,
				    peer->assoc);
			} else if (peer->keynumber == peer->sndauto.seq)
			    {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_AUTO | CRYPTO_RESP,
				    peer->hcookie, peer->associd);
			}
			break;

		/*
		 * In client mode, the public key, host cookie and
		 * autokey values are required. In broadcast client
		 * mode, these values must be acquired during the
		 * client/server exchange to avoid having to wait until
		 * the next key list regeneration. Otherwise, the poor
		 * dude may die a lingering death until becoming
		 * unreachable and attempting rebirth. Note that we ask
		 * for the cookie at each key list regeneration anyway.
		 */
		case MODE_CLIENT:
			if (peer->cmmd != 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, (peer->cmmd >> 16) |
				    CRYPTO_RESP, peer->hcookie,
				    peer->associd);
				peer->cmmd = 0;
			}
#ifdef PUBKEY
			if (crypto_enable && crypto_flags &
			    CRYPTO_FLAG_PUBL && peer->pubkey == 0) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_NAME, peer->hcookie,
				    peer->assoc);
			} else
#endif /* PUBKEY */
			if (peer->pcookie.tstamp == 0 ||
			    peer->keynumber == peer->sndauto.seq) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_PRIV, peer->hcookie,
				    peer->assoc);
			} else if (peer->recauto.tstamp == 0 &&
			    peer->flags & FLAG_MCAST2) {
				sendlen += crypto_xmit((u_int32 *)&xpkt,
				    sendlen, CRYPTO_AUTO, peer->hcookie,
				    peer->assoc);
			}
			break;
		}

		/*
		 * If extension fields are present, we must use a
		 * private value of zero. Most intricate.
		 */
		if (sendlen > LEN_PKT_NOMAC)
			session_key(&peer->dstadr->sin,
			    (peer->hmode == MODE_BROADCAST) ?
	    		    &peer->dstadr->bcast : &peer->srcadr,
			    xkeyid, 0, 2);
	} 
#endif /* AUTOKEY */
	xkeyid = peer->keyid;
	get_systime(&peer->xmt);
	L_ADD(&peer->xmt, &sys_authdelay);
	HTONL_FP(&peer->xmt, &xpkt.xmt);
	pktlen = sendlen + authencrypt(xkeyid, (u_int32 *)&xpkt,
	    sendlen);
#ifdef AUTOKEY
	if (xkeyid > NTP_MAXKEY)
		authtrust(xkeyid, 0);
#endif /* AUTOKEY */
	get_systime(&xmt_tx);
	sendpkt(&peer->srcadr, find_rtt ? any_interface : peer->dstadr,
	    ((peer->cast_flags & MDF_MCAST) && !find_rtt) ?
	    ((peer->cast_flags & MDF_ACAST) ? -7 : peer->ttl) : -7,
	    &xpkt, pktlen);

	/*
	 * Calculate the encryption delay. Keep the minimum over
	 * the latest two samples.
	 */
	L_SUB(&xmt_tx, &peer->xmt);
	L_ADD(&xmt_tx, &sys_authdelay);
	sys_authdly[1] = sys_authdly[0];
	sys_authdly[0] = xmt_tx.l_uf;
	if (sys_authdly[0] < sys_authdly[1])
		sys_authdelay.l_uf = sys_authdly[0];
	else
		sys_authdelay.l_uf = sys_authdly[1];
	peer->sent++;
#ifdef AUTOKEY
#ifdef DEBUG
	if (debug)
		printf(
		    "transmit: at %ld %s mode %d keyid %08x len %d mac %d index %d\n",
		    current_time, ntoa(&peer->srcadr), peer->hmode,
		    xkeyid, sendlen, pktlen - sendlen, peer->keynumber);
#endif
#else
#ifdef DEBUG
	if (debug)
		printf(
		    "transmit: at %ld %s mode %d keyid %08x len %d mac %d\n",
		    current_time, ntoa(&peer->srcadr), peer->hmode,
		    xkeyid, sendlen, pktlen - sendlen);
#endif
#endif /* AUTOKEY */
}


/*
 * fast_xmit - Send packet for nonpersistent association. Note that
 * neither the source or destination can be a broadcast address.
 */
static void
fast_xmit(
	struct recvbuf *rbufp,	/* receive packet pointer */
	int xmode,		/* transmit mode */
	keyid_t xkeyid		/* transmit key ID */
	)
{
	struct pkt xpkt;	/* transmit packet structure */
	struct pkt *rpkt;	/* receive packet structure */
	l_fp xmt_ts;		/* transmit timestamp */
	l_fp xmt_tx;		/* transmit timestamp after authent */
	int sendlen, pktlen;

	/*
	 * Initialize transmit packet header fields from the receive
	 * buffer provided. We leave some fields intact as received.
	 */
	rpkt = &rbufp->recv_pkt;
	xpkt.li_vn_mode = PKT_LI_VN_MODE(sys_leap,
	    PKT_VERSION(rpkt->li_vn_mode), xmode);
	xpkt.stratum = STRATUM_TO_PKT(sys_stratum);
	xpkt.ppoll = rpkt->ppoll;
	xpkt.precision = sys_precision;
	xpkt.rootdelay = HTONS_FP(DTOFP(sys_rootdelay));
	xpkt.rootdispersion = HTONS_FP(DTOUFP(sys_rootdispersion +
	    LOGTOD(sys_precision)));
	xpkt.refid = sys_refid;
	HTONL_FP(&sys_reftime, &xpkt.reftime);
	xpkt.org = rpkt->xmt;
	HTONL_FP(&rbufp->recv_time, &xpkt.rec);

	/*
	 * If the received packet contains a MAC, the transmitted packet
	 * is authenticated and contains a MAC. If not, the transmitted
	 * packet is not authenticated.
	 */
	sendlen = LEN_PKT_NOMAC;
	if (rbufp->recv_length == sendlen) {
		get_systime(&xmt_ts);
		HTONL_FP(&xmt_ts, &xpkt.xmt);
		sendpkt(&rbufp->recv_srcadr, rbufp->dstadr, -10, &xpkt,
		    sendlen);
#ifdef DEBUG
		if (debug)
			printf("transmit: at %ld %s mode %d\n",
			    current_time, ntoa(&rbufp->recv_srcadr),
			    xmode);
#endif
		return;
	}

	/*
	 * The received packet contains a MAC, so the transmitted packet
	 * must be authenticated. For private-key cryptography, use the
	 * predefined private keys to generate the cryptosum. For
	 * autokeys in client/server mode, use the server private value
	 * values to generate the cookie, which is unique for every
	 * source-destination-key ID combination. For symmetric passive
	 * mode, which is the only other mode to get here, flip the
	 * addresses and do the same. If an extension field is present,
	 * do what needs, but with private value of zero so the poor
	 * jerk can decode it. If no extension field is present, use the
	 * cookie to generate the session key.
	 */
#ifdef AUTOKEY
	if (xkeyid > NTP_MAXKEY) {
		keyid_t cookie;
		u_int code;

		if (xmode == MODE_SERVER)
			cookie = session_key(&rbufp->recv_srcadr,
			    &rbufp->dstadr->sin, 0, sys_private, 0);
		else
			cookie = session_key(&rbufp->dstadr->sin,
			    &rbufp->recv_srcadr, 0, sys_private, 0);
		if (rbufp->recv_length >= sendlen + MAX_MAC_LEN + 2 *
		    sizeof(u_int32)) {
			session_key(&rbufp->dstadr->sin,
			    &rbufp->recv_srcadr, xkeyid, 0, 2);
			code = (htonl(rpkt->exten[0]) >> 16) |
			    CRYPTO_RESP;
			sendlen += crypto_xmit((u_int32 *)&xpkt,
			    sendlen, code, cookie,
			    (int)htonl(rpkt->exten[1]));
		} else {
			session_key(&rbufp->dstadr->sin,
			    &rbufp->recv_srcadr, xkeyid, cookie, 2);
		}
	}
#endif /* AUTOKEY */
	get_systime(&xmt_ts);
	L_ADD(&xmt_ts, &sys_authdelay);
	HTONL_FP(&xmt_ts, &xpkt.xmt);
	pktlen = sendlen + authencrypt(xkeyid, (u_int32 *)&xpkt,
	    sendlen);
#ifdef AUTOKEY
	if (xkeyid > NTP_MAXKEY)
		authtrust(xkeyid, 0);
#endif /* AUTOKEY */
	get_systime(&xmt_tx);
	sendpkt(&rbufp->recv_srcadr, rbufp->dstadr, -9, &xpkt, pktlen);

	/*
	 * Calculate the encryption delay. Keep the minimum over the
	 * latest two samples.
	 */
	L_SUB(&xmt_tx, &xmt_ts);
	L_ADD(&xmt_tx, &sys_authdelay);
	sys_authdly[1] = sys_authdly[0];
	sys_authdly[0] = xmt_tx.l_uf;
	if (sys_authdly[0] < sys_authdly[1])
		sys_authdelay.l_uf = sys_authdly[0];
	else
		sys_authdelay.l_uf = sys_authdly[1];
#ifdef DEBUG
	if (debug)
		printf(
		    "transmit: at %ld %s mode %d keyid %08x len %d mac %d\n",
		    current_time, ntoa(&rbufp->recv_srcadr),
		    xmode, xkeyid, sendlen, pktlen - sendlen);
#endif
}


#ifdef AUTOKEY
/*
 * key_expire - purge the key list
 */
void
key_expire(
	struct peer *peer	/* peer structure pointer */
	)
{
	int i;

 	if (peer->keylist != NULL) {
		for (i = 0; i <= peer->keynumber; i++)
			authtrust(peer->keylist[i], 0);
		free(peer->keylist);
		peer->keylist = NULL;
	}
	peer->keynumber = peer->sndauto.seq = 0;
}
#endif /* AUTOKEY */

/*
 * Find the precision of this particular machine
 */
#define DUSECS	1000000 /* us in a s */
#define HUSECS	(1 << 20) /* approx DUSECS for shifting etc */
#define MINSTEP 5	/* minimum clock increment (us) */
#define MAXSTEP 20000	/* maximum clock increment (us) */
#define MINLOOPS 5	/* minimum number of step samples */

/*
 * This routine calculates the differences between successive calls to
 * gettimeofday(). If a difference is less than zero, the us field
 * has rolled over to the next second, so we add a second in us. If
 * the difference is greater than zero and less than MINSTEP, the
 * clock has been advanced by a small amount to avoid standing still.
 * If the clock has advanced by a greater amount, then a timer interrupt
 * has occurred and this amount represents the precision of the clock.
 * In order to guard against spurious values, which could occur if we
 * happen to hit a fat interrupt, we do this for MINLOOPS times and
 * keep the minimum value obtained.
 */
int
default_get_precision(void)
{
	struct timeval tp;
#if !defined(SYS_WINNT) && !defined(VMS) && !defined(_SEQUENT_)
	struct timezone tzp;
#elif defined(VMS) || defined(_SEQUENT_)
	struct timezone {
		int tz_minuteswest;
		int tz_dsttime;
	} tzp;
#endif /* defined(VMS) || defined(_SEQUENT_) */
	long last;
	int i;
	long diff;
	long val;
	long usec;
#ifdef HAVE_GETCLOCK
	struct timespec ts;
#endif
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	u_long freq;
	size_t j;

	/* Try to see if we can find the frequency of of the counter
	 * which drives our timekeeping
	 */
	j = sizeof freq;
	i = sysctlbyname("kern.timecounter.frequency", &freq, &j , 0,
	    0);
	if (i)
		i = sysctlbyname("machdep.tsc_freq", &freq, &j , 0, 0);
	if (i)
		i = sysctlbyname("machdep.i586_freq", &freq, &j , 0, 0);
	if (i)
		i = sysctlbyname("machdep.i8254_freq", &freq, &j , 0,
		    0);
	if (!i) {
		for (i = 1; freq ; i--)
			freq >>= 1;
		return (i);
	}
#endif
	usec = 0;
	val = MAXSTEP;
#ifdef HAVE_GETCLOCK
	(void) getclock(TIMEOFDAY, &ts);
	tp.tv_sec = ts.tv_sec;
	tp.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
	GETTIMEOFDAY(&tp, &tzp);
#endif /* not HAVE_GETCLOCK */
	last = tp.tv_usec;
	for (i = 0; i < MINLOOPS && usec < HUSECS;) {
#ifdef HAVE_GETCLOCK
		(void) getclock(TIMEOFDAY, &ts);
		tp.tv_sec = ts.tv_sec;
		tp.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
		GETTIMEOFDAY(&tp, &tzp);
#endif /* not HAVE_GETCLOCK */
		diff = tp.tv_usec - last;
		last = tp.tv_usec;
		if (diff < 0)
			diff += DUSECS;
		usec += diff;
		if (diff > MINSTEP) {
			i++;
			if (diff < val)
				val = diff;
		}
	}
	NLOG(NLOG_SYSINFO)
		msyslog(LOG_INFO, "precision = %ld usec", val);
	if (usec >= HUSECS)
		val = MINSTEP;	/* val <= MINSTEP; fast machine */
	diff = HUSECS;
	for (i = 0; diff > val; i--)
		diff >>= 1;
	return (i);
}

/*
 * init_proto - initialize the protocol module's data
 */
void
init_proto(void)
{
	l_fp dummy;

	/*
	 * Fill in the sys_* stuff.  Default is don't listen to
	 * broadcasting, authenticate.
	 */
	sys_leap = LEAP_NOTINSYNC;
	sys_stratum = STRATUM_UNSPEC;
	sys_precision = (s_char)default_get_precision();
	sys_rootdelay = 0;
	sys_rootdispersion = 0;
	sys_refid = 0;
	L_CLR(&sys_reftime);
	sys_peer = 0;
	get_systime(&dummy);
	sys_bclient = 0;
	sys_bdelay = DEFBROADDELAY;
#if defined(DES) || defined(MD5)
	sys_authenticate = 1;
#else
	sys_authenticate = 0;
#endif
	L_CLR(&sys_authdelay);
	sys_authdly[0] = sys_authdly[1] = 0;
	sys_stattime = 0;
	sys_badstratum = 0;
	sys_oldversionpkt = 0;
	sys_newversionpkt = 0;
	sys_badlength = 0;
	sys_unknownversion = 0;
	sys_processed = 0;
	sys_badauth = 0;
	sys_manycastserver = 0;
#ifdef AUTOKEY
	sys_automax = 1 << NTP_AUTOMAX;
#endif /* AUTOKEY */

	/*
	 * Default these to enable
	 */
	ntp_enable = 1;
#ifndef KERNEL_FLL_BUG
	kern_enable = 1;
#endif
	msyslog(LOG_DEBUG, "kern_enable is %d", kern_enable);
	stats_control = 1;

	/*
	 * Some system clocks should only be adjusted in 10ms increments.
	 */
#if defined RELIANTUNIX_CLOCK
	systime_10ms_ticks = 1;		  /* Reliant UNIX */
#elif defined SCO5_CLOCK
	if (sys_precision >= (s_char)-10) /* pre-SCO OpenServer 5.0.6 */
		systime_10ms_ticks = 1;
#endif
	if (systime_10ms_ticks)
		msyslog(LOG_INFO, "using 10ms tick adjustments");
}


/*
 * proto_config - configure the protocol module
 */
void
proto_config(
	int item,
	u_long value,
	double dvalue
	)
{
	/*
	 * Figure out what he wants to change, then do it
	 */
	switch (item) {
	case PROTO_KERNEL:

		/*
		 * Turn on/off kernel discipline
		 */
		kern_enable = (int)value;
		break;

	case PROTO_NTP:

		/*
		 * Turn on/off clock discipline
		 */
		ntp_enable = (int)value;
		break;

	case PROTO_MONITOR:

		/*
		 * Turn on/off monitoring
		 */
		if (value)
			mon_start(MON_ON);
		else
			mon_stop(MON_ON);
		break;

	case PROTO_FILEGEN:

		/*
		 * Turn on/off statistics
		 */
		stats_control = (int)value;
		break;

	case PROTO_BROADCLIENT:

		/*
		 * Turn on/off facility to listen to broadcasts
		 */
		sys_bclient = (int)value;
		if (value)
			io_setbclient();
		else
			io_unsetbclient();
		break;

	case PROTO_MULTICAST_ADD:

		/*
		 * Add muliticast group address
		 */
		io_multicast_add(value);
		break;

	case PROTO_MULTICAST_DEL:

		/*
		 * Delete multicast group address
		 */
		io_multicast_del(value);
		break;

	case PROTO_BROADDELAY:

		/*
		 * Set default broadcast delay
		 */
		sys_bdelay = dvalue;
		break;

	case PROTO_AUTHENTICATE:

		/*
		 * Specify the use of authenticated data
		 */
		sys_authenticate = (int)value;
		break;

	default:

		/*
		 * Log this error
		 */
		msyslog(LOG_ERR,
		    "proto_config: illegal item %d, value %ld",
			item, value);
		break;
	}
}


/*
 * proto_clr_stats - clear protocol stat counters
 */
void
proto_clr_stats(void)
{
	sys_badstratum = 0;
	sys_oldversionpkt = 0;
	sys_newversionpkt = 0;
	sys_unknownversion = 0;
	sys_badlength = 0;
	sys_processed = 0;
	sys_badauth = 0;
	sys_stattime = current_time;
	sys_limitrejected = 0;
}
