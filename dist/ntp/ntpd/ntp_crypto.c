/*	$NetBSD: ntp_crypto.c,v 1.2 2003/05/17 01:14:33 itojun Exp $	*/

/*
 * ntp_crypto.c - NTP version 4 public key routines
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef AUTOKEY
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "ntpd.h"
#include "ntp_stdlib.h"
#include "ntp_string.h"
#include "ntp_crypto.h"

/*
 * Extension field message formats
 *
 *   +-------+-------+   +-------+-------+   +-------+-------+
 * 0 |   3   |  len  |   |   4   |  len  |   |   5   |  len  |
 *   +-------+-------+   +-------+-------+   +-------+-------+
 * 1 |    assoc ID   |   |    assoc ID   |   |    assoc ID   |
 *   +---------------+   +---------------+   +---------------+
 * 2 |   timestamp   |   |   timestamp   |   |   timestamp   |
 *   +---------------+   +---------------+   +---------------+
 * 3 |   final seq   |   |     cookie    |   |   value len   |
 *   +---------------+   +---------------+   +---------------+
 * 4 |   final key   |   | signature len |   |               |
 *   +---------------+   +---------------+   =     value     =
 * 5 | signature len |   |               |   |               |
 *   +---------------+   =   signature   =   +---------------+
 * 6 |               |   |               |   | signature len |
 *   =   signature   =   +---------------+   +---------------+
 * 7 |               |   CRYPTO_PRIV rsp     |               |
 *   +---------------+                       =   signature   =
 *   CRYPTO_AUTO rsp                         |               |
 *                                           +---------------+
 *                                           CRYPTO_DH rsp
 *                                           CRYPTO_NAME rsp
 *                                           
 *   CRYPTO_PUBL  1  request/respond for public key
 *   CRYPTO_ASSOC 2  request/respond association ID
 *   CRYPTO_AUTO  3  request/respond autokey values
 *   CRYPTO_PRIV  4  request/respond cookie
 *   CRYPTO_DH    5  request public value/respond signature
 *   CRYPTO_NAME  6  request/respond host name
 *
 *   Note: requests carry the association ID of the receiver; responses
 *   carry the association ID of the sender.
 */

#ifdef PUBKEY
/*
 * Cryptodefines
 */
#define MAX_KEYLEN	1024	/* maximum key length */

/*
 * Cryptodata
 */
static R_DH_PARAMS dh_params;	/* Diffie-Hellman parameters */
static u_int dh_keyLen;		/* Diffie-Hellman key length */
static u_char *dh_public;	/* Diffie-Hellman public value */
static u_char *dh_private;	/* Diffie-Hellman private value */
static R_RSA_PRIVATE_KEY private_key; /* RSA private key */
static R_RSA_PUBLIC_KEY public_key; /* RSA public key */
static u_char *dh_sign = NULL;	/* Diffie-Hellman public signature */
static struct value host;	/* host name, timestamp and signature */

int	crypto_enable;		/* master switch */
int	crypto_flags;		/* flags that wave cryptically */
char	*private_key_file = NULL; /* private key file */
char	*public_key_file = NULL; /* public key file */
char	*dh_params_file = NULL;	/* D-H parameters file */
char	*keysdir = "/usr/local/etc/"; /* crypto keys directory */

/*
 * Cryptotypes
 */
static	int	crypto_read	P((u_char *, u_char *, u_int));
static	void	crypto_line	P((FILE *, u_char **, u_int *));
#endif /* PUBKEY */


/*
 * session_key - generate session key
 *
 * This routine generates a session key from the source address,
 * destination address, key ID and private value. The value of the
 * session key is the MD5 hash of these values, while the next key ID is
 * the first four octets of the hash.
 */
keyid_t				/* returns next key ID */
session_key(
	struct sockaddr_in *srcadr, /* source address */
	struct sockaddr_in *dstadr, /* destination address */
	keyid_t keyno,		/* key ID */
	keyid_t private,	/* private value */
	u_long lifetime 	/* key lifetime */
	)
{
	MD5_CTX ctx;		/* MD5 context */
	keyid_t keyid;		/* key identifer */
	u_int32 header[4];	/* data in network byte order */
	u_char digest[16];	/* message digest */

	/*
	 * Generate the session key and key ID. If the lifetime is
	 * greater than zero, install the key and call it trusted.
	 */
	header[0] = srcadr->sin_addr.s_addr;
	header[1] = dstadr->sin_addr.s_addr;
	header[2] = htonl(keyno);
	header[3] = htonl(private);
	MD5Init(&ctx);
	MD5Update(&ctx, (u_char *)header, sizeof(header));
	MD5Final(digest, &ctx);
	memcpy(&keyid, digest, 4);
	keyid = ntohl(keyid);
	if (lifetime != 0) {
		MD5auth_setkey(keyno, digest, 16);
		authtrust(keyno, lifetime);
	}
#ifdef DEBUG
	if (debug > 1)
		printf(
		    "session_key: %s > %s %08x %08x hash %08x life %lu\n",
		    numtoa(header[0]), numtoa(header[1]), keyno,
		    private, keyid, lifetime);
#endif
	return (keyid);
}


/*
 * make_keylist - generate key list
 *
 * This routine constructs a pseudo-random sequence by repeatedly
 * hashing the session key starting from a given source address,
 * destination address, private value and the next key ID of the
 * preceeding session key. The last entry on the list is saved along
 * with its sequence number and public signature.
 */
void
make_keylist(
	struct peer *peer	/* peer structure pointer */
	)
{
	struct autokey *ap;	/* autokey pointer */
	keyid_t keyid;		/* next key ID */
	keyid_t cookie;		/* private value */
	l_fp tstamp;		/* NTP timestamp */
	u_long ltemp;
	int i;
#ifdef PUBKEY
	R_SIGNATURE_CTX ctx;	/* signature context */
	int rval;		/* return value */
	u_int len;
#endif /* PUBKEY */

	/*
	 * Allocate the key list if necessary.
	 */
	L_CLR(&tstamp);
	if (sys_leap != LEAP_NOTINSYNC)
		get_systime(&tstamp);
	if (peer->keylist == NULL)
		peer->keylist = (keyid_t *)emalloc(sizeof(keyid_t) *
		    NTP_MAXSESSION);

	/*
	 * Generate an initial key ID which is unique and greater than
	 * NTP_MAXKEY.
	 */
	while (1) {
		keyid = (u_long)RANDOM & 0xffffffff;
		if (keyid <= NTP_MAXKEY)
			continue;
		if (authhavekey(keyid))
			continue;
		break;
	}

	/*
	 * Generate up to NTP_MAXSESSION session keys. Stop if the
	 * next one would not be unique or not a session key ID or if
	 * it would expire before the next poll. The private value
	 * included in the hash is zero if broadcast mode, the peer
	 * cookie if client mode or the host cookie if symmetric modes.
	 */
	ltemp = sys_automax;
	peer->hcookie = session_key(&peer->dstadr->sin, &peer->srcadr,
	    0, sys_private, 0);
	if (peer->hmode == MODE_BROADCAST) {
		cookie = 0;
	} else {
#ifdef PUBKEY
		cookie = peer->pcookie.key;
#else
		if (peer->hmode == MODE_CLIENT)
			cookie = peer->pcookie.key;
		else
			cookie = peer->hcookie ^ peer->pcookie.key;
#endif /* PUBKEY */
	}
	for (i = 0; i < NTP_MAXSESSION; i++) {
		peer->keylist[i] = keyid;
		peer->keynumber = i;
		keyid = session_key(&peer->dstadr->sin, (peer->hmode ==
		    MODE_BROADCAST) ? &peer->dstadr->bcast :
		    &peer->srcadr, keyid, cookie, ltemp);
		ltemp -= 1 << peer->hpoll;
		if (auth_havekey(keyid) || keyid <= NTP_MAXKEY ||
		    ltemp <= (1 << (peer->hpoll + 1)))
			break;
	}

	/*
	 * Save the last session key ID, sequence number and timestamp,
	 * then sign these values for later retrieval by the clients. Be
	 * careful not to use invalid key media.
	 */
	ap = &peer->sndauto;
	ap->tstamp = htonl(tstamp.l_ui);
	ap->seq = htonl(peer->keynumber);
	ap->key = htonl(keyid);
	ap->siglen = 0;
#if DEBUG
	if (debug)
		printf("make_keys: %d %08x %08x ts %u\n", ntohl(ap->seq),
		    ntohl(ap->key), cookie, ntohl(ap->tstamp));
#endif
#ifdef PUBKEY
	if(!crypto_enable)
		return;
	if (private_key.bits < MIN_RSA_MODULUS_BITS ||
	    private_key.bits > MAX_RSA_MODULUS_BITS) {
		rval = -1;
	} else {
		if (ap->sig == NULL)
			ap->sig = emalloc(MAX_SIGNATURE_LEN);
		R_SignInit(&ctx, DA_MD5);
		R_SignUpdate(&ctx, (u_char *)ap, 12);
		rval = R_SignFinal(&ctx, ap->sig, &len, &private_key);
		ap->siglen = htonl(len);
	}
	if (rval != 0)
		msyslog(LOG_ERR, "make_keylist: signature fails %x",
		    rval);
#endif /* PUBKEY */
}


/*
 * crypto_recv - parse extension fields
 *
 * This routine is called when the packet has been matched to an
 * association and passed sanity, format and authentication checks. We
 * believe the extension field values only if the public key is valid
 * and the signature has valid length and is verified.
 */
void
crypto_recv(
	struct peer *peer,	/* peer structure pointer */
	struct recvbuf *rbufp	/* packet buffer pointer */
	)
{
	u_int32 *pkt;		/* packet pointer */
	struct autokey *ap;	/* autokey pointer */
	struct cookie *cp;	/* cookie pointer */
	int has_mac;		/* length of MAC field */
	int authlen;		/* offset of MAC field */
	int len;		/* extension field length */
	u_int code;		/* extension field opcode */
	u_int tstamp;		/* extension field timestamp */
	int i;
	u_int temp;
#ifdef PUBKEY
	R_SIGNATURE_CTX ctx;	/* signature context */
	u_char dh_key[MAX_DH_LEN]; /* Diffie-Hellman agreed key */
	u_int modulus;
	int rval;
	int j;
#endif /* PUBKEY */

	/*
	 * Initialize. Note that the packet has already been checked for
	 * valid format and extension field lengths.
	 */
	pkt = (u_int32 *)&rbufp->recv_pkt;
	authlen = LEN_PKT_NOMAC;
	while ((has_mac = rbufp->recv_length - authlen) > MAX_MAC_LEN) {
		i = authlen / 4;
		len = ntohl(pkt[i]) & 0xffff;
		code = (ntohl(pkt[i]) >> 16) & 0xffff;
		tstamp = ntohl(pkt[i + 2]);
		if (code & CRYPTO_RESP)
			peer->assoc = ntohl(pkt[i + 1]);
#ifdef DEBUG
		if (debug)
			printf(
			    "crypto_recv: ext offset %d len %d code %x assoc ID %d\n",
			    authlen, len, code, (u_int32)ntohl(pkt[i +
			    1]));
#endif
		switch (code) {

		/*
		 * Exchange association IDs. This is used in multicast
		 * server mode and is a NOP here.
		 */
		case CRYPTO_ASSOC | CRYPTO_RESP:
			break;

		/*
		 * Install autokey values in broadcast client and
		 * symmetric modes. We believe the values only if the
		 * public key is valid and the signature has valid
		 * length and is verified. However, we mark as authentic
		 * only if the timestamp is nonzero.
		 */
		case CRYPTO_AUTO | CRYPTO_RESP:
			ap = (struct autokey *)&pkt[i + 2];
#ifdef PUBKEY
			temp = public_key.bits / 8;
			if (!crypto_enable) {
				rval = 0;
			} else if (tstamp < peer->recauto.tstamp) {
				break;
			} else if (peer->pubkey == NULL || temp !=
			    ntohl(ap->siglen)) {
				rval = -1;
			} else {
				R_VerifyInit(&ctx, DA_MD5);
				R_VerifyUpdate(&ctx, (u_char *)ap, 12);
				rval = R_VerifyFinal(&ctx,
				    (u_char *)&ap->sig, temp,
				    (R_RSA_PUBLIC_KEY *)peer->pubkey);
			}
#ifdef DEBUG
			if (debug)
				printf(
				    "crypto_recv: verify %x autokey %d %08x ts %u (%u)\n",
				    rval, ntohl(ap->seq),
				    ntohl(ap->key), tstamp,
				    peer->recauto.tstamp);
#endif
			if (rval != 0) {
				peer->flags &= ~FLAG_AUTOKEY;
				break;
			}
			if (tstamp != 0)
				peer->flags |= FLAG_AUTOKEY;
#endif /* PUBKEY */
			peer->flash &= ~TEST10;
			peer->recauto.tstamp = ntohl(ap->tstamp);
			peer->recauto.seq = ntohl(ap->seq);
			peer->recauto.key = ntohl(ap->key);
			peer->pkeyid = peer->recauto.key;
			break;

		/*
		 * Install session cookie in client mode. We believe the
		 * value only if the public key is valid and the
		 * signature has valid length and is verified. However,
		 * we mark as authentic only if the value is nonzero.
		 */
		case CRYPTO_PRIV | CRYPTO_RESP:
			cp = (struct cookie *)&pkt[i + 2];
#ifdef PUBKEY
			temp = public_key.bits / 8;
			if (!crypto_enable) {
				rval = 0;
			} else if (tstamp < peer->pcookie.tstamp) {
				break;
			} else if (peer->pubkey == NULL || temp !=
			    ntohl(cp->siglen)) {
				rval = -1;
			} else {
				R_VerifyInit(&ctx, DA_MD5);
				R_VerifyUpdate(&ctx, (u_char *)cp, 8);
				rval = R_VerifyFinal(&ctx,
				    (u_char *)&cp->sig, temp,
				    (R_RSA_PUBLIC_KEY *)peer->pubkey);
			}
			temp = cp->key;
#ifdef DEBUG
			if (debug)
				printf(
			    "crypto_recv: verify %x cookie %08x ts %u\n",
				    rval, temp, tstamp);
#endif
			if (rval != 0) {
				peer->flags &= ~FLAG_AUTOKEY;
				break;
			}
			if (tstamp != 0)
				peer->flags |= FLAG_AUTOKEY;
#else
			temp = cp->key;
#endif /* PUBKEY */
			peer->flash &= ~TEST10;
			if (temp != peer->pcookie.key) {
				key_expire(peer);
				peer->pcookie.tstamp = tstamp;
				peer->pcookie.key = temp;
			}
			break;

#ifdef PUBKEY
		/*
		 * Verify Diffie-Hellman public value and compute key
		 * agreement in symmetric modes. We believe the
		 * value only if the public key is valid and the
		 * signature has valid length and is verified. 
		 */
		case CRYPTO_DH:
			peer->cmmd = ntohl(pkt[i]);
			/* fall through */

		case CRYPTO_DH | CRYPTO_RESP:
			temp = ntohl(pkt[i + 3]);
			j = i + 4 + temp / 4;
			if (tstamp < peer->pcookie.tstamp) {
				break;
			} else if (peer->pubkey == NULL ||
			    ntohl(pkt[j]) != public_key.bits / 8) {
				rval = -1;
			} else if (temp != dh_params.primeLen ||
			    dh_public == NULL) {
				rval = -2;
			} else {
				R_VerifyInit(&ctx, DA_MD5);
				R_VerifyUpdate(&ctx, (u_char *)&pkt[i +
				    2], temp + 8);
				rval = R_VerifyFinal(&ctx,
				    (u_char *)&pkt[j + 1],
				    public_key.bits / 8,
				    (R_RSA_PUBLIC_KEY *)peer->pubkey);
			}

			/*
			 * Run the agreement algorithm and stash the key
			 * value. We use only the first u_int32 for the
			 * host cookie. Wasteful.
			 */
			if (rval != 0) {
				temp = 0;
			} else {
				rval = R_ComputeDHAgreedKey(dh_key,
				    (u_char *)&pkt[i + 4], dh_private,
				    dh_keyLen, &dh_params);
				temp = ntohl(*(u_int32 *)dh_key);
			}
#ifdef DEBUG
			if (debug)
				printf(
				    "crypto_recv: verify %x d-h %08x ts %u\n",
				    rval, temp, tstamp);
#endif
			if (rval != 0) {
				peer->flags &= ~FLAG_AUTOKEY;
				peer->cmmd = 0;
				break;
			}
			peer->flash &= ~TEST10;
			if (temp != peer->pcookie.key) {
				key_expire(peer);
				peer->pcookie.tstamp = tstamp;
				peer->pcookie.key = temp;
			}
			break;

		/*
		 * Receive remote host name and install public key from
		 * file.
		 */
		case CRYPTO_NAME | CRYPTO_RESP:
			temp = ntohl(pkt[i + 3]);
			j = i + 4 + temp / 4;
			if (tstamp < peer->pcookie.tstamp) {
				break;
			} else if (ntohl(pkt[j]) != public_key.bits / 8)
			    {
				rval = -1;
			} else if (crypto_public(peer, (char *)&pkt[i +
			    4])) {
				R_VerifyInit(&ctx, DA_MD5);

				R_VerifyUpdate(&ctx, (char *)&pkt[i +
				    2], 8 + temp);

				rval = R_VerifyFinal(&ctx,
				    (u_char *)&pkt[j + 1],
				    public_key.bits / 8,
				    (R_RSA_PUBLIC_KEY *)peer->pubkey);
				if (rval != 0) {
					free(peer->pubkey);
					peer->pubkey = NULL;
				}
			} else {
				rval = -2;
			}
#ifdef DEBUG
			if (debug)

				printf(
				    "crypto_recv: verify %x host %s ts %u\n",
				    rval, (char *)&pkt[i + 4], tstamp); 
#endif
			break;

		/*
		 * Install peer public key. This is rather raucus, since
		 * the extension field is in network order and the first
		 * word is a u_int32; however, the corresponding word of
		 * the key is a u_int, which can be 32 or 64 bits
		 * depending on architecture. We don't do anything
		 * unless the length and modulus are valid. Picky,
		 * picky.
		 */
		case CRYPTO_PUBL | CRYPTO_RESP:
			temp = sizeof(R_RSA_PUBLIC_KEY) - sizeof(u_int);
			if (ntohl(pkt[i + 2]) != temp + 4)
				break;
			modulus = ntohl(pkt[i + 3]);
			if ( modulus < MIN_RSA_MODULUS_BITS ||
			    modulus > MAX_RSA_MODULUS_BITS)
				break;
			if (peer->pubkey == NULL)
				peer->pubkey =
				    emalloc(sizeof(R_RSA_PUBLIC_KEY));
			((R_RSA_PUBLIC_KEY *)peer->pubkey)->bits =
			    modulus;
			memcpy(
			    ((R_RSA_PUBLIC_KEY *)peer->pubkey)->modulus,
			    (u_char *)&(pkt[i + 4]), temp);
			break;
#endif /* PUBKEY */

		/*
		 * For other requests, save the request code for later;
		 * for unknown responses or errors, just ignore for now.
		 */
		default:
			if (code & (CRYPTO_RESP | CRYPTO_ERROR))
				break;
			peer->cmmd = ntohl(pkt[i]);
			break;

		}
		authlen += len;
	}
	return;
}


/*
 * crypto_xmit - construct extension fields
 *
 * This routine is called both when an association is configured and
 * when one is not. The only case where this matters now is to retrieve
 * the autokey information, in which case the caller has to provide the
 * association ID to match the association.
 */
int				/* return length of extension field */
crypto_xmit(
	u_int32 *xpkt,		/* packet pointer */
	int start,		/* offset to extension field */
	u_int code,		/* extension field code */
	keyid_t cookie,		/* session cookie */
	int associd		/* association ID */
	)
{
	struct peer *peer;	/* peer structure pointer */
	struct autokey *ap;	/* autokey pointer */
	struct cookie *cp;	/* cookie pointer */
	int len;		/* extension field length */
	u_int opcode;		/* extension field opcode */
	int i;
#ifdef PUBKEY
	R_SIGNATURE_CTX ctx;	/* signature context */
	struct value *vp;	/* value pointer */
	int rval;		/* return value */
	u_int temp;
	int j;
#endif /* PUBKEY */

	/*
	 * Generate the requested extension field request code, length
	 * and association ID.
	 */
	i = start / 4;
	opcode = code;
	xpkt[i + 1] = htonl(associd);
	len = 8;
	switch (opcode) {

	/*
	 * Exchange association IDs. This is used in multicast server
	 * mode and is a NOP here.
	 */
	case CRYPTO_ASSOC | CRYPTO_RESP:
		break;

	/*
	 * Find peer and send autokey data and signature in broadcast
	 * server and symmetric modes.
	 */
	case CRYPTO_AUTO | CRYPTO_RESP:
		peer = findpeerbyassoc(associd);
		if (peer == NULL) {
			opcode |= CRYPTO_ERROR;
			break;
		}
		ap = (struct autokey *)&xpkt[i + 2];
		ap->tstamp = peer->sndauto.tstamp;
		ap->seq = peer->sndauto.seq;
		ap->key = peer->sndauto.key;
		ap->siglen = 0;
		len += 16;
#ifdef PUBKEY
		if (!crypto_enable)
			break;
		ap->siglen = peer->sndauto.siglen;
		temp = ntohl(peer->sndauto.siglen);
		if (temp > 0)
			memcpy(&ap->sig, peer->sndauto.sig, temp);
		len += temp;
#endif /* PUBKEY */
		break;

	/*
	 * Send peer cookie and signature in server mode.
	 */
	case CRYPTO_PRIV | CRYPTO_RESP:
		cp = (struct cookie *)&xpkt[i + 2];
		cp->tstamp = htonl(sys_revoketime.l_ui);
		cp->key = htonl(cookie);
		cp->siglen = 0;
		len += 12;
#ifdef PUBKEY
		if (!crypto_enable)
			break;
		cp->siglen = htonl(public_key.bits / 8);
		if (private_key.bits < MIN_RSA_MODULUS_BITS ||
		    private_key.bits > MAX_RSA_MODULUS_BITS) {
			rval = -1;
		} else {
			R_SignInit(&ctx, DA_MD5);
			R_SignUpdate(&ctx, (u_char *)cp, 8);
			rval = R_SignFinal(&ctx, (u_char *)&cp->sig,
			    &temp, &private_key);
		}
		if (rval != 0) {
			cp->siglen = 0;
			msyslog(LOG_ERR,
			    "crypto_xmit: cookie signature fails %x",
			    rval);
			break;
		}
		len += temp;
#endif /* PUBKEY */
		break;

#ifdef PUBKEY
	/*
	 * Send Diffie-Hellman public value, timestamp and signature.
	 */
	case CRYPTO_DH:
	case CRYPTO_DH | CRYPTO_RESP:
		vp = (struct value *)&xpkt[i + 2];
		vp->tstamp = htonl(sys_revoketime.l_ui);
		vp->vallen = 0;
		len += 8;
		temp = dh_params.primeLen;
		if (dh_sign == NULL)
			break;
		vp->vallen = htonl(temp);
		memcpy((u_char *)&vp->val, dh_public, temp);
		len += temp;
		j = i + 4 + temp / 4;
		temp = public_key.bits / 8;
		xpkt[j++] = htonl(temp);
		memcpy((u_char *)&xpkt[j], dh_sign, temp);
		len += temp + 4;
		break;

	/*
	 * Send host name, timestamp and signature.
	 */
	case CRYPTO_NAME | CRYPTO_RESP:
		vp = (struct value *)&xpkt[i + 2];
		vp->tstamp = htonl(sys_revoketime.l_ui);
		vp->vallen = host.vallen;
		len += 8;
		temp = ntohl(host.vallen);
		if (temp == 0)
			break;
		memcpy((u_char *)&vp->val, host.val, temp);
		len += temp;
		j = i + 4 + temp / 4;
		temp = public_key.bits / 8;
		xpkt[j++] = htonl(temp);
		memcpy((u_char *)&xpkt[j], host.sig, temp);
		len += temp + 4;
		break;

	/*
	 * Send public key. We send the public key only if it exists and
	 * is valid. This is used primarily for testing.
	 */
	case CRYPTO_PUBL | CRYPTO_RESP:
		xpkt[i + 2] = 0;
		len += 4;
		if (public_key.bits < MIN_RSA_MODULUS_BITS ||
		    public_key.bits > MAX_RSA_MODULUS_BITS)
			break;
		temp = sizeof(R_RSA_PUBLIC_KEY) - sizeof(u_int);
		xpkt[i + 2] = htonl(temp + 4);
		xpkt[i + 3] = htonl(public_key.bits);
		memcpy((u_char *)&xpkt[i + 4],
		    (u_char *)&public_key.modulus, temp);
		len += temp + 4;
		break;
#endif /* PUBKEY */

	/*
	 * Default - Fall through for requests; for unknown responses,
	 * flag as error.
	 */
	default:
		if (opcode & CRYPTO_RESP)
			opcode |= CRYPTO_ERROR;
		break;
	}

	/*
	 * Round up the field length to a multiple of 8 bytes and save
	 * the request code and length.
	 */
	len = ((len + 7) / 8) * 8;
	if (len >= 4) {
		xpkt[i] = htonl((u_int32)((opcode << 16) | len));
#ifdef DEBUG
		if (debug)
			printf(
			    "crypto_xmit: ext offset %d len %d code %x assoc ID %d\n",
			    start, len, code, associd);
#endif
	}
	return (len);
}


#ifdef PUBKEY
/*
 * crypto_agree - compute public and private Diffie-Hellman values from
 * given prime and generator, then sign with private key.
 */
void
crypto_agree(void)
{
	u_int len, temp;
	R_RANDOM_STRUCT randomstr;
	R_SIGNATURE_CTX ctx;	/* signature context */
	int rval;
	int i;

	/*
	 * Sign host name and timestamp.
	 */
	host.tstamp = htonl(sys_revoketime.l_ui);
	host.vallen = htonl(sys_hostnamelen);
	host.val = sys_hostname;
	host.siglen = dh_params.primeLen;
	if (host.sig == NULL)
		host.sig = emalloc(dh_params.primeLen);
	R_SignInit(&ctx, DA_MD5);
	R_SignUpdate(&ctx, (u_char *)&host, 8);
	R_SignUpdate(&ctx, host.val, sys_hostnamelen);
	rval = R_SignFinal(&ctx, host.sig, &temp, &private_key);
	if (rval != 0) {
		msyslog(LOG_ERR,
		    "crypto_agree: host signature fails %x", rval);
		return;
	}

	/*
	 * Compute Diffie-Hellman public value. Note that the length of
	 * the private value is set arbitrarily to half the prime
	 * length.
	 */
	if (dh_params.primeLen == 0) {
		msyslog(LOG_ERR,
		    "unavailable d-h parameters");
		return;
	}
	R_RandomInit(&randomstr);
	R_GetRandomBytesNeeded(&len, &randomstr);
	for (i = 0; i < len; i++) {
		temp = random();
		R_RandomUpdate(&randomstr, (u_char *)&temp, 1);
	}
	if (dh_private == NULL)
		dh_private = (u_char *)emalloc(dh_keyLen);
	if (dh_public == NULL)
		dh_public = (u_char *)emalloc(dh_params.primeLen);
	rval = R_SetupDHAgreement(dh_public, dh_private, dh_keyLen,
	    &dh_params, &randomstr);
	if (rval != 0) {
		msyslog(LOG_ERR, "invalid d-h parameters");
		return;
	}

	/*
	 * Sigfie-Hellman public value and save for later.
	 */
	if (dh_sign == NULL)
		dh_sign = emalloc(dh_params.primeLen);
	R_SignInit(&ctx, DA_MD5);
	R_SignUpdate(&ctx, (char *)&host.tstamp, 4);
	temp = htonl(dh_params.primeLen);
	R_SignUpdate(&ctx, (char *)&temp, 4);
	R_SignUpdate(&ctx, dh_public, dh_params.primeLen);
	rval = R_SignFinal(&ctx, dh_sign, &temp, &private_key);
	if (rval != 0) {
		msyslog(LOG_ERR,
		    "crypto_agree: d-h signature fails %x", rval);
		return;
	}
#ifdef DEBUG
	if (debug)
		printf(
		    "cypto_agree: host %s d-h prime %d gen %d\n",
		    host.val, dh_params.primeLen,
		    dh_params.generatorLen);
#endif
}


/*
 * crypto_read - read RSA key, decode and check for errors
 */
static int
crypto_read(
	u_char *cp,		/* file name */
	u_char *key,		/* key pointer */
	u_int keylen		/* key length */
	)
{
	FILE *str;
	u_char buf[MAX_KEYLEN];
	u_char encoded_key[MAX_KEYLEN];
	char filename[MAXFILENAME];
	u_int modulus;
	u_int buflen;
	char *rptr;
	int rval;

	/*
	 * Open the key file and discard comment lines.
	 */
	if (strlen(cp) == 0 || strlen(cp) >= MAXFILENAME - 1) {
		msyslog(LOG_ERR, "invalid key file name length %d",
		    strlen(filename));
		return (0);
	} else if (*cp == '/') {
		strcpy(filename, cp);
	} else {
		snprintf(filename, MAXFILENAME - 1, "%s%s", keysdir,
		    cp);
	}
	str = fopen(filename, "r");
	if (str == NULL) {
		msyslog(LOG_ERR, "key file %s not found", filename);
		return (0);
	}
	while ((rptr = fgets(buf, MAX_KEYLEN - 1, str)) != NULL) {
		buflen = strlen(buf);
		if (buflen < 1)
			continue;
		if (*buf == '#' || *buf == '\r' || *buf == '\0')
			continue;
		break;
	}

	/*
	 * We are rather paranoid here, since an intruder can cause a
	 * coredump by infiltrating a naughty key. The line must contain
	 * a single integer followed by a PEM encoded, null-terminated
	 * string.
	 */
	if (rptr == NULL) {
		msyslog(LOG_ERR, "invalid key file %s", filename);
		return (0);
	}
	/* XXX sizeof(encoded_key) == 1024 */
	if (sscanf(buf, "%d %1023s", &modulus, encoded_key) != 2) {
		msyslog(LOG_ERR, "invalid key format %s", filename);
		return (0);
	}

	/*
	 * Initialize the key with the decoded PEM string, but leave
	 * room for the modulus length in the key structure.
	 */
	rval = R_DecodePEMBlock(&key[sizeof(u_int)], &buflen,
	    encoded_key, strlen(encoded_key));
	if (rval != 0) {
		msyslog(LOG_ERR, "invalid key %s %x", filename, rval);
		return (0);
	}

	/*
	 * Make sure the structure has the required length.
	 */
	buflen += sizeof(u_int);
	if (buflen != keylen) {
		msyslog(LOG_ERR, "invalid key length %s %d", filename,
		    buflen);
		return (0);
	}

	/*
	 * Make sure the modulus length is within limits.
	 */
	if (modulus < MIN_RSA_MODULUS_BITS || modulus >
	    MAX_RSA_MODULUS_BITS) {
		msyslog(LOG_ERR, "invalid key modulus %s %d", filename,
		    modulus);
		return (0);
	}
	((u_int *)key)[0] = modulus;
#ifdef DEBUG
	if (debug)
		printf(
		    "crypto_read: RSA key file %s length %d modulus %d\n",
		    cp, keylen, modulus);
#endif
	return (1);
}


/*
 * crypto_public - read and install the public key from the public key
 * file. The name of the file is in the form "ntpkey_host", where host
 * is the DNS canonical name of the host. If the file is not found or
 * has errors, we just keep going and expect the host to fetch the
 * public key from the peer via the extension field.
 */
int
crypto_public(
	struct peer *peer,	/* peer structure pointer */
	u_char *cp		/* canonical host name */
	)
{
	R_RSA_PUBLIC_KEY keybuf;
	u_int keylen = sizeof(R_RSA_PUBLIC_KEY);
	char filename[MAXFILENAME];

	snprintf(filename, MAXFILENAME - 1, "ntpkey_%s", cp);
	if (!crypto_read(filename, (u_char *)&keybuf, keylen))
		return (0);
	if (peer->keystr != NULL)
		free(peer->keystr);
	peer->keystr = emalloc(strlen(filename) + 1);
	strcpy(peer->keystr, filename);
	if (peer->pubkey == NULL)
		peer->pubkey = emalloc(keylen);
	memcpy(peer->pubkey, (char *)&keybuf, keylen);
	return (1);
}


/*
 * crypto_line - read, decode and install Diffie-Hellman prime and
 * generator. Be very parannoyed here if every little thing is not
 * exactly right.
 */
static void
crypto_line(
	FILE *str,		/* file handle */
	u_char **key,		/* decoded string pointer */
	u_int *len		/* length */
	)
{
	u_char buf[MAX_KEYLEN];
	u_char encoded_key[MAX_KEYLEN];
	u_int temp, temp1, temp2;
	char *rptr;

	/*
	 * Read key and length. Bail out if the length word doesn't
	 * match the decoded string length and in other cases.
	 */
	*key = NULL;
	*len = 0;
	while ((rptr = fgets(buf, MAX_KEYLEN - 1, str)) != NULL) {
		if (strlen(buf) < 1)
			continue;
		if (*buf == '#' || *buf == '\r' || *buf == '\0')
			continue;
		break;
	}
	/* XXX sizeof(encoded_key) == 1024 */
	if (sscanf(buf, "%d %1023s", &temp, encoded_key) != 2)
		return;
	if (temp > MAX_DH_LEN)
		return;
	temp2 = DECODED_CONTENT_LEN(strlen(encoded_key));
	if (R_DecodePEMBlock(buf, &temp1, encoded_key,
	    strlen(encoded_key)))
		return;
	if (temp != temp1 || temp > temp2)
		return;
	*key = (u_char *)emalloc(temp2);
	memcpy(*key, buf, temp2);
	*len = temp;
	return;
}


/*
 * crypto_setup - read RSA private key, RSA public key and Diffie-
 * Hellman parameter files and initialize cryptographic data.
 */
void
crypto_setup(void)
{
	FILE *str;
	char hostname[MAXFILENAME];
	char filename[MAXFILENAME];

	/*
	 * Load RSA private key from file.
	 */
	if (private_key_file == NULL)
		private_key_file = "ntpkey";
	crypto_read(private_key_file, (u_char *)&private_key,
	    sizeof(R_RSA_PRIVATE_KEY));

	/*
	 * Load RSA public key from file, default "ntpkey_host", where
	 * host is the DNS name of this machine.
	 */
	if (public_key_file == NULL) {
		gethostname(hostname, MAXFILENAME);
		snprintf(filename, MAXFILENAME - 1, "ntpkey_%s",
			 hostname);
		public_key_file = emalloc(strlen(filename) + 1);
		strcpy(public_key_file, filename);
	}
	crypto_read(public_key_file, (u_char *)&public_key,
	    sizeof(R_RSA_PUBLIC_KEY));

	/*
	 * Load Diffie-Hellman key agreement parameters from file.
	 */
	if (dh_params_file == NULL)
		dh_params_file = "ntpkey_dh";
	if (*dh_params_file == '/')
		strcpy(filename, dh_params_file);
	else
		snprintf(filename, MAXFILENAME - 1, "%s%s", keysdir,
			 dh_params_file);

	str = fopen(filename, "r");
	if (str == NULL) {
		msyslog(LOG_ERR, "key file %s not found",
		    filename);
		return;
	}

	/*
	 * Read prime and generator. Note that the private value length
	 * is set arbitrarily at half the prime length.
	 */
	crypto_line(str, &dh_params.prime, &dh_params.primeLen);
	crypto_line(str, &dh_params.generator, &dh_params.generatorLen);
	if (dh_params.primeLen == 0|| dh_params.generatorLen == 0) {
		msyslog(LOG_ERR,
		    "invalid Diffie-Hellman file format or value");
		return;
	}
	dh_keyLen = dh_params.primeLen / 2;
}


/*
 * crypto_config - configure crypto data.
 */
void
crypto_config(
	int item,		/* configuration item */
	char *cp		/* file name */
	)
{
	switch (item) {

	/*
	 * Initialize flags
	 */
	case CRYPTO_CONF_FLAGS:
		crypto_flags = atoi(cp);
		break;
	/*
	 * Override the default Diffie-Hellman parameter file name.
	 */
	case CRYPTO_CONF_DH:
		dh_params_file = emalloc(strlen(cp) + 1);
		strcpy(dh_params_file, cp);
		break;

	/*
	 * Override the default RSA private key file name.
	 */
	case CRYPTO_CONF_PRIV:
		private_key_file = emalloc(strlen(cp) + 1);
		strcpy(private_key_file, cp);
		break;

	/*
	 * Override the default RSA public key file name.
	 */
	case CRYPTO_CONF_PUBL:
		public_key_file = emalloc(strlen(cp) + 1);
		strcpy(public_key_file, cp);
		break;

	/*
	 * Override the default crypto keys directory.
	 */
	case CRYPTO_CONF_KEYS:
		keysdir = emalloc(strlen(cp) + 1);
		strcpy(keysdir, cp);
		break;
	}
}


/*
 * crypto_init () - initialize things.
 */
void
crypto_init(void)
{
	/*
	 * Zeroize sensitive crypto data.
	 */
	memset((char *)&private_key, 0, sizeof(private_key));
	memset((char *)&public_key, 0, sizeof(public_key));
	memset((char *)&dh_params, 0, sizeof(dh_params));
	memset((char *)&host, 0, sizeof(host));
}
#endif /* PUBKEY */
#endif /* AUTOKEY */
