/* menu system definitions. */

#ifndef MENU_DEFS_H
#define MENU_DEFS_H
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>

#define DYNAMIC_MENUS

struct menudesc;
typedef
struct menu_ent {
	char   *opt_name;
	int	opt_menu;
	int	opt_flags;
	int	(*opt_action)(struct menudesc *);
} menu_ent ;

#define OPT_SUB    1
#define OPT_ENDWIN 2
#define OPT_EXIT   4
#define OPT_NOMENU -1

typedef
struct menudesc {
	char     *title;
	int      y, x;
	int	 h, w;
	int	 mopt;
	int      numopts;
	int	 cursel;
	int	 topline;
	menu_ent *opts;
	WINDOW   *mw;
	char     *helpstr;
	char     *exitstr;
	void    (*post_act)(void);
	void    (*exit_act)(void);
} menudesc ;

/* defines for mopt field. */
#define MC_NOEXITOPT 1
#define MC_NOBOX 2
#define MC_SCROLL 4
#define MC_NOSHORTCUT 8
#define MC_VALID 256

/* initilization flag */
extern int __m_endwin;

/* Prototypes */
int menu_init (void);
void process_menu (int num);
void __menu_initerror (void);
int new_menu (char * title, menu_ent * opts, int numopts, 
	int x, int y, int h, int w, int mopt,
	void (*post_act)(void), void (*exit_act)(void), char * help);
void free_menu (int menu_no);

/* Menu names */
#define MENU_netbsd	0
#define MENU_utility	1
#define MENU_yesno	2
#define MENU_noyes	3
#define MENU_ok	4
#define MENU_layout	5
#define MENU_layoutparts	6
#define MENU_sizechoice	7
#define MENU_fspartok	8
#define MENU_editfsparts	9
#define MENU_edfspart	10
#define MENU_selfskind	11
#define MENU_selfskindlfs	12
#define MENU_distmedium	13
#define MENU_distset	14
#define MENU_md_distcustom	15
#define MENU_ftpsource	16
#define MENU_nfssource	17
#define MENU_nfsbadmount	18
#define MENU_fdremount	19
#define MENU_fdok	20
#define MENU_cdromsource	21
#define MENU_cdrombadmount	22
#define MENU_localfssource	23
#define MENU_localfsbadmount	24
#define MENU_localdirsource	25
#define MENU_localdirbad	26
#define MENU_namesrv6	27
#define MENU_ip6autoconf	28
#define MENU_dhcpautoconf	29

#define DYN_MENU_START	30
#define MAX_STRLEN 45
#endif
