/* $NetBSD: configmenu.c,v 1.23 2026/04/27 18:20:02 martin Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeffrey C. Rizzo
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* configmenu.c -- post-installation system configuration menu. */

#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"


static int set_network(struct menudesc*, void *);
static int set_timezone_menu(struct menudesc *, void *);
static int set_root_shell(struct menudesc *, void *);
static int change_root_password(struct menudesc *, void *);
static int add_new_user(struct menudesc *, void *);
#if CHECK_ENTROPY
static int add_entropy(struct menudesc *, void *);
#endif
static int set_binpkg(struct menudesc *, void *);
static int set_pkgsrc(struct menudesc *, void *);
static void config_list_init(void);
static void get_rootsh(void);
static int toggle_rcvar(struct menudesc *, void *);
static int toggle_mdnsd(struct menudesc *, void *);
static void configmenu_hdr(struct menudesc *, void *);
static int check_root_password(void);
static int run_bin_sh(struct menudesc *, void *);
static int time_and_date_setup(struct menudesc *menu, void *arg);
static void time_and_date_hdr(struct menudesc *, void *);

char pkgpath[STRSIZE];
char pkgsrcpath[STRSIZE];

extern const char *tz_default;
char cur_date_time[28];
static const char generic_date_time_fmt[] = "%a, %d %b %Y %X";

enum {
	CONFIGOPT_NETCONF,
	CONFIGOPT_TZ,
	CONFIGOPT_SETDATETIME,
	CONFIGOPT_ROOTSH,
	CONFIGOPT_ROOTPW,
	CONFIGOPT_BINPKG,
	CONFIGOPT_PKGSRC,
	CONFIGOPT_SSHD,
	CONFIGOPT_NTPD,
	CONFIGOPT_NTPDATE,
	CONFIGOPT_MDNSD,
	CONFIGOPT_XDM,
	CONFIGOPT_CGD,
	CONFIGOPT_LVM,
	CONFIGOPT_RAIDFRAME,
	CONFIGOPT_ADDUSER,
	CONFIGOPT_ADD_ENTROPY,
	CONFIGOPT_RUN_SH,
	CONFIGOPT_LAST
};

typedef struct configinfo {
	const char	*optname;
	uint		opt;
	const char	*rcvar;
	int		(*action)(struct menudesc *, void *);
	const char	*setting;
} configinfo;


configinfo config_list[] = {
	{MSG_Configure_network, CONFIGOPT_NETCONF, NULL, set_network, MSG_configure},
	{MSG_timezone, CONFIGOPT_TZ, NULL, set_timezone_menu, NULL},
	{MSG_setdatetime, CONFIGOPT_SETDATETIME, NULL, time_and_date_setup, NULL},
	{MSG_Root_shell, CONFIGOPT_ROOTSH, NULL, set_root_shell, NULL},
	{MSG_change_rootpw, CONFIGOPT_ROOTPW, NULL, change_root_password, MSG_change},
	{MSG_enable_binpkg, CONFIGOPT_BINPKG, NULL, set_binpkg, MSG_install},
	{MSG_get_pkgsrc, CONFIGOPT_PKGSRC, NULL, set_pkgsrc, MSG_install},
	{MSG_enable_sshd, CONFIGOPT_SSHD, "sshd", toggle_rcvar, NULL},
	{MSG_enable_ntpd, CONFIGOPT_NTPD, "ntpd", toggle_rcvar, NULL},
	{MSG_run_ntpdate, CONFIGOPT_NTPDATE, "ntpdate", toggle_rcvar, NULL},
	{MSG_enable_mdnsd, CONFIGOPT_MDNSD, "mdnsd", toggle_mdnsd, NULL},
	{MSG_enable_xdm, CONFIGOPT_XDM, "xdm", toggle_rcvar, NULL},
	{MSG_enable_cgd, CONFIGOPT_CGD, "cgd", toggle_rcvar, NULL},
	{MSG_enable_lvm, CONFIGOPT_LVM, "lvm", toggle_rcvar, NULL},
	{MSG_enable_raid, CONFIGOPT_RAIDFRAME, "raidframe", toggle_rcvar, NULL},
	{MSG_add_a_user, CONFIGOPT_ADDUSER, NULL, add_new_user, ""},
#if CHECK_ENTROPY
	{MSG_Configure_entropy, CONFIGOPT_ADD_ENTROPY, NULL, add_entropy, ""},
#endif
	{MSG_Run_bin_sh, CONFIGOPT_RUN_SH, NULL, run_bin_sh, ""},
	{NULL,		CONFIGOPT_LAST,	NULL, NULL, NULL}
};

static void
config_list_init(void)
{
	int i;

	for (i=0; i < CONFIGOPT_LAST; i++) {
		switch (i) {
		case CONFIGOPT_TZ:
			get_tz_default();
			config_list[CONFIGOPT_TZ].setting = tz_default;
			break;
		case CONFIGOPT_SETDATETIME:
			{
				time_t now = time(NULL);
				struct tm *lt = localtime(&now);
				strftime(cur_date_time, sizeof(cur_date_time),
				     generic_date_time_fmt, lt);
				config_list[CONFIGOPT_SETDATETIME].setting =
				    cur_date_time;
				break; 
			}
		case CONFIGOPT_ROOTSH:
			get_rootsh();
			break;
		case CONFIGOPT_ROOTPW:
			if (check_root_password())
				config_list[i].setting = MSG_password_set;
			else
				config_list[i].setting = MSG_empty;
			break;
		default:
			if (config_list[i].rcvar != NULL) {
				if (check_rcvar(config_list[i].rcvar))
					config_list[i].setting = MSG_YES;
				else
					config_list[i].setting = MSG_NO;
			}
			break;
		}
	}
}

static void
get_rootsh(void)
{
	static char *buf = NULL;

	if (buf != NULL)
		free(buf);

	if (target_already_root())
		collect(T_OUTPUT, &buf,
		    "/usr/bin/awk -F: '$1==\"root\" { print $NF; exit }'"
		    " /etc/passwd");
	else
		collect(T_OUTPUT, &buf,
		    "chroot %s /usr/bin/awk -F: '$1==\"root\" { print $NF; exit }'"
		    " /etc/passwd",target_prefix());

	config_list[CONFIGOPT_ROOTSH].setting = (const char *)buf;
}

static void
set_config(menudesc *menu, int opt, void *arg)
{
	configinfo	**configp = arg;
	configinfo	*config = configp[opt];
	const char	*optname, *setting;

	optname = config->optname;
	setting = msg_string(config->setting);

	wprintw(menu->mw, opt == CONFIGOPT_SETDATETIME ?
	    "%-30s %40s" : "%-50s %-10s", msg_string(optname), setting);
}

static int
init_config_menu(configinfo *conf, menu_ent *me, configinfo **ce)
{
	int	opt;
	int	configopts;

	for (configopts = 0; ; conf++) {
		opt = conf->opt;
		if (opt == CONFIGOPT_LAST)
			break;
#if CHECK_ENTROPY
		if (opt == CONFIGOPT_ADD_ENTROPY && entropy_needed() == 0)
			continue;
#endif
		*ce = conf;
		memset(me, 0, sizeof(*me));
		me->opt_action = conf->action;
		configopts++;
		ce++;
		me++;
	}

	return configopts;
}

static int
/*ARGSUSED*/
set_timezone_menu(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;
	set_timezone();
	get_tz_default();
	confp[menu->cursel]->setting = tz_default;
	return 0;
}

static int
set_root_shell(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;

	process_menu(MENU_rootsh, &confp[menu->cursel]->setting);
	if (run_program(RUN_PROGRESS | RUN_CHROOT,
		"chpass -s %s root", confp[menu->cursel]->setting) != 0)
		confp[menu->cursel]->setting = MSG_failed;
	return 0;
}

static int
set_network(struct menudesc *menu, void *arg)
{
	network_up = 0;
	if (config_network(1))
		mnt_net_config();
	return 0;
}

static int
check_root_password(void)
{
	char *buf;
	int rval;

	if (target_already_root())
		collect(T_OUTPUT, &buf, "getent passwd root | cut -d: -f2");
	else
		collect(T_OUTPUT, &buf, "chroot %s getent passwd root | "
		    "chroot %s cut -d: -f2",
		    target_prefix(), target_prefix());

	if (logfp)
		fprintf(logfp,"buf %s strlen(buf) %zu\n", buf, strlen(buf));

	if (strlen(buf) <= 1)  /* newline */
		rval = 0;
	else
		rval = 1;
	free(buf);
	return rval;
}

#if CHECK_ENTROPY
static int
add_entropy(struct menudesc *menu, void *arg)
{
	do_add_entropy();
	return 0;
}
#endif

static int
add_new_user(struct menudesc *menu, void *arg)
{
	char username[STRSIZE] = "";
	int inwheel=0;

	msg_prompt(MSG_addusername, NULL, username, sizeof username -1);
	if (strlen(username) == 0)
		return 0;
	inwheel = ask_yesno(MSG_addusertowheel);
	ushell = "/bin/csh";
	process_menu(MENU_usersh, NULL);
	if (inwheel)
		run_program(RUN_PROGRESS | RUN_CHROOT,
		    "/usr/sbin/useradd -m -s %s -G wheel %s",
		    ushell, username);
	else
		run_program(RUN_PROGRESS | RUN_CHROOT,
		    "/usr/sbin/useradd -m -s %s %s", ushell, username);
	run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
	    "passwd -l %s", username);
	return 0;
}

void
root_pw_setup(void)
{
	msg_display(MSG_force_rootpw);
	run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT | RUN_STDSCR,
	    "passwd -l root");
}

static int
change_root_password(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;

	msg_display(MSG_rootpw);
	if (ask_yesno(NULL)) {
		if (run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
			"passwd -l root") == 0)
			confp[menu->cursel]->setting = MSG_password_set;
		else
			confp[menu->cursel]->setting = MSG_failed;
	}
	return 0;
}

static int
set_binpkg(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;
	char additional_pkgs[STRSIZE] = {0};
	int allok = 0;
	arg_rv parm;

	if (config_network(0))
		mnt_net_config();

	do {
		parm.rv = -1;
		parm.arg = additional_pkgs;
		process_menu(MENU_binpkg, &parm);
		if (parm.rv == SET_SKIP) {
			confp[menu->cursel]->setting = MSG_abandoned;
			return 0;
		}

		/*
		 * Make sure we have the TLS certs in a usable state
		 * (if target is a new installation)
		 */
		if (pkg.xfer == XFER_HTTPS)
			run_program(RUN_CHROOT | RUN_SILENT,
			    "/bin/sh /etc/rc.d/certctl_init onestart");

		make_url(pkgpath, &pkg, pkg_dir);
		if (run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
			"pkg_add %s/pkgin", pkgpath) == 0) {
			allok = 1;
		}
	} while (allok == 0);

	/* configure pkgin to use $pkgpath as a repository */
	replace("/usr/pkg/etc/pkgin/repositories.conf", "s,^[^#].*$,%s,",
	    pkgpath);

	run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
		"/usr/pkg/bin/pkgin -y update");

	if (strlen(additional_pkgs) > 0)
		run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
		"/usr/pkg/bin/pkgin -y install %s", additional_pkgs);

	hit_enter_to_continue(MSG_binpkg_installed, NULL);

	confp[menu->cursel]->setting = MSG_DONE;
	return 0;
}

static int
set_pkgsrc(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;
	distinfo dist;

	dist.name = "pkgsrc";
	dist.set = SET_PKGSRC;
	dist.desc = "source for 3rd-party packages";
	dist.marker_file = NULL;

	int status = SET_RETRY;

	do {
		status = get_pkgsrc();
		if (status == SET_OK) {
			status = extract_file(&dist, 0);
			continue;
		} else if (status == SET_SKIP) {
			confp[menu->cursel]->setting = MSG_abandoned;
			return 0;
		}
		if (!ask_yesno(MSG_retry_pkgsrc_network)) {
			confp[menu->cursel]->setting = MSG_abandoned;
			return 1;
		}
	}
	while (status == SET_RETRY);

	confp[menu->cursel]->setting = MSG_DONE;
	return 0;
}

static int
toggle_rcvar(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;
	int s;
	const char *setting, *varname;
	char pattern[STRSIZE];
	char buf[STRSIZE];
	char *cp;
	int found = 0;
	FILE *fp;

	varname = confp[menu->cursel]->rcvar;

	s = check_rcvar(varname);

	/* we're toggling, so invert the sense */
	if (s) {
		confp[menu->cursel]->setting = MSG_NO;
		setting = "NO";
	} else {
		confp[menu->cursel]->setting = MSG_YES;
		setting = "YES";
	}

	if (!(fp = fopen(target_expand("/etc/rc.conf"), "r"))) {
		msg_fmt_display(MSG_openfail, "%s%s",
		    target_expand("/etc/rc.conf"), strerror(errno));
		hit_enter_to_continue(NULL, NULL);
		return 0;
	}

	while (fgets(buf, sizeof buf, fp) != NULL) {
		cp = buf + strspn(buf, " \t"); /* Skip initial spaces */
		if (strncmp(cp, varname, strlen(varname)) == 0) {
			cp += strlen(varname);
			if (*cp != '=')
				continue;
			buf[strlen(buf) - 1] = 0;
			snprintf(pattern, sizeof pattern,
					"s,^%s$,%s=%s,",
					buf, varname, setting);
			found = 1;
			break;
		}
	}

	fclose(fp);

	if (!found) {
		add_rc_conf("%s=%s\n", varname, setting);
		if (logfp) {
			fprintf(logfp, "adding %s=%s\n", varname, setting);
			fflush(logfp);
		}
	} else {
		if (logfp) {
			fprintf(logfp, "replacement pattern is %s\n", pattern);
			fflush(logfp);
		}
		replace("/etc/rc.conf", "%s", pattern);
	}

	return 0;
}

static int
toggle_mdnsd(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;
	int s;
	const char *setting, *varname;

	varname = confp[menu->cursel]->rcvar;

	s = check_rcvar(varname);

	/* we're toggling, so invert the sense */
	if (s) {
		confp[menu->cursel]->setting = MSG_NO;
		setting = "files dns";
	} else {
		confp[menu->cursel]->setting = MSG_YES;
		setting = "files multicast_dns dns";
	}

	if (logfp) {
		fprintf(logfp, "setting hosts: %s\n", setting);
		fflush(logfp);
	}
	replace("/etc/nsswitch.conf", "s/^hosts:.*/hosts:\t\t%s/", setting);

	toggle_rcvar(menu, arg);

	return 0;
}

static int
run_bin_sh(struct menudesc *menu, void *arg)
{
	endwin();
	system("/bin/sh -i -E");

	return 0;
}

static int
time_and_date_setup(struct menudesc *menu, void *arg)
{
	do_time_and_date_setup("");
	return 0;
}

static void
configmenu_hdr(struct menudesc *menu, void *arg)
{
	msg_display(MSG_configmenu);
}

void
do_configmenu(struct install_partition_desc *install)
{
	int		menu_no;
	int		opts;
	menu_ent	me[CONFIGOPT_LAST];
	configinfo	*ce[CONFIGOPT_LAST];

	memset(me, 0, sizeof(me));

	/* if the target isn't mounted already, figure it out. */
	if (install != NULL && target_mounted() == 0) {
		partman_go = 0;
		if (find_disks(msg_string(MSG_configure_prior), true) < 0)
			return;

		if (mount_disks(install) != 0)
			return;
	}

	config_list_init();
	make_url(pkgpath, &pkg, pkg_dir);
	opts = init_config_menu(config_list, me, ce);

	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	menu_no = new_menu(NULL, me, opts, 0, -4, 0, 70,
		MC_SCROLL | MC_NOBOX | MC_DFLTEXIT,
		configmenu_hdr, set_config, NULL, NULL,
		MSG_doneconfig);

	process_menu(menu_no, ce);
	free_menu(menu_no);
}

static const msg struct_tm_opt_names[] =
{ MSG_year, MSG_month, MSG_day, MSG_hour, MSG_minute, MSG_time_ok };

struct date_and_time_menu_arg
{
	const char *header;
	struct tm new_time;
};

static void
fill_date_time_val(char *buf, size_t sizeof_buf, int opt,
    const struct date_and_time_menu_arg *time_opts)
{
	switch (opt) {
	case 0:
		strftime(buf, sizeof_buf, "%Y", &time_opts->new_time);
		break;
	case 1:
		strftime(buf, sizeof_buf, "%m", &time_opts->new_time);
		break;
	case 2:
		strftime(buf, sizeof_buf, "%d", &time_opts->new_time);
		break;
	case 3:
		strftime(buf, sizeof_buf, "%H", &time_opts->new_time);
		break;
	case 4:
		strftime(buf, sizeof_buf, "%M", &time_opts->new_time);
		break;
	}
}

static void
update_new_time(menudesc *menu, int opt, void *arg)
{
	char buf[50];

	fill_date_time_val(buf, sizeof(buf), opt, arg);

	wprintw(menu->mw, "%-30s %-10s", msg_string(struct_tm_opt_names[opt]),
	    buf);
}

static int
set_new_time(menudesc *menu, void *arg)
{
	struct date_and_time_menu_arg *ma = arg;
	struct timeval tv;

	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = mktime(&ma->new_time);
	settimeofday(&tv, NULL);

	return 1;
}

static int
time_and_date_update(struct menudesc *menu, void *arg)
{
	struct date_and_time_menu_arg *ma = arg;
	char buf[50];
	struct tm saved_time;

	buf[0] = 0;

	msg_prompt_win(struct_tm_opt_names[menu->cursel], -1, 18, 0, 0,
	    "", buf, sizeof(buf));

	saved_time = ma->new_time;
	switch (menu->cursel) {
	case 0:
		ma->new_time.tm_year = atoi(buf)-1900;
		break;
	case 1:
		ma->new_time.tm_mon = atoi(buf)-1;
		break;
	case 2:
		ma->new_time.tm_mday = atoi(buf);
		break;
	case 3:
		ma->new_time.tm_hour = atoi(buf);
		break;
	case 4:
		ma->new_time.tm_min = atoi(buf);
		break;
	}

	if (mktime(&ma->new_time) == -1)
		ma->new_time = saved_time;

	return 0;
}

static void
time_and_date_hdr(struct menudesc *menuitem, void *arg)
{
	const struct date_and_time_menu_arg *ma = arg;

	strftime(cur_date_time, sizeof(cur_date_time),
	     generic_date_time_fmt, &ma->new_time);
	
	msg_display_subst(MSG_date_and_time_hdr, 2, ma->header, cur_date_time);
}

void
do_time_and_date_setup(const char *header)
{
	int menu_no;
#define NUM_DATE_TIME_ITEMS	6	/* year, month, day, hour, min,
					   save */
	menu_ent me[CONFIGOPT_LAST];
	struct date_and_time_menu_arg menu_arg;
	time_t now;

	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	memset(me, 0, sizeof(me));
	for (size_t i = 0; i < NUM_DATE_TIME_ITEMS-1; i++)
		me[i].opt_action = time_and_date_update;
	me[NUM_DATE_TIME_ITEMS-1].opt_action = set_new_time;
	me[NUM_DATE_TIME_ITEMS-1].opt_flags = OPT_EXIT;

	now = time(NULL);
	menu_arg.header =  msg_string(header);
	menu_arg.new_time = *localtime(&now);

	menu_no = new_menu(NULL, me, NUM_DATE_TIME_ITEMS, 0, -4, 0, 70,
		MC_SCROLL | MC_NOBOX | MC_DFLTEXIT,
		time_and_date_hdr, update_new_time, NULL, NULL,
		MSG_cancel);

	process_menu(menu_no, &menu_arg);
	free_menu(menu_no);
}
