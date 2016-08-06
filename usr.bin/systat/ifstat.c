/*	$NetBSD: ifstat.c,v 1.4.2.2 2016/08/06 00:19:12 pgoyette Exp $	*/

/*
 * Copyright (c) 2003, Trent Nelson, <trent@arpa.com>.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTIFSTAT_ERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: releng/10.1/usr.bin/systat/ifstat.c 247037 2013-02-20 14:19:09Z melifaro $
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ifstat.c,v 1.4.2.2 2016/08/06 00:19:12 pgoyette Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <net/if.h>

#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>

#include "systat.h"
#include "extern.h"
#include "convtbl.h"

				/* Column numbers */

#define C1	0		/*  0-19 */
#define C2	20		/* 20-39 */
#define C3	40		/* 40-59 */
#define C4	60		/* 60-80 */
#define C5	80		/* Used for label positioning. */

static const int col2 = C2;
static const int col3 = C3;
static const int col4 = C4;

SLIST_HEAD(, if_stat)		curlist;

struct if_stat {
	SLIST_ENTRY(if_stat)	 link;
  	char	if_name[IF_NAMESIZE];
	struct  ifdatareq if_mib;
	struct	timeval tv;
	struct	timeval tv_lastchanged;
	u_long	if_in_curtraffic;
	u_long	if_out_curtraffic;
	u_long	if_in_traffic_peak;
	u_long	if_out_traffic_peak;
	u_long	if_in_curpps;
	u_long	if_out_curpps;
	u_long	if_in_pps_peak;
	u_long	if_out_pps_peak;
	u_int	if_row;			/* Index into ifmib sysctl */
	u_int	if_ypos;		/* 0 if not being displayed */
	u_int	display;
	u_int	match;
};

extern	 int curscale;
extern	 char *matchline;
extern	 int showpps;
extern	 int needsort;

static	 int needclear = 0;

static	 void  right_align_string(struct if_stat *);
static	 void  getifmibdata(const int, struct ifdatareq *);
static	 void  sort_interface_list(void);
static	 u_int getifnum(void);

#define IFSTAT_ERR(n, s)	do {					\
	putchar('\014');						\
	closeifstat(wnd);						\
	err((n), (s));							\
} while (0)

#define TOPLINE 5
#define TOPLABEL \
"      Interface           Traffic               Peak                Total"

#define STARTING_ROW	(TOPLINE + 1)
#define ROW_SPACING	(3)

#define IN_col2		(showpps ? ifp->if_in_curpps : ifp->if_in_curtraffic)
#define OUT_col2	(showpps ? ifp->if_out_curpps : ifp->if_out_curtraffic)
#define IN_col3		(showpps ? \
		ifp->if_in_pps_peak : ifp->if_in_traffic_peak)
#define OUT_col3	(showpps ? \
		ifp->if_out_pps_peak : ifp->if_out_traffic_peak)
#define IN_col4		(showpps ?				\
	ifp->if_mib.ifdr_data.ifi_ipackets : ifp->if_mib.ifdr_data.ifi_ibytes)
#define OUT_col4	(showpps ?					\
	ifp->if_mib.ifdr_data.ifi_opackets : ifp->if_mib.ifdr_data.ifi_obytes)

#define EMPTY_COLUMN 	"                    "
#define CLEAR_COLUMN(y, x)	mvprintw((y), (x), "%20s", EMPTY_COLUMN);

#define DOPUTRATE(c, r, d)	do {					\
	CLEAR_COLUMN(r, c);						\
	if (showpps) {							\
		mvprintw(r, (c), "%10.3f %cp%s  ",			\
			 convert(d##_##c, curscale),			\
			 *get_string(d##_##c, curscale),		\
			 "/s");						\
	}								\
	else {								\
		mvprintw(r, (c), "%10.3f %s%s  ",			\
			 convert(d##_##c, curscale),			\
			 get_string(d##_##c, curscale),			\
			 "/s");						\
	}								\
} while (0)

#define DOPUTTOTAL(c, r, d)	do {					\
	CLEAR_COLUMN((r), (c));						\
	if (showpps) {							\
		mvprintw((r), (c), "%12.3f %cp  ",			\
			 convert(d##_##c, SC_AUTO),			\
			 *get_string(d##_##c, SC_AUTO));		\
	}								\
	else {								\
		mvprintw((r), (c), "%12.3f %s  ",			\
			 convert(d##_##c, SC_AUTO),			\
			 get_string(d##_##c, SC_AUTO));			\
	}								\
} while (0)

#define PUTRATE(c, r)	do {						\
	DOPUTRATE(c, (r), IN);						\
	DOPUTRATE(c, (r)+1, OUT);					\
} while (0)

#define PUTTOTAL(c, r)	do {						\
	DOPUTTOTAL(c, (r), IN);						\
	DOPUTTOTAL(c, (r)+1, OUT);					\
} while (0)

#define PUTNAME(p) do {							\
	mvprintw(p->if_ypos, 0, "%s", p->if_name);			\
	mvprintw(p->if_ypos, col2-3, "%s", (const char *)"in");		\
	mvprintw(p->if_ypos+1, col2-3, "%s", (const char *)"out");	\
} while (0)

WINDOW *
openifstat(void)
{
	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closeifstat(WINDOW *w)
{
	struct if_stat	*node = NULL;

	while (!SLIST_EMPTY(&curlist)) {
		node = SLIST_FIRST(&curlist);
		SLIST_REMOVE_HEAD(&curlist, link);
		free(node);
	}

	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}

	return;
}

void
labelifstat(void)
{

	wmove(wnd, TOPLINE, 0);
	wclrtoeol(wnd);
	mvprintw(TOPLINE, 0, "%s", TOPLABEL);

	return;
}

void
showifstat(void)
{
	struct	if_stat *ifp = NULL;
	
	SLIST_FOREACH(ifp, &curlist, link) {
		if (ifp->display == 0 || (ifp->match == 0) ||
		    ifp->if_ypos > (u_int)(LINES - 3 - 1))
			continue;
		PUTNAME(ifp);
		PUTRATE(col2, ifp->if_ypos);
		PUTRATE(col3, ifp->if_ypos);
		PUTTOTAL(col4, ifp->if_ypos);
	}

	return;
}

int
initifstat(void)
{
	struct   if_stat *p = NULL;
	u_int	 n = 0, i = 0;

	n = getifnum();
	if (n <= 0)
		return (-1);

	SLIST_INIT(&curlist);

	for (i = 0; i < n; i++) {
		p = (struct if_stat *)calloc(1, sizeof(struct if_stat));
		if (p == NULL)
			IFSTAT_ERR(1, "out of memory");
		SLIST_INSERT_HEAD(&curlist, p, link);
		p->if_row = i+1;
		getifmibdata(p->if_row, &p->if_mib);
		right_align_string(p);
		p->match = 1;

		/*
		 * Initially, we only display interfaces that have
		 * received some traffic.
		 */
		if (p->if_mib.ifdr_data.ifi_ibytes != 0)
			p->display = 1;
	}

	sort_interface_list();

	return (1);
}

void
fetchifstat(void)
{
	struct	if_stat *ifp = NULL;
	struct	timeval tv, new_tv, old_tv;
	double	elapsed = 0.0;
	u_int	new_inb, new_outb, old_inb, old_outb = 0;
	u_int	new_inp, new_outp, old_inp, old_outp = 0;

	SLIST_FOREACH(ifp, &curlist, link) {
		/*
		 * Grab a copy of the old input/output values before we
		 * call getifmibdata().
		 */
		old_inb = ifp->if_mib.ifdr_data.ifi_ibytes;
		old_outb = ifp->if_mib.ifdr_data.ifi_obytes;
		old_inp = ifp->if_mib.ifdr_data.ifi_ipackets;
		old_outp = ifp->if_mib.ifdr_data.ifi_opackets;
		TIMESPEC_TO_TIMEVAL(&ifp->tv_lastchanged, &ifp->if_mib.ifdr_data.ifi_lastchange);

		(void)gettimeofday(&new_tv, NULL);
		(void)getifmibdata(ifp->if_row, &ifp->if_mib);

		new_inb = ifp->if_mib.ifdr_data.ifi_ibytes;
		new_outb = ifp->if_mib.ifdr_data.ifi_obytes;
		new_inp = ifp->if_mib.ifdr_data.ifi_ipackets;
		new_outp = ifp->if_mib.ifdr_data.ifi_opackets;

		/* Display interface if it's received some traffic. */
		if (new_inb > 0 && old_inb == 0) {
			ifp->display = 1;
			needsort = 1;
		}

		/*
		 * The rest is pretty trivial.  Calculate the new values
		 * for our current traffic rates, and while we're there,
		 * see if we have new peak rates.
		 */
		old_tv = ifp->tv;
		timersub(&new_tv, &old_tv, &tv);
		elapsed = tv.tv_sec + (tv.tv_usec * 1e-6);

		ifp->if_in_curtraffic = new_inb - old_inb;
		ifp->if_out_curtraffic = new_outb - old_outb;

		ifp->if_in_curpps = new_inp - old_inp;
		ifp->if_out_curpps = new_outp - old_outp;

		/*
		 * Rather than divide by the time specified on the comm-
		 * and line, we divide by ``elapsed'' as this is likely
		 * to be more accurate.
		 */
		ifp->if_in_curtraffic /= elapsed;
		ifp->if_out_curtraffic /= elapsed;
		ifp->if_in_curpps /= elapsed;
		ifp->if_out_curpps /= elapsed;

		if (ifp->if_in_curtraffic > ifp->if_in_traffic_peak)
			ifp->if_in_traffic_peak = ifp->if_in_curtraffic;

		if (ifp->if_out_curtraffic > ifp->if_out_traffic_peak)
			ifp->if_out_traffic_peak = ifp->if_out_curtraffic;

		if (ifp->if_in_curpps > ifp->if_in_pps_peak)
			ifp->if_in_pps_peak = ifp->if_in_curpps;

		if (ifp->if_out_curpps > ifp->if_out_pps_peak)
			ifp->if_out_pps_peak = ifp->if_out_curpps;

		ifp->tv.tv_sec = new_tv.tv_sec;
		ifp->tv.tv_usec = new_tv.tv_usec;

	}

	if (needsort)
		sort_interface_list();

	return;
}

/*
 * We want to right justify our interface names against the first column
 * (first sixteen or so characters), so we need to do some alignment.
 */
static void
right_align_string(struct if_stat *ifp)
{
	if (ifp == NULL || ifp->if_mib.ifdr_name[0] == '\0')
		return;

	snprintf(ifp->if_name, IF_NAMESIZE, "%*s", IF_NAMESIZE - 1,
	    ifp->if_mib.ifdr_name);
}

static int
check_match(const char *ifname) 
{
	char *p = matchline, *c, t;
	int match = 0, mlen;
	
	if (matchline == NULL)
		return (0);

	/* Strip leading whitespaces */
	while (*p == ' ')
		p ++;

	c = p;
	while ((mlen = strcspn(c, " ;,")) != 0) {
		p = c + mlen;
		t = *p;
		if (p - c > 0) {
			*p = '\0';
			if (fnmatch(c, ifname, FNM_CASEFOLD) == 0) {
				*p = t;
				return (1);
			}
			*p = t;
			c = p + strspn(p, " ;,");
		}
		else {
			c = p + strspn(p, " ;,");
		}
	}

	return (match);
}

/*
 * This function iterates through our list of interfaces, identifying
 * those that are to be displayed (ifp->display = 1).  For each interf-
 * rface that we're displaying, we generate an appropriate position for
 * it on the screen (ifp->if_ypos).
 *
 * This function is called any time a change is made to an interface's
 * ``display'' state.
 */
void
sort_interface_list(void)
{
	struct	if_stat	*ifp = NULL;
	u_int	y = STARTING_ROW;
	
	SLIST_FOREACH(ifp, &curlist, link) {
		if (matchline && !check_match(ifp->if_mib.ifdr_name))
			ifp->match = 0;
		else
			ifp->match = 1;
		if (ifp->display && ifp->match) {
			ifp->if_ypos = y;
			y += ROW_SPACING;
		}
	}
	
	needsort = 0;
	needclear = 1;
}

static
unsigned int
getifnum(void)
{
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifa = NULL;
	int num = 0;

	if (getifaddrs(&ifaddrs) == 0) {
	  for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
	    if (ifa->ifa_addr &&
		ifa->ifa_addr->sa_family == AF_LINK)
	      num++;
	  }
  
	  freeifaddrs(ifaddrs);
	}

	return num;
}

static void
getifmibdata(int row, struct ifdatareq *data)
{
	struct ifaddrs *ifaddrs = NULL;
	struct ifaddrs *ifa = NULL;
	int found = 0;
	int num = 0;

	if (getifaddrs(&ifaddrs) != 0) {
	  IFSTAT_ERR(2, "getifmibdata() error getting interface data");
	  return;
	}

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
	  if (ifa->ifa_addr &&
	      ifa->ifa_addr->sa_family == AF_LINK) {
	    num++;

	    /*
	     * expecting rows to start with 1 not 0, 
	     * see freebsd "man ifmib"
	     */
	    if (num == row) {
	      found = 1;
	      data->ifdr_data = *(struct if_data *)ifa->ifa_data;
	      strncpy(data->ifdr_name, ifa->ifa_name, IF_NAMESIZE);
	      break;
	    }
	  }
	}

	freeifaddrs(ifaddrs);

	if (!found) {
	  IFSTAT_ERR(2, "getifmibdata() error finding row");
	}
}

int
cmdifstat(const char *cmd, const char *args)
{
	int	retval = 0;

	retval = ifcmd(cmd, args);
	/* ifcmd() returns 1 on success */
	if (retval == 1) {
		showifstat();
		refresh();
		if (needclear) {
			werase(wnd);
			labelifstat();
			needclear = 0;
		}
	}

	return (retval);
}

void
ifstat_scale(char* args)
{
	cmdifstat("scale", args ? args : "");
}

void
ifstat_pps(char* args)
{
	cmdifstat("pps", "");
}

void
ifstat_match(char* args)
{
	cmdifstat("match", args ? args : "");

	/*
	 * force erase after match command because it is possible for
	 * another command to be sent in the interval before the window
	 * finishes redrawing completely.  That stale data remains in window
	 * and never gets overwritten because there are fewer interfaces
	 * being drawn on screen.  Only an issue for match command because
	 * pps and scale don't change the number of interfaces being drawn.
	 */
	werase(wnd);
	labelifstat();
}
