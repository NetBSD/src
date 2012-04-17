/*	$NetBSD: crypto.h,v 1.1.1.1.6.1 2012/04/17 00:03:50 yamt Exp $	*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ntp_fp.h>
#include <ntp.h>
#include <ntp_stdlib.h>
#ifdef OPENSSL
# include "openssl/evp.h"
#else
# include <ntp_md5.h>		/* provides clone of OpenSSL MD5 API */
#endif
#include "utilities.h"
#include "sntp-opts.h"

#define LEN_PKT_MAC	LEN_PKT_NOMAC + sizeof(u_int32)

/* #include "sntp-opts.h" */

struct key {
	struct key *next;
	int key_id;
	int key_len;
	char type[10];
	char key_seq[64];
};

int auth_init(const char *keyfile, struct key **keys);
void get_key(int key_id, struct key **d_key);
int make_mac(char *pkt_data, int pkt_size, int mac_size, struct key *cmp_key, char *digest);
int auth_md5(char *pkt_data, int pkt_size, int mac_size, struct key *cmp_key);

#endif
