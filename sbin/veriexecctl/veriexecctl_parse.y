%{
/*
 * Parser for verified exec fingerprint file.
 *
 * $NetBSD: veriexecctl_parse.y,v 1.4 2005/01/19 20:42:04 xtraeme Exp $
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/verified_exec.h>

/* yacc internal function */
static int     yygrowstack(void);
int yylex(void);
void yyerror(const char *);

/* function prototypes */
static int
convert(char *fp, unsigned int count, unsigned char *out);

/* ioctl parameter struct */
static struct verified_exec_params params;
extern int fd;
extern int lineno;

%}

%union {
  char *string;
  int  intval;
}

%token EOL
%token <string> PATH
%token <string> STRING

%%

statement: /* empty */
  | statement path type fingerprint flags eol
  | statement error eol {
				yyclearin; /* discard lookahead */
				yyerrok;   /* no more error */
				fprintf(stderr,
					"skipping to next fingerprint\n");
			}
  ;

path: PATH 
{
	if (strlen($1) >= MAXPATHLEN) {
		yyerror("Path >= MAXPATHLEN");
		YYERROR;
	}
	strncpy(params.file, $1, MAXPATHLEN);
	params.type = VERIEXEC_DIRECT;
};

type: STRING
{
	if (strcasecmp($1, "md5") == 0) {
		params.fp_type = FINGERPRINT_TYPE_MD5;
	} else if (strcasecmp($1, "sha1") == 0) {
		params.fp_type = FINGERPRINT_TYPE_SHA1;
	} else {
		fprintf(stderr, "%s %s at %d, %s\n",
			"veriexecctl: bad fingerprint type", $1, lineno,
			"assuming MD5");
		params.fp_type = FINGERPRINT_TYPE_MD5;
	}
};


fingerprint: STRING
{
	unsigned int count;

	if (params.fp_type == FINGERPRINT_TYPE_SHA1)
		count = SHA1_FINGERPRINTLEN;
	else
		count = MD5_FINGERPRINTLEN;
	
	if (convert($1, count, params.fingerprint) < 0) {
		fprintf(stderr,
			"veriexecctl: bad fingerprint at line %d\n",
			lineno);
	}
};

flags: /* empty */
	| flag_spec flags;

flag_spec: STRING
{
	params.type = VERIEXEC_DIRECT;
	if (strcasecmp($1, "indirect") == 0) {
		params.type = VERIEXEC_INDIRECT;
	} else if (strcasecmp($1, "file") == 0) {
		params.type = VERIEXEC_FILE;
	}
};

eol: EOL
{
	if (!YYRECOVERING) /* Don't do the ioctl if we saw an error */
		do_ioctl();
};

%%
		
/*
 * Convert: takes the hexadecimal string pointed to by fp and converts
 * it to a "count" byte binary number which is stored in the array pointed to
 * by out.  Returns -1 if the conversion fails.
 */
static int
convert(char *fp, unsigned int count, unsigned char *out)
{
        int i, value, error = 0;
        
        for (i = 0; i < count; i++) {
                if ((fp[2*i] >= '0') && (fp[2*i] <= '9')) {
                        value = 16 * (fp[2*i] - '0');
                } else if ((fp[2*i] >= 'a') && (fp[2*i] <= 'f')) {
                        value = 16 * (10 + fp[2*i] - 'a');
                } else {
                        error = -1;
                        break;
                }
                
                if ((fp[2*i + 1] >= '0') && (fp[2*i + 1] <= '9')) {
                        value += fp[2*i + 1] - '0';
                } else if ((fp[2*i + 1] >= 'a') && (fp[2*i + 1] <= 'f')) {
                        value += fp[2*i + 1] - 'a' + 10;
                } else {
                        error = -1;
                        break;
                }

                out[i] = value;
        }
        
        return error;
}

/*
 * Perform the load of the fingerprint.  Assumes that the fingerprint
 * pseudo-device is opened and the file handle is in fd.
 */
static void
do_ioctl(void)
{
	if (ioctl(fd, VERIEXECLOAD, &params) < 0)
		fprintf(stderr,	"Ioctl failed with error `%s' on file %s\n",
			strerror(errno), params.file);
}
