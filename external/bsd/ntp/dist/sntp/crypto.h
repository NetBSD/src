/*	$NetBSD: crypto.h,v 1.1.1.1 2009/12/13 16:57:10 kardel Exp $	*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ntp_fp.h>
#include <ntp.h>
#include <ntp_md5.h>
#include <ntp_stdlib.h>

#include "utilities.h"
#include "sntp-opts.h"

#define LEN_PKT_MAC	LEN_PKT_NOMAC + sizeof(u_int32)

/* #include "sntp-opts.h" */

struct key {
	int key_id;
	int key_len;
	char type;
	char key_seq[16];
	struct key *next;
};

int auth_md5(char *pkt_data, int mac_size, struct key *cmp_key);
int auth_init(const char *keyfile, struct key **keys);
void get_key(int key_id, struct key **d_key);


#endif
