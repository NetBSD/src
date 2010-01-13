/*	$NetBSD: net.c,v 1.124 2010/01/13 07:48:57 martin Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* net.c -- routines to fetch files off the network. */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#ifdef INET6
#include <sys/sysctl.h>
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_media.h>
#include <arpa/inet.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/sysctl.h>

int network_up = 0;
/* Access to network information */
static char *net_devices;
static char *net_up;
static char net_dev[STRSIZE];
static char net_domain[STRSIZE];
static char net_host[STRSIZE];
static char net_ip[SSTRSIZE];
static char net_srv_ip[SSTRSIZE];
static char net_mask[SSTRSIZE];
static char net_namesvr[STRSIZE];
static char net_defroute[STRSIZE];
static char net_media[STRSIZE];
static char sl_flags[STRSIZE];
static int net_dhcpconf;
#define DHCPCONF_IPADDR         0x01
#define DHCPCONF_NAMESVR        0x02
#define DHCPCONF_HOST           0x04
#define DHCPCONF_DOMAIN         0x08
#ifdef INET6
static char net_ip6[STRSIZE];
char net_namesvr6[STRSIZE];
static int net_ip6conf;
#define IP6CONF_AUTOHOST        0x01    
#endif


/* URL encode unsafe characters.  */

static char *url_encode (char *dst, const char *src, const char *ep,
				const char *safe_chars,
				int encode_leading_slash);

static void write_etc_hosts(FILE *f);

#define DHCPCD "/sbin/dhcpcd"
#include <signal.h>
static int config_dhcp(char *);
static void get_dhcp_value(char *, size_t, const char *);

#ifdef INET6
static int is_v6kernel (void);
static void init_v6kernel (int);
static int get_v6wait (void);
#endif

/*
 * URL encode unsafe characters.  See RFC 1738.
 *
 * Copies src string to dst, encoding unsafe or reserved characters
 * in %hex form as it goes, and returning a pointer to the result.
 * The result is always a nul-terminated string even if it had to be
 * truncated to avoid overflowing the available space.
 *
 * This url_encode() function does not operate on complete URLs, it
 * operates on strings that make up parts of URLs.  For example, in a
 * URL like "ftp://username:password@host/path", the username, password,
 * host and path should each be encoded separately before they are
 * joined together with the punctuation characters.
 *
 * In most ordinary use, the path portion of a URL does not start with
 * a slash; the slash is a separator between the host portion and the
 * path portion, and is dealt with by software outside the url_encode()
 * function.  However, it is valid for url_encode() to be passed a
 * string that does begin with a slash.  For example, the string might
 * represent a password, or a path part of a URL that the user really
 * does want to begin with a slash.
 *
 * len is the length of the destination buffer.  The result will be
 * truncated if necessary to fit in the destination buffer.
 *
 * safe_chars is a string of characters that should not be encoded.  If
 * safe_chars is non-NULL, any characters in safe_chars as well as any
 * alphanumeric characters will be copied from src to dst without
 * encoding.  Some potentially useful settings for this parameter are:
 *
 *	NULL		Everything is encoded (even alphanumerics)
 *	""		Everything except alphanumerics are encoded
 *	"/"		Alphanumerics and '/' remain unencoded
 *	"$-_.+!*'(),"	Consistent with a strict reading of RFC 1738
 *	"$-_.+!*'(),/"	As above, except '/' is not encoded
 *	"-_.+!,/"	As above, except shell special characters are encoded
 *
 * encode_leading_slash is a flag that determines whether or not to
 * encode a leading slash in a string.  If this flag is set, and if the
 * first character in the src string is '/', then the leading slash will
 * be encoded (as "%2F"), even if '/' is one of the characters in the
 * safe_chars string.  Note that only the first character of the src
 * string is affected by this flag, and that leading slashes are never
 * deleted, but either retained unchanged or encoded.
 *
 * Unsafe and reserved characters are defined in RFC 1738 section 2.2.
 * The most important parts are:
 *
 *      The characters ";", "/", "?", ":", "@", "=" and "&" are the
 *      characters which may be reserved for special meaning within a
 *      scheme. No other characters may be reserved within a scheme.
 *      [...]
 *
 *      Thus, only alphanumerics, the special characters "$-_.+!*'(),",
 *      and reserved characters used for their reserved purposes may be
 *      used unencoded within a URL.
 *
 */

#define RFC1738_SAFE				"$-_.+!*'(),"
#define RFC1738_SAFE_LESS_SHELL			"-_.+!,"
#define RFC1738_SAFE_LESS_SHELL_PLUS_SLASH	"-_.+!,/"

static char *
url_encode(char *dst, const char *src, const char *ep,
	const char *safe_chars, int encode_leading_slash)
{
	int ch;

	ep--;

	for (; dst < ep; src++) {
		ch = *src & 0xff;
		if (ch == 0)
			break;
		if (safe_chars != NULL &&
		    (ch != '/' || !encode_leading_slash) &&
		    (isalnum(ch) || strchr(safe_chars, ch))) {
			*dst++ = ch;
		} else {
			/* encode this char */
			if (ep - dst < 3)
				break;
			snprintf(dst, ep - dst, "%%%02X", ch);
			dst += 3;
		}
		encode_leading_slash = 0;
	}
	*dst = '\0';
	return dst;
}


static const char *ignored_if_names[] = {
	"eon",			/* netiso */
	"gre",			/* net */
	"ipip",			/* netinet */
	"gif",			/* netinet6 */
	"faith",		/* netinet6 */
	"lo",			/* net */
#if 0
	"mdecap",		/* netinet -- never in IF list (?) XXX */
#endif
	"nsip",			/* netns */
	"ppp",			/* net */
#if 0
	"sl",			/* net */
#endif
	"strip",		/* net */
	"tun",			/* net */
	/* XXX others? */
	NULL,
};

static void
get_ifconfig_info(void)
{
	char *textbuf;
	char *t, *nt;
	const char **ignore;
	int textsize;
	ulong fl;
	char *cp;

	free(net_devices);
	net_devices = NULL;
	free(net_up);
	net_up = NULL;

	/* Get ifconfig information */

	textsize = collect(T_OUTPUT, &textbuf, "/sbin/ifconfig -a 2>/dev/null");
	if (textsize < 0) {
		if (logging)
			(void)fprintf(logfp,
			    "Aborting: Could not run ifconfig.\n");
		(void)fprintf(stderr, "Could not run ifconfig.");
		exit(1);
	}

	for (t = textbuf; t != NULL && *t != 0; t = nt) {
		/* find entry for next interface */
		for (nt = t; (nt = strchr(nt, '\n')); ) {
			if (*++nt != '\t')
				break;
		}
		if (memcmp(t, "lo0:", 4) == 0)
			/* completely ignore loopback interface */
			continue;
		cp = strchr(t, '=');
		if (cp == NULL)
			break;
		/* get interface flags */
		fl = strtoul(cp + 1, &cp, 16);
		if (*cp != '<')
			break;

		for (ignore = ignored_if_names; *ignore != NULL; ignore++) {
			size_t len = strlen(*ignore);
			if (strncmp(t, *ignore, len) == 0 &&
			    isdigit((unsigned char)t[len]))
				break;
		}
		if (*ignore != NULL)
			continue;

		if (fl & IFF_UP) {
			/* This interface might be connected to the server */
			cp = strchr(t, ':');
			if (cp == NULL)
				break;
			asprintf(&cp, "%s%.*s ",
			    net_up ? net_up : "", (int)(cp - t), t);
			free(net_up);
			net_up = cp;
		}

		cp = strchr(t, ':');
		if (cp == NULL)
			break;
		asprintf(&cp, "%s%.*s ",
		    net_devices ? net_devices : "", (int)(cp - t), t);
		free(net_devices);
		net_devices = cp;
	}
	free(textbuf);
}

static int
do_ifreq(struct ifreq *ifr, unsigned long cmd)
{
	int sock;
	int rval;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		return -1;

	memset(ifr, 0, sizeof *ifr);
	strncpy(ifr->ifr_name, net_dev, sizeof ifr->ifr_name);
	rval = ioctl(sock, cmd, ifr);
	close(sock);

	return rval;
}

static int
do_ifmreq(struct ifmediareq *ifmr, unsigned long cmd)
{
	int sock;
	int rval;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		return -1;

	memset(ifmr, 0, sizeof *ifmr);
	strncpy(ifmr->ifm_name, net_dev, sizeof ifmr->ifm_name);
	rval = ioctl(sock, cmd, ifmr);
	close(sock);

	return rval;
}

/* Fill in defaults network values for the selected interface */
static void
get_ifinterface_info(void)
{
	struct ifreq ifr;
	struct ifmediareq ifmr;
	struct sockaddr_in *sa_in = (void*)&ifr.ifr_addr;
	int modew;
	const char *media_opt;
	const char *sep;

	if (do_ifreq(&ifr, SIOCGIFADDR) == 0 && sa_in->sin_addr.s_addr != 0)
		strlcpy(net_ip, inet_ntoa(sa_in->sin_addr), sizeof net_ip);

	if (do_ifreq(&ifr, SIOCGIFNETMASK) == 0 && sa_in->sin_addr.s_addr != 0)
		strlcpy(net_mask, inet_ntoa(sa_in->sin_addr), sizeof net_mask);

	if (do_ifmreq(&ifmr, SIOCGIFMEDIA) == 0) {
		/* Get the name of the media word */
		modew = ifmr.ifm_current;
		strlcpy(net_media, get_media_subtype_string(modew),
		    sizeof net_media);
		/* and add any media options */
		sep = " mediaopt ";
		while ((media_opt = get_media_option_string(&modew)) != NULL) {
			strlcat(net_media, sep, sizeof net_media);
			strlcat(net_media, media_opt, sizeof net_media);
			sep = ",";
		}
	}
}

#ifndef INET6
#define get_if6interface_info()
#else
static void
get_if6interface_info(void)
{
	char *textbuf, *t;
	int textsize;

	textsize = collect(T_OUTPUT, &textbuf,
	    "/sbin/ifconfig %s inet6 2>/dev/null", net_dev);
	if (textsize >= 0) {
		char *p;

		(void)strtok(textbuf, "\n"); /* ignore first line */
		while ((t = strtok(NULL, "\n")) != NULL) {
			if (strncmp(t, "\tinet6 ", 7) != 0)
				continue;
			t += 7;
			if (strstr(t, "tentative") || strstr(t, "duplicated"))
				continue;
			if (strncmp(t, "fe80:", 5) == 0)
				continue;

			p = t;
			while (*p && *p != ' ' && *p != '\n')
				p++;
			*p = '\0';
			strlcpy(net_ip6, t, sizeof(net_ip6));
			break;
		}
	}
	free(textbuf);
}
#endif

static void
get_host_info(void)
{
	char hostname[MAXHOSTNAMELEN + 1];
	char *dot;

	/* Check host (and domain?) name */
	if (gethostname(hostname, sizeof(hostname)) == 0 && hostname[0] != 0) {
		hostname[sizeof(hostname) - 1] = 0;
		/* check for a . */
		dot = strchr(hostname, '.');
		if (dot == NULL) {
			/* if not found its just a host, punt on domain */
			strlcpy(net_host, hostname, sizeof net_host);
		} else {
			/* split hostname into host/domain parts */
			*dot++ = 0;
			strlcpy(net_host, hostname, sizeof net_host);
			strlcpy(net_domain, dot, sizeof net_domain);
		}
	}
}

/*
 * recombine name parts split in get_host_info and config_network
 * (common code moved here from write_etc_hosts)
 */
static char *
recombine_host_domain(void)
{
	static char recombined[MAXHOSTNAMELEN + 1];
	int l = strlen(net_host) - strlen(net_domain);

	strlcpy(recombined, net_host, sizeof(recombined));

	if (l <= 0 ||
	    net_host[l - 1] != '.' ||
	    strcasecmp(net_domain, net_host + l) != 0) {
		/* net_host isn't an FQDN. */
		strlcat(recombined, ".", sizeof(recombined));
		strlcat(recombined, net_domain, sizeof(recombined));
	}
	return recombined;
}

#ifdef INET6
static int
is_v6kernel(void)
{
	int s;

	s = socket(PF_INET6, SOCK_DGRAM, 0);
	if (s < 0)
		return 0;
	close(s);
	return 1;
}

/*
 * initialize as v6 client.
 * we are sure that we will never become router with boot floppy :-)
 * (include and use sysctl(8) if you are willing to)
 */
static void
init_v6kernel(int autoconf)
{
	int v;
	int mib[4] = {CTL_NET, PF_INET6, IPPROTO_IPV6, 0};

	mib[3] = IPV6CTL_FORWARDING;
	v = 0;
	(void)sysctl(mib, 4, NULL, NULL, (void *)&v, sizeof(v));

	mib[3] = IPV6CTL_ACCEPT_RTADV;
	v = autoconf ? 1 : 0;
	(void)sysctl(mib, 4, NULL, NULL, (void *)&v, sizeof(v));
}

static int
get_v6wait(void)
{
	size_t len = sizeof(int);
	int v;
	int mib[4] = {CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_DAD_COUNT};

	len = sizeof(v);
	if (sysctl(mib, 4, (void *)&v, &len, NULL, 0) < 0) {
		/* warn("sysctl(net.inet6.ip6.dadcount)"); */
		return 1;	/* guess */
	}
	return v;
}
#endif

static int
handle_license(const char *dev)
{
	static struct {
		const char *dev;
		const char *lic;
	} licdev[] = {
		{ "iwi", "/libdata/firmware/if_iwi/LICENSE.ipw2200-fw" },
		{ "ipw", "/libdata/firmware/if_ipw/LICENSE" },
	};

	size_t i;

	for (i = 0; i < __arraycount(licdev); i++)
		if (strncmp(dev, licdev[i].dev, 3) == 0) {
			char buf[64];
			int val;
			size_t len = sizeof(int);
			(void)snprintf(buf, sizeof(buf), "hw.%s.accept_eula",
			    licdev[i].dev);
			if (sysctlbyname(buf, &val, &len, NULL, 0) != -1
			    && val != 0)
				return 1;
			msg_display(MSG_license, dev, licdev[i].lic);
			process_menu(MENU_yesno, NULL);
			if (yesno) {
				val = 1;
				if (sysctlbyname(buf, NULL, NULL, &val,
				    0) == -1)
					return 0;
				add_sysctl_conf("%s=1", buf);
				return 1;
			} else
				return 0;
		}
	return 1;
}

/*
 * Get the information to configure the network, configure it and
 * make sure both the gateway and the name server are up.
 */
int
config_network(void)
{
	char *tp;
	char *defname;
	const char *prompt;
	char *textbuf;
	int  octet0;
	int  dhcp_config;
	int  nfs_root = 0;
 	int  slip = 0;
 	int  pid, status;
 	char **ap, *slcmd[10], *in_buf;
 	char buffer[STRSIZE];
 	struct statvfs sb;

	int l;
	char dhcp_host[STRSIZE];
#ifdef INET6
	int v6config = 1;
#endif

	FILE *f;
	time_t now;

	if (network_up)
		return (1);

	get_ifconfig_info();

	if (net_up != NULL) {
		/* XXX: some retry loops come here... */
		/* active interfaces found */
		msg_display(MSG_netup, net_up);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return 0;
	}

	if (net_devices == NULL) {
		/* No network interfaces found! */
		msg_display(MSG_nonet);
		process_menu(MENU_ok, NULL);
		return (-1);
	}
	network_up = 1;

again:
	tp = strchr(net_devices, ' ');
	asprintf(&defname, "%.*s", (int)(tp - net_devices), net_devices);
	for (prompt = MSG_asknetdev;; prompt = MSG_badnet) {
		msg_prompt(prompt, defname, net_dev, sizeof net_dev - 1,
		    net_devices);
		l = strlen(net_dev);
		net_dev[l] = ' ';
		net_dev[l + 1] = 0;
		tp = strstr(net_devices, net_dev);
		if (tp == NULL)
			continue;
		if (tp != net_devices && tp[-1] != ' ')
			continue;
		net_dev[l] = 0;
		break;
	}
	free(defname);
	if (!handle_license(net_dev))
		goto done;

	slip = net_dev[0] == 's' && net_dev[1] == 'l' &&
	    isdigit((unsigned char)net_dev[2]);

	/* If root is on NFS do not reconfigure the interface. */
	if (statvfs("/", &sb) == 0 && strcmp(sb.f_fstypename, "nfs") == 0) {
		nfs_root = 1;
		dhcp_config = 0;
		get_ifinterface_info();
		get_if6interface_info();
		get_host_info();
	} else if (slip) {
		dhcp_config = 0;
	} else {
		/* Preload any defaults we can find */
		get_ifinterface_info();
		get_if6interface_info();
		get_host_info();

		/* domain and host */
		msg_display(MSG_netinfo);

		/* ethernet medium */
		for (;;) {
			msg_prompt_add(MSG_net_media, net_media, net_media,
					sizeof net_media);

			/*
			 * ifconfig does not allow media specifiers on
			 * IFM_MANUAL interfaces.  Our UI gives no way
			 * to set an option back
			 * to null-string if it gets accidentally set.
			 * Check for plausible alternatives.
			 */
			if (strcmp(net_media, "<default>") == 0 ||
			    strcmp(net_media, "default") == 0 ||
			    strcmp(net_media, "<manual>") == 0 ||
			    strcmp(net_media, "manual") == 0 ||
			    strcmp(net_media, "<none>") == 0 ||
			    strcmp(net_media, "none") == 0 ||
			    strcmp(net_media, " ") == 0) {
				*net_media = '\0';
			}

			if (*net_media == '\0')
				break;
			/*
			 * We must set the media type here - to give dhcp
			 * a chance
			 */
			if (run_program(0, "/sbin/ifconfig %s media %s",
				    net_dev, net_media) == 0)
				break;
			/* Failed to set - output the supported values */
			if (collect(T_OUTPUT, &textbuf, "/sbin/ifconfig -m %s |"
				    "while IFS=; read line;"
				    " do [ \"$line\" = \"${line#*media}\" ] || "
				    "echo $line;"
				    " done", net_dev ) > 0)
				msg_display(textbuf);
			free(textbuf);
		}

		net_dhcpconf = 0;
		/* try a dhcp configuration */
		dhcp_config = config_dhcp(net_dev);
		if (dhcp_config) {
			/* Get newly configured data off interface. */
			get_ifinterface_info();
			get_if6interface_info();
			get_host_info();

			net_dhcpconf |= DHCPCONF_IPADDR;

			/*
			 * Extract default route from output of
			 * 'route -n show'
			 */
			if (collect(T_OUTPUT, &textbuf,
				    "/sbin/route -n show | "
				    "while read dest gateway flags;"
				    " do [ \"$dest\" = default ] && {"
					" echo $gateway; break; };"
				    " done" ) > 0)
				strlcpy(net_defroute, textbuf,
				    sizeof net_defroute);
			free(textbuf);

			/* pull nameserver info out of /etc/resolv.conf */
			if (collect(T_OUTPUT, &textbuf,
				    "cat /etc/resolv.conf 2>/dev/null |"
				    " while read keyword address rest;"
				    " do [ \"$keyword\" = nameserver "
					" -a \"${address#*:}\" = "
					"\"${address}\" ] && {"
					    " echo $address; break; };"
				    " done" ) > 0)
				strlcpy(net_namesvr, textbuf,
				    sizeof net_namesvr);
			free(textbuf);
			if (net_namesvr[0] != '\0')
				net_dhcpconf |= DHCPCONF_NAMESVR;

			/* pull domainname out of leases file */
			get_dhcp_value(net_domain, sizeof(net_domain),
			    "domain-name");
			if (net_domain[0] != '\0')
				net_dhcpconf |= DHCPCONF_DOMAIN;

			/* pull hostname out of leases file */
			dhcp_host[0] = 0;
			get_dhcp_value(dhcp_host, sizeof(dhcp_host),
			    "host-name");
			if (dhcp_host[0] != '\0') {
				net_dhcpconf |= DHCPCONF_HOST;
				strlcpy(net_host, dhcp_host, sizeof net_host);
			}
		}
	}

	msg_prompt_add(MSG_net_domain, net_domain, net_domain,
	    sizeof net_domain);
	msg_prompt_add(MSG_net_host, net_host, net_host, sizeof net_host);

	if (!dhcp_config) {
		/* Manually configure IPv4 */
		if (!nfs_root)
			msg_prompt_add(MSG_net_ip, net_ip, net_ip,
			    sizeof net_ip);
		if (slip)
			msg_prompt_add(MSG_net_srv_ip, net_srv_ip, net_srv_ip,
			    sizeof net_srv_ip);
		else if (!nfs_root) {
			/* We don't want netmasks for SLIP */
			octet0 = atoi(net_ip);
			if (!net_mask[0]) {
				if (0 <= octet0 && octet0 <= 127)
					strlcpy(net_mask, "0xff000000",
				    	sizeof(net_mask));
				else if (128 <= octet0 && octet0 <= 191)
					strlcpy(net_mask, "0xffff0000",
				    	sizeof(net_mask));
				else if (192 <= octet0 && octet0 <= 223)
					strlcpy(net_mask, "0xffffff00",
				    	sizeof(net_mask));
			}
			msg_prompt_add(MSG_net_mask, net_mask, net_mask,
			    sizeof net_mask);
		}
		msg_prompt_add(MSG_net_defroute, net_defroute, net_defroute,
		    sizeof net_defroute);
	}

	if (!dhcp_config || net_namesvr[0] == 0)
		msg_prompt_add(MSG_net_namesrv, net_namesvr, net_namesvr,
		    sizeof net_namesvr);

#ifdef INET6
	/* IPv6 autoconfiguration */
	if (!is_v6kernel())
		v6config = 0;
	else if (v6config) {
		process_menu(MENU_noyes, deconst(MSG_Perform_IPv6_autoconfiguration));
		v6config = yesno ? 1 : 0;
		net_ip6conf |= yesno ? IP6CONF_AUTOHOST : 0;
	}

	if (v6config) {
		process_menu(MENU_namesrv6, NULL);
		if (!yesno)
			msg_prompt_add(MSG_net_namesrv6, net_namesvr6,
			    net_namesvr6, sizeof net_namesvr6);
	}
#endif

	/* confirm the setting */
	if (slip)
		msg_display(MSG_netok_slip, net_domain, net_host, net_dev,
			*net_ip == '\0' ? "<none>" : net_ip,
			*net_srv_ip == '\0' ? "<none>" : net_srv_ip,
			*net_mask == '\0' ? "<none>" : net_mask,
			*net_namesvr == '\0' ? "<none>" : net_namesvr,
			*net_defroute == '\0' ? "<none>" : net_defroute,
			*net_media == '\0' ? "<default>" : net_media);
	else
		msg_display(MSG_netok, net_domain, net_host, net_dev,
			*net_ip == '\0' ? "<none>" : net_ip,
			*net_mask == '\0' ? "<none>" : net_mask,
			*net_namesvr == '\0' ? "<none>" : net_namesvr,
			*net_defroute == '\0' ? "<none>" : net_defroute,
			*net_media == '\0' ? "<default>" : net_media);
#ifdef INET6
	msg_display_add(MSG_netokv6,
		     !is_v6kernel() ? "<not supported>" :
			(v6config ? "yes" : "no"),
		     *net_namesvr6 == '\0' ? "<none>" : net_namesvr6);
#endif
done:
	process_menu(MENU_yesno, deconst(MSG_netok_ok));
	if (!yesno)
		msg_display(MSG_netagain);
	if (!yesno)
		goto again;

	/*
	 * we may want to perform checks against inconsistent configuration,
	 * like IPv4 DNS server without IPv4 configuration.
	 */

	/* Create /etc/resolv.conf if a nameserver was given */
	if (net_namesvr[0] != '\0'
#ifdef INET6
	    || net_namesvr6[0] != '\0'
#endif
		) {
		f = fopen("/etc/resolv.conf", "w");
		if (f == NULL) {
			if (logging)
				(void)fprintf(logfp,
				    "%s", msg_string(MSG_resolv));
			(void)fprintf(stderr, "%s", msg_string(MSG_resolv));
			exit(1);
		}
		scripting_fprintf(NULL, "cat <<EOF >/etc/resolv.conf\n");
		time(&now);
		/* NB: ctime() returns a string ending in  '\n' */
		scripting_fprintf(f, ";\n; BIND data file\n; %s %s;\n",
		    "Created by NetBSD sysinst on", ctime(&now));
		if (net_domain[0] != '\0')
			scripting_fprintf(f, "search %s\n", net_domain);
		if (net_namesvr[0] != '\0')
			scripting_fprintf(f, "nameserver %s\n", net_namesvr);
#ifdef INET6
		if (net_namesvr6[0] != '\0')
			scripting_fprintf(f, "nameserver %s\n", net_namesvr6);
#endif
		scripting_fprintf(NULL, "EOF\n");
		fflush(NULL);
		fclose(f);
	}

	run_program(0, "/sbin/ifconfig lo0 127.0.0.1");

#ifdef INET6
	if (v6config && !nfs_root) {
		init_v6kernel(1);
		run_program(0, "/sbin/ifconfig %s up", net_dev);
		sleep(get_v6wait() + 1);
		run_program(RUN_DISPLAY, "/sbin/rtsol -D %s", net_dev);
		sleep(get_v6wait() + 1);
	}
#endif

	if (net_ip[0] != '\0') {
		if (slip) {
			/* XXX: needs 'ifconfig sl0 create' much earlier */
			/* Set SLIP interface UP */
			run_program(0, "/sbin/ifconfig %s inet %s %s up",
			    net_dev, net_ip, net_srv_ip);
			strcpy(sl_flags, "-s 115200 -l /dev/tty00");
			msg_prompt_win(MSG_slattach, -1, 12, 70, 0,
				sl_flags, sl_flags, 255);

			/* XXX: wtf isn't run_program() used here? */
			pid = fork();
			if (pid == 0) {
				strcpy(buffer, "/sbin/slattach ");
				strcat(buffer, sl_flags);
				in_buf = buffer;

				for (ap = slcmd; (*ap = strsep(&in_buf, " ")) != NULL;)
				if (**ap != '\0')
					++ap;

				execvp(slcmd[0], slcmd);
			} else
				wait4(pid, &status, WNOHANG, 0);
		} else if (!nfs_root) {
			if (net_mask[0] != '\0') {
				run_program(0, "/sbin/ifconfig %s inet %s netmask %s",
				    net_dev, net_ip, net_mask);
			} else {
				run_program(0, "/sbin/ifconfig %s inet %s",
			    	net_dev, net_ip);
			}
		}
	}

	/* Set host name */
	if (net_host[0] != '\0')
	  	sethostname(net_host, strlen(net_host));

	/* Set a default route if one was given */
	if (!nfs_root && net_defroute[0] != '\0') {
		run_program(RUN_DISPLAY | RUN_PROGRESS,
				"/sbin/route -n flush -inet");
		run_program(RUN_DISPLAY | RUN_PROGRESS,
				"/sbin/route -n add default %s", net_defroute);
	}

	/*
	 * wait a couple of seconds for the interface to go live.
	 */
	if (!nfs_root) {
		msg_display_add(MSG_wait_network);
		sleep(5);
	}

	/*
	 * ping should be verbose, so users can see the cause
	 * of a network failure.
	 */

#ifdef INET6
	if (v6config && network_up) {
		network_up = !run_program(RUN_DISPLAY | RUN_PROGRESS, 
		    "/sbin/ping6 -v -c 3 -n -I %s ff02::2", net_dev);

		if (net_namesvr6[0] != '\0')
			network_up = !run_program(RUN_DISPLAY | RUN_PROGRESS, 
			    "/sbin/ping6 -v -c 3 -n %s", net_namesvr6);
	}
#endif

	if (net_namesvr[0] != '\0' && network_up)
		network_up = !run_program(RUN_DISPLAY | RUN_PROGRESS, 
		    "/sbin/ping -v -c 5 -w 5 -o -n %s", net_namesvr);

	if (net_defroute[0] != '\0' && network_up)
		network_up = !run_program(RUN_DISPLAY | RUN_PROGRESS, 
		    "/sbin/ping -v -c 5 -w 5 -o -n %s", net_defroute);
	fflush(NULL);

	return network_up;
}

static int
ftp_fetch(const char *set_name)
{
	const char *ftp_opt;
	char ftp_user_encoded[STRSIZE];
	char ftp_dir_encoded[STRSIZE];
	char *cp, *set_dir2;
	int rval;

	/*
	 * Invoke ftp to fetch the file.
	 *
	 * ftp.pass is quite likely to contain unsafe characters
	 * that need to be encoded in the URL (for example,
	 * "@", ":" and "/" need quoting).  Let's be
	 * paranoid and also encode ftp.user and ftp.dir.  (For
	 * example, ftp.dir could easily contain '~', which is
	 * unsafe by a strict reading of RFC 1738).
	 */
	if (strcmp("ftp", ftp.user) == 0 && ftp.pass[0] == 0) {
		/* do anon ftp */
		ftp_opt = "-a ";
		ftp_user_encoded[0] = 0;
	} else {
		ftp_opt = "";
		cp = url_encode(ftp_user_encoded, ftp.user,
			ftp_user_encoded + sizeof ftp_user_encoded - 1,
			RFC1738_SAFE_LESS_SHELL, 0);
		*cp++ = ':';
		cp = url_encode(cp, ftp.pass,
			ftp_user_encoded + sizeof ftp_user_encoded - 1,
			NULL, 0);
		*cp++ = '@';
		*cp = 0;
	}

	cp = url_encode(ftp_dir_encoded, ftp.dir,
			ftp_dir_encoded + sizeof ftp_dir_encoded - 1,
			RFC1738_SAFE_LESS_SHELL_PLUS_SLASH, 1);
	if (cp != ftp_dir_encoded && cp[-1] != '/')
		*cp++ = '/';

	set_dir2 = set_dir;
	while (*set_dir2 == '/')
		++set_dir2;

	url_encode(cp, set_dir2,
			ftp_dir_encoded + sizeof ftp_dir_encoded,
			RFC1738_SAFE_LESS_SHELL_PLUS_SLASH, 0);

	rval = run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_XFER_DIR, 
		    "/usr/bin/ftp %s%s://%s%s/%s/%s%s",
		    ftp_opt, ftp.xfer_type, ftp_user_encoded, ftp.host,
		    ftp_dir_encoded, set_name, dist_postfix);

	return rval ? SET_RETRY : SET_OK;
}

static int
do_config_network(void)
{
	int ret;

	while ((ret = config_network()) <= 0) {
		if (ret < 0)
			return (-1);
		msg_display(MSG_netnotup);
		process_menu(MENU_yesno, NULL);
		if (!yesno) {
			msg_display(MSG_netnotup_continueanyway);
			process_menu(MENU_yesno, NULL);
			if (!yesno)
				return -1;
			network_up = 1;
			break;
		}
	}
	return 0;
}

int
get_via_ftp(const char *xfer_type)
{

	if (do_config_network() != 0)
		return SET_RETRY;

	process_menu(MENU_ftpsource, deconst(xfer_type));

	/* We'll fetch each file just before installing it */
	fetch_fn = ftp_fetch;
	ftp.xfer_type = xfer_type;
	snprintf(ext_dir, sizeof ext_dir, "%s/%s", target_prefix(),
	    xfer_dir + (*xfer_dir == '/'));

	return SET_OK;
}

int
get_via_nfs(void)
{
	struct statvfs sb;

	if (do_config_network() != 0)
		return SET_RETRY;

	/* If root is on NFS and we have sets, skip this step. */
	if (statvfs(set_dir, &sb) == 0 &&
	    strcmp(sb.f_fstypename, "nfs") == 0) {
	    	strlcpy(ext_dir, set_dir, sizeof ext_dir);
		return SET_OK;
	}

	/* Get server and filepath */
	process_menu(MENU_nfssource, NULL);

	/* Mount it */
	if (run_program(0, "/sbin/mount -r -o -2,-i,-r=1024 -t nfs %s:%s /mnt2",
	    nfs_host, nfs_dir))
		return SET_RETRY;

	mnt2_mounted = 1;

	snprintf(ext_dir, sizeof ext_dir, "/mnt2/%s", set_dir);

	/* return location, don't clean... */
	return SET_OK;
}

/*
 * write the new contents of /etc/hosts to the specified file
 */
static void
write_etc_hosts(FILE *f)
{
	scripting_fprintf(f, "#\n");
	scripting_fprintf(f, "# Added by NetBSD sysinst\n");
	scripting_fprintf(f, "#\n");

	if (net_domain[0] != '\0')
		scripting_fprintf(f, "127.0.0.1	localhost.%s\n", net_domain);

	scripting_fprintf(f, "%s\t", net_ip);
	if (net_domain[0] != '\0')
		scripting_fprintf(f, "%s ", recombine_host_domain());
	scripting_fprintf(f, "%s\n", net_host);
}

/*
 * Write the network config info the user entered via menus into the
 * config files in the target disk.  Be careful not to lose any
 * information we don't immediately add back, in case the install
 * target is the currently-active root.
 */
void
mnt_net_config(void)
{
	char ifconfig_fn[STRSIZE];
	FILE *ifconf = NULL;

	if (!network_up)
		return;
	process_menu(MENU_yesno, deconst(MSG_mntnetconfig));
	if (!yesno)
		return;

	/* Write hostname to /etc/rc.conf */
	if ((net_dhcpconf & DHCPCONF_HOST) == 0)
		add_rc_conf("hostname=%s\n", recombine_host_domain());

	/* If not running in target, copy resolv.conf there. */
	if ((net_dhcpconf & DHCPCONF_NAMESVR) == 0) {
#ifndef INET6
		if (net_namesvr[0] != '\0')
			dup_file_into_target("/etc/resolv.conf");
#else
		/*
		 * not sure if it is a good idea, to allow dhcp config to
		 * override IPv6 configuration
		 */
		if (net_namesvr[0] != '\0' || net_namesvr6[0] != '\0')
			dup_file_into_target("/etc/resolv.conf");
#endif
	}

	/*
	 * bring the interface up, it will be necessary for IPv6, and
	 * it won't make trouble with IPv4 case either
	 */
	snprintf(ifconfig_fn, sizeof ifconfig_fn, "/etc/ifconfig.%s", net_dev);
	ifconf = target_fopen(ifconfig_fn, "w");
	if (ifconf != NULL) {
		scripting_fprintf(NULL, "cat <<EOF >>%s%s\n",
		    target_prefix(), ifconfig_fn);
		scripting_fprintf(ifconf, "up\n");
		if (*net_media != '\0')
			scripting_fprintf(ifconf, "media %s\n", net_media);
		scripting_fprintf(NULL, "EOF\n");
	}

	if ((net_dhcpconf & DHCPCONF_IPADDR) == 0) {
		FILE *hosts;

		/* Write IPaddr and netmask to /etc/ifconfig.if[0-9] */
		if (ifconf != NULL) {
			scripting_fprintf(NULL, "cat <<EOF >>%s%s\n",
			    target_prefix(), ifconfig_fn);
			if (*net_media != '\0')
				scripting_fprintf(ifconf,
				    "%s netmask %s media %s\n",
				    net_ip, net_mask, net_media);
			else
				scripting_fprintf(ifconf, "%s netmask %s\n",
				    net_ip, net_mask);
			scripting_fprintf(NULL, "EOF\n");
		}

		/*
		 * Add IPaddr/hostname to  /etc/hosts.
		 * Be careful not to clobber any existing contents.
		 * Relies on ordered search of /etc/hosts. XXX YP?
		 */
		hosts = target_fopen("/etc/hosts", "a");
		if (hosts != 0) {
			scripting_fprintf(NULL, "cat <<EOF >>%s/etc/hosts\n",
			    target_prefix());
			write_etc_hosts(hosts);
			(void)fclose(hosts);
			scripting_fprintf(NULL, "EOF\n");

			fclose(hosts);
		}

		add_rc_conf("defaultroute=\"%s\"\n", net_defroute);
	} else {
		add_rc_conf("ifconfig_%s=dhcp\n", net_dev);
        }

#ifdef INET6
	if ((net_ip6conf & IP6CONF_AUTOHOST) != 0) {
		add_rc_conf("ip6mode=autohost\n");
		if (ifconf != NULL) {
			scripting_fprintf(NULL, "cat <<EOF >>%s%s\n",
			    target_prefix(), ifconfig_fn);
			scripting_fprintf(ifconf, "!rtsol $int\n");
			scripting_fprintf(NULL, "EOF\n");
		}
	}
#endif

	if (ifconf)
		fclose(ifconf);

	fflush(NULL);
}

int
config_dhcp(char *inter)
{
	int dhcpautoconf;

	/*
	 * Don't bother checking for an existing instance of dhcpcd, just
	 * ask it to renew the lease.  It will fork and daemonize if there
	 * wasn't already an instance.
	 */

	if (!file_mode_match(DHCPCD, S_IFREG))
		return 0;
	process_menu(MENU_yesno, deconst(MSG_Perform_DHCP_autoconfiguration));
	if (yesno) {
		/* spawn off dhcpcd and wait for parent to exit */
		dhcpautoconf = run_program(RUN_DISPLAY | RUN_PROGRESS,
		    "%s -d -n %s", DHCPCD, inter);
		return dhcpautoconf ? 0 : 1;
	}
	return 0;
}

static void
get_dhcp_value(char *targ, size_t l, const char *var)
{
	static const char *lease_data = "/tmp/dhcpcd-lease";
	FILE *fp;
	char *line;
	size_t len, var_len;

	if ((fp = fopen(lease_data, "r")) == NULL) {
		warn("Could not open %s", lease_data);
		*targ = '\0';
		return;
	}

	var_len = strlen(var);

	while ((line = fgetln(fp, &len)) != NULL) {
		if (line[len - 1] == '\n')
			--len;
		if (len <= var_len)
			continue;
		if (memcmp(line, var, var_len))
			continue;
		if (line[var_len] != '=')
			continue;
		line += var_len + 1;
		len -= var_len + 1;
		strlcpy(targ, line, l > len ? len + 1: l);
		break;
	}

	fclose(fp);
}
