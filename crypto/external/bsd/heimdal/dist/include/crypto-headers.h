/*	$NetBSD: crypto-headers.h,v 1.1.1.2 2017/01/28 20:46:42 christos Exp $	*/

#ifndef __crypto_header__
#define __crypto_header__

#ifndef PACKAGE_NAME
#error "need config.h"
#endif

#ifdef KRB5
#include <krb5/krb5-types.h>
#endif

#include <hcrypto/evp.h>
#include <hcrypto/des.h>
#include <hcrypto/md4.h>
#include <hcrypto/md5.h>
#include <hcrypto/sha.h>
#include <hcrypto/rc4.h>
#include <hcrypto/rc2.h>
#include <hcrypto/ui.h>
#include <hcrypto/rand.h>
#include <hcrypto/engine.h>
#include <hcrypto/pkcs12.h>
#include <hcrypto/hmac.h>

#endif /* __crypto_header__ */
