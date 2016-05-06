/*
 * stubs to make sure both that the symbols don't vanish, leaving existing
 * programs stranded, and that SSL2 is reliably dead. dead dead dead.
 */

#include "ssl_locl.h"

#ifdef OPENSSL_NO_SSL2

/*
 * now in s2_meth.c
const SSL_METHOD *SSLv2_method(void) { return NULL; }
const SSL_METHOD *SSLv2_server_method(void) { return NULL; }
const SSL_METHOD *SSLv2_client_method(void) { return NULL; }
 *
 */

const SSL_CIPHER ssl2_ciphers[0];
const char ssl2_version_str[] = "";


int ssl2_enc_init(SSL *s, int client) { return 0; }
int ssl2_generate_key_material(SSL *s) { return 0; }
int ssl2_enc(SSL *s, int send_data) { return 0; }
void ssl2_mac(SSL *s, unsigned char *mac, int send_data) { }
const SSL_CIPHER *ssl2_get_cipher_by_char(const unsigned char *p) { return NULL; }
int ssl2_put_cipher_by_char(const SSL_CIPHER *c, unsigned char *p) { return 0; }
int ssl2_part_read(SSL *s, unsigned long f, int i) { return 0; }
int ssl2_do_write(SSL *s) { return 0; }
int ssl2_set_certificate(SSL *s, int type, int len,
                         const unsigned char *data) { return 0; }
void ssl2_return_error(SSL *s, int reason) {}
void ssl2_write_error(SSL *s) {}
int ssl2_num_ciphers(void) { return 0; }
const SSL_CIPHER *ssl2_get_cipher(unsigned int u) { return NULL; }
int ssl2_new(SSL *s) { return 0; }
void ssl2_free(SSL *s) {}
int ssl2_accept(SSL *s) { return 0; }
int ssl2_connect(SSL *s) { return 0; }
int ssl2_read(SSL *s, void *buf, int len) { return 0; }
int ssl2_peek(SSL *s, void *buf, int len) { return 0; }
int ssl2_write(SSL *s, const void *buf, int len) { return 0; }
int ssl2_shutdown(SSL *s) { return 0; }
void ssl2_clear(SSL *s) {}
long ssl2_ctrl(SSL *s, int cmd, long larg, void *parg) { return 0; }
long ssl2_ctx_ctrl(SSL_CTX *s, int cmd, long larg, void *parg) { return 0; }
long ssl2_callback_ctrl(SSL *s, int cmd, void (*fp) (void)) { return 0; }
long ssl2_ctx_callback_ctrl(SSL_CTX *s, int cmd, void (*fp) (void)) { return 0; }
int ssl2_pending(const SSL *s) { return 0; }
long ssl2_default_timeout(void) { return 0; }

#endif
