/* $NetBSD: configmenu.c,v 1.2.2.2 2012/04/17 00:02:49 yamt Exp $ */

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
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"


static int set_network(struct menudesc*, void *);
static int set_timezone_menu(struct menudesc *, void *);
static int set_root_shell(struct menudesc *, void *);
static int change_root_password(struct menudesc *, void *);
static int set_binpkg(struct menudesc *, void *);
static int set_pkgsrc(struct menudesc *, void *);
static void config_list_init(void);
static void get_rootsh(void);
static int toggle_rcvar(struct menudesc *, void *);
static void configmenu_hdr(struct menudesc *, void *);
static int check_root_password(void);

char pkgpath[STRSIZE];
char pkgsrcpath[STRSIZE];

extern const char *tz_default;

enum {
	CONFIGOPT_NETCONF,
	CONFIGOPT_TZ,
	CONFIGOPT_ROOTSH,
	CONFIGOPT_ROOTPW,
	CONFIGOPT_BINPKG,
	CONFIGOPT_PKGSRC,
	CONFIGOPT_SSHD,
	CONFIGOPT_NTPD,
	CONFIGOPT_NTPDATE,
	CONFIGOPT_MDNSD,
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
	{MSG_Root_shell, CONFIGOPT_ROOTSH, NULL, set_root_shell, NULL},
	{MSG_change_rootpw, CONFIGOPT_ROOTPW, NULL, change_root_password, MSG_change},
	{MSG_enable_binpkg, CONFIGOPT_BINPKG, NULL, set_binpkg, MSG_configure},
	{MSG_get_pkgsrc, CONFIGOPT_PKGSRC, NULL, set_pkgsrc, MSG_install},
	{MSG_enable_sshd, CONFIGOPT_SSHD, "sshd", toggle_rcvar, NULL},
	{MSG_enable_ntpd, CONFIGOPT_NTPD, "ntpd", toggle_rcvar, NULL},
	{MSG_run_ntpdate, CONFIGOPT_NTPDATE, "ntpdate", toggle_rcvar, NULL},
	{MSG_enable_mdnsd, CONFIGOPT_MDNSD, "mdnsd", toggle_rcvar, NULL},
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

	wprintw(menu->mw, "%-50s %-10s", msg_string(optname), setting);
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
		*ce = conf;
		me->opt_menu = OPT_NOMENU;
		me->opt_flags = 0;
		me->opt_name = NULL;  /* NULL so set_config will draw */
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
	run_program(RUN_PROGRESS | RUN_CHROOT,
		"chpass -s %s root", confp[menu->cursel]->setting);
	return 0;
}

static int
set_network(struct menudesc *menu, void *arg)
{
	network_up = 0;
	if (config_network())
		mnt_net_config();
	return 0;
}

static int
check_root_password(void)
{
	char *buf;
	int rval;

	collect(T_OUTPUT, &buf, "chroot %s getent passwd root | chroot %s cut -d: -f2",
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

static int
change_root_password(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;

	msg_display(MSG_rootpw);
	process_menu(MENU_yesno, NULL);
	if (yesno)
		run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
			"passwd -l root");
	confp[menu->cursel]->setting = MSG_password_set;
	return 0;
}

static int
set_binpkg(struct menudesc *menu, void *arg)
{
	configinfo **confp = arg;

	char pattern[STRSIZE];

	process_menu(MENU_binpkg, NULL);
	make_url(pkgpath, &pkg, pkg_dir);
	if ( run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
		"pkg_add %s/pkgin", pkgpath) != 0) {
		msg_display(MSG_pkgin_failed);
		process_menu(MENU_ok, NULL);
		confp[menu->cursel]->setting = MSG_failed;
		return 0;
	}

	/* configure pkgin to use $pkgpath as a repository */
	snprintf(pattern, STRSIZE, "s,^[^#].*$,%s,", pkgpath);
	replace("/usr/pkg/etc/pkgin/repositories.conf", pattern);

	run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
		"/usr/pkg/bin/pkgin update");

	msg_display(MSG_binpkg_installed);
	process_menu(MENU_ok, NULL);
	
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
		process_menu(MENU_yesno, deconst(MSG_retry_pkgsrc_network));
		if (!yesno) {
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
		msg_display(MSG_rcconf_delete_failed, varname);
		process_menu(MENU_ok, NULL);
		return -1;
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
		replace("/etc/rc.conf", pattern);
	}

	return 0;
}

static void
configmenu_hdr(struct menudesc *menu, void *arg)
{
	msg_display(MSG_configmenu);
}

void
do_configmenu()
{
	int		menu_no;
	int		opts;
	menu_ent	me[CONFIGOPT_LAST];
	configinfo	*ce[CONFIGOPT_LAST];

        wrefresh(curscr);
        wmove(stdscr, 0, 0);
        wclear(stdscr);
        wrefresh(stdscr);
	
	/* if the target isn't mounted already, figure it out. */
	if (target_mounted() == 0) {
		if (find_disks(msg_string(MSG_configure_prior)) < 0)
			return;

		if (mount_disks() != 0)
			return;
	}

	config_list_init();
	make_url(pkgpath, &pkg, pkg_dir);
	opts = init_config_menu(config_list, me, ce);

	menu_no = new_menu(NULL, me, opts, 0, -4, 0, 70,
		MC_SCROLL | MC_NOBOX | MC_DFLTEXIT,
		configmenu_hdr, set_config, NULL, "XXX Help String",
		MSG_doneconfig);

	process_menu(menu_no, ce);
	free_menu(menu_no);

	sanity_check();

}
