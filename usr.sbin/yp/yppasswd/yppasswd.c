/*
 * Copyright (c) 1992/3 Theo de Raadt <deraadt@fsa.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LINT
static char rcsid[] = "$Id: yppasswd.c,v 1.3 1993/06/04 15:52:21 brezak Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yppasswd.h>

uid_t uid;

extern char *crypt(), *getpass();

static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

to64(s, v, n)
char *s;
long v;
int n;
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}

char *
getnewpasswd(pw, oldp)
register struct passwd *pw;
char **oldp;
{
	static char buf[14];
	char salt[9], *p=NULL, *t;
	int tries = 0;

	printf("Changing YP password for %s.\n", pw->pw_name);

	if(oldp)
		*oldp = NULL;

	if(pw->pw_passwd) {
		p = getpass("Old password:");
		if( strcmp(crypt(p, pw->pw_passwd), pw->pw_passwd)) {
			fprintf(stderr, "Sorry.\n");
			return NULL;
		}
	}
	if(oldp)
		*oldp = strdup(p);

	buf[0] = '\0';
	while(1) {
		p = getpass("New password:");
		if(*p == '\0') {
			printf("Password unchanged.\n");
			return NULL;
		}
		if (strlen(p) <= 5 && (uid != 0 || ++tries < 2)) {
			printf("Please enter a longer password.\n");
			continue;
		}
		strcpy(buf, p);
		if( strcmp(buf, getpass("Retype new password:")) != 0)
			break;
		printf("Mismatch - password unchanged.\n");
		return NULL;
	}

	/* grab a random printable character that isn't a colon */
	srandom((int)time((time_t *)NULL));
	to64(&salt[0], random(), 2);
	return strdup(crypt(buf, salt));
}

main(argc, argv)
char **argv;
{
	struct yppasswd yppasswd;
	struct passwd *pw;
	char *domainname, *master;
	int r, rpcport, status;
	char *s;

	uid = getuid();

	r = yp_get_default_domain(&domainname);
	if(r) {
		fprintf(stderr, "yppasswd: can't get local yp domain. Reason: %s\n",
			yperr_string(r));
		exit(1);
	}

	r = yp_master(domainname, "passwd.byname", &master);
	if(r) {
		fprintf(stderr, "yppasswd: can't find the master ypserver. Reason: %s\n",
			yperr_string(r));
		exit(1);
	}

	rpcport = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE, IPPROTO_UDP);
	if(rpcport==0) {
		fprintf(stderr, "yppasswd: yppasswdd not running on master\n");
		fprintf(stderr, "Can't change password.\n");
		exit(1);
	}

	if(rpcport >= IPPORT_RESERVED) {
		fprintf(stderr, "yppasswd: yppasswd daemon running on illegal port.\n");
		exit(1);
	}

	if (!(pw = getpwuid(uid))) {
		fprintf(stderr, "yppasswd: unknown user (uid=%d).\n", uid);
		exit(1);
	}
	if (uid != pw->pw_uid) {
		fprintf(stderr, "yppasswd: cannot change passwd.\n");
		exit(1);
	}

	printf("Changing password for %s on %s.\n", pw->pw_name, master);
	s = getnewpasswd(pw, &yppasswd.oldpass);
	if(s == NULL)
		exit(1);

	yppasswd.newpw.pw_passwd = s;
	yppasswd.newpw.pw_name = pw->pw_name;
	yppasswd.newpw.pw_uid = pw->pw_uid;
	yppasswd.newpw.pw_gid = pw->pw_gid;
	yppasswd.newpw.pw_gecos = pw->pw_gecos;
	yppasswd.newpw.pw_dir = pw->pw_dir;
	yppasswd.newpw.pw_shell = pw->pw_shell;

	r = callrpc(master, YPPASSWDPROG, YPPASSWDVERS, YPPASSWDPROC_UPDATE,
		xdr_yppasswd, &yppasswd, xdr_int, &status);
	if (r)
		printf("Couldn't change NIS password.\n");
	else
		printf("The YP password has been changed on %s.\n", master);
	exit(0);
}
