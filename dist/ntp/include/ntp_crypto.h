/*	$NetBSD: ntp_crypto.h,v 1.1.1.1 2000/04/22 14:52:42 simonb Exp $	*/

/*
 * ntp_crypto.h - definitions for cryptographic operations
 */
#ifdef AUTOKEY
#include "global.h"
#include "md5.h"
#ifdef PUBKEY
#include "rsaref.h"
#include "rsa.h"
#endif /* PUBKEY */

/*
 * Extension field definitions
 */
#define CRYPTO_NULL	0	/* no operation */
#define CRYPTO_PUBL	1	/* public key */
#define CRYPTO_ASSOC	2	/* association ID */
#define CRYPTO_AUTO	3	/* autokey values */
#define CRYPTO_PRIV	4	/* cookie value (client/server) */
#define CRYPTO_DH	5	/* Diffie-Hellman value (symmetric) */
#define CRYPTO_NAME	6	/* host name */
#define CRYPTO_RESP	0x80	/* response */
#define CRYPTO_ERROR	0x40	/* error */

/*
 * Cryptoflags
 */
#define CRYPTO_FLAG_NONE  0x00	/* nothing happening */
#define CRYPTO_FLAG_PUBL  0x01	/* read peer public key from file */

#ifdef PUBKEY

#define MAX_DH_LEN (DH_PRIME_LEN(1024)) /* max agreed key length */

/*
 * Configuration codes
 */
#define CRYPTO_CONF_NONE  0	/* nothing doing */
#define CRYPTO_CONF_FLAGS 1	/* initialize flags */
#define CRYPTO_CONF_PRIV  2	/* load private key from file */
#define CRYPTO_CONF_PUBL  3	/* load public key from file */
#define CRYPTO_CONF_DH    4	/* load Diffie_Hellman pars from file */
#define CRYPTO_CONF_KEYS  5 	/* set keys directory path */
#endif /* PUBKEY */

/*
 * Function prototypes
 */
extern  void    crypto_recv     P((struct peer *, struct recvbuf *));
extern  int     crypto_xmit     P((u_int32 *, int, u_int, keyid_t,
				    int));
extern	keyid_t	session_key	P((struct sockaddr_in *, struct
				    sockaddr_in *, keyid_t, keyid_t,
				    u_long));
extern	void	make_keylist	P((struct peer *));
extern	void	key_expire	P((struct peer *));
extern	void	crypto_agree	P((void));
#ifdef PUBKEY
extern	void	crypto_init	P((void));
extern	void	crypto_config	P((int, char *));
extern	void	crypto_setup	P((void));
extern	int	crypto_public	P((struct peer *, u_char *));

/*
 * Cryptographic values
 */
extern	int			crypto_enable;
extern	int			crypto_flags;
extern	char *			private_key_file;
extern	char *			public_key_file;
extern	char *			dh_params_file;
#endif /* PUBKEY */
#endif /* AUTOKEY */
