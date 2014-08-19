/*	$NetBSD: ssh-agent.c,v 1.9.8.2 2014/08/19 23:45:25 tls Exp $	*/
/* $OpenBSD: ssh-agent.c,v 1.177 2013/07/20 01:50:20 djm Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * The authentication agent program.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"
__RCSID("$NetBSD: ssh-agent.c,v 1.9.8.2 2014/08/19 23:45:25 tls Exp $");
#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/param.h>

#include <openssl/evp.h>
#include <openssl/md5.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "xmalloc.h"
#include "ssh.h"
#include "rsa.h"
#include "buffer.h"
#include "key.h"
#include "authfd.h"
#include "compat.h"
#include "log.h"
#include "misc.h"
#include "getpeereid.h"

#ifdef ENABLE_PKCS11
#include "ssh-pkcs11.h"
#endif

typedef enum {
	AUTH_UNUSED,
	AUTH_SOCKET,
	AUTH_CONNECTION
} sock_type;

typedef struct {
	int fd;
	sock_type type;
	Buffer input;
	Buffer output;
	Buffer request;
} SocketEntry;

u_int sockets_alloc = 0;
SocketEntry *sockets = NULL;

typedef struct identity {
	TAILQ_ENTRY(identity) next;
	Key *key;
	char *comment;
	char *provider;
	time_t death;
	u_int confirm;
} Identity;

typedef struct {
	int nentries;
	TAILQ_HEAD(idqueue, identity) idlist;
} Idtab;

/* private key table, one per protocol version */
Idtab idtable[3];

int max_fd = 0;

/* pid of shell == parent of agent */
pid_t parent_pid = -1;
time_t parent_alive_interval = 0;

/* pathname and directory for AUTH_SOCKET */
char socket_name[MAXPATHLEN];
char socket_dir[MAXPATHLEN];

/* locking */
int locked = 0;
char *lock_passwd = NULL;

extern char *__progname;

/* Default lifetime in seconds (0 == forever) */
static long lifetime = 0;

static void
close_socket(SocketEntry *e)
{
	close(e->fd);
	e->fd = -1;
	e->type = AUTH_UNUSED;
	buffer_free(&e->input);
	buffer_free(&e->output);
	buffer_free(&e->request);
}

static void
idtab_init(void)
{
	int i;

	for (i = 0; i <=2; i++) {
		TAILQ_INIT(&idtable[i].idlist);
		idtable[i].nentries = 0;
	}
}

/* return private key table for requested protocol version */
static Idtab *
idtab_lookup(int version)
{
	if (version < 1 || version > 2)
		fatal("internal error, bad protocol version %d", version);
	return &idtable[version];
}

static void
free_identity(Identity *id)
{
	key_free(id->key);
	free(id->provider);
	free(id->comment);
	free(id);
}

/* return matching private key for given public key */
static Identity *
lookup_identity(Key *key, int version)
{
	Identity *id;

	Idtab *tab = idtab_lookup(version);
	TAILQ_FOREACH(id, &tab->idlist, next) {
		if (key_equal(key, id->key))
			return (id);
	}
	return (NULL);
}

/* Check confirmation of keysign request */
static int
confirm_key(Identity *id)
{
	char *p;
	int ret = -1;

	p = key_fingerprint(id->key, SSH_FP_MD5, SSH_FP_HEX);
	if (ask_permission("Allow use of key %s?\nKey fingerprint %s.",
	    id->comment, p))
		ret = 0;
	free(p);

	return (ret);
}

/* send list of supported public keys to 'client' */
static void
process_request_identities(SocketEntry *e, int version)
{
	Idtab *tab = idtab_lookup(version);
	Identity *id;
	Buffer msg;

	buffer_init(&msg);
	buffer_put_char(&msg, (version == 1) ?
	    SSH_AGENT_RSA_IDENTITIES_ANSWER : SSH2_AGENT_IDENTITIES_ANSWER);
	buffer_put_int(&msg, tab->nentries);
	TAILQ_FOREACH(id, &tab->idlist, next) {
		if (id->key->type == KEY_RSA1) {
			buffer_put_int(&msg, BN_num_bits(id->key->rsa->n));
			buffer_put_bignum(&msg, id->key->rsa->e);
			buffer_put_bignum(&msg, id->key->rsa->n);
		} else {
			u_char *blob;
			u_int blen;
			key_to_blob(id->key, &blob, &blen);
			buffer_put_string(&msg, blob, blen);
			free(blob);
		}
		buffer_put_cstring(&msg, id->comment);
	}
	buffer_put_int(&e->output, buffer_len(&msg));
	buffer_append(&e->output, buffer_ptr(&msg), buffer_len(&msg));
	buffer_free(&msg);
}

/* ssh1 only */
static void
process_authentication_challenge1(SocketEntry *e)
{
	u_char buf[32], mdbuf[16], session_id[16];
	u_int response_type;
	BIGNUM *challenge;
	Identity *id;
	int i, len;
	Buffer msg;
	MD5_CTX md;
	Key *key;

	buffer_init(&msg);
	key = key_new(KEY_RSA1);
	if ((challenge = BN_new()) == NULL)
		fatal("process_authentication_challenge1: BN_new failed");

	(void) buffer_get_int(&e->request);			/* ignored */
	buffer_get_bignum(&e->request, key->rsa->e);
	buffer_get_bignum(&e->request, key->rsa->n);
	buffer_get_bignum(&e->request, challenge);

	/* Only protocol 1.1 is supported */
	if (buffer_len(&e->request) == 0)
		goto failure;
	buffer_get(&e->request, session_id, 16);
	response_type = buffer_get_int(&e->request);
	if (response_type != 1)
		goto failure;

	id = lookup_identity(key, 1);
	if (id != NULL && (!id->confirm || confirm_key(id) == 0)) {
		Key *private = id->key;
		/* Decrypt the challenge using the private key. */
		if (rsa_private_decrypt(challenge, challenge, private->rsa) <= 0)
			goto failure;

		/* The response is MD5 of decrypted challenge plus session id. */
		len = BN_num_bytes(challenge);
		if (len <= 0 || len > 32) {
			logit("process_authentication_challenge: bad challenge length %d", len);
			goto failure;
		}
		memset(buf, 0, 32);
		BN_bn2bin(challenge, buf + 32 - len);
		MD5_Init(&md);
		MD5_Update(&md, buf, 32);
		MD5_Update(&md, session_id, 16);
		MD5_Final(mdbuf, &md);

		/* Send the response. */
		buffer_put_char(&msg, SSH_AGENT_RSA_RESPONSE);
		for (i = 0; i < 16; i++)
			buffer_put_char(&msg, mdbuf[i]);
		goto send;
	}

failure:
	/* Unknown identity or protocol error.  Send failure. */
	buffer_put_char(&msg, SSH_AGENT_FAILURE);
send:
	buffer_put_int(&e->output, buffer_len(&msg));
	buffer_append(&e->output, buffer_ptr(&msg), buffer_len(&msg));
	key_free(key);
	BN_clear_free(challenge);
	buffer_free(&msg);
}

/* ssh2 only */
static void
process_sign_request2(SocketEntry *e)
{
	u_char *blob, *data, *signature = NULL;
	u_int blen, dlen, slen = 0;
	extern int datafellows;
	int odatafellows;
	int ok = -1, flags;
	Buffer msg;
	Key *key;

	odatafellows = datafellows;
	datafellows = 0;

	blob = buffer_get_string(&e->request, &blen);
	data = buffer_get_string(&e->request, &dlen);

	flags = buffer_get_int(&e->request);
	if (flags & SSH_AGENT_OLD_SIGNATURE)
		datafellows = SSH_BUG_SIGBLOB;

	key = key_from_blob(blob, blen);
	if (key != NULL) {
		Identity *id = lookup_identity(key, 2);
		if (id != NULL && (!id->confirm || confirm_key(id) == 0))
			ok = key_sign(id->key, &signature, &slen, data, dlen);
		key_free(key);
	}
	buffer_init(&msg);
	if (ok == 0) {
		buffer_put_char(&msg, SSH2_AGENT_SIGN_RESPONSE);
		buffer_put_string(&msg, signature, slen);
	} else {
		buffer_put_char(&msg, SSH_AGENT_FAILURE);
	}
	buffer_put_int(&e->output, buffer_len(&msg));
	buffer_append(&e->output, buffer_ptr(&msg),
	    buffer_len(&msg));
	buffer_free(&msg);
	free(data);
	free(blob);
	free(signature);
	datafellows = odatafellows;
}

/* shared */
static void
process_remove_identity(SocketEntry *e, int version)
{
	u_int blen, bits;
	int success = 0;
	Key *key = NULL;
	u_char *blob;

	switch (version) {
	case 1:
		key = key_new(KEY_RSA1);
		bits = buffer_get_int(&e->request);
		buffer_get_bignum(&e->request, key->rsa->e);
		buffer_get_bignum(&e->request, key->rsa->n);

		if (bits != key_size(key))
			logit("Warning: identity keysize mismatch: actual %u, announced %u",
			    key_size(key), bits);
		break;
	case 2:
		blob = buffer_get_string(&e->request, &blen);
		key = key_from_blob(blob, blen);
		free(blob);
		break;
	}
	if (key != NULL) {
		Identity *id = lookup_identity(key, version);
		if (id != NULL) {
			/*
			 * We have this key.  Free the old key.  Since we
			 * don't want to leave empty slots in the middle of
			 * the array, we actually free the key there and move
			 * all the entries between the empty slot and the end
			 * of the array.
			 */
			Idtab *tab = idtab_lookup(version);
			if (tab->nentries < 1)
				fatal("process_remove_identity: "
				    "internal error: tab->nentries %d",
				    tab->nentries);
			TAILQ_REMOVE(&tab->idlist, id, next);
			free_identity(id);
			tab->nentries--;
			success = 1;
		}
		key_free(key);
	}
	buffer_put_int(&e->output, 1);
	buffer_put_char(&e->output,
	    success ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
}

static void
process_remove_all_identities(SocketEntry *e, int version)
{
	Idtab *tab = idtab_lookup(version);
	Identity *id;

	/* Loop over all identities and clear the keys. */
	while ((id = TAILQ_FIRST(&tab->idlist)) != NULL) {
		TAILQ_REMOVE(&tab->idlist, id, next);
		free_identity(id);
	}

	/* Mark that there are no identities. */
	tab->nentries = 0;

	/* Send success. */
	buffer_put_int(&e->output, 1);
	buffer_put_char(&e->output, SSH_AGENT_SUCCESS);
}

/* removes expired keys and returns number of seconds until the next expiry */
static time_t
reaper(void)
{
	time_t deadline = 0, now = monotime();
	Identity *id, *nxt;
	int version;
	Idtab *tab;

	for (version = 1; version < 3; version++) {
		tab = idtab_lookup(version);
		for (id = TAILQ_FIRST(&tab->idlist); id; id = nxt) {
			nxt = TAILQ_NEXT(id, next);
			if (id->death == 0)
				continue;
			if (now >= id->death) {
				debug("expiring key '%s'", id->comment);
				TAILQ_REMOVE(&tab->idlist, id, next);
				free_identity(id);
				tab->nentries--;
			} else
				deadline = (deadline == 0) ? id->death :
				    MIN(deadline, id->death);
		}
	}
	if (deadline == 0 || deadline <= now)
		return 0;
	else
		return (deadline - now);
}

static void
process_add_identity(SocketEntry *e, int version)
{
	Idtab *tab = idtab_lookup(version);
	Identity *id;
	int type, success = 0, confirm = 0;
	char *type_name, *comment, *curve;
	time_t death = 0;
	Key *k = NULL;
	BIGNUM *exponent;
	EC_POINT *q;
	u_char *cert;
	u_int len;

	switch (version) {
	case 1:
		k = key_new_private(KEY_RSA1);
		(void) buffer_get_int(&e->request);		/* ignored */
		buffer_get_bignum(&e->request, k->rsa->n);
		buffer_get_bignum(&e->request, k->rsa->e);
		buffer_get_bignum(&e->request, k->rsa->d);
		buffer_get_bignum(&e->request, k->rsa->iqmp);

		/* SSH and SSL have p and q swapped */
		buffer_get_bignum(&e->request, k->rsa->q);	/* p */
		buffer_get_bignum(&e->request, k->rsa->p);	/* q */

		/* Generate additional parameters */
		rsa_generate_additional_parameters(k->rsa);
		break;
	case 2:
		type_name = buffer_get_string(&e->request, NULL);
		type = key_type_from_name(type_name);
		switch (type) {
		case KEY_DSA:
			k = key_new_private(type);
			buffer_get_bignum2(&e->request, k->dsa->p);
			buffer_get_bignum2(&e->request, k->dsa->q);
			buffer_get_bignum2(&e->request, k->dsa->g);
			buffer_get_bignum2(&e->request, k->dsa->pub_key);
			buffer_get_bignum2(&e->request, k->dsa->priv_key);
			break;
		case KEY_DSA_CERT_V00:
		case KEY_DSA_CERT:
			cert = buffer_get_string(&e->request, &len);
			if ((k = key_from_blob(cert, len)) == NULL)
				fatal("Certificate parse failed");
			free(cert);
			key_add_private(k);
			buffer_get_bignum2(&e->request, k->dsa->priv_key);
			break;
		case KEY_ECDSA:
			k = key_new_private(type);
			k->ecdsa_nid = key_ecdsa_nid_from_name(type_name);
			curve = buffer_get_string(&e->request, NULL);
			if (k->ecdsa_nid != key_curve_name_to_nid(curve))
				fatal("%s: curve names mismatch", __func__);
			free(curve);
			k->ecdsa = EC_KEY_new_by_curve_name(k->ecdsa_nid);
			if (k->ecdsa == NULL)
				fatal("%s: EC_KEY_new_by_curve_name failed",
				    __func__);
			q = EC_POINT_new(EC_KEY_get0_group(k->ecdsa));
			if (q == NULL)
				fatal("%s: BN_new failed", __func__);
			if ((exponent = BN_new()) == NULL)
				fatal("%s: BN_new failed", __func__);
			buffer_get_ecpoint(&e->request,
				EC_KEY_get0_group(k->ecdsa), q);
			buffer_get_bignum2(&e->request, exponent);
			if (EC_KEY_set_public_key(k->ecdsa, q) != 1)
				fatal("%s: EC_KEY_set_public_key failed",
				    __func__);
			if (EC_KEY_set_private_key(k->ecdsa, exponent) != 1)
				fatal("%s: EC_KEY_set_private_key failed",
				    __func__);
			if (key_ec_validate_public(EC_KEY_get0_group(k->ecdsa),
			    EC_KEY_get0_public_key(k->ecdsa)) != 0)
				fatal("%s: bad ECDSA public key", __func__);
			if (key_ec_validate_private(k->ecdsa) != 0)
				fatal("%s: bad ECDSA private key", __func__);
			BN_clear_free(exponent);
			EC_POINT_free(q);
			break;
		case KEY_ECDSA_CERT:
			cert = buffer_get_string(&e->request, &len);
			if ((k = key_from_blob(cert, len)) == NULL)
				fatal("Certificate parse failed");
			free(cert);
			key_add_private(k);
			if ((exponent = BN_new()) == NULL)
				fatal("%s: BN_new failed", __func__);
			buffer_get_bignum2(&e->request, exponent);
			if (EC_KEY_set_private_key(k->ecdsa, exponent) != 1)
				fatal("%s: EC_KEY_set_private_key failed",
				    __func__);
			if (key_ec_validate_public(EC_KEY_get0_group(k->ecdsa),
			    EC_KEY_get0_public_key(k->ecdsa)) != 0 ||
			    key_ec_validate_private(k->ecdsa) != 0)
				fatal("%s: bad ECDSA key", __func__);
			BN_clear_free(exponent);
			break;
		case KEY_RSA:
			k = key_new_private(type);
			buffer_get_bignum2(&e->request, k->rsa->n);
			buffer_get_bignum2(&e->request, k->rsa->e);
			buffer_get_bignum2(&e->request, k->rsa->d);
			buffer_get_bignum2(&e->request, k->rsa->iqmp);
			buffer_get_bignum2(&e->request, k->rsa->p);
			buffer_get_bignum2(&e->request, k->rsa->q);

			/* Generate additional parameters */
			rsa_generate_additional_parameters(k->rsa);
			break;
		case KEY_RSA_CERT_V00:
		case KEY_RSA_CERT:
			cert = buffer_get_string(&e->request, &len);
			if ((k = key_from_blob(cert, len)) == NULL)
				fatal("Certificate parse failed");
			free(cert);
			key_add_private(k);
			buffer_get_bignum2(&e->request, k->rsa->d);
			buffer_get_bignum2(&e->request, k->rsa->iqmp);
			buffer_get_bignum2(&e->request, k->rsa->p);
			buffer_get_bignum2(&e->request, k->rsa->q);
			break;
		default:
			free(type_name);
			buffer_clear(&e->request);
			goto send;
		}
		free(type_name);
		break;
	}
	/* enable blinding */
	switch (k->type) {
	case KEY_RSA:
	case KEY_RSA_CERT_V00:
	case KEY_RSA_CERT:
	case KEY_RSA1:
		if (RSA_blinding_on(k->rsa, NULL) != 1) {
			error("process_add_identity: RSA_blinding_on failed");
			key_free(k);
			goto send;
		}
		break;
	}
	comment = buffer_get_string(&e->request, NULL);
	if (k == NULL) {
		free(comment);
		goto send;
	}
	while (buffer_len(&e->request)) {
		switch ((type = buffer_get_char(&e->request))) {
		case SSH_AGENT_CONSTRAIN_LIFETIME:
			death = monotime() + buffer_get_int(&e->request);
			break;
		case SSH_AGENT_CONSTRAIN_CONFIRM:
			confirm = 1;
			break;
		default:
			error("process_add_identity: "
			    "Unknown constraint type %d", type);
			free(comment);
			key_free(k);
			goto send;
		}
	}
	success = 1;
	if (lifetime && !death)
		death = monotime() + lifetime;
	if ((id = lookup_identity(k, version)) == NULL) {
		id = xcalloc(1, sizeof(Identity));
		id->key = k;
		TAILQ_INSERT_TAIL(&tab->idlist, id, next);
		/* Increment the number of identities. */
		tab->nentries++;
	} else {
		key_free(k);
		free(id->comment);
	}
	id->comment = comment;
	id->death = death;
	id->confirm = confirm;
send:
	buffer_put_int(&e->output, 1);
	buffer_put_char(&e->output,
	    success ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
}

/* XXX todo: encrypt sensitive data with passphrase */
static void
process_lock_agent(SocketEntry *e, int lock)
{
	int success = 0;
	char *passwd;

	passwd = buffer_get_string(&e->request, NULL);
	if (locked && !lock && strcmp(passwd, lock_passwd) == 0) {
		locked = 0;
		memset(lock_passwd, 0, strlen(lock_passwd));
		free(lock_passwd);
		lock_passwd = NULL;
		success = 1;
	} else if (!locked && lock) {
		locked = 1;
		lock_passwd = xstrdup(passwd);
		success = 1;
	}
	memset(passwd, 0, strlen(passwd));
	free(passwd);

	buffer_put_int(&e->output, 1);
	buffer_put_char(&e->output,
	    success ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
}

static void
no_identities(SocketEntry *e, u_int type)
{
	Buffer msg;

	buffer_init(&msg);
	buffer_put_char(&msg,
	    (type == SSH_AGENTC_REQUEST_RSA_IDENTITIES) ?
	    SSH_AGENT_RSA_IDENTITIES_ANSWER : SSH2_AGENT_IDENTITIES_ANSWER);
	buffer_put_int(&msg, 0);
	buffer_put_int(&e->output, buffer_len(&msg));
	buffer_append(&e->output, buffer_ptr(&msg), buffer_len(&msg));
	buffer_free(&msg);
}

#ifdef ENABLE_PKCS11
static void
process_add_smartcard_key(SocketEntry *e)
{
	char *provider = NULL, *pin;
	int i, type, version, count = 0, success = 0, confirm = 0;
	time_t death = 0;
	Key **keys = NULL, *k;
	Identity *id;
	Idtab *tab;

	provider = buffer_get_string(&e->request, NULL);
	pin = buffer_get_string(&e->request, NULL);

	while (buffer_len(&e->request)) {
		switch ((type = buffer_get_char(&e->request))) {
		case SSH_AGENT_CONSTRAIN_LIFETIME:
			death = monotime() + buffer_get_int(&e->request);
			break;
		case SSH_AGENT_CONSTRAIN_CONFIRM:
			confirm = 1;
			break;
		default:
			error("process_add_smartcard_key: "
			    "Unknown constraint type %d", type);
			goto send;
		}
	}
	if (lifetime && !death)
		death = monotime() + lifetime;

	count = pkcs11_add_provider(provider, pin, &keys);
	for (i = 0; i < count; i++) {
		k = keys[i];
		version = k->type == KEY_RSA1 ? 1 : 2;
		tab = idtab_lookup(version);
		if (lookup_identity(k, version) == NULL) {
			id = xcalloc(1, sizeof(Identity));
			id->key = k;
			id->provider = xstrdup(provider);
			id->comment = xstrdup(provider); /* XXX */
			id->death = death;
			id->confirm = confirm;
			TAILQ_INSERT_TAIL(&tab->idlist, id, next);
			tab->nentries++;
			success = 1;
		} else {
			key_free(k);
		}
		keys[i] = NULL;
	}
send:
	free(pin);
	free(provider);
	free(keys);
	buffer_put_int(&e->output, 1);
	buffer_put_char(&e->output,
	    success ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
}

static void
process_remove_smartcard_key(SocketEntry *e)
{
	char *provider = NULL, *pin = NULL;
	int version, success = 0;
	Identity *id, *nxt;
	Idtab *tab;

	provider = buffer_get_string(&e->request, NULL);
	pin = buffer_get_string(&e->request, NULL);
	free(pin);

	for (version = 1; version < 3; version++) {
		tab = idtab_lookup(version);
		for (id = TAILQ_FIRST(&tab->idlist); id; id = nxt) {
			nxt = TAILQ_NEXT(id, next);
			if (!strcmp(provider, id->provider)) {
				TAILQ_REMOVE(&tab->idlist, id, next);
				free_identity(id);
				tab->nentries--;
			}
		}
	}
	if (pkcs11_del_provider(provider) == 0)
		success = 1;
	else
		error("process_remove_smartcard_key:"
		    " pkcs11_del_provider failed");
	free(provider);
	buffer_put_int(&e->output, 1);
	buffer_put_char(&e->output,
	    success ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
}
#endif /* ENABLE_PKCS11 */

/* dispatch incoming messages */

static void
process_message(SocketEntry *e)
{
	u_int msg_len, type;
	u_char *cp;

	if (buffer_len(&e->input) < 5)
		return;		/* Incomplete message. */
	cp = buffer_ptr(&e->input);
	msg_len = get_u32(cp);
	if (msg_len > 256 * 1024) {
		close_socket(e);
		return;
	}
	if (buffer_len(&e->input) < msg_len + 4)
		return;

	/* move the current input to e->request */
	buffer_consume(&e->input, 4);
	buffer_clear(&e->request);
	buffer_append(&e->request, buffer_ptr(&e->input), msg_len);
	buffer_consume(&e->input, msg_len);
	type = buffer_get_char(&e->request);

	/* check wheter agent is locked */
	if (locked && type != SSH_AGENTC_UNLOCK) {
		buffer_clear(&e->request);
		switch (type) {
		case SSH_AGENTC_REQUEST_RSA_IDENTITIES:
		case SSH2_AGENTC_REQUEST_IDENTITIES:
			/* send empty lists */
			no_identities(e, type);
			break;
		default:
			/* send a fail message for all other request types */
			buffer_put_int(&e->output, 1);
			buffer_put_char(&e->output, SSH_AGENT_FAILURE);
		}
		return;
	}

	debug("type %d", type);
	switch (type) {
	case SSH_AGENTC_LOCK:
	case SSH_AGENTC_UNLOCK:
		process_lock_agent(e, type == SSH_AGENTC_LOCK);
		break;
	/* ssh1 */
	case SSH_AGENTC_RSA_CHALLENGE:
		process_authentication_challenge1(e);
		break;
	case SSH_AGENTC_REQUEST_RSA_IDENTITIES:
		process_request_identities(e, 1);
		break;
	case SSH_AGENTC_ADD_RSA_IDENTITY:
	case SSH_AGENTC_ADD_RSA_ID_CONSTRAINED:
		process_add_identity(e, 1);
		break;
	case SSH_AGENTC_REMOVE_RSA_IDENTITY:
		process_remove_identity(e, 1);
		break;
	case SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES:
		process_remove_all_identities(e, 1);
		break;
	/* ssh2 */
	case SSH2_AGENTC_SIGN_REQUEST:
		process_sign_request2(e);
		break;
	case SSH2_AGENTC_REQUEST_IDENTITIES:
		process_request_identities(e, 2);
		break;
	case SSH2_AGENTC_ADD_IDENTITY:
	case SSH2_AGENTC_ADD_ID_CONSTRAINED:
		process_add_identity(e, 2);
		break;
	case SSH2_AGENTC_REMOVE_IDENTITY:
		process_remove_identity(e, 2);
		break;
	case SSH2_AGENTC_REMOVE_ALL_IDENTITIES:
		process_remove_all_identities(e, 2);
		break;
#ifdef ENABLE_PKCS11
	case SSH_AGENTC_ADD_SMARTCARD_KEY:
	case SSH_AGENTC_ADD_SMARTCARD_KEY_CONSTRAINED:
		process_add_smartcard_key(e);
		break;
	case SSH_AGENTC_REMOVE_SMARTCARD_KEY:
		process_remove_smartcard_key(e);
		break;
#endif /* ENABLE_PKCS11 */
	default:
		/* Unknown message.  Respond with failure. */
		error("Unknown message %d", type);
		buffer_clear(&e->request);
		buffer_put_int(&e->output, 1);
		buffer_put_char(&e->output, SSH_AGENT_FAILURE);
		break;
	}
}

static void
new_socket(sock_type type, int fd)
{
	u_int i, old_alloc, new_alloc;

	set_nonblock(fd);

	if (fd > max_fd)
		max_fd = fd;

	for (i = 0; i < sockets_alloc; i++)
		if (sockets[i].type == AUTH_UNUSED) {
			sockets[i].fd = fd;
			buffer_init(&sockets[i].input);
			buffer_init(&sockets[i].output);
			buffer_init(&sockets[i].request);
			sockets[i].type = type;
			return;
		}
	old_alloc = sockets_alloc;
	new_alloc = sockets_alloc + 10;
	sockets = xrealloc(sockets, new_alloc, sizeof(sockets[0]));
	for (i = old_alloc; i < new_alloc; i++)
		sockets[i].type = AUTH_UNUSED;
	sockets_alloc = new_alloc;
	sockets[old_alloc].fd = fd;
	buffer_init(&sockets[old_alloc].input);
	buffer_init(&sockets[old_alloc].output);
	buffer_init(&sockets[old_alloc].request);
	sockets[old_alloc].type = type;
}

static int
prepare_select(fd_set **fdrp, fd_set **fdwp, int *fdl, u_int *nallocp,
    struct timeval **tvpp)
{
	u_int i, sz;
	int n = 0;
	static struct timeval tv;
	time_t deadline;

	for (i = 0; i < sockets_alloc; i++) {
		switch (sockets[i].type) {
		case AUTH_SOCKET:
		case AUTH_CONNECTION:
			n = MAX(n, sockets[i].fd);
			break;
		case AUTH_UNUSED:
			break;
		default:
			fatal("Unknown socket type %d", sockets[i].type);
			break;
		}
	}

	sz = howmany(n+1, NFDBITS) * sizeof(fd_mask);
	if (*fdrp == NULL || sz > *nallocp) {
		free(*fdrp);
		free(*fdwp);
		*fdrp = xmalloc(sz);
		*fdwp = xmalloc(sz);
		*nallocp = sz;
	}
	if (n < *fdl)
		debug("XXX shrink: %d < %d", n, *fdl);
	*fdl = n;
	memset(*fdrp, 0, sz);
	memset(*fdwp, 0, sz);

	for (i = 0; i < sockets_alloc; i++) {
		switch (sockets[i].type) {
		case AUTH_SOCKET:
		case AUTH_CONNECTION:
			FD_SET(sockets[i].fd, *fdrp);
			if (buffer_len(&sockets[i].output) > 0)
				FD_SET(sockets[i].fd, *fdwp);
			break;
		default:
			break;
		}
	}
	deadline = reaper();
	if (parent_alive_interval != 0)
		deadline = (deadline == 0) ? parent_alive_interval :
		    MIN(deadline, parent_alive_interval);
	if (deadline == 0) {
		*tvpp = NULL;
	} else {
		tv.tv_sec = deadline;
		tv.tv_usec = 0;
		*tvpp = &tv;
	}
	return (1);
}

static void
after_select(fd_set *readset, fd_set *writeset)
{
	struct sockaddr_un sunaddr;
	socklen_t slen;
	char buf[1024];
	int len, sock;
	u_int i, orig_alloc;
	uid_t euid;
	gid_t egid;

	for (i = 0, orig_alloc = sockets_alloc; i < orig_alloc; i++)
		switch (sockets[i].type) {
		case AUTH_UNUSED:
			break;
		case AUTH_SOCKET:
			if (FD_ISSET(sockets[i].fd, readset)) {
				slen = sizeof(sunaddr);
				sock = accept(sockets[i].fd,
				    (struct sockaddr *)&sunaddr, &slen);
				if (sock < 0) {
					error("accept from AUTH_SOCKET: %s",
					    strerror(errno));
					break;
				}
				if (getpeereid(sock, &euid, &egid) < 0) {
					error("getpeereid %d failed: %s",
					    sock, strerror(errno));
					close(sock);
					break;
				}
				if ((euid != 0) && (getuid() != euid)) {
					error("uid mismatch: "
					    "peer euid %u != uid %u",
					    (u_int) euid, (u_int) getuid());
					close(sock);
					break;
				}
				new_socket(AUTH_CONNECTION, sock);
			}
			break;
		case AUTH_CONNECTION:
			if (buffer_len(&sockets[i].output) > 0 &&
			    FD_ISSET(sockets[i].fd, writeset)) {
				len = write(sockets[i].fd,
				    buffer_ptr(&sockets[i].output),
				    buffer_len(&sockets[i].output));
				if (len == -1 && (errno == EAGAIN ||
				    errno == EINTR))
					continue;
				if (len <= 0) {
					close_socket(&sockets[i]);
					break;
				}
				buffer_consume(&sockets[i].output, len);
			}
			if (FD_ISSET(sockets[i].fd, readset)) {
				len = read(sockets[i].fd, buf, sizeof(buf));
				if (len == -1 && (errno == EAGAIN ||
				    errno == EINTR))
					continue;
				if (len <= 0) {
					close_socket(&sockets[i]);
					break;
				}
				buffer_append(&sockets[i].input, buf, len);
				process_message(&sockets[i]);
			}
			break;
		default:
			fatal("Unknown type %d", sockets[i].type);
		}
}

static void
cleanup_socket(void)
{
	if (socket_name[0])
		unlink(socket_name);
	if (socket_dir[0])
		rmdir(socket_dir);
}

void
cleanup_exit(int i)
{
	cleanup_socket();
	_exit(i);
}

/*ARGSUSED*/
__dead static void
cleanup_handler(int sig)
{
	cleanup_socket();
#ifdef ENABLE_PKCS11
	pkcs11_terminate();
#endif
	_exit(2);
}

static void
check_parent_exists(void)
{
	/*
	 * If our parent has exited then getppid() will return (pid_t)1,
	 * so testing for that should be safe.
	 */
	if (parent_pid != -1 && getppid() != parent_pid) {
		/* printf("Parent has died - Authentication agent exiting.\n"); */
		cleanup_socket();
		_exit(2);
	}
}

__dead static void
usage(void)
{
	fprintf(stderr, "usage: %s [options] [command [arg ...]]\n",
	    __progname);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -c          Generate C-shell commands on stdout.\n");
	fprintf(stderr, "  -s          Generate Bourne shell commands on stdout.\n");
	fprintf(stderr, "  -k          Kill the current agent.\n");
	fprintf(stderr, "  -d          Debug mode.\n");
	fprintf(stderr, "  -a socket   Bind agent socket to given name.\n");
	fprintf(stderr, "  -t life     Default identity lifetime (seconds).\n");
	exit(1);
}

static void
csh_setenv(const char *name, const char *value)
{
	printf("setenv %s %s;\n", name, value);
}

static void
csh_unsetenv(const char *name)
{
	printf("unsetenv %s;\n", name);
}

static void
sh_setenv(const char *name, const char *value)
{
	printf("%s=%s; export %s;\n", name, value, name);
}

static void
sh_unsetenv(const char *name)
{
	printf("unset %s;\n", name);
}
int
main(int ac, char **av)
{
	int c_flag = 0, d_flag = 0, k_flag = 0, s_flag = 0;
	int sock, fd, ch, result, saved_errno;
	u_int nalloc;
	char *shell, *pidstr, *agentsocket = NULL;
	fd_set *readsetp = NULL, *writesetp = NULL;
	struct sockaddr_un sunaddr;
	struct rlimit rlim;
	extern int optind;
	extern char *optarg;
	pid_t pid;
	char pidstrbuf[1 + 3 * sizeof pid];
	struct timeval *tvp = NULL;
	size_t len;
	void (*f_setenv)(const char *, const char *);
	void (*f_unsetenv)(const char *);

	/* Ensure that fds 0, 1 and 2 are open or directed to /dev/null */
	sanitise_stdfd();

	/* drop */
	setegid(getgid());
	setgid(getgid());

	OpenSSL_add_all_algorithms();

	while ((ch = getopt(ac, av, "cdksa:t:")) != -1) {
		switch (ch) {
		case 'c':
			if (s_flag)
				usage();
			c_flag++;
			break;
		case 'k':
			k_flag++;
			break;
		case 's':
			if (c_flag)
				usage();
			s_flag++;
			break;
		case 'd':
			if (d_flag)
				usage();
			d_flag++;
			break;
		case 'a':
			agentsocket = optarg;
			break;
		case 't':
			if ((lifetime = convtime(optarg)) == -1) {
				fprintf(stderr, "Invalid lifetime\n");
				usage();
			}
			break;
		default:
			usage();
		}
	}
	ac -= optind;
	av += optind;

	if (ac > 0 && (c_flag || k_flag || s_flag || d_flag))
		usage();

	if (ac == 0 && !c_flag && !s_flag) {
		shell = getenv("SHELL");
		if (shell != NULL && (len = strlen(shell)) > 2 &&
		    strncmp(shell + len - 3, "csh", 3) == 0)
			c_flag = 1;
	}
	if (c_flag) {
		f_setenv = csh_setenv;
		f_unsetenv = csh_unsetenv;
	} else {
		f_setenv = sh_setenv;
		f_unsetenv = sh_unsetenv;
	}
	if (k_flag) {
		const char *errstr = NULL;

		pidstr = getenv(SSH_AGENTPID_ENV_NAME);
		if (pidstr == NULL) {
			fprintf(stderr, "%s not set, cannot kill agent\n",
			    SSH_AGENTPID_ENV_NAME);
			exit(1);
		}
		pid = (int)strtonum(pidstr, 2, INT_MAX, &errstr);
		if (errstr) {
			fprintf(stderr,
			    "%s=\"%s\", which is not a good PID: %s\n",
			    SSH_AGENTPID_ENV_NAME, pidstr, errstr);
			exit(1);
		}
		if (kill(pid, SIGTERM) == -1) {
			perror("kill");
			exit(1);
		}
		(*f_unsetenv)(SSH_AUTHSOCKET_ENV_NAME);
		(*f_unsetenv)(SSH_AGENTPID_ENV_NAME);
		printf("echo Agent pid %ld killed;\n", (long)pid);
		exit(0);
	}
	parent_pid = getpid();

	if (agentsocket == NULL) {
		/* Create private directory for agent socket */
		mktemp_proto(socket_dir, sizeof(socket_dir));
		if (mkdtemp(socket_dir) == NULL) {
			perror("mkdtemp: private socket dir");
			exit(1);
		}
		snprintf(socket_name, sizeof socket_name, "%s/agent.%ld", socket_dir,
		    (long)parent_pid);
	} else {
		/* Try to use specified agent socket */
		socket_dir[0] = '\0';
		strlcpy(socket_name, agentsocket, sizeof socket_name);
	}

	/*
	 * Create socket early so it will exist before command gets run from
	 * the parent.
	 */
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		*socket_name = '\0'; /* Don't unlink any existing file */
		cleanup_exit(1);
	}
	memset(&sunaddr, 0, sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	strlcpy(sunaddr.sun_path, socket_name, sizeof(sunaddr.sun_path));
	if (bind(sock, (struct sockaddr *)&sunaddr, sizeof(sunaddr)) < 0) {
		perror("bind");
		*socket_name = '\0'; /* Don't unlink any existing file */
		cleanup_exit(1);
	}
	if (listen(sock, SSH_LISTEN_BACKLOG) < 0) {
		perror("listen");
		cleanup_exit(1);
	}

	/*
	 * Fork, and have the parent execute the command, if any, or present
	 * the socket data.  The child continues as the authentication agent.
	 */
	if (d_flag) {
		log_init(__progname, SYSLOG_LEVEL_DEBUG1, SYSLOG_FACILITY_AUTH, 1);
		(*f_setenv)(SSH_AUTHSOCKET_ENV_NAME, socket_name);
		printf("echo Agent pid %ld;\n", (long)parent_pid);
		goto skip;
	}
	pid = fork();
	if (pid == -1) {
		perror("fork");
		cleanup_exit(1);
	}
	if (pid != 0) {		/* Parent - execute the given command. */
		close(sock);
		snprintf(pidstrbuf, sizeof pidstrbuf, "%ld", (long)pid);
		if (ac == 0) {
			(*f_setenv)(SSH_AUTHSOCKET_ENV_NAME, socket_name);
			(*f_setenv)(SSH_AGENTPID_ENV_NAME, pidstrbuf);
			printf("echo Agent pid %ld;\n", (long)pid);
			exit(0);
		}
		if (setenv(SSH_AUTHSOCKET_ENV_NAME, socket_name, 1) == -1 ||
		    setenv(SSH_AGENTPID_ENV_NAME, pidstrbuf, 1) == -1) {
			perror("setenv");
			exit(1);
		}
		execvp(av[0], av);
		perror(av[0]);
		exit(1);
	}
	/* child */
	log_init(__progname, SYSLOG_LEVEL_INFO, SYSLOG_FACILITY_AUTH, 0);

	if (setsid() == -1) {
		error("setsid: %s", strerror(errno));
		cleanup_exit(1);
	}

	(void)chdir("/");

	if (sock != STDERR_FILENO + 1) {
		if (dup2(sock, STDERR_FILENO + 1) == -1) {
			error("dup2: %s", strerror(errno));
			cleanup_exit(1);
		}
		close(sock);
		sock = STDERR_FILENO + 1;
	}
#if defined(F_CLOSEM)
	if (fcntl(sock + 1, F_CLOSEM, 0) == -1) {
		error("fcntl F_CLOSEM: %s", strerror(errno));
		cleanup_exit(1);
	}
#else
	{
		int nfiles;
#if defined(_SC_OPEN_MAX)
		nfiles = sysconf(_SC_OPEN_MAX);
#elif defined(RLIMIT_NOFILE)
		if (getrlimit(RLIMIT_CORE, &rlim) < 0) {
			error("getrlimit RLIMIT_NOFILE: %s", strerror(errno));
			cleanup_exit(1);
		}
		nfiles = rlim.rlim_cur;
#elif defined(OPEN_MAX)
		nfiles = OPEN_MAX;
#elif defined(NOFILE)
		nfiles = NOFILE;
#else
		nfiles = 1024;
#endif
		for (fd = sock + 1; fd < nfiles; fd++)
			close(fd);
	}
#endif
	if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		if (dup2(fd, STDIN_FILENO) == -1 ||
		    dup2(fd, STDOUT_FILENO) == -1 ||
		    dup2(fd, STDERR_FILENO) == -1) {
			error("dup2: %s", strerror(errno));
			cleanup_exit(1);
		}
		if (fd > STDERR_FILENO)
			close(fd);
	}

	/* deny core dumps, since memory contains unencrypted private keys */
	rlim.rlim_cur = rlim.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &rlim) < 0) {
		error("setrlimit RLIMIT_CORE: %s", strerror(errno));
		cleanup_exit(1);
	}

skip:

#ifdef ENABLE_PKCS11
	pkcs11_init(0);
#endif
	new_socket(AUTH_SOCKET, sock);
	if (ac > 0)
		parent_alive_interval = 10;
	idtab_init();
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, d_flag ? cleanup_handler : SIG_IGN);
	signal(SIGHUP, cleanup_handler);
	signal(SIGTERM, cleanup_handler);
	nalloc = 0;

	while (1) {
		prepare_select(&readsetp, &writesetp, &max_fd, &nalloc, &tvp);
		result = select(max_fd + 1, readsetp, writesetp, NULL, tvp);
		saved_errno = errno;
		if (parent_alive_interval != 0)
			check_parent_exists();
		(void) reaper();	/* remove expired keys */
		if (result < 0) {
			if (saved_errno == EINTR)
				continue;
			fatal("select: %s", strerror(saved_errno));
		} else if (result > 0)
			after_select(readsetp, writesetp);
	}
	/* NOTREACHED */
}
