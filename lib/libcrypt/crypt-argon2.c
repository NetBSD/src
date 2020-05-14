#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <argon2.h>

#include <err.h>
#include "crypt.h"

/* defaults pulled from run.c */
#define HASHLEN		32
#define T_COST_DEF 	3 
#define LOG_M_COST_DEF 	12 /* 2^12 = 4 MiB */
#define LANES_DEF 	1
#define THREADS_DEF 	1
#define OUTLEN_DEF 	32
#define MAX_PASS_LEN 	128

#define ARGON2_CONTEXT_INITIALIZER	\
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	T_COST_DEF, LOG_M_COST_DEF,\
	LANES_DEF, THREADS_DEF, \
	ARGON2_VERSION_NUMBER, 0, 0, ARGON2_DEFAULT_FLAGS}

#define ARGON2_ARGON2_STR	"argon2"
#define ARGON2_ARGON2I_STR	"argon2i"
#define ARGON2_ARGON2D_STR	"argon2d"
#define ARGON2_ARGON2ID_STR	"argon2id"

/* getnum also declared in pw_getsalt.c */
/* maybe move to util.h?? */
static int
getnum(const char *str, size_t *num)
{
        char *ep;
        unsigned long rv;
 
        if (str == NULL) {
                *num = 0;
                return 0;
        }

        rv = strtoul(str, &ep, 0);

        if (str == ep || *ep) {
                errno = EINVAL;
                return -1;
        }

        if (errno == ERANGE && rv == ULONG_MAX)
                return -1;
        *num = (size_t)rv;
        return 0;  
}

/* process params to argon2 */
/* we don't force param order as input, */
/* but we do provide the expected order to argon2 api */
static int decode_option(argon2_context * ctx, argon2_type * atype, const char * option) 
{
	size_t tmp=0;
        char * in = 0,*inp;
        char * a=0;
        char * p=0;
	size_t sl;
	int    error=0;

        in = (char *)strdup(option);
	inp = in;

	if (*inp == '$') inp++;

	a = strsep(&inp, "$");

	sl = strlen(a);

	if (sl == strlen(ARGON2_ARGON2I_STR) && 
	   !(strcmp(ARGON2_ARGON2I_STR, a))) {
		*atype=Argon2_i;
	} else if (sl == strlen(ARGON2_ARGON2D_STR) && 
	        !(strcmp(ARGON2_ARGON2D_STR, a))) {
		*atype=Argon2_d;
	}
	else if (sl == strlen(ARGON2_ARGON2ID_STR) && 
	        !(strcmp(ARGON2_ARGON2ID_STR, a))) {
		*atype=Argon2_id;
	} else { /* default to id, we assume simple mistake */
		/* don't abandon yet */
		*atype=Argon2_id;
	}

	a = strsep(&inp, "$");

	if ((getnum(a, &tmp))<0) { /* on error, default to current */
				/* should start thinking about aborting */
		ctx->version = ARGON2_VERSION_NUMBER;
	} else {
		ctx->version = tmp;
	}

	a = strsep(&inp, "$");

	/* parse labelled argon2 params */
	/* m_cost (m)
	 * t_cost (t)
	 * threads (p)
	 */
	while ((p = strsep(&a, ","))) {
		switch (*p) {
			case 'm':
				p += strlen("m=");
				if ((getnum(p, &tmp)) < 0) {
					--error;
				} else {
					ctx->m_cost = tmp;
				}
				break;
			case 't':
				p += strlen("t=");
				if ((getnum(p, &tmp)) < 0) {
					--error;
				} else {
					ctx->t_cost = tmp;
				}
				break;
			case 'p':
				p += strlen("p=");
				if ((getnum(p, &tmp)) < 0) {
					--error;
				} else {
					ctx->threads = tmp;
				}
				break;
			default:
				return -1;

		}
	}

	a = strsep(&inp, "$");

	snprintf((char *)ctx->salt,ctx->saltlen, "%s", a);

	a = strsep(&inp, "$");

	if (*a) {
		snprintf((char *)ctx->pwd,ctx->pwdlen, "%s", a);
	} else {
		/* don't care if passwd hash is missing */
		/* if missing, most likely coming from */
		/* pwhash or similar */ 
	}

	/* free our token buffer */
        free(in);

	/* 0 on success, <0 otherwise */
        return error;
}

char * 
__crypt_argon2(const char *pw, const char * salt)
{
	/* we use the libargon2 api to generate */
	/* return code */
	int rc=0;
	/* output buffer */
	char ebuf[32];
	/* ptr into argon2 encoded buffer */
	char * blkp=0;
	/* argon2 variable, default to id */
	argon2_type atype = Argon2_id;
	/* default to current argon2 version */
	int version=ARGON2_VERSION_NUMBER;
	/* argon2 context to collect params */
	argon2_context ctx = ARGON2_CONTEXT_INITIALIZER;
	/* argon2 encoded buffer */
	char encodebuf[256];
	/* argon2 salt buffer */
	char saltbuf[128];
	/* argon2 pwd buffer */
	char pwdbuf[128];
	/* returned static buffer */
	static char rbuf[512];

	/* clear buffers */
	memset(encodebuf, 0, sizeof(encodebuf));
	memset(saltbuf, 0, sizeof(saltbuf));
	memset(pwdbuf, 0, sizeof(pwdbuf));
	memset(rbuf, 0, sizeof(rbuf));

	/* we use static buffers to avoid allocation */
	/* and easier cleanup */
	ctx.out = (uint8_t *)ebuf;
	ctx.outlen = sizeof(ebuf);

	ctx.out = (uint8_t *)encodebuf;
	ctx.outlen = sizeof(encodebuf);

	ctx.salt = (uint8_t *)saltbuf;
	ctx.saltlen = sizeof(saltbuf);

	ctx.pwd= (uint8_t *)pwdbuf;
	ctx.pwdlen = sizeof(pwdbuf);

	/* decode salt string to argon2 params */
	/* argon2 context for param collection */
	rc = decode_option(&ctx, &atype, salt);

	if (rc < 0) {
	/* unable to parse input params */
		return 0;
	}

	rc = argon2_hash(ctx.t_cost, ctx.m_cost,
		ctx.threads, pw, strlen(pw), ctx.salt, strlen((char*)ctx.salt),
		ebuf, sizeof(ebuf), encodebuf, sizeof(encodebuf), atype, ctx.version);

	if (rc != ARGON2_OK) {
		fprintf(stderr, "Failed: %s\n", argon2_error_message(rc));
		return 0;
	}

	/* get encoded passwd */
	if ((blkp = strrchr(encodebuf, '$')) == NULL) {
		return 0;
	}

	/* skip over '$' */
	blkp++;

	/* we don't use encoded here because it base64 encodes salt */
	/* same encoding format as argon2 api, but with original salt */
	snprintf(rbuf, sizeof(rbuf)-1, "$%s$v=%d$m=%d,t=%d,p=%d$%s$%s",
			argon2_type2string(atype,0),
			version,
			ctx.m_cost,
			ctx.t_cost,
			ctx.threads,
			ctx.salt,
			blkp);

	/* clear buffers */
	memset(encodebuf, 0, sizeof(encodebuf));
	memset(saltbuf, 0, sizeof(saltbuf));
	memset(pwdbuf, 0, sizeof(pwdbuf));

	/* return encoded str */
	return rbuf;
}
