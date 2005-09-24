/*	$NetBSD: rmd160.c,v 1.2 2005/09/24 22:40:32 elad Exp $	*/

#include <crypto/rmd160.h>	/* this hash type */
#include <md5.h>		/* the hash we're replacing */

#define HASHTYPE	"RMD160"
#define HASHLEN		40

#define MD5Filter	RMD160Filter
#define MD5String	RMD160String
#define MD5TestSuite	RMD160TestSuite
#define MD5TimeTrial	RMD160TimeTrial

#define MD5Data		RMD160Data
#define MD5Init		RMD160Init
#define MD5Update	RMD160Update
#define MD5End		RMD160End

#define MD5_CTX		RMD160_CTX

#include "md5.c"
