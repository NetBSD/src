/* $OpenBSD: kexc25519s.c,v 1.4 2014/01/12 08:13:13 djm Exp $ */
/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2010 Damien Miller.  All rights reserved.
 * Copyright (c) 2013 Aris Adamantiadis.  All rights reserved.
 *
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

#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include "xmalloc.h"
#include "buffer.h"
#include "key.h"
#include "cipher.h"
#include "kex.h"
#include "log.h"
#include "packet.h"
#include "ssh2.h"

void
kexc25519_server(Kex *kex)
{
	Key *server_host_private, *server_host_public;
	u_char *server_host_key_blob = NULL, *signature = NULL;
	u_char server_key[CURVE25519_SIZE];
	u_char *client_pubkey = NULL;
	u_char server_pubkey[CURVE25519_SIZE];
	u_char *hash;
	u_int slen, sbloblen, hashlen;
	Buffer shared_secret;

	/* generate private key */
	kexc25519_keygen(server_key, server_pubkey);
#ifdef DEBUG_KEXECDH
	dump_digest("server private key:", server_key, sizeof(server_key));
#endif

	if (kex->load_host_public_key == NULL ||
	    kex->load_host_private_key == NULL)
		fatal("Cannot load hostkey");
	server_host_public = kex->load_host_public_key(kex->hostkey_type);
	if (server_host_public == NULL)
		fatal("Unsupported hostkey type %d", kex->hostkey_type);
	server_host_private = kex->load_host_private_key(kex->hostkey_type);

	debug("expecting SSH2_MSG_KEX_ECDH_INIT");
	packet_read_expect(SSH2_MSG_KEX_ECDH_INIT);
	client_pubkey = packet_get_string(&slen);
	if (slen != CURVE25519_SIZE)
		fatal("Incorrect size for server Curve25519 pubkey: %d", slen);
	packet_check_eom();

#ifdef DEBUG_KEXECDH
	dump_digest("client public key:", client_pubkey, CURVE25519_SIZE);
#endif

	buffer_init(&shared_secret);
	kexc25519_shared_key(server_key, client_pubkey, &shared_secret);

	/* calc H */
	key_to_blob(server_host_public, &server_host_key_blob, &sbloblen);
	kex_c25519_hash(
	    kex->hash_alg,
	    kex->client_version_string,
	    kex->server_version_string,
	    buffer_ptr(&kex->peer), buffer_len(&kex->peer),
	    buffer_ptr(&kex->my), buffer_len(&kex->my),
	    server_host_key_blob, sbloblen,
	    client_pubkey,
	    server_pubkey,
	    buffer_ptr(&shared_secret), buffer_len(&shared_secret),
	    &hash, &hashlen
	);

	/* save session id := H */
	if (kex->session_id == NULL) {
		kex->session_id_len = hashlen;
		kex->session_id = xmalloc(kex->session_id_len);
		memcpy(kex->session_id, hash, kex->session_id_len);
	}

	/* sign H */
	kex->sign(server_host_private, server_host_public, &signature, &slen,
	    hash, hashlen);

	/* destroy_sensitive_data(); */

	/* send server hostkey, ECDH pubkey 'Q_S' and signed H */
	packet_start(SSH2_MSG_KEX_ECDH_REPLY);
	packet_put_string(server_host_key_blob, sbloblen);
	packet_put_string(server_pubkey, sizeof(server_pubkey));
	packet_put_string(signature, slen);
	packet_send();

	free(signature);
	free(server_host_key_blob);
	/* have keys, free server key */
	free(client_pubkey);

	kex_derive_keys(kex, hash, hashlen,
	    buffer_ptr(&shared_secret), buffer_len(&shared_secret));
	buffer_free(&shared_secret);
	kex_finish(kex);
}
