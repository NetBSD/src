/* $NetBSD: sha512.c,v 1.1 2005/08/24 19:59:09 elad Exp $ */

#include <crypto/sha2.h>	/* this hash type */
#include <md5.h>		/* the hash we're replacing */

#define HASHTYPE	"SHA512"
#define HASHLEN		128

#define MD5Filter	SHA512_Filter
#define MD5String	SHA512_String
#define MD5TestSuite	SHA512_TestSuite
#define MD5TimeTrial	SHA512_TimeTrial

#define MD5Data		SHA512_Data
#define MD5Init		SHA512_Init
#define MD5Update	SHA512_Update
#define MD5End		SHA512_End

#define MD5_CTX		SHA512_CTX

#include "md5.c"
