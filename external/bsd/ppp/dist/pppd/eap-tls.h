/*
 * eap-tls.h
 *
 * Copyright (c) Beniamino Galvani 2005 All rights reserved.
 *               Jan Just Keijser  2006-2019 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef __EAP_TLS_H__
#define __EAP_TLS_H__

#include "eap.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>

#define EAP_TLS_FLAGS_LI        128    /* length included flag */
#define EAP_TLS_FLAGS_MF        64     /* more fragments flag */
#define EAP_TLS_FLAGS_START     32     /* start flag */

#define EAP_TLS_MAX_LEN         65536  /* max eap tls packet size */

struct eaptls_session
{
    u_char *data;               /* buffered data */
    int datalen;                /* buffered data len */
    int offset;                 /* from where to send */
    int tlslen;                 /* total length of tls data */
    bool frag;                  /* packet is fragmented */
    bool tls_v13;               /* whether we've negotiated TLSv1.3 */
    SSL_CTX *ctx;
    SSL *ssl;                   /* ssl connection */
    BIO *from_ssl;
    BIO *into_ssl;
    char peer[MAXWORDLEN];      /* peer name */
    char peercertfile[MAXWORDLEN];
    bool alert_sent;
    u_char alert_sent_desc;
    bool alert_recv;
    u_char alert_recv_desc;
    char rtx[EAP_TLS_MAX_LEN];  /* retransmission buffer */
    int rtx_len;
    int mtu;                    /* unit mtu */
};


SSL_CTX *eaptls_init_ssl(int init_server, char *cacertfile, char *capath,
            char *certfile, char *peer_certfile, char *privkeyfile);
int eaptls_init_ssl_server(eap_state * esp);
int eaptls_init_ssl_client(eap_state * esp);
void eaptls_free_session(struct eaptls_session *ets);

int eaptls_is_init_finished(struct eaptls_session *ets);

int eaptls_receive(struct eaptls_session *ets, u_char * inp, int len);
int eaptls_send(struct eaptls_session *ets, u_char ** outp);
void eaptls_retransmit(struct eaptls_session *ets, u_char ** outp);

int get_eaptls_secret(int unit, char *client, char *server,
              char *clicertfile, char *servcertfile, char *cacertfile,
              char *capath, char *pkfile, int am_server);

#ifdef MPPE
#include "mppe.h"   /* MPPE_MAX_KEY_LEN */
extern u_char mppe_send_key[MPPE_MAX_KEY_LEN];
extern u_char mppe_recv_key[MPPE_MAX_KEY_LEN];
extern int mppe_keys_set;

void eaptls_gen_mppe_keys(struct eaptls_session *ets, int client);
#endif

#endif
