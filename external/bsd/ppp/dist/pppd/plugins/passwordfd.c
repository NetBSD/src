
/*
 *  Author: Arvin Schnell <arvin@suse.de>
 *
 *  This plugin let's you pass the password to the pppd via
 *  a file descriptor. That's easy and secure - no fiddling
 *  with pap- and chap-secrets files.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#include <pppd/pppd.h>
#include <pppd/upap.h>
#include <pppd/chap.h>
#include <pppd/eap.h>
#include <pppd/options.h>

char pppd_version[] = PPPD_VERSION;

static char save_passwd[MAXSECRETLEN];

static int pwfd_read_password(char **argv)
{
    ssize_t readgood, red;
    int passwdfd;
    char passwd[MAXSECRETLEN];

    if (!ppp_int_option(argv[0], &passwdfd))
	return 0;

    readgood = 0;
    do {
	red = read (passwdfd, passwd + readgood, MAXSECRETLEN - 1 - readgood);
	if (red == 0)
	    break;
	if (red < 0) {
	    error ("Can't read secret from fd\n");
	    readgood = -1;
	    break;
	}
	readgood += red;
    } while (readgood < MAXSECRETLEN - 1);

    close (passwdfd);

    if (readgood < 0)
	return 0;

    passwd[readgood] = 0;
    strcpy (save_passwd, passwd);

    return 1;
}

static struct option options[] = {
    { "passwordfd", o_special, pwfd_read_password,
      "Receive password on this file descriptor" },
    { NULL }
};

static int pwfd_check (void)
{
    return 1;
}

static int pwfd_passwd (char *user, char *passwd)
{
    if (passwd != NULL)
	strcpy(passwd, save_passwd);
    return 1;
}

void plugin_init (void)
{
    ppp_add_options (options);

    pap_check_hook = pwfd_check;
    pap_passwd_hook = pwfd_passwd;

    chap_check_hook = pwfd_check;
    chap_passwd_hook = pwfd_passwd;

#ifdef PPP_WITH_EAPTLS
    eaptls_passwd_hook = pwfd_passwd;
#endif
}
