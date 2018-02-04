/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b daemon - config file processing
 *	-----------------------------------
 *
 *	$Id: rc_config.c,v 1.27 2018/02/04 09:01:12 mrg Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Sat Jan  6 12:57:36 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/callout.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

#include "isdnd.h"
#include "rc_parse.h"

#include "monitor.h"

extern int lineno;
extern char *yytext;

extern FILE *yyin;
extern int yyparse(void);

static void set_config_defaults(void);
static void check_config(void);
static void print_config(void);
static void parse_valid(char *dt);
static int lookup_l4_driver(const char *name);
void init_currrent_cfg_state(void);
static void set_isppp_auth(struct cfg_entry*);
static void set_autoupdown(struct cfg_entry*);
void flush_config(void);

static int nregexpr = 0;
static int nregprog = 0;
static struct cfg_entry * current_cfe = NULL;
struct isdn_ctrl_state * cur_ctrl = NULL;

/*---------------------------------------------------------------------------*
 *	called from main to read and process config file
 *---------------------------------------------------------------------------*/
void
configure(const char *filename, int reread)
{
	extern void reset_scanner(FILE *inputfile);
	
	set_config_defaults();

	yyin = fopen(filename, "r");

	if (reread)
	{
		reset_scanner(yyin);
		current_cfe = NULL;
	}

	if (yyin == NULL)
	{
		logit(LL_ERR, "cannot fopen file [%s]", filename);
		exit(1);
	}

	yyparse();
	
	monitor_fixup_rights();

	check_config();		/* validation and consistency check */

	fclose(yyin);

	if (do_print)
	{
		if (config_error_flag)
		{
			logit(LL_ERR, "there were %d error(s) in the configuration file, terminating!", config_error_flag);
			exit(1);
		}
		print_config();
		do_exit(0);
	}
}

/*---------------------------------------------------------------------------*
 *	yacc error routine
 *---------------------------------------------------------------------------*/
void
yyerror(const char *msg)
{
	logit(LL_ERR, "configuration error: %s at line %d, token \"%s\"", msg, lineno+1, yytext);
	config_error_flag++;
}

/*
 * Prepare a new default entry
 */
void
init_currrent_cfg_state()
{
	if (current_cfe != NULL) {
		add_cfg_entry(current_cfe);
	}
	current_cfe = malloc(sizeof(struct cfg_entry));
	memset(current_cfe, 0, sizeof(struct cfg_entry));

	current_cfe->isdncontroller = INVALID;
	current_cfe->isdnchannel = CHAN_ANY;
	current_cfe->usrdevice = INVALID;
	current_cfe->usrdeviceunit = INVALID;
	current_cfe->remote_numbers_handling = RNH_LAST;
	current_cfe->dialin_reaction = REACT_IGNORE;
	current_cfe->b1protocol = BPROT_NONE;
	current_cfe->unitlength = UNITLENGTH_DEFAULT;
	current_cfe->earlyhangup = EARLYHANGUP_DEFAULT;
	current_cfe->ratetype = INVALID_RATE;
	current_cfe->unitlengthsrc = ULSRC_NONE;
	current_cfe->answerprog = ANSWERPROG_DEF;	 	
	current_cfe->callbackwait = CALLBACKWAIT_MIN;
	current_cfe->calledbackwait = CALLEDBACKWAIT_MIN;		
	current_cfe->dialretries = DIALRETRIES_DEF;
	current_cfe->recoverytime = RECOVERYTIME_MIN;
	current_cfe->dialouttype = DIALOUT_NORMAL;
	current_cfe->inout = DIR_INOUT;
	current_cfe->ppp_expect_auth = AUTH_UNDEF;
	current_cfe->ppp_send_auth = AUTH_UNDEF;
	current_cfe->ppp_auth_flags = AUTH_RECHALLENGE | AUTH_REQUIRED;
	current_cfe->cdid = CDID_UNUSED;
	current_cfe->state = ST_IDLE;
	current_cfe->aoc_valid = AOC_INVALID;
	current_cfe->autoupdown = AUTOUPDOWN_YES;
}

/*---------------------------------------------------------------------------*
 *	fill all config entries with default values
 *---------------------------------------------------------------------------*/
static void
set_config_defaults(void)
{
	int i;

	/* system section cleanup */
        
	nregprog = nregexpr = 0;

	rt_prio = RTPRIO_NOTUSED;

	mailer[0] = '\0';
	mailto[0] = '\0';       
        
	/* clean regular expression table */
        
	for (i=0; i < MAX_RE; i++)
	{
		if (rarr[i].re_expr)
			free(rarr[i].re_expr);
		rarr[i].re_expr = NULL;
	        
		if (rarr[i].re_prog)
			free(rarr[i].re_prog);
		rarr[i].re_prog = NULL;

		rarr[i].re_flg = 0;
	}

	strlcpy(rotatesuffix, "", sizeof(rotatesuffix));
}

static void
set_autoupdown(struct cfg_entry *cep)
{
        struct ifaddrs *res = NULL, *p;
	struct ifreq ifr;
	int r, s, cnt, in6;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifr, 0, sizeof ifr);
	snprintf(ifr.ifr_name, sizeof ifr.ifr_name, "%s%d", cep->usrdevicename, cep->usrdeviceunit);
	r = ioctl(s, SIOCGIFFLAGS, &ifr);

	/*
	 * See if this interface has got any valid addresses - if not,
	 * leave it alone.
	 */
	if (r >= 0 && !(ifr.ifr_flags & IFF_UP)) {
		cnt = in6 = 0;
		if (getifaddrs(&res) == 0) {
			for (p = res; p; p = p->ifa_next) {
				if (p->ifa_addr == NULL)
					continue;
				if (p->ifa_addr->sa_family == AF_LINK)
					continue;
				if (strcmp(p->ifa_name, ifr.ifr_name) != 0)
					continue;
				if (p->ifa_addr->sa_family == AF_INET6)
					in6 = 1;
				cnt++;
			}
			freeifaddrs(res);
		}

		if (in6)
			cnt--;	/* XXX - heuristic to adjust for INET6 local scope */

		/* Ok, we have some addres - so UP the interface */
		if (cnt > 0) {
			ifr.ifr_flags |= IFF_UP;
			r = ioctl(s, SIOCSIFFLAGS, &ifr);
			if (r >= 0)
				cep->autoupdown |= AUTOUPDOWN_DONE;
		}
	}

	close(s);
}

static void
set_isppp_auth(struct cfg_entry *cep)
{
	struct spppauthcfg spcfg;
	int s;
	int doioctl = 0;

	if (cep->ppp_expect_auth == AUTH_UNDEF 
	   && cep->ppp_send_auth == AUTH_UNDEF)
		return;

	if (cep->ppp_expect_auth == AUTH_NONE 
	   || cep->ppp_send_auth == AUTH_NONE)
		doioctl = 1;

	if ((cep->ppp_expect_auth == AUTH_CHAP 
	     || cep->ppp_expect_auth == AUTH_PAP)
	    && cep->ppp_expect_name != NULL
	    && cep->ppp_expect_password != NULL)
		doioctl = 1;

	if ((cep->ppp_send_auth == AUTH_CHAP || cep->ppp_send_auth == AUTH_PAP)
			&& cep->ppp_send_name != NULL
			&& cep->ppp_send_password != NULL)
		doioctl = 1;

	if (!doioctl)
		return;

	memset(&spcfg, 0, sizeof spcfg);
	snprintf(spcfg.ifname, sizeof(spcfg.ifname), "%s%d",
		cep->usrdevicename, cep->usrdeviceunit);

	/* use a random AF to create the socket */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		logit(LL_ERR, "ERROR opening control socket at line %d!", lineno);
		config_error_flag++;
		return;
	}

	if (ioctl(s, SPPPGETAUTHCFG, &spcfg) == -1) {
		logit(LL_ERR, "ERROR fetching active PPP authentication info for %s at line %d!", spcfg.ifname, lineno);
		close(s);
		config_error_flag++;
		return;
	}
	if (cep->ppp_expect_auth != AUTH_UNDEF)
	{
		if (cep->ppp_expect_auth == AUTH_NONE)
		{
			spcfg.hisauth = SPPP_AUTHPROTO_NONE;
		}
		else if ((cep->ppp_expect_auth == AUTH_CHAP 
			  || cep->ppp_expect_auth == AUTH_PAP)
			 && cep->ppp_expect_name != NULL
			 && cep->ppp_expect_password != NULL)
		{
			spcfg.hisauth = cep->ppp_expect_auth == AUTH_PAP ? SPPP_AUTHPROTO_PAP : SPPP_AUTHPROTO_CHAP;
			spcfg.hisname = cep->ppp_expect_name;
			spcfg.hisname_length = strlen(cep->ppp_expect_name)+1;
			spcfg.hissecret = cep->ppp_expect_password;
			spcfg.hissecret_length = strlen(cep->ppp_expect_password)+1;
		}
	}
	if (cep->ppp_send_auth != AUTH_UNDEF)
	{
		if (cep->ppp_send_auth == AUTH_NONE)
		{
			spcfg.myauth = SPPP_AUTHPROTO_NONE;
		}
		else if ((cep->ppp_send_auth == AUTH_CHAP 
			  || cep->ppp_send_auth == AUTH_PAP)
			 && cep->ppp_send_name != NULL
			 && cep->ppp_send_password != NULL)
		{
			spcfg.myauth = cep->ppp_send_auth == AUTH_PAP ? SPPP_AUTHPROTO_PAP : SPPP_AUTHPROTO_CHAP;
			spcfg.myname = cep->ppp_send_name;
			spcfg.myname_length = strlen(cep->ppp_send_name)+1;
			spcfg.mysecret = cep->ppp_send_password;
			spcfg.mysecret_length = strlen(cep->ppp_send_password)+1;

			if (cep->ppp_auth_flags & AUTH_REQUIRED)
				spcfg.hisauthflags &= ~SPPP_AUTHFLAG_NOCALLOUT;
			else
				spcfg.hisauthflags |= SPPP_AUTHFLAG_NOCALLOUT;

			if (cep->ppp_auth_flags & AUTH_RECHALLENGE)
				spcfg.hisauthflags &= ~SPPP_AUTHFLAG_NORECHALLENGE;
			else
				spcfg.hisauthflags |= SPPP_AUTHFLAG_NORECHALLENGE;
		}
	}

	if (ioctl(s, SPPPSETAUTHCFG, &spcfg) == -1) {
		logit(LL_ERR, "ERROR setting new PPP authentication parameters for %s at line %d!", spcfg.ifname, lineno);
		config_error_flag++;
	}
	close(s);
}

/*---------------------------------------------------------------------------*
 *	extract values from config and fill table
 *---------------------------------------------------------------------------*/
void
cfg_setval(int keyword)
{
	int i;
	
	switch (keyword)
	{
	case ACCTALL:
		acct_all = yylval.booln;
		DBGL(DL_RCCF, (logit(LL_DBG, "system: acctall = %d", yylval.booln)));
		break;
		
	case ACCTFILE:
		strlcpy(acctfile, yylval.str, sizeof(acctfile));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: acctfile = %s", yylval.str)));
		break;

	case ALERT:
		if (yylval.num < MINALERT)
		{
			yylval.num = MINALERT;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: alert < %d, min = %d", current_cfe->name, MINALERT, yylval.num)));
		}
		else if (yylval.num > MAXALERT)
		{
			yylval.num = MAXALERT;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: alert > %d, min = %d", current_cfe->name, MAXALERT, yylval.num)));
		}
			
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: alert = %d", current_cfe->name, yylval.num)));
		current_cfe->alert = yylval.num;
		break;

	case ALIASING:
		DBGL(DL_RCCF, (logit(LL_DBG, "system: aliasing = %d", yylval.booln)));
		aliasing = yylval.booln;
		break;

	case ALIASFNAME:
		strlcpy(aliasfile, yylval.str, sizeof(aliasfile));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: aliasfile = %s", yylval.str)));
		break;

	case ANSWERPROG:
		if ((current_cfe->answerprog = strdup(yylval.str)) == NULL)
		{
			logit(LL_ERR, "entry %s: answerstring, malloc failed!", current_cfe->name);
			do_exit(1);
		}
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: answerprog = %s", current_cfe->name, yylval.str)));
		break;
		
	case B1PROTOCOL:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: b1protocol = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "raw")))
			current_cfe->b1protocol = BPROT_NONE;
		else if (!(strcmp(yylval.str, "hdlc")))
			current_cfe->b1protocol = BPROT_RHDLC;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"b1protocol\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case BEEPCONNECT:
		do_bell = yylval.booln;
		DBGL(DL_RCCF, (logit(LL_DBG, "system: beepconnect = %d", yylval.booln)));
		break;

	case BUDGETCALLBACKPERIOD:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-callbackperiod = %d", current_cfe->name, yylval.num)));
		current_cfe->budget_callbackperiod = yylval.num;
		break;

	case BUDGETCALLBACKNCALLS:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-callbackncalls = %d", current_cfe->name, yylval.num)));
		current_cfe->budget_callbackncalls = yylval.num;
		break;
		
	case BUDGETCALLOUTPERIOD:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-calloutperiod = %d", current_cfe->name, yylval.num)));
		current_cfe->budget_calloutperiod = yylval.num;
		break;

	case BUDGETCALLOUTNCALLS:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-calloutncalls = %d", current_cfe->name, yylval.num)));
		current_cfe->budget_calloutncalls = yylval.num;
		break;

	case AUTOUPDOWN:
		current_cfe->autoupdown = yylval.booln;
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: autoupdown = %d", current_cfe->name, yylval.booln)));
		break;

	case BUDGETCALLBACKSFILEROTATE:
		current_cfe->budget_callbacksfile_rotate = yylval.booln;
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-callbacksfile-rotate = %d", current_cfe->name, yylval.booln)));
		break;
		
	case BUDGETCALLBACKSFILE:
		{
			FILE *fp;
			int s, l;
			int n;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-callbacksfile = %s", current_cfe->name, yylval.str)));
			fp = fopen(yylval.str, "r");
			if (fp != NULL)
			{
				if ((fscanf(fp, "%d %d %d", (int *)&s, (int *)&l, &n)) != 3)
				{
					DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: initializing budget-callbacksfile %s", current_cfe->name, yylval.str)));
					fclose(fp);
					fp = fopen(yylval.str, "w");
					if (fp != NULL) {
						fprintf(fp, "%d %d %d", (int)time(NULL), (int)time(NULL), 0);
						fclose(fp);
					}
				} else
					fclose(fp);
			}
			else
			{
				DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: creating budget-callbacksfile %s", current_cfe->name, yylval.str)));
				fp = fopen(yylval.str, "w");
				if (fp != NULL)
					fprintf(fp, "%d %d %d", (int)time(NULL), (int)time(NULL), 0);
				fclose(fp);
			}

			fp = fopen(yylval.str, "r");
			if (fp != NULL)
			{
				if ((fscanf(fp, "%d %d %d", (int *)&s, (int *)&l, &n)) == 3)
				{
					if ((current_cfe->budget_callbacks_file = strdup(yylval.str)) == NULL)
					{
						logit(LL_ERR, "entry %s: budget-callbacksfile, malloc failed!", current_cfe->name);
						do_exit(1);
					}
					DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: using callbacksfile %s", current_cfe->name, yylval.str)));
				}
				fclose(fp);
			}
		}
		break;

	case BUDGETCALLOUTSFILEROTATE:
		current_cfe->budget_calloutsfile_rotate = yylval.booln;
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-calloutsfile-rotate = %d", current_cfe->name, yylval.booln)));
		break;

	case BUDGETCALLOUTSFILE:
		{
			FILE *fp;
			int s, l;
			int n;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: budget-calloutsfile = %s", current_cfe->name, yylval.str)));
			fp = fopen(yylval.str, "r");
			if (fp != NULL)
			{
				if ((fscanf(fp, "%d %d %d", (int *)&s, (int *)&l, &n)) != 3)
				{
					DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: initializing budget-calloutsfile %s", current_cfe->name, yylval.str)));
					fclose(fp);
					fp = fopen(yylval.str, "w");
					if (fp != NULL)
						fprintf(fp, "%d %d %d", (int)time(NULL), (int)time(NULL), 0);
					fclose(fp);
				}
			}
			else
			{
				DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: creating budget-calloutsfile %s", current_cfe->name, yylval.str)));
				fp = fopen(yylval.str, "w");
				if (fp != NULL)
					fprintf(fp, "%d %d %d", (int)time(NULL), (int)time(NULL), 0);
				fclose(fp);
			}

			fp = fopen(yylval.str, "r");
			if (fp != NULL)
			{
				if ((fscanf(fp, "%d %d %d", (int *)&s, (int *)&l, &n)) == 3)
				{
					if ((current_cfe->budget_callouts_file = strdup(yylval.str)) == NULL)
					{
						logit(LL_ERR, "entry %s: budget-calloutsfile, malloc failed!", current_cfe->name);
						do_exit(1);
					}
					DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: using calloutsfile %s", current_cfe->name, yylval.str)));
				}
				fclose(fp);
			}
		}
		break;
	
	case CALLBACKWAIT:
		if (yylval.num < CALLBACKWAIT_MIN)
		{
			yylval.num = CALLBACKWAIT_MIN;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: callbackwait < %d, min = %d", current_cfe->name, CALLBACKWAIT_MIN, yylval.num)));
		}

		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: callbackwait = %d", current_cfe->name, yylval.num)));
		current_cfe->callbackwait = yylval.num;
		break;
		
	case CALLEDBACKWAIT:
		if (yylval.num < CALLEDBACKWAIT_MIN)
		{
			yylval.num = CALLEDBACKWAIT_MIN;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: calledbackwait < %d, min = %d", current_cfe->name, CALLEDBACKWAIT_MIN, yylval.num)));
		}

		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: calledbackwait = %d", current_cfe->name, yylval.num)));
		current_cfe->calledbackwait = yylval.num;
		break;

	case CONNECTPROG:
		if ((current_cfe->connectprog = strdup(yylval.str)) == NULL)
		{
			logit(LL_ERR, "entry %s: connectprog, malloc failed!", current_cfe->name);
			do_exit(1);
		}
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: connectprog = %s", current_cfe->name, yylval.str)));
		break;
		
	case DIALOUTTYPE:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: dialouttype = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "normal")))
			current_cfe->dialouttype = DIALOUT_NORMAL;
		else if (!(strcmp(yylval.str, "calledback")))
			current_cfe->dialouttype = DIALOUT_CALLEDBACK;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"dialout-type\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case DIALRETRIES:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: dialretries = %d", current_cfe->name, yylval.num)));
		current_cfe->dialretries = yylval.num;
		break;

	case DIALRANDINCR:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: dialrandincr = %d", current_cfe->name, yylval.booln)));
		current_cfe->dialrandincr = yylval.booln;
		break;

	case DIRECTION:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: direction = %s", current_cfe->name, yylval.str)));

		if (!(strcmp(yylval.str, "inout")))
			current_cfe->inout = DIR_INOUT;
		else if (!(strcmp(yylval.str, "in")))
			current_cfe->inout = DIR_INONLY;
		else if (!(strcmp(yylval.str, "out")))
			current_cfe->inout = DIR_OUTONLY;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"direction\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case DISCONNECTPROG:
		if ((current_cfe->disconnectprog = strdup(yylval.str)) == NULL)
		{
			logit(LL_ERR, "entry %s: disconnectprog, malloc failed!", current_cfe->name);
			do_exit(1);
		}
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: disconnectprog = %s", current_cfe->name, yylval.str)));
		break;

	case DOWNTRIES:
		if (yylval.num > DOWN_TRIES_MAX)
			yylval.num = DOWN_TRIES_MAX;
		else if (yylval.num < DOWN_TRIES_MIN)
			yylval.num = DOWN_TRIES_MIN;
	
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: downtries = %d", current_cfe->name, yylval.num)));
		current_cfe->downtries = yylval.num;
		break;

	case DOWNTIME:
		if (yylval.num > DOWN_TIME_MAX)
			yylval.num = DOWN_TIME_MAX;
		else if (yylval.num < DOWN_TIME_MIN)
			yylval.num = DOWN_TIME_MIN;
	
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: downtime = %d", current_cfe->name, yylval.num)));
		current_cfe->downtime = yylval.num;
		break;

	case EARLYHANGUP:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: earlyhangup = %d", current_cfe->name, yylval.num)));
		current_cfe->earlyhangup = yylval.num;
		break;

	case EXTCALLATTR:
		DBGL(DL_RCCF, (logit(LL_DBG, "system: extcallattr = %d", yylval.booln)));
		extcallattr = yylval.booln;
		break;

	case FIRMWARE:
		DBGL(DL_RCCF, (logit(LL_DBG, "controller %d: firmware = %s", cur_ctrl->isdnif, yylval.str)));
		cur_ctrl->firmware = strdup(yylval.str);
		break;

	case HOLIDAYFILE:
		strlcpy(holidayfile, yylval.str, sizeof(holidayfile));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: holidayfile = %s", yylval.str)));
		break;

	case IDLE_ALG_OUT:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: idle-algorithm-outgoing = %s", current_cfe->name, yylval.str)));

		if (!(strcmp(yylval.str, "fix-unit-size")))
		{
			current_cfe->shorthold_algorithm = SHA_FIXU;
		}
		else if (!(strcmp(yylval.str, "var-unit-size")))
		{
			current_cfe->shorthold_algorithm = SHA_VARU;
		}
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"idle-algorithm-outgoing\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case IDLETIME_IN:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: idle_time_in = %d", current_cfe->name, yylval.num)));
		current_cfe->idle_time_in = yylval.num;
		break;
		
	case IDLETIME_OUT:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: idle_time_out = %d", current_cfe->name, yylval.num)));
		current_cfe->idle_time_out = yylval.num;
		break;

	case ISDNCONTROLLER:
		current_cfe->isdncontroller = yylval.num;
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: isdncontroller = %d", current_cfe->name, yylval.num)));
		break;

	case ISDNCHANNEL:
		if (yylval.num == 0 || yylval.num == -1) {
			current_cfe->isdnchannel = CHAN_ANY;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: isdnchannel = any", current_cfe->name)));
		} else if (yylval.num > MAX_BCHAN) {
			logit(LL_DBG, "entry %s: isdnchannel value out of range", current_cfe->name);
			config_error_flag++;
		} else {
			current_cfe->isdnchannel = yylval.num - 1;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: isdnchannel = %d", current_cfe->name, yylval.num)));
		}
		break;

	case ISDNTIME:
		DBGL(DL_RCCF, (logit(LL_DBG, "system: isdntime = %d", yylval.booln)));
		isdntime = yylval.booln;
		break;

	case ISDNTXDELIN:
		current_cfe->isdntxdelin = yylval.num;
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: isdntxdel-incoming = %d", current_cfe->name, yylval.num)));
		break;

	case ISDNTXDELOUT:
		current_cfe->isdntxdelout = yylval.num;
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: isdntxdel-outgoing = %d", current_cfe->name, yylval.num)));
		break;

	case LOCAL_PHONE_DIALOUT:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: local_phone_dialout = %s", current_cfe->name, yylval.str)));
		strlcpy(current_cfe->local_phone_dialout, yylval.str,
		    sizeof(current_cfe->local_phone_dialout));
		break;

	case LOCAL_PHONE_INCOMING:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: local_phone_incoming = %s", current_cfe->name, yylval.str)));
		strlcpy(current_cfe->local_phone_incoming, yylval.str,
		    sizeof(current_cfe->local_phone_incoming));
		break;

	case MAILER:
		strlcpy(mailer, yylval.str, sizeof(mailer));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: mailer = %s", yylval.str)));
		break;

	case MAILTO:
		strlcpy(mailto, yylval.str, sizeof(mailto));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: mailto = %s", yylval.str)));
		break;

	case MONITORPORT:
		monitorport = yylval.num;
		DBGL(DL_RCCF, (logit(LL_DBG, "system: monitorport = %d", yylval.num)));
		break;

	case MONITORSW:
		if (yylval.booln && inhibit_monitor)
		{
			do_monitor = 0;
			DBGL(DL_RCCF, (logit(LL_DBG, "system: monitor-enable overriden by command line flag")));
		}
		else
		{
			do_monitor = yylval.booln;
			DBGL(DL_RCCF, (logit(LL_DBG, "system: monitor-enable = %d", yylval.booln)));
		}
		break;

	case NAME:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: name = %s", current_cfe->name, yylval.str)));
		strlcpy(current_cfe->name, yylval.str,
		    sizeof(current_cfe->name));
		break;

	case PPP_AUTH_RECHALLENGE:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-auth-rechallenge = %d", current_cfe->name, yylval.booln)));
		if (yylval.booln)
			current_cfe->ppp_auth_flags |= AUTH_RECHALLENGE;
		else
			current_cfe->ppp_auth_flags &= ~AUTH_RECHALLENGE;
		break;

	case PPP_AUTH_PARANOID:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-auth-paranoid = %d", current_cfe->name, yylval.booln)));
		if (yylval.booln)
			current_cfe->ppp_auth_flags |= AUTH_REQUIRED;
		else
			current_cfe->ppp_auth_flags &= ~AUTH_REQUIRED;
		break;

	case PPP_EXPECT_AUTH:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-expect-auth = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "none")))
			current_cfe->ppp_expect_auth = AUTH_NONE;
		else if (!(strcmp(yylval.str, "pap")))
			current_cfe->ppp_expect_auth = AUTH_PAP;
		else if (!(strcmp(yylval.str, "chap")))
			current_cfe->ppp_expect_auth = AUTH_CHAP;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"ppp-expect-auth\" at line %d!", lineno);
			config_error_flag++;
			break;
		}
		break;

	case PPP_EXPECT_NAME:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-expect-name = %s", current_cfe->name, yylval.str)));
		if (current_cfe->ppp_expect_name)
		    free(current_cfe->ppp_expect_name);
		current_cfe->ppp_expect_name = strdup(yylval.str);
		break;

	case PPP_EXPECT_PASSWORD:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-expect-password = %s", current_cfe->name, yylval.str)));
		if (current_cfe->ppp_expect_password)
		    free(current_cfe->ppp_expect_password);
		current_cfe->ppp_expect_password = strdup(yylval.str);
		break;

	case PPP_SEND_AUTH:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-send-auth = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "none")))
			current_cfe->ppp_send_auth = AUTH_NONE;
		else if (!(strcmp(yylval.str, "pap")))
			current_cfe->ppp_send_auth = AUTH_PAP;
		else if (!(strcmp(yylval.str, "chap")))
			current_cfe->ppp_send_auth = AUTH_CHAP;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"ppp-send-auth\" at line %d!", lineno);
			config_error_flag++;
			break;
		}
		break;

	case PPP_SEND_NAME:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-send-name = %s", current_cfe->name, yylval.str)));
		if (current_cfe->ppp_send_name)
		    free(current_cfe->ppp_send_name);
		current_cfe->ppp_send_name = strdup(yylval.str);
		break;

	case PPP_SEND_PASSWORD:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ppp-send-password = %s", current_cfe->name, yylval.str)));
		if (current_cfe->ppp_send_password)
		    free(current_cfe->ppp_send_password);
		current_cfe->ppp_send_password = strdup(yylval.str);
		break;

	case PROTOCOL:
		DBGL(DL_RCCF, (logit(LL_DBG, "controller %d: protocol = %s", cur_ctrl->isdnif, yylval.str)));
		if (!(strcmp(yylval.str, "dss1")))
			cur_ctrl->protocol = PROTOCOL_DSS1;
		else if (!(strcmp(yylval.str, "d64s")))
			cur_ctrl->protocol = PROTOCOL_D64S;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"protocol\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case REACTION:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: dialin_reaction = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "accept")))
			current_cfe->dialin_reaction = REACT_ACCEPT;
		else if (!(strcmp(yylval.str, "reject")))
			current_cfe->dialin_reaction = REACT_REJECT;
		else if (!(strcmp(yylval.str, "ignore")))
			current_cfe->dialin_reaction = REACT_IGNORE;
		else if (!(strcmp(yylval.str, "answer")))
			current_cfe->dialin_reaction = REACT_ANSWER;
		else if (!(strcmp(yylval.str, "callback")))
			current_cfe->dialin_reaction = REACT_CALLBACK;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"dialin_reaction\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case REMOTE_PHONE_DIALOUT:
		if (current_cfe->remote_numbers_count >= MAXRNUMBERS)
		{
			logit(LL_ERR, "ERROR parsing config file: too many remote numbers at line %d!", lineno);
			config_error_flag++;
			break;
		}				
		
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: remote_phone_dialout #%d = %s",
			current_cfe->name, current_cfe->remote_numbers_count, yylval.str)));

		strlcpy(current_cfe->remote_numbers[current_cfe->remote_numbers_count].number,
		    yylval.str,
		    sizeof(current_cfe->remote_numbers[current_cfe->remote_numbers_count].number));
		current_cfe->remote_numbers[current_cfe->remote_numbers_count].flag = 0;

		current_cfe->remote_numbers_count++;
		
		break;

	case REMOTE_NUMBERS_HANDLING:			
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: remdial_handling = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "next")))
			current_cfe->remote_numbers_handling = RNH_NEXT;
		else if (!(strcmp(yylval.str, "last")))
			current_cfe->remote_numbers_handling = RNH_LAST;
		else if (!(strcmp(yylval.str, "first")))
			current_cfe->remote_numbers_handling = RNH_FIRST;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"remdial_handling\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case REMOTE_PHONE_INCOMING:
		{
			int n;
			n = current_cfe->incoming_numbers_count;
			if (n >= MAX_INCOMING)
			{
				logit(LL_ERR, "ERROR parsing config file: too many \"remote_phone_incoming\" entries at line %d!", lineno);
				config_error_flag++;
				break;
			}
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: remote_phone_incoming #%d = %s", current_cfe->name, n, yylval.str)));
			strlcpy(current_cfe->remote_phone_incoming[n].number,
			    yylval.str,
			    sizeof(current_cfe->remote_phone_incoming[n].number));
			current_cfe->incoming_numbers_count++;
		}
		break;

	case RATESFILE:
		strlcpy(ratesfile, yylval.str, sizeof(ratesfile));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: ratesfile = %s", yylval.str)));
		break;

	case RATETYPE:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: ratetype = %d", current_cfe->name, yylval.num)));
		current_cfe->ratetype = yylval.num;
		break;
	
	case RECOVERYTIME:
		if (yylval.num < RECOVERYTIME_MIN)
		{
			yylval.num = RECOVERYTIME_MIN;
			DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: recoverytime < %d, min = %d", current_cfe->name, RECOVERYTIME_MIN, yylval.num)));
		}

		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: recoverytime = %d", current_cfe->name, yylval.num)));
		current_cfe->recoverytime = yylval.num;
		break;
	
	case REGEXPR:
		if (nregexpr >= MAX_RE)
		{
			logit(LL_ERR, "system: regexpr #%d >= MAX_RE", nregexpr);
			config_error_flag++;
			break;
		}

		if ((i = regcomp(&(rarr[nregexpr].re), yylval.str, REG_EXTENDED|REG_NOSUB)) != 0)
		{
			char buf[256];
			regerror(i, &(rarr[nregexpr].re), buf, sizeof(buf));
			logit(LL_ERR, "system: regcomp error for %s: [%s]", yylval.str, buf);
			config_error_flag++;
			break;
		}
		else
		{
			if ((rarr[nregexpr].re_expr = strdup(yylval.str)) == NULL)
			{
				logit(LL_ERR, "system: regexpr malloc error error for %s", yylval.str);
				config_error_flag++;
				break;
			}

			DBGL(DL_RCCF, (logit(LL_DBG, "system: regexpr %s stored into slot %d", yylval.str, nregexpr)));
			
			if (rarr[nregexpr].re_prog != NULL)
				rarr[nregexpr].re_flg = 1;
			
			nregexpr++;
			
		}
		break;

	case REGPROG:
		if (nregprog >= MAX_RE)
		{
			logit(LL_ERR, "system: regprog #%d >= MAX_RE", nregprog);
			config_error_flag++;
			break;
		}
		if ((rarr[nregprog].re_prog = strdup(yylval.str)) == NULL)
		{
			logit(LL_ERR, "system: regprog malloc error error for %s", yylval.str);
			config_error_flag++;
			break;
		}

		DBGL(DL_RCCF, (logit(LL_DBG, "system: regprog %s stored into slot %d", yylval.str, nregprog)));
		
		if (rarr[nregprog].re_expr != NULL)
			rarr[nregprog].re_flg = 1;

		nregprog++;
		break;

	case ROTATESUFFIX:
		strlcpy(rotatesuffix, yylval.str, sizeof(rotatesuffix));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: rotatesuffix = %s", yylval.str)));
		break;

	case RTPRIO:
#ifdef USE_RTPRIO
		rt_prio = yylval.num;
		if (rt_prio < RTP_PRIO_MIN || rt_prio > RTP_PRIO_MAX)
		{
			config_error_flag++;
			logit(LL_ERR, "system: error, rtprio (%d) out of range!", yylval.num);
		}
		else
		{
			DBGL(DL_RCCF, (logit(LL_DBG, "system: rtprio = %d", yylval.num)));
		}
#else
		rt_prio = RTPRIO_NOTUSED;
#endif
		break;

	case TINAINITPROG:
		strlcpy(tinainitprog, yylval.str, sizeof(tinainitprog));
		DBGL(DL_RCCF, (logit(LL_DBG, "system: tinainitprog = %s", yylval.str)));
		break;

	case UNITLENGTH:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: unitlength = %d", current_cfe->name, yylval.num)));
		current_cfe->unitlength = yylval.num;
		break;

	case UNITLENGTHSRC:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: unitlengthsrc = %s", current_cfe->name, yylval.str)));
		if (!(strcmp(yylval.str, "none")))
			current_cfe->unitlengthsrc = ULSRC_NONE;
		else if (!(strcmp(yylval.str, "cmdl")))
			current_cfe->unitlengthsrc = ULSRC_CMDL;
		else if (!(strcmp(yylval.str, "conf")))
			current_cfe->unitlengthsrc = ULSRC_CONF;
		else if (!(strcmp(yylval.str, "rate")))
			current_cfe->unitlengthsrc = ULSRC_RATE;
		else if (!(strcmp(yylval.str, "aocd")))
			current_cfe->unitlengthsrc = ULSRC_DYN;
		else
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"unitlengthsrc\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case USRDEVICENAME:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: usrdevicename = %s", current_cfe->name, yylval.str)));
		strncpy(current_cfe->usrdevicename, yylval.str, sizeof(current_cfe->usrdevicename));
		current_cfe->usrdevice = lookup_l4_driver(yylval.str);
		if (current_cfe->usrdevice < 0)
		{
			logit(LL_ERR, "ERROR parsing config file: unknown parameter for keyword \"usrdevicename\" at line %d!", lineno);
			config_error_flag++;
		}
		break;

	case USRDEVICEUNIT:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: usrdeviceunit = %d", current_cfe->name, yylval.num)));
		current_cfe->usrdeviceunit = yylval.num;
		break;

	case USEACCTFILE:
		useacctfile = yylval.booln;
		DBGL(DL_RCCF, (logit(LL_DBG, "system: useacctfile = %d", yylval.booln)));
		break;

	case USEDOWN:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: usedown = %d", current_cfe->name, yylval.booln)));
		current_cfe->usedown = yylval.booln;
		break;

	case VALID:
		DBGL(DL_RCCF, (logit(LL_DBG, "entry %s: valid = %s", current_cfe->name, yylval.str)));
		parse_valid(yylval.str);
		break;

	default:
		logit(LL_ERR, "ERROR parsing config file: unknown keyword at line %d!", lineno);
		config_error_flag++;
		break;			
	}
}

/*---------------------------------------------------------------------------*
 *	parse a date/time range
 *---------------------------------------------------------------------------*/
static void
parse_valid(char *dt)
{
	/* a valid string consists of some days of week separated by
	 * commas, where 0=sunday, 1=monday .. 6=saturday and a special
	 * value of 7 which is a holiday from the holiday file.
	 * after the days comes an optional (!) time range in the form
	 * aa:bb-cc:dd, this format is fixed to be parsable by sscanf.
	 * Valid specifications looks like this:
	 * 1,2,3,4,5,09:00-18:00	Monday-Friday 9-18h
	 * 1,2,3,4,5,18:00-09:00	Monday-Friday 18-9h
	 * 6				Saturday (whole day)
	 * 0,7				Sunday and Holidays
	 */

	int day = 0;
	int fromhr = 0;
	int frommin = 0;
	int tohr = 0;
	int tomin = 0;
	int ret;
	
	for (;;)
	{
		if ( ( ((*dt >= '0') && (*dt <= '9')) && (*(dt+1) == ':') ) ||
		    ( ((*dt >= '0') && (*dt <= '2')) && ((*(dt+1) >= '0') && (*(dt+1) <= '9')) && (*(dt+2) == ':') ) )
		{
			/* dt points to time spec */
			ret = sscanf(dt, "%d:%d-%d:%d", &fromhr, &frommin, &tohr, &tomin);
			if (ret !=4)
			{
				logit(LL_ERR, "ERROR parsing config file: timespec [%s] error at line %d!", dt, lineno);
				config_error_flag++;
				return;
			}

			if (fromhr < 0 || fromhr > 24 || tohr < 0 || tohr > 24 ||
			   frommin < 0 || frommin > 59 || tomin < 0 || tomin > 59)
			{
				logit(LL_ERR, "ERROR parsing config file: invalid time [%s] at line %d!", dt, lineno);
				config_error_flag++;
				return;
			}
			break;
		}
		else if ((*dt >= '0') && (*dt <= '7'))
		{
			/* dt points to day spec */
			day |= 1 << (*dt - '0');
			dt++;
			continue;
		}
		else if (*dt == ',')
		{
			/* dt points to delimiter */
			dt++;
			continue;
		}
		else if (*dt == '\0')
		{
			/* dt points to end of string */
			break;
		}
		else
		{
			/* dt points to illegal character */
			logit(LL_ERR, "ERROR parsing config file: illegal character [%c=0x%x] in date/time spec at line %d!", *dt, *dt, lineno);
			config_error_flag++;
			return;
		}
	}
	current_cfe->day = day;
	current_cfe->fromhr = fromhr;
	current_cfe->frommin = frommin;
	current_cfe->tohr = tohr;
	current_cfe->tomin = tomin;
}

void
flush_config()
{
	if (current_cfe != NULL) {
		add_cfg_entry(current_cfe);
	}
}

/*---------------------------------------------------------------------------*
 *	configuration validation and consistency check
 *---------------------------------------------------------------------------*/
static void
check_config(void)
{
	struct cfg_entry *cep = NULL;
	int i;
	int error = 0;

	/* regular expression table */
	
	for (i=0; i < MAX_RE; i++)
	{
		if ((rarr[i].re_expr != NULL) && (rarr[i].re_prog == NULL))
		{
			logit(LL_ERR, "check_config: regular expression %d without program!", i);
			error++;
		}
		if ((rarr[i].re_prog != NULL) && (rarr[i].re_expr == NULL))
		{
			logit(LL_ERR, "check_config: regular expression program %d without expression!", i);
			error++;
		}
	}

	/* entry sections */
	
	for (cep = get_first_cfg_entry(); cep; cep = NEXT_CFE(cep)) {

		/* does this entry have a bchannel driver configured? */
		if (cep->usrdevice < 0) {
			logit(LL_ERR, "check_config: usrdevicename not set in entry \"%s\"!", 
			    cep->name);
			error++;
		}
		if (cep->usrdeviceunit < 0) {
			logit(LL_ERR, "check_config: usrdeviceunit not set in entry \"%s\"!",
			    cep->name);
			error++;
		}

		/* numbers used for dialout */
		
		if ((cep->inout != DIR_INONLY) && (cep->dialin_reaction != REACT_ANSWER))
		{
			if (cep->remote_numbers_count == 0)
			{
				logit(LL_ERR, "check_config: remote-phone-dialout not set in entry \"%s\"!",
				    cep->name);
				error++;
			}
		}

		/* numbers used for incoming calls */
		
		if (cep->inout != DIR_OUTONLY)
		{
			if (strlen(cep->local_phone_incoming) == 0)
			{
				logit(LL_ERR, "check_config: local-phone-incoming not set in entry \"%s\"!",
				    cep->name);
				error++;
			}
			if (cep->incoming_numbers_count == 0)
			{
				logit(LL_ERR, "check_config: remote-phone-incoming not set in entry \"%s\"!",
				    cep->name);
				error++;
			}
		}

		if ((cep->dialin_reaction == REACT_ANSWER) && (cep->b1protocol != BPROT_NONE))
		{
			logit(LL_ERR, "check_config: b1protocol not raw for telephony in entry \"%s\"!",
			    cep->name);
			error++;
		}

		if ((cep->ppp_send_auth == AUTH_PAP) || (cep->ppp_send_auth == AUTH_CHAP))
		{
			if (cep->ppp_send_name == NULL)
			{
				logit(LL_ERR, "check_config: no remote authentification name in entry \"%s\"!",
				    cep->name);
				error++;
			}
			if (cep->ppp_send_password == NULL)
			{
				logit(LL_ERR, "check_config: no remote authentification password in entry \"%s\"!",
				    cep->name);
				error++;
			}
		}
		if ((cep->ppp_expect_auth == AUTH_PAP) || (cep->ppp_expect_auth == AUTH_CHAP))
		{
			if (cep->ppp_expect_name == NULL)
			{
				logit(LL_ERR, "check_config: no local authentification name in entry \"%s\"!",
				    cep->name);
				error++;
			}
			if (cep->ppp_expect_password == NULL)
			{
				logit(LL_ERR, "check_config: no local authentification secret in entry \"%s\"!",
				    cep->name);
				error++;
			}
		}

		if (cep->ppp_expect_auth != AUTH_UNDEF 
		   || cep->ppp_send_auth != AUTH_UNDEF)
			set_isppp_auth(cep);

		/*
		 * Only if AUTOUPDOWN_YES is the only bit set, otherwise
		 * we already have handled this interface.
		 */
		if (cep->autoupdown == AUTOUPDOWN_YES)
			set_autoupdown(cep);
	}
	if (error) {
		logit(LL_ERR, "check_config: %d error(s) in configuration file, exiting!",
		    error);
		do_exit(1);
	}
}

/*---------------------------------------------------------------------------*
 *	print the configuration
 *---------------------------------------------------------------------------*/
static void
print_config(void)
{
#define PFILE stdout

#ifdef I4B_EXTERNAL_MONITOR
	extern struct monitor_rights * monitor_next_rights(const struct monitor_rights *r);
	struct monitor_rights *m_rights;
#endif
	struct cfg_entry *cep = NULL;
	int i, j;
	time_t now;
	char mytime[64];

	time(&now);
	strlcpy(mytime, ctime(&now), sizeof(mytime));
	mytime[strlen(mytime)-1] = '\0';

	fprintf(PFILE, "#---------------------------------------------------------------------------\n");
	fprintf(PFILE, "# system section (generated %s)\n", mytime);
	fprintf(PFILE, "#---------------------------------------------------------------------------\n");
	fprintf(PFILE, "system\n");
	fprintf(PFILE, "useacctfile     = %s\n", useacctfile ? "on\t\t\t\t# update accounting information file" : "off\t\t\t\t# don't update accounting information file");
	fprintf(PFILE, "acctall         = %s\n", acct_all ? "on\t\t\t\t# put all events into accounting file" : "off\t\t\t\t# put only charged events into accounting file");
	fprintf(PFILE, "acctfile        = %s\t\t# accounting information file\n", acctfile);
	fprintf(PFILE, "ratesfile       = %s\t\t# charging rates database file\n", ratesfile);

#ifdef USE_RTPRIO
	if (rt_prio == RTPRIO_NOTUSED)
		fprintf(PFILE, "# rtprio is unused\n");
	else
		fprintf(PFILE, "rtprio          = %d\t\t\t\t# isdnd runs at realtime priority\n", rt_prio);
#endif

	/* regular expression table */
	
	for (i=0; i < MAX_RE; i++)
	{
		if (rarr[i].re_expr != NULL)
		{
			fprintf(PFILE, "regexpr         = \"%s\"\t\t# scan logfile for this expression\n", rarr[i].re_expr);
		}
		if (rarr[i].re_prog != NULL)
		{
			fprintf(PFILE, "regprog         = %s\t\t# program to run when expression is matched\n", rarr[i].re_prog);
		}
	}

#ifdef I4B_EXTERNAL_MONITOR

	fprintf(PFILE, "monitor-allowed = %s\n", do_monitor ? "on\t\t\t\t# remote isdnd monitoring allowed" : "off\t\t\t\t# remote isdnd monitoring disabled");
	fprintf(PFILE, "monitor-port    = %d\t\t\t\t# TCP/IP port number used for remote monitoring\n", monitorport);

	m_rights = monitor_next_rights(NULL);
	if (m_rights != NULL)
	{
		const char *s = "error\n";
		char b[512];

		for ( ; m_rights != NULL; m_rights = monitor_next_rights(m_rights))
		{
			if (m_rights->local)
			{
				fprintf(PFILE, "monitor         = \"%s\"\t\t# local socket name for monitoring\n", m_rights->name);
			}
			else
			{
				struct in_addr ia;
				ia.s_addr = ntohl(m_rights->net);

				switch (m_rights->mask)
				{
				case 0xffffffff:
					s = "32";
					break;
				case 0xfffffffe:
					s = "31";
					break;
				case 0xfffffffc:
					s = "30";
					break;
				case 0xfffffff8:
					s = "29";
					break;
				case 0xfffffff0:
					s = "28";
					break;
				case 0xffffffe0:
					s = "27";
					break;
				case 0xffffffc0:
					s = "26";
					break;
				case 0xffffff80:
					s = "25";
					break;
				case 0xffffff00:
					s = "24";
					break;
				case 0xfffffe00:
					s = "23";
					break;
				case 0xfffffc00:
					s = "22";
					break;
				case 0xfffff800:
					s = "21";
					break;
				case 0xfffff000:
					s = "20";
					break;
				case 0xffffe000:
					s = "19";
					break;
				case 0xffffc000:
					s = "18";
					break;
				case 0xffff8000:
					s = "17";
					break;
				case 0xffff0000:
					s = "16";
					break;
				case 0xfffe0000:
					s = "15";
					break;
				case 0xfffc0000:
					s = "14";
					break;
				case 0xfff80000:
					s = "13";
					break;
				case 0xfff00000:
					s = "12";
					break;
				case 0xffe00000:
					s = "11";
					break;
				case 0xffc00000:
					s = "10";
					break;
				case 0xff800000:
					s = "9";
					break;
				case 0xff000000:
					s = "8";
					break;
				case 0xfe000000:
					s = "7";
					break;
				case 0xfc000000:
					s = "6";
					break;
				case 0xf8000000:
					s = "5";
					break;
				case 0xf0000000:
					s = "4";
					break;
				case 0xe0000000:
					s = "3";
					break;
				case 0xc0000000:
					s = "2";
					break;
				case 0x80000000:
					s = "1";
					break;
				case 0x00000000:
					s = "0";
					break;
				}
				fprintf(PFILE, "monitor         = \"%s/%s\"\t\t# host (net/mask) allowed to connect for monitoring\n", inet_ntoa(ia), s);
			}
			b[0] = '\0';
			
			if ((m_rights->rights) & I4B_CA_COMMAND_FULL)
				strlcat(b, "fullcmd,", sizeof(b));
			if ((m_rights->rights) & I4B_CA_COMMAND_RESTRICTED)
				strlcat(b, "restrictedcmd,", sizeof(b));
			if ((m_rights->rights) & I4B_CA_EVNT_CHANSTATE)
				strlcat(b, "channelstate,", sizeof(b));
			if ((m_rights->rights) & I4B_CA_EVNT_CALLIN)
				strlcat(b, "callin,", sizeof(b));
			if ((m_rights->rights) & I4B_CA_EVNT_CALLOUT)
				strlcat(b, "callout,", sizeof(b));
			if ((m_rights->rights) & I4B_CA_EVNT_I4B)
				strlcat(b, "logevents,", sizeof(b));

			if (strlen(b) > 0 && b[strlen(b)-1] == ',')
				b[strlen(b)-1] = '\0';
				
			fprintf(PFILE, "monitor-access  = %s\t\t# monitor access rights\n", b);
		}
	}
	
#endif
	/* entry sections */
	
	for (cep = get_first_cfg_entry(); cep; cep = NEXT_CFE(cep)) {
		fprintf(PFILE, "\n");
		fprintf(PFILE, "#---------------------------------------------------------------------------\n");
		fprintf(PFILE, "# entry section %d\n", i);
		fprintf(PFILE, "#---------------------------------------------------------------------------\n");
		fprintf(PFILE, "entry\n");

		fprintf(PFILE, "name                  = %s\t\t# name for this entry section\n", cep->name);

		fprintf(PFILE, "isdncontroller        = %d\t\t# ISDN card number used for this entry\n", cep->isdncontroller);
		fprintf(PFILE, "isdnchannel           = ");
		switch (cep->isdnchannel)
		{
		case CHAN_ANY:
			fprintf(PFILE, "-1\t\t# any ISDN B-channel may be used\n");
			break;
		default:
			fprintf(PFILE, "1\t\t# only ISDN B-channel %d may be used\n", cep->isdnchannel);
			break;
		}

		fprintf(PFILE, "usrdevicename         = %s\t\t# name of userland ISDN B-channel device\n", cep->usrdevicename);
		fprintf(PFILE, "usrdeviceunit         = %d\t\t# unit number of userland ISDN B-channel device\n", cep->usrdeviceunit);

		fprintf(PFILE, "b1protocol            = %s\n", cep->b1protocol ? "hdlc\t\t# B-channel layer 1 protocol is HDLC" : "raw\t\t# No B-channel layer 1 protocol used");

		fprintf(PFILE, "direction             = ");
		switch (cep->inout)
		{
		case DIR_INONLY:
			fprintf(PFILE, "in\t\t# only incoming connections allowed\n");
			break;
		case DIR_OUTONLY:
			fprintf(PFILE, "out\t\t# only outgoing connections allowed\n");
			break;
		case DIR_INOUT:
			fprintf(PFILE, "inout\t\t# incoming and outgoing connections allowed\n");
			break;
		}
		
		if (cep->remote_numbers_count > 1)
		{
			for (j = 0; j < cep->remote_numbers_count; j++)
				fprintf(PFILE, "remote-phone-dialout  = %s\t\t# telephone number %d for dialing out to remote\n", cep->remote_numbers[j].number, j+1);

			fprintf(PFILE, "remdial-handling      = ");
	
			switch (cep->remote_numbers_handling)
			{
			case RNH_NEXT:
				fprintf(PFILE, "next\t\t# use next number after last successful for new dial\n");
				break;
			case RNH_LAST:
				fprintf(PFILE, "last\t\t# use last successful number for new dial\n");
				break;
			case RNH_FIRST:
				fprintf(PFILE, "first\t\t# always start with first number for new dial\n");
				break;
			}

			if (cep->local_phone_dialout[0])
				fprintf(PFILE, "local-phone-dialout   = %s\t\t# show this number to remote when dialling out\n",
				    cep->local_phone_dialout);
			fprintf(PFILE, "dialout-type          = %s\n", cep->dialouttype ? "calledback\t\t# i am called back by remote" : "normal\t\t# i am not called back by remote");
		}

		if (!(cep->inout == DIR_OUTONLY))
		{
			int n;
			
			fprintf(PFILE, "local-phone-incoming  = %s\t\t# incoming calls must match this (mine) telephone number\n", cep->local_phone_incoming);
			for (n = 0; n < cep->incoming_numbers_count; n++)
				fprintf(PFILE, "remote-phone-incoming = %s\t\t# this is a valid remote number to call me\n",
					cep->remote_phone_incoming[n].number);

			fprintf(PFILE, "dialin-reaction       = ");
			switch (cep->dialin_reaction)
			{
			case REACT_ACCEPT:
				fprintf(PFILE, "accept\t\t# i accept a call from remote and connect\n");
				break;
			case REACT_REJECT:
				fprintf(PFILE, "reject\t\t# i reject the call from remote\n");
				break;
			case REACT_IGNORE:
				fprintf(PFILE, "ignore\t\t# i ignore the call from remote\n");
				break;
			case REACT_ANSWER:
				fprintf(PFILE, "answer\t\t# i will start telephone answering when remote calls in\n");
				break;
			case REACT_CALLBACK:
				fprintf(PFILE, "callback\t\t# when remote calls in, i will hangup and call back\n");
				break;
			}
		}

		{
			const char *s;
			switch (cep->ppp_expect_auth)
			{
			case AUTH_NONE:
				s = "none";
				break;
			case AUTH_PAP:
				s = "pap";
				break;
			case AUTH_CHAP:
				s = "chap";
				break;
			default:
				s = NULL;
				break;
			}
			if (s != NULL)
			{
				fprintf(PFILE, "ppp-expect-auth       = %s\t\t# the auth protocol we expect to receive on dial-in (none,pap,chap)\n", s);
				if (cep->ppp_expect_auth != AUTH_NONE)
				{
					fprintf(PFILE, "ppp-expect-name       = %s\t\t# the user name allowed in\n", cep->ppp_expect_name);
					fprintf(PFILE, "ppp-expect-password   = %s\t\t# the key expected from the other side\n", cep->ppp_expect_password);
					fprintf(PFILE, "ppp-auth-paranoid     = %s\t\t# do we require remote to authenticate even if we dial out\n", cep->ppp_auth_flags & AUTH_REQUIRED ? "yes" : "no");
				}
			}
			switch (cep->ppp_send_auth)
			{
			case AUTH_NONE:
				s = "none";
				break;
			case AUTH_PAP:
				s = "pap";
				break;
			case AUTH_CHAP:
				s = "chap";
				break;
			default:
				s = NULL;
				break;
			}
			if (s != NULL)
			{
				fprintf(PFILE, "ppp-send-auth         = %s\t\t# the auth protocol we use when dialing out (none,pap,chap)\n", s);
				if (cep->ppp_send_auth != AUTH_NONE)
				{
					fprintf(PFILE, "ppp-send-name         = %s\t\t# our PPP account used for dial-out\n", cep->ppp_send_name);
					fprintf(PFILE, "ppp-send-password     = %s\t\t# the key sent to the other side\n", cep->ppp_send_password);
				}
			}
			if (cep->ppp_send_auth == AUTH_CHAP ||
			   cep->ppp_expect_auth == AUTH_CHAP) {
				fprintf(PFILE, "ppp-auth-rechallenge   = %s\t\t# rechallenge CHAP connections once in a while\n", cep->ppp_auth_flags & AUTH_RECHALLENGE ? "yes" : "no");
			}
		}

		if (cep->autoupdown == AUTOUPDOWN_NO)
			fprintf(PFILE, "autoupdown = no\n");

		{
			const char *s;
			fprintf(PFILE, "idletime-outgoing     = %d\t\t# outgoing call idle timeout\n", cep->idle_time_out);

			switch ( cep->shorthold_algorithm )
			{
			case SHA_FIXU:
				s = "fix-unit-size";
				break;
			case SHA_VARU:
				s = "var-unit-size";
				break;
			default:
				s = "error!!!";
				break;
			}

			fprintf(PFILE, "idle-algorithm-outgoing     = %s\t\t# outgoing call idle algorithm\n", s);
		}

		if (!(cep->inout == DIR_OUTONLY))
			fprintf(PFILE, "idletime-incoming     = %d\t\t# incoming call idle timeout\n", cep->idle_time_in);

		{		
	 		fprintf(PFILE, "unitlengthsrc         = ");
			switch (cep->unitlengthsrc)
			{
			case ULSRC_NONE:
				fprintf(PFILE, "none\t\t# no unit length specified, using default\n");
				break;
			case ULSRC_CMDL:
				fprintf(PFILE, "cmdl\t\t# using unit length specified on commandline\n");
				break;
			case ULSRC_CONF:
				fprintf(PFILE, "conf\t\t# using unitlength specified by unitlength-keyword\n");
				fprintf(PFILE, "unitlength            = %d\t\t# fixed unitlength\n", cep->unitlength);
				break;
			case ULSRC_RATE:
				fprintf(PFILE, "rate\t\t# using unitlength specified in rate database\n");
				fprintf(PFILE, "ratetype              = %d\t\t# type of rate from rate database\n", cep->ratetype);
				break;
			case ULSRC_DYN:
				fprintf(PFILE, "aocd\t\t# using dynamically calculated unitlength based on AOCD subscription\n");
				fprintf(PFILE, "ratetype              = %d\t\t# type of rate from rate database\n", cep->ratetype);
				break;
			}

			fprintf(PFILE, "earlyhangup           = %d\t\t# early hangup safety time\n", cep->earlyhangup);

		}
		
		{
			fprintf(PFILE, "answerprog            = %s\t\t# program used to answer incoming telephone calls\n", cep->answerprog);
			fprintf(PFILE, "alert                 = %d\t\t# number of seconds to wait before accepting a call\n", cep->alert);
		}

		{		
			if (cep->dialin_reaction == REACT_CALLBACK)
				fprintf(PFILE, "callbackwait          = %d\t\t# i am waiting this time before calling back remote\n", cep->callbackwait);
	
			if (cep->dialouttype == DIALOUT_CALLEDBACK)
				fprintf(PFILE, "calledbackwait        = %d\t\t# i am waiting this time for a call back from remote\n", cep->calledbackwait);
	
			if (!(cep->inout == DIR_INONLY))
			{
				fprintf(PFILE, "dialretries           = %d\t\t# number of dialing retries\n", cep->dialretries);
				fprintf(PFILE, "recoverytime          = %d\t\t# time to wait between dialling retries\n", cep->recoverytime);
				fprintf(PFILE, "dialrandincr          = %s\t\t# use random dialing time addon\n", cep->dialrandincr ? "on" : "off");

				fprintf(PFILE, "usedown               = %s\n", cep->usedown ? "on\t\t# ISDN device switched off on excessive dial failures" : "off\t\t# no device switchoff on excessive dial failures");
				if (cep->usedown)
				{
					fprintf(PFILE, "downtries             = %d\t\t# number of dialretries failures before switching off\n", cep->downtries);
					fprintf(PFILE, "downtime              = %d\t\t# time device is switched off\n", cep->downtime);
				}
			}
		}
	}
	fprintf(PFILE, "\n");	
}

static int
lookup_l4_driver(const char *name)
{
	msg_l4driver_lookup_t query;
	int e;

	memset(&query, 0, sizeof query);
	strncpy(query.name, name, sizeof query.name);
	e = ioctl(isdnfd, I4B_L4DRIVER_LOOKUP, &query);
	if (e != 0) return -1;
	return query.driver_id;
}

