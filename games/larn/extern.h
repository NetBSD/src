/*	$NetBSD: extern.h,v 1.13 2009/08/12 08:04:05 dholland Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* action.c */
void act_remove_gems(int);
void act_sit_throne(int);
void act_drink_fountain(void);
void act_wash_fountain(void);
void act_desecrate_altar(void);
void act_donation_pray(void);
void act_just_pray(void);
void act_ignore_altar(void);
void act_open_chest(int, int);

/* bill.c */
void mailbill(void);

/* config.c */

/* create.c */
void makeplayer(void);
void newcavelevel(int);
void eat(int, int);
int fillmonst(int);

/* data.c */

/* diag.c */
void diag(void);
int dcount(int);
void diagdrawscreen(void);
int savegame(char *);
void restoregame(char *);

/* display.c */
void bottomline(void);
void bottomhp(void);
void bottomspell(void);
void bottomdo(void);
void bot_linex(void);
void bottomgold(void);
void draws(int, int, int, int);
void drawscreen(void);
void showcell(int, int);
void show1cell(int, int);
void showplayer(void);
int moveplayer(int);
void seemagic(int);

/* fortune.c */
const char *fortune(void);

/* global.c */
void raiselevel(void);
void loselevel(void);
void raiseexperience(long);
void loseexperience(long);
void losehp(int);
void losemhp(int);
void raisehp(int);
void raisemhp(int);
void raisemspells(int);
void losemspells(int);
int makemonst(int);
void positionplayer(void);
void recalc(void);
void quit(void);
void more(void);
int take(int, int);
int drop_object(int);
void enchantarmor(void);
void enchweapon(void);
int pocketfull(void);
int nearbymonst(void);
int stealsomething(void);
int emptyhanded(void);
void creategem(void);
void adjustcvalues(int, int);
int getpassword(void);
int getyn(void);
int packweight(void);
int rnd(int);
int rund(int);

/* help.c */
void help(void);
void welcome(void);

/* io.c */
void setupvt100(void);
void clearvt100(void);
int ttgetch(void);
void scbr(void);
void sncbr(void);
void newgame(void);
void lprintf(const char *, ...) __attribute__((__format__(__printf__, 1, 2)));
void lprint(long);
void lwrite(char *, int);
long lgetc(void);
long larn_lrint(void);
void lrfill(char *, int);
char *lgetw(void);
char *lgetl(void);
int lcreat(char *);
int lopen(char *);
int lappend(char *);
void lrclose(void);
void lwclose(void);
void lprcat(const char *);
void cursor(int, int);
void cursors(void);
void init_term(void);
void cl_line(int, int);
void cl_up(int, int);
void cl_dn(int, int);
void standout(const char *);
void set_score_output(void);
void lflush(void);
char *tmcapcnv(char *, char *);
void beep(void);

/* main.c */
int main(int, char **);
void qshowstr(void);
void show3(int);
void parse2(void);
unsigned long readnum(long);
void szero(char *);

/* monster.c */
void createmonster(int);
void createitem(int, int);
void cast(void);
void godirect(int, int, const char *, int, int);
int vxy(int *, int *);
void hitmonster(int, int);
void hitplayer(int, int);
void dropgold(int);
void something(int);
int newobject(int, int *);
void checkloss(int);
int annihilate(void);
int newsphere(int, int, int, int);
int rmsphere(int, int);

/* moreobj.c */
void oaltar(void);
void othrone(int);
void odeadthrone(void);
void ochest(void);
void ofountain(void);
void fntchange(int);

/* movem.c */
void movemonst(void);

/* nap.c */
void nap(int);

/* object.c */
void lookforobject(void);
void oteleport(int);
void quaffpotion(int);
void adjusttime(long);
void read_scroll(int);
void readbook(int);
void iopts(void);
void ignore(void);

/* regen.c */
void regen(void);

/* savelev.c */
void savelevel(void);
void getlevel(void);

/* scores.c */
int makeboard(void);
int hashewon(void);
long paytaxes(long);
void showscores(void);
void showallscores(void);
void died(int);
void diedlog(void);
int getplid(char *);

/* signal.c */
void sigsetup(void);

/* store.c */
void dndstore(void);
void oschool(void);
void obank(void);
void obank2(void);
void ointerest(void);
void otradepost(void);
void olrs(void);

/* tok.c */
int yylex(void);
void flushall(void);
void sethard(int);
void readopts(void);
