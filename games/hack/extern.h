/*	$NetBSD: extern.h,v 1.5 2002/05/26 00:12:12 wiz Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#ifndef _EXTERN_H_
#define _EXTERN_H_
#include <stdarg.h>
#include <stdio.h>

/* alloc.c */
long *alloc __P((unsigned));
long *enlarge __P((char *, unsigned));

/* hack.apply.c */
int doapply __P((void));
int holetime __P((void));
void dighole __P((void));

/* hack.bones.c */
void savebones __P((void));
int getbones __P((void));

/* hack.c */
void unsee __P((void));
void seeoff __P((int));
void domove __P((void));
void movobj __P((struct obj *, int, int));
int dopickup __P((void));
void pickup __P((int));
void lookaround __P((void));
int monster_nearby __P((void));
int rroom __P((int, int));
int cansee __P((xchar, xchar));
int sgn __P((int));
void setsee __P((void));
void nomul __P((int));
int abon __P((void));
int dbon __P((void));
void losestr __P((int));
void losehp __P((int, const char *));
void losehp_m __P((int, struct monst *));
void losexp __P((void));
int inv_weight __P((void));
int inv_cnt __P((void));
long newuexp __P((void));

/* hack.cmd.c */
void rhack __P((const char *));
int doextcmd __P((void));
char lowc __P((int));
char unctrl __P((int));
int movecmd __P((int));
int getdir __P((boolean));
void confdir __P((void));
int finddir __P((void));
int isroom __P((int, int));
int isok __P((int, int));

/* hack.do.c */
int dodrop __P((void));
void dropx __P((struct obj *));
void dropy __P((struct obj *));
int doddrop __P((void));
int dodown __P((void));
int doup __P((void));
void goto_level __P((int, boolean));
int donull __P((void));
int dopray __P((void));
int dothrow __P((void));
struct obj *splitobj __P((struct obj *, int));
void more_experienced __P((int, int));
void set_wounded_legs __P((long, int));
void heal_legs __P((void));

/* hack.do_name.c */
coord getpos __P((int, const char *));
int do_mname __P((void));
void do_oname __P((struct obj *));
int ddocall __P((void));
void docall __P((struct obj *));
char *xmonnam __P((struct monst *, int));
char *lmonnam __P((struct monst *));
char *monnam __P((struct monst *));
char *Monnam __P((struct monst *));
char *amonnam __P((struct monst *, const char *));
char *Amonnam __P((struct monst *, const char *));
char *Xmonnam __P((struct monst *));
char *visctrl __P((int));

/* hack.do_wear.c */
void off_msg __P((struct obj *));
int doremarm __P((void));
int doremring __P((void));
int dorr __P((struct obj *));
int cursed __P((struct obj *));
int armoroff __P((struct obj *));
int doweararm __P((void));
int dowearring __P((void));
void ringoff __P((struct obj *));
void find_ac __P((void));
void glibr __P((void));
struct obj *some_armor __P((void));
void corrode_armor __P((void));

/* hack.dog.c */
void makedog __P((void));
void initedog __P((struct monst *));
void losedogs __P((void));
void keepdogs __P((void));
void fall_down __P((struct monst *));
int dogfood __P((struct obj *));
int dog_move __P((struct monst *, int));
int inroom __P((xchar, xchar));
int tamedog __P((struct monst *, struct obj *));

/* hack.eat.c */
void init_uhunger __P((void));
int opentin __P((void));
int Meatdone __P((void));
int doeat __P((void));
void gethungry __P((void));
void morehungry __P((int));
void lesshungry __P((int));
int unfaint __P((void));
void newuhs __P((boolean));
int poisonous __P((struct obj *));
int eatcorpse __P((struct obj *));

/* hack.end.c */
int dodone __P((void));
void done1 __P((int));
void done_intr __P((int));
void done_hangup __P((int));
void done_in_by __P((struct monst *));
void done __P((const char *));
void topten __P((void));
void outheader __P((void));
struct toptenentry;
int outentry __P((int, struct toptenentry *, int));
char *itoa __P((int));
const char *ordin __P((int));
void clearlocks __P((void));
void hangup __P((int)) __attribute__((__noreturn__));
char *eos __P((char *));
void charcat __P((char *, int));
void prscore __P((int, char **));

/* hack.engrave.c */
struct engr *engr_at __P((xchar, xchar));
int sengr_at __P((const char *, xchar, xchar));
void u_wipe_engr __P((int));
void wipe_engr_at __P((xchar, xchar, xchar));
void read_engr_at __P((int, int));
void make_engr_at __P((int, int, const char *));
int doengrave __P((void));
void save_engravings __P((int));
void rest_engravings __P((int));
void del_engr __P((struct engr *));

/* hack.fight.c */
int hitmm __P((struct monst *, struct monst *));
void mondied __P((struct monst *));
void monstone __P((struct monst *));
int fightm __P((struct monst *));
int thitu __P((int, int, const char *));
boolean hmon __P((struct monst *, struct obj *, int));
int attack __P((struct monst *));

/* hack.invent.c */
struct obj *addinv __P((struct obj *));
void useup __P((struct obj *));
void freeinv __P((struct obj *));
void delobj __P((struct obj *));
void freeobj __P((struct obj *));
void freegold __P((struct gold *));
void deltrap __P((struct trap *));
struct monst *m_at __P((int, int));
struct obj *o_at __P((int, int));
struct obj *sobj_at __P((int, int, int));
int carried __P((struct obj *));
int carrying __P((int));
struct obj *o_on __P((unsigned int, struct obj *));
struct trap *t_at __P((int, int));
struct gold *g_at __P((int, int));
struct obj *mkgoldobj __P((long));
struct obj *getobj __P((const char *, const char *));
int ckunpaid __P((struct obj *));
int ggetobj __P((const char *, int (*fn)(struct obj *), int));
int askchain __P((struct obj *, char *, int, int (*)(struct obj *), 
    int (*)(struct obj *), int));
char obj_to_let __P((struct obj *));
void prinv __P((struct obj *));
int ddoinv __P((void));
void doinv __P((char *));
int dotypeinv __P((void));
int dolook __P((void));
void stackobj __P((struct obj *));
int merged __P((struct obj *, struct obj *, int));
int countgold __P((void));
int doprgold __P((void));
int doprwep __P((void));
int doprarm __P((void));
int doprring __P((void));
int digit __P((int));

/* hack.ioctl.c */
void getioctls __P((void));
void setioctls __P((void));
int dosuspend __P((void));

/* hack.lev.c */
void savelev __P((int, xchar));
void bwrite __P((int, const void *, unsigned));
void saveobjchn __P((int, struct obj *));
void savemonchn __P((int, struct monst *));
void savegoldchn __P((int, struct gold *));
void savetrapchn __P((int, struct trap *));
void getlev __P((int, int, xchar));
void mread __P((int, char *, unsigned));
void mklev __P((void));

/* hack.main.c */
void glo __P((int));
void askname __P((void));
void impossible __P((const char *, ...))
    __attribute__((__format__(__printf__, 1, 2)));
void stop_occupation __P((void));

/* hack.makemon.c */
struct monst *makemon __P((const struct permonst *, int, int));
coord enexto __P((xchar, xchar));
int goodpos __P((int, int));
void rloc __P((struct monst *));
struct monst *mkmon_at __P((int, int, int));

/* hack.mhitu.c */
int mhitu __P((struct monst *));
int hitu __P((struct monst *, int));

/* hack.mklev.c */
void makelevel __P((void));
int makerooms __P((void));
void addrs __P((int, int, int, int));
void addrsx __P((int, int, int, int, boolean));
struct mkroom;
int comp __P((const void *, const void *));
coord finddpos __P((int, int, int, int));
int okdoor __P((int, int));
void dodoor __P((int, int, struct mkroom *));
void dosdoor __P((int, int, struct mkroom *, int));
int maker __P((schar, schar, schar, schar));
void makecorridors __P((void));
void join __P((int, int));
void make_niches __P((void));
void makevtele __P((void));
void makeniche __P((boolean));
void mktrap __P((int, int, struct mkroom *));

/* hack.mkmaze.c */
void makemaz __P((void));
void walkfrom __P((int, int));
void move __P((int *, int *, int));
int okay __P((int, int, int));
coord mazexy __P((void));

/* hack.mkobj.c */
struct obj *mkobj_at __P((int, int, int));
void mksobj_at __P((int, int, int));
struct obj *mkobj __P((int));
struct obj *mksobj __P((int));
int letter __P((int));
int weight __P((struct obj *));
void mkgold __P((long, int, int));

/* hack.mkshop.c */
void mkshop __P((void));
void mkzoo __P((int));
const struct permonst *morguemon __P((void));
void mkswamp __P((void));
int nexttodoor __P((int, int));
int has_dnstairs __P((struct mkroom *));
int has_upstairs __P((struct mkroom *));
int isbig __P((struct mkroom *));
int dist2 __P((int, int, int, int));
int sq __P((int));

/* hack.mon.c */
void movemon __P((void));
void justswld __P((struct monst *, const char *));
void youswld __P((struct monst *, int, int, const char *));
int dochugw __P((struct monst *));
int dochug __P((struct monst *));
int m_move __P((struct monst *, int));
void mpickgold __P((struct monst *));
void mpickgems __P((struct monst *));
int mfndpos __P((struct monst *, coord[9 ], int[9 ], int));
int dist __P((int, int));
void poisoned __P((const char *, const char *));
void mondead __P((struct monst *));
void replmon __P((struct monst *, struct monst *));
void relmon __P((struct monst *));
void monfree __P((struct monst *));
void dmonsfree __P((void));
void unstuck __P((struct monst *));
void killed __P((struct monst *));
void kludge __P((const char *, const char *));
void rescham __P((void));
int newcham __P((struct monst *, const struct permonst *));
void mnexto __P((struct monst *));
int ishuman __P((struct monst *));
void setmangry __P((struct monst *));
int canseemon __P((struct monst *));

/* hack.monst.c */

/* hack.o_init.c */
int letindex __P((int));
void init_objects __P((void));
int probtype __P((int));
void setgemprobs __P((void));
void oinit __P((void));
void savenames __P((int));
void restnames __P((int));
int dodiscovered __P((void));
int interesting_to_discover __P((int));

/* hack.objnam.c */
char *strprepend __P((char *, char *));
char *sitoa __P((int));
char *typename __P((int));
char *xname __P((struct obj *));
char *doname __P((struct obj *));
void setan __P((const char *, char *));
char *aobjnam __P((struct obj *, const char *));
char *Doname __P((struct obj *));
struct obj *readobjnam __P((char *));

/* hack.options.c */
void initoptions __P((void));
void parseoptions __P((char *, boolean));
int doset __P((void));

/* hack.pager.c */
int dowhatis __P((void));
void intruph __P((int));
void page_more __P((FILE *, int));
void set_whole_screen __P((void));
int readnews __P((void));
void set_pager __P((int));
int page_line __P((const char *));
void cornline __P((int, const char *));
int dohelp __P((void));
int page_file __P((const char *, boolean));
int dosh __P((void));
int child __P((int));

/* hack.potion.c */
int dodrink __P((void));
void pluslvl __P((void));
void strange_feeling __P((struct obj *, const char *));
void potionhit __P((struct monst *, struct obj *));
void potionbreathe __P((struct obj *));
int dodip __P((void));
void ghost_from_bottle __P((void));

/* hack.pri.c */
void swallowed __P((void));
void panic __P((const char *, ...))
    __attribute__((__format__(__printf__, 1, 2)));
void atl __P((int, int, int));
void on_scr __P((int, int));
void tmp_at __P((schar, schar));
void Tmp_at __P((schar, schar));
void setclipped __P((void)) __attribute__((__noreturn__));
void at __P((xchar, xchar, int));
void prme __P((void));
int doredraw __P((void));
void docrt __P((void));
void docorner __P((int, int));
void curs_on_u __P((void));
void pru __P((void));
void prl __P((int, int));
char news0 __P((xchar, xchar));
void newsym __P((int, int));
void mnewsym __P((int, int));
void nosee __P((int, int));
void prl1 __P((int, int));
void nose1 __P((int, int));
int vism_at __P((int, int));
void pobj __P((struct obj *));
void unpobj __P((struct obj *));
void seeobjs __P((void));
void seemons __P((void));
void pmon __P((struct monst *));
void unpmon __P((struct monst *));
void nscr __P((void));
void cornbot __P((int));
void bot __P((void));
void mstatusline __P((struct monst *));
void cls __P((void));

/* hack.read.c */
int doread __P((void));
int identify __P((struct obj *));
void litroom __P((boolean));
int monstersym __P((int));

/* hack.rip.c */
void outrip __P((void));
void center __P((int, char *));

/* hack.rumors.c */
void init_rumors __P((FILE *));
int skipline __P((FILE *));
void outline __P((FILE *));
void outrumor __P((void));
int used __P((int));

/* hack.save.c */
int dosave __P((void));
int dosave0 __P((int));
int dorecover __P((int));
struct obj *restobjchn __P((int));
struct monst *restmonchn __P((int));

/* hack.search.c */
int findit __P((void));
int dosearch __P((void));
int doidtrap __P((void));
void wakeup __P((struct monst *));
void seemimic __P((struct monst *));

/* hack.shk.c */
void obfree __P((struct obj *, struct obj *));
void paybill __P((void));
char *shkname __P((struct monst *));
void shkdead __P((struct monst *));
void replshk __P((struct monst *, struct monst *));
int inshop __P((void));
int dopay __P((void));
struct bill_x;
struct obj *bp_to_obj __P((struct bill_x *));
void addtobill __P((struct obj *));
void splitbill __P((struct obj *, struct obj *));
void subfrombill __P((struct obj *));
int doinvbill __P((int));
int shkcatch __P((struct obj *));
int shk_move __P((struct monst *));
void shopdig __P((int));
int online __P((int, int));
int follower __P((struct monst *));

/* hack.shknam.c */
void findname __P((char *, int));

/* hack.steal.c */
long somegold __P((void));
void stealgold __P((struct monst *));
int stealarm __P((void));
int steal __P((struct monst *));
void mpickobj __P((struct monst *, struct obj *));
int stealamulet __P((struct monst *));
void relobj __P((struct monst *, int));

/* hack.termcap.c */
void startup __P((void));
void start_screen __P((void));
void end_screen __P((void));
void curs __P((int, int));
void nocmov __P((int, int));
void cmov __P((int, int));
int xputc __P((int));
void xputs __P((char *));
void cl_end __P((void));
void clear_screen __P((void));
void home __P((void));
void standoutbeg __P((void));
void standoutend __P((void));
void backsp __P((void));
void bell __P((void));
void delay_output __P((void));
void cl_eos __P((void));

/* hack.timeout.c */
void timeout __P((void));
void stoned_dialogue __P((void));

/* hack.topl.c */
int doredotopl __P((void));
void redotoplin __P((void));
void remember_topl __P((void));
void addtopl __P((const char *));
void xmore __P((const char *));
void more __P((void));
void cmore __P((const char *));
void clrlin __P((void));
void pline __P((const char *, ...))
    __attribute__((__format__(__printf__, 1, 2)));
void vpline __P((const char *, va_list))
    __attribute__((__format__(__printf__, 1, 0)));
void putsym __P((int));
void putstr __P((const char *));

/* hack.track.c */
void initrack __P((void));
void settrack __P((void));
coord *gettrack __P((int, int));

/* hack.trap.c */
struct trap *maketrap __P((int, int, int));
void dotrap __P((struct trap *));
int mintrap __P((struct monst *));
void selftouch __P((const char *));
void float_up __P((void));
void float_down __P((void));
void vtele __P((void));
void tele __P((void));
void teleds __P((int, int));
int teleok __P((int, int));
int dotele __P((void));
void placebc __P((int));
void unplacebc __P((void));
void level_tele __P((void));
void drown __P((void));

/* hack.tty.c */
void gettty __P((void));
void settty __P((const char *));
void setctty __P((void));
void setftty __P((void));
void error __P((const char *, ...))
    __attribute__((__format__(__printf__, 1, 2),__noreturn__));
void getlin __P((char *));
void getret __P((void));
void cgetret __P((const char *));
void xwaitforspace __P((const char *));
char *parse __P((void));
char readchar __P((void));
void end_of_input __P((void)) __attribute__((__noreturn__));

/* hack.u_init.c */
void u_init __P((void));
struct trobj;
void ini_inv __P((struct trobj *));
void wiz_inv __P((void));
void plnamesuffix __P((void));
int role_index __P((int));

/* hack.unix.c */
void setrandom __P((void));
struct tm *getlt __P((void));
int getyear __P((void));
char *getdate __P((void));
int phase_of_the_moon __P((void));
int night __P((void));
int midnight __P((void));
void gethdate __P((char *));
int uptodate __P((int));
int veryold __P((int));
void getlock __P((void));
void getmailstatus __P((void));
void ckmailstatus __P((void));
void newmail __P((void));
void mdrush __P((struct monst *, boolean));
void readmail __P((void));
void regularize __P((char *));

/* hack.vault.c */
void setgd __P((void));
int gd_move __P((void));
void gddead __P((void));
void replgd __P((struct monst *, struct monst *));
void invault __P((void));

/* hack.version.c */
int doversion __P((void));

/* hack.wield.c */
void setuwep __P((struct obj *));
int dowield __P((void));
void corrode_weapon __P((void));
int chwepon __P((struct obj *, int));

/* hack.wizard.c */
void amulet __P((void));
int wiz_hit __P((struct monst *));
void inrange __P((struct monst *));
void aggravate __P((void));
void clonewiz __P((struct monst *));

/* hack.worm.c */
#ifndef NOWORM
int getwn __P((struct monst *));
void initworm __P((struct monst *));
void worm_move __P((struct monst *));
void worm_nomove __P((struct monst *));
void wormdead __P((struct monst *));
void wormhit __P((struct monst *));
void wormsee __P((unsigned));
struct wseg;
void pwseg __P((struct wseg *));
void cutworm __P((struct monst *, xchar, xchar, uchar));
void remseg __P((struct wseg *));
#endif

/* hack.worn.c */
void setworn __P((struct obj *, long));
void setnotworn __P((struct obj *));

/* hack.zap.c */
void bhitm __P((struct monst *, struct obj *));
int bhito __P((struct obj *, struct obj *));
int dozap __P((void));
const char *exclam __P((int));
void hit __P((const char *, struct monst *, const char *));
void miss __P((const char *, struct monst *));
struct monst *bhit __P((int, int, int, int,
    void (*)(struct monst *, struct obj *),
    int (*)(struct obj *, struct obj *),
    struct obj *));
struct monst *boomhit __P((int, int));
char dirlet __P((int, int));
void buzz __P((int, xchar, xchar, int, int));
int zhit __P((struct monst *, int));
int revive __P((struct obj *));
void rloco __P((struct obj *));
void fracture_rock __P((struct obj *));
void burn_scrolls __P((void));

/* rnd.c */
int rn1 __P((int, int));
int rn2 __P((int));
int rnd __P((int));
int d __P((int, int));
#endif /* _EXTERN_H_ */
