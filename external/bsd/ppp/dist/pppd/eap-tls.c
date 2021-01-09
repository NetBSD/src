/* * eap-tls.c - EAP-TLS implementation for PPP
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

#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/ui.h>
#include <openssl/x509v3.h>

#include "pppd.h"
#include "eap.h"
#include "eap-tls.h"
#include "fsm.h"
#include "lcp.h"
#include "pathnames.h"

typedef struct pw_cb_data
{
    const void *password;
    const char *prompt_info;
} PW_CB_DATA;

/* The openssl configuration file and engines can be loaded only once */
static CONF   *ssl_config  = NULL;
static ENGINE *cert_engine = NULL;
static ENGINE *pkey_engine = NULL;

/* TLSv1.3 do we have a session ticket ? */
static int have_session_ticket = 0;

int ssl_verify_callback(int, X509_STORE_CTX *); 
void ssl_msg_callback(int write_p, int version, int ct, const void *buf,
              size_t len, SSL * ssl, void *arg);
int ssl_new_session_cb(SSL *s, SSL_SESSION *sess);

X509 *get_X509_from_file(char *filename);
int ssl_cmp_certs(char *filename, X509 * a); 

#ifdef MPPE

#define EAPTLS_MPPE_KEY_LEN     32

/*
 *  OpenSSL 1.1+ introduced a generic TLS_method()
 *  For older releases we substitute the appropriate method
 */

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#define TLS_method SSLv23_method

#define SSL3_RT_HEADER  0x100

#ifndef SSL_CTX_set_max_proto_version
/** Mimics SSL_CTX_set_max_proto_version for OpenSSL < 1.1 */
static inline int SSL_CTX_set_max_proto_version(SSL_CTX *ctx, long tls_ver_max)
{
    long sslopt = 0;

    if (tls_ver_max < TLS1_VERSION)
    {
        sslopt |= SSL_OP_NO_TLSv1;
    }
#ifdef SSL_OP_NO_TLSv1_1
    if (tls_ver_max < TLS1_1_VERSION)
    {
        sslopt |= SSL_OP_NO_TLSv1_1;
    }
#endif
#ifdef SSL_OP_NO_TLSv1_2
    if (tls_ver_max < TLS1_2_VERSION)
    {
        sslopt |= SSL_OP_NO_TLSv1_2;
    }
#endif
    SSL_CTX_set_options(ctx, sslopt);

    return 1;
}
#endif /* SSL_CTX_set_max_proto_version */

#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */


/*
 *  Generate keys according to RFC 2716 and add to reply
 */
void eaptls_gen_mppe_keys(struct eaptls_session *ets, int client)
{
    unsigned char  out[4*EAPTLS_MPPE_KEY_LEN];
    const char    *prf_label;
    size_t         prf_size;
    unsigned char  eap_tls13_context[] = { EAPT_TLS };
    unsigned char *context = NULL;
    size_t         context_len = 0;
    unsigned char *p;

    dbglog("EAP-TLS generating MPPE keys");
    if (ets->tls_v13)
    {
        prf_label = "EXPORTER_EAP_TLS_Key_Material";
        context   = eap_tls13_context;
        context_len = 1;
    }
    else
    {
        prf_label = "client EAP encryption";
    }

    dbglog("EAP-TLS PRF label = %s", prf_label);
    prf_size = strlen(prf_label);
    if (SSL_export_keying_material(ets->ssl, out, sizeof(out), prf_label, prf_size, 
                                   context, context_len, 0) != 1)
    {
        warn( "EAP-TLS: Failed generating keying material" );
        return;
    }   

    /* 
     * We now have the master send and receive keys.
     * From these, generate the session send and receive keys.
     * (see RFC3079 / draft-ietf-pppext-mppe-keys-03.txt for details)
     */
    if (client)
    {
        p = out;
        BCOPY( p, mppe_send_key, sizeof(mppe_send_key) );
        p += EAPTLS_MPPE_KEY_LEN;
        BCOPY( p, mppe_recv_key, sizeof(mppe_recv_key) );
    }
    else
    {
        p = out;
        BCOPY( p, mppe_recv_key, sizeof(mppe_recv_key) );
        p += EAPTLS_MPPE_KEY_LEN;
        BCOPY( p, mppe_send_key, sizeof(mppe_send_key) );
    }

    mppe_keys_set = 1;
}

#endif /* MPPE */

void log_ssl_errors( void )
{
    unsigned long ssl_err = ERR_get_error();

    if (ssl_err != 0)
        dbglog("EAP-TLS SSL error stack:");
    while (ssl_err != 0) {
        dbglog( ERR_error_string( ssl_err, NULL ) );
        ssl_err = ERR_get_error();
    }
}


int password_callback (char *buf, int size, int rwflag, void *u)
{
    if (buf)
    {
        strlcpy (buf, passwd, size);
        return strlen (buf);
    }
    return 0;
}


CONF *eaptls_ssl_load_config( void )
{
    CONF        *config;
    int          ret_code;
    long         error_line = 33;

    config = NCONF_new( NULL );
    dbglog( "Loading OpenSSL config file" );
    ret_code = NCONF_load( config, _PATH_OPENSSLCONFFILE, &error_line );
    if (ret_code == 0)
    {
        warn( "EAP-TLS: Error in OpenSSL config file %s at line %d", _PATH_OPENSSLCONFFILE, error_line );
        NCONF_free( config );
        config = NULL;
        ERR_clear_error();
    }

    dbglog( "Loading OpenSSL built-ins" );
    ENGINE_load_builtin_engines();
    OPENSSL_load_builtin_modules();
   
    dbglog( "Loading OpenSSL configured modules" );
    if (CONF_modules_load( config, NULL, 0 ) <= 0 )
    {
        warn( "EAP-TLS: Error loading OpenSSL modules" );
        log_ssl_errors();
        config = NULL;
    }

    return config;
}

ENGINE *eaptls_ssl_load_engine( char *engine_name )
{
    ENGINE      *e = NULL;

    dbglog( "Enabling OpenSSL auto engines" );
    ENGINE_register_all_complete();

    dbglog( "Loading OpenSSL '%s' engine support", engine_name );
    e = ENGINE_by_id( engine_name );
    if (!e) 
    {
        dbglog( "EAP-TLS: Cannot load '%s' engine support, trying 'dynamic'", engine_name );
        e = ENGINE_by_id( "dynamic" );
        if (e)
        {
            if (!ENGINE_ctrl_cmd_string(e, "SO_PATH", engine_name, 0)
             || !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0))
            {
                warn( "EAP-TLS: Error loading dynamic engine '%s'", engine_name );
                log_ssl_errors();
                ENGINE_free(e);
                e = NULL;
            }
        }
        else
        {
            warn( "EAP-TLS: Cannot load dynamic engine support" );
        }
    }

    if (e)
    {
        dbglog( "Initialising engine" );
        if(!ENGINE_set_default(e, ENGINE_METHOD_ALL))
        {
            warn( "EAP-TLS: Cannot use that engine" );
            log_ssl_errors();
            ENGINE_free(e);
            e = NULL;
        }
    }

    return e;
}



/*
 * Initialize the SSL stacks and tests if certificates, key and crl
 * for client or server use can be loaded.
 */
SSL_CTX *eaptls_init_ssl(int init_server, char *cacertfile, char *capath,
            char *certfile, char *peer_certfile, char *privkeyfile)
{
    char        *cert_engine_name = NULL;
    char        *cert_identifier = NULL;
    char        *pkey_engine_name = NULL;
    char        *pkey_identifier = NULL;
    SSL_CTX     *ctx;
    SSL         *ssl;
    X509_STORE  *certstore;
    X509_LOOKUP *lookup;
    X509        *tmp;
    int          ret;
#if defined(TLS1_2_VERSION)
    long         tls_version = TLS1_2_VERSION; 
#elif defined(TLS1_1_VERSION)
    long         tls_version = TLS1_1_VERSION; 
#else
    long         tls_version = TLS1_VERSION; 
#endif

    /*
     * Without these can't continue 
     */
    if (!(cacertfile[0] || capath[0]))
    {
        error("EAP-TLS: CA certificate file or path missing");
        return NULL;
    }

    if (!certfile[0])
    {
        error("EAP-TLS: Certificate missing");
        return NULL;
    }

    if (!privkeyfile[0])
    {
        error("EAP-TLS: Private key missing");
        return NULL;
    }

    SSL_library_init();
    SSL_load_error_strings();

    /* load the openssl config file only once and load it before triggering
       the loading of a global openssl config file via SSL_CTX_new()
     */
    if (!ssl_config)
        ssl_config = eaptls_ssl_load_config();

    ctx = SSL_CTX_new(TLS_method());

    if (!ctx) {
        error("EAP-TLS: Cannot initialize SSL CTX context");
        goto fail;
    }

    /* if the certificate filename is of the form engine:id. e.g.
        pkcs11:12345
       then we try to load and use this engine.
       If the certificate filename starts with a / or . then we
       ALWAYS assume it is a file and not an engine/pkcs11 identifier
     */
    if ( index( certfile, '/' ) == NULL && index( certfile, '.') == NULL )
    {
        cert_identifier = index( certfile, ':' );

        if (cert_identifier)
        {
            cert_engine_name = certfile;
            *cert_identifier = '\0';
            cert_identifier++;

            dbglog( "Found certificate engine '%s'", cert_engine_name );
            dbglog( "Found certificate identifier '%s'", cert_identifier );
        }
    }

    /* if the privatekey filename is of the form engine:id. e.g.
        pkcs11:12345
       then we try to load and use this engine.
       If the privatekey filename starts with a / or . then we
       ALWAYS assume it is a file and not an engine/pkcs11 identifier
     */
    if ( index( privkeyfile, '/' ) == NULL && index( privkeyfile, '.') == NULL )
    {
        pkey_identifier = index( privkeyfile, ':' );

        if (pkey_identifier)
        {
            pkey_engine_name = privkeyfile;
            *pkey_identifier = '\0';
            pkey_identifier++;

            dbglog( "Found privatekey engine '%s'", pkey_engine_name );
            dbglog( "Found privatekey identifier '%s'", pkey_identifier );
        }
    }

    if (cert_identifier && pkey_identifier)
    {
        if (strlen( cert_identifier ) == 0)
        {
            if (strlen( pkey_identifier ) == 0)
                error( "EAP-TLS: both the certificate and privatekey identifiers are missing!" );
            else
            {
                dbglog( "Substituting privatekey identifier for certificate identifier" );
                cert_identifier = pkey_identifier;
            }
        }
        else
        {
            if (strlen( pkey_identifier ) == 0)
            {
                dbglog( "Substituting certificate identifier for privatekey identifier" );
                pkey_identifier = cert_identifier;
            }
        }
    }

    if (ssl_config && cert_engine_name)
        cert_engine = eaptls_ssl_load_engine( cert_engine_name );

    if (ssl_config && pkey_engine_name)
    {
        /* don't load the same engine twice */
        if ( cert_engine && strcmp( cert_engine_name, pkey_engine_name) == 0 )
            pkey_engine = cert_engine;
        else
            pkey_engine = eaptls_ssl_load_engine( pkey_engine_name );
    }

    SSL_CTX_set_default_passwd_cb (ctx, password_callback);

    if (strlen(cacertfile) == 0) cacertfile = NULL;
    if (strlen(capath) == 0)     capath = NULL;

    if (!SSL_CTX_load_verify_locations(ctx, cacertfile, capath))
    {
        error("EAP-TLS: Cannot load verify locations");
        if (cacertfile) dbglog("CA certificate file = [%s]", cacertfile);
        if (capath) dbglog("CA certificate path = [%s]", capath);
        goto fail;
    }

    if (init_server)
        SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(cacertfile));

    if (cert_engine)
    {
        struct
        {
            const char *s_slot_cert_id;
            X509 *cert;
        } cert_info;

        cert_info.s_slot_cert_id = cert_identifier;
        cert_info.cert = NULL;
        
        if (!ENGINE_ctrl_cmd( cert_engine, "LOAD_CERT_CTRL", 0, &cert_info, NULL, 0 ) )
        {
            error( "EAP-TLS: Error loading certificate with id '%s' from engine", cert_identifier );
            goto fail;
        }

        if (cert_info.cert)
        {
            dbglog( "Got the certificate, adding it to SSL context" );
            dbglog( "subject = %s", X509_NAME_oneline( X509_get_subject_name( cert_info.cert ), NULL, 0 ) );
            if (SSL_CTX_use_certificate(ctx, cert_info.cert) <= 0)
            {
                error("EAP-TLS: Cannot use PKCS11 certificate %s", cert_identifier);
                goto fail;
            }
        }
        else
        {
            warn("EAP-TLS: Cannot load PKCS11 key %s", cert_identifier);
            log_ssl_errors();
        }
    }
    else
    {
        if (!SSL_CTX_use_certificate_chain_file(ctx, certfile))
        {
            error( "EAP-TLS: Cannot use public certificate %s", certfile );
            goto fail;
        }
    }


    /*
     *  Check the Before and After dates of the certificate
     */
    ssl = SSL_new(ctx);
    tmp = SSL_get_certificate(ssl);

    ret = X509_cmp_time(X509_get_notBefore(tmp), NULL);
    if (ret == 0)
    {    
        warn( "EAP-TLS: Failed to read certificate notBefore field.");
    }    
    if (ret > 0) 
    {    
        warn( "EAP-TLS: Your certificate is not yet valid!");
    }    

    ret = X509_cmp_time(X509_get_notAfter(tmp), NULL);
    if (ret == 0)
    {    
        warn( "EAP-TLS: Failed to read certificate notAfter field.");
    }    
    if (ret < 0)
    {
        warn( "EAP-TLS: Your certificate has expired!");
    }
    SSL_free(ssl);

    if (pkey_engine)
    {
        EVP_PKEY   *pkey = NULL;
        PW_CB_DATA  cb_data;

        cb_data.password = passwd;
        cb_data.prompt_info = pkey_identifier;

        if (passwd[0] != 0)
        {
            UI_METHOD* transfer_pin = UI_create_method("transfer_pin");

            int writer (UI *ui, UI_STRING *uis)
            {
                PW_CB_DATA* cb_data = (PW_CB_DATA*)UI_get0_user_data(ui);
                UI_set_result(ui, uis, cb_data->password);
                return 1;
            };
            int stub (UI* ui) {return 1;};
            int stub_reader (UI *ui, UI_STRING *uis) {return 1;};

            UI_method_set_writer(transfer_pin,  writer);
            UI_method_set_opener(transfer_pin,  stub);
            UI_method_set_closer(transfer_pin,  stub);
            UI_method_set_flusher(transfer_pin, stub);
            UI_method_set_reader(transfer_pin,  stub_reader);

            dbglog( "Using our private key '%s' in engine", pkey_identifier );
            pkey = ENGINE_load_private_key(pkey_engine, pkey_identifier, transfer_pin, &cb_data);

            if (transfer_pin) UI_destroy_method(transfer_pin);
        }
        else {
            dbglog( "Loading private key '%s' from engine", pkey_identifier );
            pkey = ENGINE_load_private_key(pkey_engine, pkey_identifier, NULL, NULL);
        }
        if (pkey)
        {
            dbglog( "Got the private key, adding it to SSL context" );
            if (SSL_CTX_use_PrivateKey(ctx, pkey) <= 0)
            {
                error("EAP-TLS: Cannot use PKCS11 key %s", pkey_identifier);
                goto fail;
            }
        }
        else
        {
            warn("EAP-TLS: Cannot load PKCS11 key %s", pkey_identifier);
            log_ssl_errors();
        }
    }
    else
    {
        if (!SSL_CTX_use_PrivateKey_file(ctx, privkeyfile, SSL_FILETYPE_PEM))
        { 
            error("EAP-TLS: Cannot use private key %s", privkeyfile);
            goto fail;
        }
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {
        error("EAP-TLS: Private key %s fails security check", privkeyfile);
        goto fail;
    }

    /* Explicitly set the NO_TICKETS flag to support Win7/Win8 clients */
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3
#ifdef SSL_OP_NO_TICKET
    | SSL_OP_NO_TICKET
#endif
    );

    /* OpenSSL 1.1.1+ does not include RC4 ciphers by default.
     * This causes totally obsolete WinXP clients to fail. If you really
     * need ppp+EAP-TLS+openssl 1.1.1+WinXP then enable RC4 cipers and
     * make sure that you use an OpenSSL that supports them

    SSL_CTX_set_cipher_list(ctx, "RC4");
     */


    /* Set up a SSL Session cache with a callback. This is needed for TLSv1.3+.
     * During the initial handshake the server signals to the client early on
     * that the handshake is finished, even before the client has sent its
     * credentials to the server. The actual connection (and moment that the
     * client sends its credentials) only starts after the arrival of the first
     * session ticket. The 'ssl_new_session_cb' catches this ticket.
     */
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_INTERNAL_STORE);
    SSL_CTX_sess_set_new_cb(ctx, ssl_new_session_cb);

    /* As EAP-TLS+TLSv1.3 is highly experimental we offer the user a chance to override */
    if (max_tls_version)
    {
        if (strncmp(max_tls_version, "1.0", 3) == 0)
            tls_version = TLS1_VERSION;
        else if (strncmp(max_tls_version, "1.1", 3) == 0)
            tls_version = TLS1_1_VERSION;
        else if (strncmp(max_tls_version, "1.2", 3) == 0)
#ifdef TLS1_2_VERSION
            tls_version = TLS1_2_VERSION;
#else
        {
            warn("TLSv1.2 not available. Defaulting to TLSv1.1");
            tls_version = TLS_1_1_VERSION;
        }
#endif
        else if (strncmp(max_tls_version, "1.3", 3) == 0)
#ifdef TLS1_3_VERSION
            tls_version = TLS1_3_VERSION;
#else
            warn("TLSv1.3 not available.");
#endif
    }

    dbglog("EAP-TLS: Setting max protocol version to 0x%X", tls_version);
    SSL_CTX_set_max_proto_version(ctx, tls_version);

    SSL_CTX_set_verify_depth(ctx, 5);
    SSL_CTX_set_verify(ctx,
               SSL_VERIFY_PEER |
               SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
               &ssl_verify_callback);

    if (crl_dir) {
        if (!(certstore = SSL_CTX_get_cert_store(ctx))) {
            error("EAP-TLS: Failed to get certificate store");
            goto fail;
        }

        if (!(lookup =
             X509_STORE_add_lookup(certstore, X509_LOOKUP_hash_dir()))) {
            error("EAP-TLS: Store lookup for CRL failed");

            goto fail;
        }

        X509_LOOKUP_add_dir(lookup, crl_dir, X509_FILETYPE_PEM);
        X509_STORE_set_flags(certstore, X509_V_FLAG_CRL_CHECK);
    }

    if (crl_file) {
        FILE     *fp  = NULL;
        X509_CRL *crl = NULL;

        fp = fopen(crl_file, "r");
        if (!fp) {
            error("EAP-TLS: Cannot open CRL file '%s'", crl_file);
            goto fail;
        }

        crl = PEM_read_X509_CRL(fp, NULL, NULL, NULL);
        if (!crl) {
            error("EAP-TLS: Cannot read CRL file '%s'", crl_file);
            goto fail;
        }

        if (!(certstore = SSL_CTX_get_cert_store(ctx))) {
            error("EAP-TLS: Failed to get certificate store");
            goto fail;
        }
        if (!X509_STORE_add_crl(certstore, crl)) {
            error("EAP-TLS: Cannot add CRL to certificate store");
            goto fail;
        }
        X509_STORE_set_flags(certstore, X509_V_FLAG_CRL_CHECK);

    }

    /*
     * If a peer certificate file was specified, it must be valid, else fail 
     */
    if (peer_certfile[0]) {
        if (!(tmp = get_X509_from_file(peer_certfile))) {
            error("EAP-TLS: Error loading client certificate from file %s",
                 peer_certfile);
            goto fail;
        }
        X509_free(tmp);
    }

    return ctx;

fail:
    log_ssl_errors();
    SSL_CTX_free(ctx);
    return NULL;
}

/*
 * Determine the maximum packet size by looking at the LCP handshake
 */

int eaptls_get_mtu(int unit)
{
    int mtu, mru;

    lcp_options *wo = &lcp_wantoptions[unit];
    lcp_options *go = &lcp_gotoptions[unit];
    lcp_options *ho = &lcp_hisoptions[unit];
    lcp_options *ao = &lcp_allowoptions[unit];

    mtu = ho->neg_mru? ho->mru: PPP_MRU;
    mru = go->neg_mru? MAX(wo->mru, go->mru): PPP_MRU;
    mtu = MIN(MIN(mtu, mru), ao->mru)- PPP_HDRLEN - 10;

    dbglog("MTU = %d", mtu);
    return mtu;
}


/*
 * Init the ssl handshake (server mode)
 */
int eaptls_init_ssl_server(eap_state * esp)
{
    struct eaptls_session *ets;
    char servcertfile[MAXWORDLEN];
    char clicertfile[MAXWORDLEN];
    char cacertfile[MAXWORDLEN];
    char capath[MAXWORDLEN];
    char pkfile[MAXWORDLEN];
    /*
     * Allocate new eaptls session 
     */
    esp->es_server.ea_session = malloc(sizeof(struct eaptls_session));
    if (!esp->es_server.ea_session)
        fatal("Allocation error");
    ets = esp->es_server.ea_session;

    if (!esp->es_server.ea_peer) {
        error("EAP-TLS: Error: client name not set (BUG)");
        return 0;
    }

    strlcpy(ets->peer, esp->es_server.ea_peer, MAXWORDLEN-1);

    dbglog( "getting eaptls secret" );
    if (!get_eaptls_secret(esp->es_unit, esp->es_server.ea_peer,
                   esp->es_server.ea_name, clicertfile,
                   servcertfile, cacertfile, capath, pkfile, 1)) {
        error( "EAP-TLS: Cannot get secret/password for client \"%s\", server \"%s\"",
                esp->es_server.ea_peer, esp->es_server.ea_name );
        return 0;
    }

    ets->mtu = eaptls_get_mtu(esp->es_unit);

    ets->ctx = eaptls_init_ssl(1, cacertfile, capath, servcertfile, clicertfile, pkfile);
    if (!ets->ctx)
        goto fail;

    if (!(ets->ssl = SSL_new(ets->ctx)))
        goto fail;

    /*
     * Set auto-retry to avoid timeouts on BIO_read
     */
    SSL_set_mode(ets->ssl, SSL_MODE_AUTO_RETRY);

    /*
     * Initialize the BIOs we use to read/write to ssl engine 
     */
    ets->into_ssl = BIO_new(BIO_s_mem());
    ets->from_ssl = BIO_new(BIO_s_mem());
    SSL_set_bio(ets->ssl, ets->into_ssl, ets->from_ssl);

    SSL_set_msg_callback(ets->ssl, ssl_msg_callback);
    SSL_set_msg_callback_arg(ets->ssl, ets);

    /*
     * Attach the session struct to the connection, so we can later
     * retrieve it when doing certificate verification
     */
    SSL_set_ex_data(ets->ssl, 0, ets);

    SSL_set_accept_state(ets->ssl);

    ets->tls_v13 = 0;

    ets->data = NULL;
    ets->datalen = 0;
    ets->alert_sent = 0;
    ets->alert_recv = 0;

    /*
     * If we specified the client certificate file, store it in ets->peercertfile,
     * so we can check it later in ssl_verify_callback()
     */
    if (clicertfile[0])
        strlcpy(&ets->peercertfile[0], clicertfile, MAXWORDLEN);
    else
        ets->peercertfile[0] = 0;

    return 1;

fail:
    SSL_CTX_free(ets->ctx);
    return 0;
}

/*
 * Init the ssl handshake (client mode)
 */
int eaptls_init_ssl_client(eap_state * esp)
{
    struct eaptls_session *ets;
    char servcertfile[MAXWORDLEN];
    char clicertfile[MAXWORDLEN];
    char cacertfile[MAXWORDLEN];
    char capath[MAXWORDLEN];
    char pkfile[MAXWORDLEN];

    /*
     * Allocate new eaptls session 
     */
    esp->es_client.ea_session = malloc(sizeof(struct eaptls_session));
    if (!esp->es_client.ea_session)
        fatal("Allocation error");
    ets = esp->es_client.ea_session;

    /*
     * If available, copy server name in ets; it will be used in cert
     * verify 
     */
    if (esp->es_client.ea_peer)
        strlcpy(ets->peer, esp->es_client.ea_peer, MAXWORDLEN-1);
    else
        ets->peer[0] = 0;
    
    ets->mtu = eaptls_get_mtu(esp->es_unit);

    dbglog( "calling get_eaptls_secret" );
    if (!get_eaptls_secret(esp->es_unit, esp->es_client.ea_name,
                   ets->peer, clicertfile,
                   servcertfile, cacertfile, capath, pkfile, 0)) {
        error( "EAP-TLS: Cannot get secret/password for client \"%s\", server \"%s\"",
                esp->es_client.ea_name, ets->peer );
        return 0;
    }

    dbglog( "calling eaptls_init_ssl" );
    ets->ctx = eaptls_init_ssl(0, cacertfile, capath, clicertfile, servcertfile, pkfile);
    if (!ets->ctx)
        goto fail;

    ets->ssl = SSL_new(ets->ctx);

    if (!ets->ssl)
        goto fail;

    /*
     * Initialize the BIOs we use to read/write to ssl engine 
     */
    dbglog( "Initializing SSL BIOs" );
    ets->into_ssl = BIO_new(BIO_s_mem());
    ets->from_ssl = BIO_new(BIO_s_mem());
    SSL_set_bio(ets->ssl, ets->into_ssl, ets->from_ssl);

    SSL_set_msg_callback(ets->ssl, ssl_msg_callback);
    SSL_set_msg_callback_arg(ets->ssl, ets);

    /*
     * Attach the session struct to the connection, so we can later
     * retrieve it when doing certificate verification
     */
    SSL_set_ex_data(ets->ssl, 0, ets);

    SSL_set_connect_state(ets->ssl);

    ets->tls_v13 = 0;

    ets->data = NULL;
    ets->datalen = 0;
    ets->alert_sent = 0;
    ets->alert_recv = 0;

    /*
     * If we specified the server certificate file, store it in
     * ets->peercertfile, so we can check it later in
     * ssl_verify_callback() 
     */
    if (servcertfile[0])
        strlcpy(ets->peercertfile, servcertfile, MAXWORDLEN);
    else
        ets->peercertfile[0] = 0;

    return 1;

fail:
    dbglog( "eaptls_init_ssl_client: fail" );
    SSL_CTX_free(ets->ctx);
    return 0;

}

void eaptls_free_session(struct eaptls_session *ets)
{
    if (ets->ssl)
        SSL_free(ets->ssl);

    if (ets->ctx)
        SSL_CTX_free(ets->ctx);

    free(ets);
}


int eaptls_is_init_finished(struct eaptls_session *ets)
{
    if (ets->ssl && SSL_is_init_finished(ets->ssl))
    {
        if (ets->tls_v13) 
            return have_session_ticket;
        else
            return 1;
    }

    return 0;
}

/*
 * Handle a received packet, reassembling fragmented messages and
 * passing them to the ssl engine
 */
int eaptls_receive(struct eaptls_session *ets, u_char * inp, int len)
{
    u_char flags;
    u_int tlslen = 0;
    u_char dummy[65536];

    if (len < 1) {
        warn("EAP-TLS: received no or invalid data");
        return 1;
    }
        
    GETCHAR(flags, inp);
    len--;

    if (flags & EAP_TLS_FLAGS_LI && len > 4) {
        /*
         * LenghtIncluded flag set -> this is the first packet of a message
        */

        /*
         * the first 4 octets are the length of the EAP-TLS message
         */
        GETLONG(tlslen, inp);
        len -= 4;

        if (!ets->data) {

            if (tlslen > EAP_TLS_MAX_LEN) {
                error("EAP-TLS: TLS message length > %d, truncated", EAP_TLS_MAX_LEN);
                tlslen = EAP_TLS_MAX_LEN;
            }

            /*
             * Allocate memory for the whole message
            */
            ets->data = malloc(tlslen);
            if (!ets->data)
                fatal("EAP-TLS: allocation error\n");

            ets->datalen = 0;
            ets->tlslen = tlslen;
        }
        else
            warn("EAP-TLS: non-first LI packet? that's odd...");
    }
    else if (!ets->data) {
        /*
         * A non fragmented message without LI flag
        */
 
        ets->data = malloc(len);
        if (!ets->data)
            fatal("EAP-TLS: memory allocation error in eaptls_receive\n");
 
        ets->datalen = 0;
        ets->tlslen = len;
    }

    if (flags & EAP_TLS_FLAGS_MF)
        ets->frag = 1;
    else
        ets->frag = 0;

    if (len < 0) {
        warn("EAP-TLS: received malformed data");
        return 1;
    }

    if (len + ets->datalen > ets->tlslen) {
        warn("EAP-TLS: received data > TLS message length");
        return 1;
    }

    BCOPY(inp, ets->data + ets->datalen, len);
    ets->datalen += len;

    if (!ets->frag) {

        /*
         * If we have the whole message, pass it to ssl 
         */

        if (ets->datalen != ets->tlslen) {
            warn("EAP-TLS: received data != TLS message length");
            return 1;
        }

        if (BIO_write(ets->into_ssl, ets->data, ets->datalen) == -1)
            log_ssl_errors();

        SSL_read(ets->ssl, dummy, 65536);

        free(ets->data);
        ets->data = NULL;
        ets->datalen = 0;
    }

    return 0;
}

/*
 * Return an eap-tls packet in outp.
 * A TLS message read from the ssl engine is buffered in ets->data.
 * At each call we control if there is buffered data and send a 
 * packet of mtu bytes.
 */
int eaptls_send(struct eaptls_session *ets, u_char ** outp)
{
    bool first = 0;
    int size;
    u_char fromtls[65536];
    int res;
    u_char *start;

    start = *outp;

    if (!ets->data)
    {
        if(!ets->alert_sent)
        {
            res = SSL_read(ets->ssl, fromtls, 65536);
        }

        /*
         * Read from ssl 
         */
        if ((res = BIO_read(ets->from_ssl, fromtls, 65536)) == -1)
        {
            warn("EAP-TLS send: No data from BIO_read");
            return 1;
        }

        ets->datalen = res;

        ets->data = malloc(ets->datalen);
        if (!ets->data)
            fatal("EAP-TLS: memory allocation error in eaptls_send\n");

        BCOPY(fromtls, ets->data, ets->datalen);

        ets->offset = 0;
        first = 1;
    }

    size = ets->datalen - ets->offset;
    
    if (size > ets->mtu) {
        size = ets->mtu;
        ets->frag = 1;
    } else
        ets->frag = 0;

    PUTCHAR(EAPT_TLS, *outp);

    /*
     * Set right flags and length if necessary 
     */
    if (ets->frag && first) {
        PUTCHAR(EAP_TLS_FLAGS_LI | EAP_TLS_FLAGS_MF, *outp);
        PUTLONG(ets->datalen, *outp);
    } else if (ets->frag) {
        PUTCHAR(EAP_TLS_FLAGS_MF, *outp);
    } else
        PUTCHAR(0, *outp);

    /*
     * Copy the data in outp 
     */
    BCOPY(ets->data + ets->offset, *outp, size);
    INCPTR(size, *outp);

    /*
     * Copy the packet in retransmission buffer 
     */
    BCOPY(start, &ets->rtx[0], *outp - start);
    ets->rtx_len = *outp - start;

    ets->offset += size;

    if (ets->offset >= ets->datalen) {

        /*
         * The whole message has been sent 
         */

        free(ets->data);
        ets->data = NULL;
        ets->datalen = 0;
        ets->offset = 0;
    }

    return 0;
}

/*
 * Get the sent packet from the retransmission buffer
 */
void eaptls_retransmit(struct eaptls_session *ets, u_char ** outp)
{
    BCOPY(ets->rtx, *outp, ets->rtx_len);
    INCPTR(ets->rtx_len, *outp);
}

/*
 * Verify a certificate.
 * Most of the work (signatures and issuer attributes checking)
 * is done by ssl; we check the CN in the peer certificate 
 * against the peer name.
 */
int ssl_verify_callback(int ok, X509_STORE_CTX * ctx)
{
    char subject[256];
    char cn_str[256];
    X509 *peer_cert;
    int err, depth;
    SSL *ssl;
    struct eaptls_session *ets;

    peer_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    dbglog("certificate verify depth: %d", depth);

    if (auth_required && !ok) {
        X509_NAME_oneline(X509_get_subject_name(peer_cert),
                  subject, 256);

        X509_NAME_get_text_by_NID(X509_get_subject_name(peer_cert),
                      NID_commonName, cn_str, 256);

        dbglog("Certificate verification error:\n depth: %d CN: %s"
               "\n err: %d (%s)\n", depth, cn_str, err,
               X509_verify_cert_error_string(err));

        return 0;
    }

    ssl = X509_STORE_CTX_get_ex_data(ctx,
                       SSL_get_ex_data_X509_STORE_CTX_idx());

    ets = (struct eaptls_session *)SSL_get_ex_data(ssl, 0);

    if (ets == NULL) {
        error("Error: SSL_get_ex_data returned NULL");
        return 0;
    }

    log_ssl_errors();

    if (!depth) 
    {
        /* This is the peer certificate */

        X509_NAME_oneline(X509_get_subject_name(peer_cert),
                  subject, 256);

        X509_NAME_get_text_by_NID(X509_get_subject_name(peer_cert),
                      NID_commonName, cn_str, 256);

        /*
         * If acting as client and the name of the server wasn't specified
         * explicitely, we can't verify the server authenticity 
         */
        if (!ets->peer[0]) {
            warn("Peer name not specified: no check");
            return ok;
        }

        /*
         * Check the CN 
         */
        if (strcmp(cn_str, ets->peer)) {
            error
                ("Certificate verification error: CN (%s) != peer_name (%s)",
                 cn_str, ets->peer);
            return 0;
        }

        warn("Certificate CN: %s , peer name %s", cn_str, ets->peer);

        /*
         * If a peer certificate file was specified, here we check it 
         */
        if (ets->peercertfile[0]) {
            if (ssl_cmp_certs(&ets->peercertfile[0], peer_cert)
                != 0) {
                error
                    ("Peer certificate doesn't match stored certificate");
                return 0;
            }
        }
    }

    return ok;
}

/*
 * Compare a certificate with the one stored in a file
 */
int ssl_cmp_certs(char *filename, X509 * a)
{
    X509 *b;
    int ret;

    if (!(b = get_X509_from_file(filename)))
        return 1;

    ret = X509_cmp(a, b);
    X509_free(b);

    return ret;

}

X509 *get_X509_from_file(char *filename)
{
    FILE *fp;
    X509 *ret;

    if (!(fp = fopen(filename, "r")))
        return NULL;

    ret = PEM_read_X509(fp, NULL, NULL, NULL);

    fclose(fp);

    return ret;
}

/*
 * Every sent & received message this callback function is invoked,
 * so we know when alert messages have arrived or are sent and
 * we can print debug information about TLS handshake.
 */
void
ssl_msg_callback(int write_p, int version, int content_type,
         const void *buf, size_t len, SSL * ssl, void *arg)
{
    char string[256];
    struct eaptls_session *ets = (struct eaptls_session *)arg;
    unsigned char code;
    const unsigned char*msg = buf;
    int hvers = msg[1] << 8 | msg[2];

    if(write_p)
        strcpy(string, " -> ");
    else
        strcpy(string, " <- ");

    switch(content_type) {

    case SSL3_RT_HEADER:
        strcat(string, "SSL/TLS Header: ");
        switch(hvers) {
        case SSL3_VERSION:
                strcat(string, "SSL 3.0");
                break;
        case TLS1_VERSION:
                strcat(string, "TLS 1.0");
                break;
        case TLS1_1_VERSION:
                strcat(string, "TLS 1.1");
                break;
        case TLS1_2_VERSION:
                strcat(string, "TLS 1.2");
                break;
        default:
            sprintf(string, "SSL/TLS Header: Unknown version (%d)", hvers);
        }
        break;

    case SSL3_RT_ALERT:
        strcat(string, "Alert: ");
        code = msg[1];

        if (write_p) {
            ets->alert_sent = 1;
            ets->alert_sent_desc = code;
        } else {
            ets->alert_recv = 1;
            ets->alert_recv_desc = code;
        }

        strcat(string, SSL_alert_desc_string_long(code));
        break;

    case SSL3_RT_CHANGE_CIPHER_SPEC:
        strcat(string, "ChangeCipherSpec");
        break;

#ifdef SSL3_RT_INNER_CONTENT_TYPE
    case SSL3_RT_INNER_CONTENT_TYPE:
        strcat(string, "InnerContentType (TLS1.3)");
        break;
#endif

    case SSL3_RT_HANDSHAKE:

        strcat(string, "Handshake: ");
        code = msg[0];

        switch(code) {
            case SSL3_MT_HELLO_REQUEST:
                strcat(string,"Hello Request");
                break;
            case SSL3_MT_CLIENT_HELLO:
                strcat(string,"Client Hello");
                break;
            case SSL3_MT_SERVER_HELLO:
                strcat(string,"Server Hello");
                break;
#ifdef SSL3_MT_NEWSESSION_TICKET
            case SSL3_MT_NEWSESSION_TICKET:
                strcat(string,"New Session Ticket");
                break;
#endif
#ifdef SSL3_MT_END_OF_EARLY_DATA
            case SSL3_MT_END_OF_EARLY_DATA:
                strcat(string,"End of Early Data");
                break;
#endif
#ifdef SSL3_MT_ENCRYPTED_EXTENSIONS
            case SSL3_MT_ENCRYPTED_EXTENSIONS:
                strcat(string,"Encryped Extensions");
                break;
#endif
            case SSL3_MT_CERTIFICATE:
                strcat(string,"Certificate");
                break;
            case SSL3_MT_SERVER_KEY_EXCHANGE:
                strcat(string,"Server Key Exchange");
                break;
            case SSL3_MT_CERTIFICATE_REQUEST:
                strcat(string,"Certificate Request");
                break;
            case SSL3_MT_SERVER_DONE:
                strcat(string,"Server Hello Done");
                break;
            case SSL3_MT_CERTIFICATE_VERIFY:
                strcat(string,"Certificate Verify");
                break;
            case SSL3_MT_CLIENT_KEY_EXCHANGE:
                strcat(string,"Client Key Exchange");
                break;
            case SSL3_MT_FINISHED:
                strcat(string,"Finished: ");
                hvers = SSL_version(ssl);
                switch(hvers){
                    case SSL3_VERSION:
                        strcat(string, "SSL 3.0");
                        break;
                    case TLS1_VERSION:
                        strcat(string, "TLS 1.0");
                        break;
                    case TLS1_1_VERSION:
                        strcat(string, "TLS 1.1");
                        break;
                    case TLS1_2_VERSION:
                        strcat(string, "TLS 1.2");
                        break;
#ifdef TLS1_3_VERSION
                    case TLS1_3_VERSION:
                        strcat(string, "TLS 1.3 (experimental)");
                        ets->tls_v13 = 1;
                        break;
#endif
                    default:
                        strcat(string, "Unknown version");
                }
                break;
            default:
                sprintf( string, "Handshake: Unknown SSL3 code received: %d", code );
        }
        break;

    default:
        sprintf( string, "SSL message contains unknown content type: %d", content_type );
    }

    /* Alert messages must always be displayed */
    if(content_type == SSL3_RT_ALERT)
        error("%s", string);
    else
        dbglog("%s", string);
}

int 
ssl_new_session_cb(SSL *s, SSL_SESSION *sess)
{
    dbglog("EAP-TLS: Post-Handshake New Session Ticket arrived:");
    have_session_ticket = 1;

    /* always return success */
    return 1;
}

