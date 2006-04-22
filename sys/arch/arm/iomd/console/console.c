/*	$NetBSD: console.c,v 1.14.6.1 2006/04/22 11:37:17 simonb Exp $	*/

/*
 * Copyright (c) 1994-1995 Melvyn Tang-Richardson
 * Copyright (c) 1994-1995 RiscBSD kernel team
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the RiscBSD kernel team
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RISCBSD TEAM ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * console.c
 *
 * Console functions
 *
 * Created      : 17/09/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.14.6.1 2006/04/22 11:37:17 simonb Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/tty.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/syslog.h>
#include <sys/kernel.h>

#include <dev/cons.h>

#include <arm/iomd/vidc.h>
#include <arm/iomd/console/console.h>
#include <machine/vconsole.h>
#include <arm/arm32/katelib.h>
#include <machine/bootconfig.h>

#include "vt.h"

#define CONSOLE_VERSION "[V203E]"

/*
 * Externals
 */

extern videomemory_t videomemory;
extern struct tty *constty;
extern int debug_flags;
#define consmap_col(x) (x & 0x3)
#define CONSMAP_BOLD	8
#define CONSMAP_ITALIC	16

/*
 * Local variables (to this file) 
 */

int locked=0;			/* Nut - is this really safe ? */
struct tty *physcon_tty[NVT];
struct vconsole vconsole_master_store;
struct vconsole *vconsole_master = &vconsole_master_store;
struct vconsole *vconsole_current;
struct vconsole *vconsole_head;
struct vconsole *vconsole_default;
extern struct vconsole *debug_vc;	/* rename this to vconsole_debug */
static char undefined_string[] = "UNDEFINED";
int lastconsole;
static int printing=0;
static int want_switch=-1;

/*
 * Prototypes
 */

int	physcon_switch(u_int);
void	physconstart(struct tty *);
void	physconinit(struct consdev *);
int	physconparam(struct tty *, struct termios *);
int	physcon_switchup(void);
int	physcon_switchdown(void);
char	physcongetchar(void);
int	physconkbd(int);

void	rpcconsolecnprobe(struct consdev *);
void	rpcconsolecninit(struct consdev *);
char	rpcconsolecngetc(dev_t);
void	rpcconsolecnputc(dev_t, char);
void	rpcconsolecnpollc(dev_t, int);

struct vconsole *find_vc(dev_t);
void	vconsole_addcharmap(struct vconsole *);
static	struct vconsole *vconsole_spawn(dev_t, struct vconsole *);
void	console_flush(void);

/*
 * Exported variables
 */

#define BLANKINIT	(10*60*70)
int vconsole_pending=0;
int vconsole_blankinit=BLANKINIT;
int vconsole_blankcounter=BLANKINIT;

/*
 * Device switch
 */
dev_type_open(physconopen);
dev_type_close(physconclose);
dev_type_read(physconread);
dev_type_write(physconwrite);
dev_type_ioctl(physconioctl);
dev_type_tty(physcontty);
dev_type_poll(physconpoll);
dev_type_mmap(physconmmap);

const struct cdevsw physcon_cdevsw = {
	physconopen, physconclose, physconread, physconwrite, physconioctl,
	nostop, physcontty, physconpoll, physconmmap, ttykqfilter, D_TTY
};

/*
 * Now list all my render engines and terminal emulators
 */

extern struct render_engine vidcrender;
extern struct terminal_emulator vt220;

/*
 * These find functions should move the console it finds to the top of
 * the list to achieve a caching type of operation.  A doubly
 * linked list would be faster methinks. 
 */

struct tty *
find_tp(dev)
	dev_t dev;
{
	struct vconsole *vc;
	struct vconsole *last=0;
	int unit = minor (dev);
	int s;

	s = spltty();

	for (vc=vconsole_head; vc != NULL; vc=vc->next) {
		if (vc->number==unit) {
			if (vc != vconsole_head) {
				last->next = vc->next;
				vc->next = vconsole_head;
				vconsole_head = vc;
			}
			(void)splx(s);
			return vc->tp;
		}
		last = vc;
	}
	(void)splx(s);
	return NULL;
}

struct vconsole *
find_vc(dev)
	dev_t dev;
{
	struct vconsole *vc;
	struct vconsole *last=NULL;
	int unit = minor (dev);
	int s;

	s = spltty();

	for (vc=vconsole_head; vc!=NULL; vc=vc->next) {
		if (vc->number==unit) {
			if (vc!=vconsole_head) {
				last->next = vc->next;
				vc->next = vconsole_head;
				vconsole_head = vc;
			}
			(void)splx(s);
			return vc;
		}
		last=vc;
	}
	(void)splx(s);
	return NULL;
}

struct tty *console_tty = NULL;

/*
 * Place a graphics driver under virtual console switching control.
 */

struct vconsole *
vconsole_spawn_re(dev, vc)
	dev_t dev;
	struct vconsole *vc;
{
	struct vconsole *new;
	register int num = minor(dev);

	MALLOC(new, struct vconsole *, sizeof(struct vconsole),
	    M_DEVBUF,M_NOWAIT );
/*	printf("spawn_re:new=%08x\n", new);*/
	memset((char *)new, 0, sizeof(struct vconsole));
	*new = *vc;
	new->number = num;
	new->next = vconsole_head;
	new->tp = vc->tp;	/* Implied */
	new->data=0;
	new->t_scrolledback=0;
	new->r_scrolledback=0;
	new->r_data=0;
	new->flags=LOSSY;
	new->vtty = 0;
	vconsole_head = new;
	new->R_INIT ( new );
	new->SPAWN ( new );
	new->data = 0;
	/*new->charmap = 0;*/
	new->flags=LOSSY;
	new->proc = curproc;
	new->vtty = 0;
	return new;
}

static struct vconsole *
vconsole_spawn(dev, vc)
	dev_t dev;
	struct vconsole *vc;
{
	struct vconsole *new;
	register int num = minor(dev);

	if ( find_vc ( dev ) != 0 )
		return 0;

	MALLOC(new, struct vconsole *, sizeof(struct vconsole),
	    M_DEVBUF, M_NOWAIT );
/*	printf("spawn:new=%08x\n", new);*/

	memset((char *)new, 0, sizeof(struct vconsole));
	*new = *vc;
	new->number = num;
	new->next = vconsole_head;
	new->tp=NULL;
	new->opened=0;
	new->data=0;
	new->t_scrolledback=0;
	new->r_scrolledback=0;
	new->r_data=0;
	new->vtty = 1;
	vconsole_head = new;
	new->R_INIT ( new );
	new->FLASH ( new, 1 );
	new->CURSOR_FLASH ( new, 1 );
	new->SPAWN ( new );
	new->vtty = 1;

	new->charmap = (int *)malloc(sizeof(int)*((new->xchars)*(new->ychars)), 
	    M_DEVBUF, M_NOWAIT);
/*	printf("spawn:charmap=%08x\n", new->charmap);*/

	if (new->charmap==0)
		return 0;
	{
  	    int counter=0;
	    for ( counter=0; counter<((new->xchars)*(new->ychars)); counter++ )
		    (new->charmap)[counter]=' ';
	}
	new->TERM_INIT ( new );
	new->proc = curproc;
	return new;
}

void
vconsole_addcharmap(vc)
	struct vconsole *vc;
{
	int counter=0;

	vc->charmap = (int *)malloc(sizeof(int)*((vc->xchars)*(vc->ychars)),
	    M_DEVBUF, M_NOWAIT);
/*	printf("vc=%08x charmap=%08x\n", vc, vc->charmap);*/
	for ( counter=0; counter<((vc->xchars)*(vc->ychars)); counter++ )
		(vc->charmap)[counter]=' ';
}

int
physconopen(dev, flag, mode, l)
	dev_t dev;
	int flag;
	int mode;
	struct lwp *l;
{
	struct vconsole *new;

	struct vconsole *vc;
	int unit = minor(dev);
	int found=0;
	int majorhack=0;
	int ret;

	/*
	 * To allow the raw consoles a permanat hook for ioctls
	 * Spawning another virtual console will actuall configure it
	 */

	if ( unit >= NVT ) {
		if ( find_vc(dev)==0 )
			return ENXIO;
	}
/*
	if (unit >= rpc_cd.cd_ndevs || !rpc_cd.cd_devs[unit])
		return ENXIO;
*/

    /* If this virtual console is the real virtual console and hasn't got  */
    /* a charmap then to allocate it one.  We can be sure addcharmap works */
    /* here since this is the open routine.  This is incase the console    */
    /* was initialised before the system was brought up                    */

	if ((unit==0)&&(vconsole_master->charmap==0)) {
		if (vconsole_master==vconsole_current)
			majorhack=1;
		vconsole_addcharmap ( vconsole_master );
		vconsole_master->flags &= ~LOSSY;
	}

    /* Check to see if this console has already been spawned */

	for ( vc = vconsole_head; vc!=NULL; vc=vc->next ) {
		if ( vc->number == unit ) {
			found=1;
			break;
		}
	}

    /* Sanity check.  If we have no default console, set it to the master one */

	if ( vconsole_default==0 )
		vconsole_default = vconsole_master;

    /* Ensure we have a vconsole structure.  Allocate one if we dont already */

	if (found==0)
		new = vconsole_spawn ( dev, vconsole_default );
	else
		new = vc;

	new->proc = l->l_proc;

    /* Initialise the terminal subsystem for this device */

#define TP (new->tp)

	if (TP == NULL)
		TP = ttymalloc();

	physcon_tty[unit] = TP;

	TP->t_oproc = physconstart;
	TP->t_param = physconparam;
	TP->t_dev = dev;
	if ((TP->t_state & TS_ISOPEN) == 0) {
		ttychars(TP);
		TP->t_iflag = TTYDEF_IFLAG;
		TP->t_oflag = TTYDEF_OFLAG;
		TP->t_cflag = TTYDEF_CFLAG;
		TP->t_lflag = TTYDEF_LFLAG;
		TP->t_ispeed = TP->t_ospeed = TTYDEF_SPEED;
		physconparam(TP, &TP->t_termios);
		ttsetwater(TP);
	} else if (TP->t_state&TS_XCLUDE &&
		   suser(l->l_proc->p_ucred, &l->l_proc->p_acflag) != 0)
		return EBUSY;
	TP->t_state |= TS_CARR_ON;

	new->opened=1;
   
	TP->t_winsize.ws_col = new->xchars;
	TP->t_winsize.ws_row = new->ychars;
	ret = (*TP->t_linesw->l_open)(dev, TP);
 
	if ( majorhack==1 ) {
		struct vconsole *vc_store;
		int counter;
		int end;
		int lines;
		int xs;

		end = msgbufp->msg_bufx-1;
		if (end>=MSGBUFSIZE) end-=MSGBUFSIZE;

	/*
	 * Try some cute things.  Count the number of lines in the msgbuf
	 * then scroll the real screen up, just to fit the msgbuf on the
	 * screen, then sink all output, and spew the msgbuf to the
	 * new consoles charmap!
	 */ 

		lines = 0; xs=0; 

 		for (counter=0;counter<end;counter++) {
			xs++;
			if (*((msgbufp->msg_bufc)+counter)==0x0a) {
				if (xs>vc->xchars) lines++;	
				lines++;
				xs=0;
			}
		}

		if ( lines < vc->ychars ) {
			counter=vc->ycur;
			while ( (counter--) > lines )
				new->PUTSTRING ( "\x0a", 1, new );
		}

		new->SLEEP(new);

		vc_store = vconsole_current;
		vconsole_current = 0; /* !!! */

		/* HAHA, cant do this */
		/* new->CLS ( new ); */

		new->PUTSTRING ( "\x0c", 1, new );

	/*
	 * Hmmm, I could really pass the whole damn thing to putstring
	 * since it doesn't have zeros, but I need to do the crlf
	 * conversion
	 */

		xs=0;
	
		if ( end < 0 )
			panic ( "msgbuf trashed reboot and try again" );

		for (counter=0;counter<end;counter++) {
			if (*((msgbufp->msg_bufc)+counter)==0x0a) {
				new->PUTSTRING ( "\x0d", 1, new );
				xs=0;
			}
			if ( (xs++)<new->xchars )
				new->PUTSTRING ((msgbufp->msg_bufc)+counter, 1, new );
		}
		vconsole_master->ycur = lines;
		vconsole_current = vc_store;
		new->WAKE(new);
	        vconsole_current->xcur = 0;

	        printf ( "\x0a" );
	    }	 

	return(ret);
}

/*
 * int physconclose(dev_t dev, int flag, int mode, struct lwp *l)
 *
 * Close the physical console
 */

int
physconclose(dev, flag, mode, l)
	dev_t dev;
	int flag;
	int mode;
	struct lwp *l;
{
	register struct tty *tp;

	tp = find_tp ( dev );
	if (tp == NULL) {
		printf("physconclose: tp=0 dev=%04x\n", dev);
		return(ENXIO);
	}
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	return(0);
}

int
physconread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = find_tp ( dev );
	if (tp == NULL) {
		printf("physconread: tp=0 dev=%04x\n", dev);
		return(ENXIO);
	}
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
physconwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp;

	tp = find_tp(dev);

	if (tp == NULL) {
		printf("physconwrite: tp=0 dev=%04x\n", dev);
		return(ENXIO);
	}

	return((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
physconpoll(dev, events, l)
	dev_t dev;
	int events;
	struct lwp *l;
{
	register struct tty *tp;

	tp = find_tp(dev);

	if (tp == NULL) {
		printf("physconpoll: tp=0 dev=%04x\n", dev);
		return(ENXIO);
	}
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
physcontty(dev)
	dev_t dev;
{
	return(find_tp(dev));
}

int ioctlconsolebug;

int
physconioctl(dev, cmd, data, flag, l)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct lwp *l;
{
	struct vconsole vconsole_new;
	struct tty *tp=(struct tty *)0xDEADDEAD ;
	int error;
	int s;
	struct vconsole *vc = find_vc(dev);

	ioctlconsolebug = cmd;

	tp = find_tp(dev);

	if ((vc==0)||(tp==0))
		return ENXIO;

	switch ( cmd ) {
	case CONSOLE_SWITCHUP:
		physcon_switchup ();
		return 0;
	case CONSOLE_SWITCHDOWN:
		physcon_switchdown ();
		return 0;
	case CONSOLE_SWITCHTO:
		if (!physcon_switch ( *(int *)data ))
			return 0;
		else
			return EINVAL;
	case CONSOLE_SWITCHPREV:
		physcon_switch ( lastconsole );
		return 0;
	case CONSOLE_CREATE:
	{
		int maj;
		maj = cdevsw_lookup_major(&physcon_cdevsw);
		if ( vconsole_spawn ( makedev ( maj, *(int *)data ),
		    vconsole_default ) == 0 )
			return ENOMEM;
		else
			return 0;
		break;
	}

	case CONSOLE_RENDERTYPE:
		strncpy ( (char *)data, vc->T_NAME, 20 );
		return 0;

	case CONSOLE_TERMTYPE:
		strncpy ( (char *)data, vc->T_NAME, 20 );
		return 0;

	case CONSOLE_LOCK:
		s = spltty();
		locked++;
		(void)splx(s);
		return 0;
	case CONSOLE_UNLOCK:
		s = spltty();
		locked--;
		if ( locked<0 )
			locked=0;
		(void)splx(s);
		return 0;
	case CONSOLE_SPAWN_VIDC:
/*
		vconsole_new = *vconsole_default;
*/
		vconsole_new = *vc;
		vconsole_new.render_engine = &vidcrender;
		if ( vconsole_spawn_re ( 
		    makedev ( cdevsw_lookup_major(&physcon_cdevsw),
			      *(int *)data ),
		    &vconsole_new ) == 0 )
			return ENOMEM;
		else
			return 0;

 	case CONSOLE_GETVC:
	    {
/*	   	struct vconsole *vc_p;	
	   	vc_p = find_vc(dev);
		*(int *)data = vc_p->number;*/
		*(int *)data = vconsole_current->number;
		return 0;
	    }

	case CONSOLE_CURSORFLASHRATE:
		vc->CURSORFLASHRATE ( vc, *(int *)data );		
		return 0;

	case CONSOLE_BLANKTIME:
		{
		int blanktime = (*(int *)data);
		struct vidc_mode *vm = &((struct vidc_info *)vc->r_data)->mode;

		/*
		 * +ve set blank time
		 * 0   force blank immediately (no time change)
		 * -ve disable blank time
		 */

		if (vm->frame_rate)
			blanktime *= vm->frame_rate;
		else
			blanktime *= 70;

		if (blanktime == 0) {
			if (vc == vconsole_current)
				vconsole_current->BLANK(vconsole_current, BLANK_OFF);
		} else {
			if (vc == vconsole_current)
				vconsole_blankinit = blanktime;
			vc->blanktime = blanktime;
			if (blanktime < 0)
				vconsole_current->BLANK(vconsole_current, BLANK_NONE);
		}
		return 0;
		}

	case CONSOLE_DEBUGPRINT:
		{
		    struct vconsole *vc_p;	

		    vc_p = find_vc(makedev(cdevsw_lookup_major(&physcon_cdevsw),
				   *(int*)data));
		    if (vc_p==0) return EINVAL;
		    printf ( "DEBUGPRINT for console %d\n", *(int*)data );
		    printf ( "flags %08x vtty %01x\n", vc_p->flags, vc_p->vtty );
		    printf ( "TTY INFO - winsize (%d, %d)\n",
				vc_p->tp->t_winsize.ws_col,
				vc_p->tp->t_winsize.ws_row);
		    vc_p->R_DEBUGPRINT ( vc_p );
		    vc_p->T_DEBUGPRINT ( vc_p );
		    return 0;
		}
		
	default: 
		error = vc->IOCTL ( vc, dev, cmd, data, flag, l );
		if (error != EPASSTHROUGH)
			return error;
		error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
		if (error != EPASSTHROUGH)
			return error;
		error = ttioctl(tp, cmd, data, flag, l);
		if (error != EPASSTHROUGH)
			return error;
	} 
	return(EPASSTHROUGH);
}

paddr_t
physconmmap(dev, offset, nprot)
	dev_t dev;
	off_t offset;
	int nprot;
{
	struct vconsole *vc = find_vc(dev);
	paddr_t physaddr;

	if (minor(dev) < 64) {
		log(LOG_WARNING, "You should no longer use ttyv to mmap a frame buffer\n");
		log(LOG_WARNING, "For vidc use /dev/vidcrender0\n");
	}
	physaddr = vc->MMAP(vc, offset, nprot);
	return(physaddr);
}

/*
 * Perform output on specified tty stream
 */

void
physconstart(tp)
	struct tty *tp;
{
	int s, len;
	struct clist *cl;
	struct vconsole *vc;
	u_char buf[128];

	s = spltty();

	vc = find_vc ( tp->t_dev );

	/* Are we ready to perform output */

	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		(void)splx(s);
		return;
	}

	tp->t_state |= TS_BUSY;

	/* Fill our buffer with the data to print */

	cl = &tp->t_outq;

	if ( vc->r_scrolledback ) vc->R_SCROLLBACKEND ( vc );
	if ( vc->t_scrolledback ) vc->T_SCROLLBACKEND ( vc );

	(void)splx(s);

    /* Apparently we do this out of spl since it _IS_ fairly expensive */
    /* and it stops the serial ports overflowing 		       */

	while ( (len = q_to_b(cl, buf, 128)) ) {
		if ( vc!=NULL )
			vc->PUTSTRING(buf, len, vc);
	}

	s = spltty ();

	tp->t_state &= ~TS_BUSY;

	if (cl->c_cc) {
		tp->t_state |= TS_TIMEOUT;
		callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, tp);
	}

	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(cl);
		}
		selwakeup(&tp->t_wsel);
	}

	if ( want_switch != -1 ) {
		physcon_switch ( want_switch );
		want_switch=-1;
	}

	(void)splx(s);
}

int
physconparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return(0);
}

int
physconkbd(key)
	int key;
{
	const char *string;
	register struct tty *tp;
	int s;

	s = spltty();

	tp = vconsole_current->tp;

	if (tp == NULL) {
		(void)splx(s);
		return(1);
	}

	if ((tp->t_state & TS_ISOPEN) == 0) {
		(void)splx(s);
		return(1);
	}

	if (key < 0x100)
		(*tp->t_linesw->l_rint)(key, tp);
	else {
	        switch (key) {
		case 0x100:
			string = "\x1b[A";
			break;
		case 0x101:
			string = "\x1b[B";
			break;
		case 0x102:
			string = "\x1b[D";
			break;
		case 0x103:
			string = "\x1b[C";
			break;
		case 0x104:
			string = "\x1b[6~";
			break;
		case 0x105:
			string = "\x1b[5~";
			break;
		case 0x108:
			string = "\x1b[2~";
			break;
		case 0x109:
			string = "\x7f";
			break;
		default:
			string = "";
			break;
		}
		while (*string != 0) {
			(*tp->t_linesw->l_rint)(*string, tp);
			++string;
		}
	}
	(void)splx(s);
	return(0);
}

static int physconinit_called = 0;

void
physconinit(cp)
	struct consdev *cp;
{
	int *test;
	int counter;

    /*
     * Incase we're called more than once.  All routines below here
     * undergo once time initialisation
     */

	if (physconinit_called)
		return;

	physconinit_called=1;

	locked=0;

	/*
	 * Create the master console
	 */

	vconsole_master->next = NULL;
	vconsole_master->number = 0;
	vconsole_master->opened = 1;
	vconsole_master->tp = NULL;

	/*
	 * Right, here I can choose some render routines
	 */

	vconsole_master->render_engine = &vidcrender;
	vconsole_master->terminal_emulator = &vt220;

	/*
	 * We will very soon loose the master console, and number 0 will
	 * become the current console so as not to waste a struct vconsole
	 */
	vconsole_current = vconsole_head = vconsole_master;
	vconsole_master->data = NULL;
	vconsole_master->vtty = 1;

	/*
	 * Perform initial checking
	 */

	if (vconsole_master->terminal_emulator->name == 0)
		vconsole_master->terminal_emulator->name = undefined_string;
	if (vconsole_master->render_engine->name == 0)
		vconsole_master->render_engine->name = undefined_string;

	/*
	 * Right, I have to assume the init and print procedures are ok
	 * or there's nothing else that can be done
	 */

	vconsole_master->R_INIT(vconsole_master);
	vconsole_master->SPAWN(vconsole_master);
	vconsole_master->TERM_INIT(vconsole_master);
	vconsole_master->flags = LOSSY;
	vconsole_master->blanktime = BLANKINIT;

	/*
	 * Now I can do some productive verification
	 */

	/* Ensure there are no zeros in the termulation and render_engine */

	test = (int *) vconsole_master->render_engine;
	for (counter=0; counter<(sizeof(struct render_engine)/4)-1; counter++)
		if (test[counter]==0)
			panic ( "Render engine %s is missins a routine",
			    vconsole_master->render_engine->name );
  
	test = (int *) vconsole_master->terminal_emulator;
	for (counter=0; counter<(sizeof(struct terminal_emulator)/4)-1; counter++)
		if (test[counter]==0)
			panic ( "Render engine %s is missing a routine",
			    vconsole_master->terminal_emulator->name );
	vconsole_master->PUTSTRING("\x0c", 1, vconsole_master);
}

#if CHUQ

/*
 * void physconputstring(char *string, int length)
 *
 * Render a string on the physical console
 */

void
physconputstring(string, length)
	char *string;
	int length;
{
	vconsole_current->PUTSTRING(string, length, vconsole_current);
}

#endif

/*
 * void physcongetchar(void)
 *
 * Get a character from the physical console
 */

int getkey_polled __P((void));

char
physcongetchar(void)
{
	return(getkey_polled());
}

void
rpcconsolecnprobe(cp)
	struct consdev *cp;
{
	int major;

/*
 * Locate the major number for the physical console device
 * We do this by searching the character device list until we find
 * the device with the open function for the physical console
 */

	major = cdevsw_lookup_major(&physcon_cdevsw);

/* Initialise the required fields */

	cp->cn_dev = makedev(major, 0);
	cp->cn_pri = CN_INTERNAL;
}

#define RPC_BUF_LEN	(64)
char rpc_buf[RPC_BUF_LEN];
int rpc_buf_ptr = 0;
static int cnpolling = 0;
#define RPC_BUF_FLUSH	\
{			\
	vconsole_current->PUTSTRING(rpc_buf, rpc_buf_ptr, vconsole_current);	\
	rpc_buf_ptr=0;	\
}

void
rpcconsolecninit(cp)
	struct consdev *cp;
{
	physconinit(cp);	/* woo Woo WOO!!!, woo, woo, yes ok bye */
}


void
rpcconsolecnputc(dev, character)
	dev_t dev;
	char character;
{

	if (rpc_buf_ptr==RPC_BUF_LEN)
		RPC_BUF_FLUSH

	rpc_buf[rpc_buf_ptr++] = character;

	if ((character == 0x0a) || (character == 0x0d) || (character == '.') || (cold == 0) || cnpolling)
		RPC_BUF_FLUSH
}

void
console_flush()
RPC_BUF_FLUSH

#if CHUQ

int
console_switchdown()
{
	physcon_switchdown();
	return 0;
}

int
console_switchup()
{
	physcon_switchup();
	return 0;
}

#endif

int
console_unblank()
{
	if (vconsole_current) {
		vconsole_blankcounter = vconsole_current->blanktime;
		vconsole_current->BLANK(vconsole_current, BLANK_NONE);
	}
	return 0;
}

int
console_scrollback ()
{
	if (vconsole_current == NULL)
		return 0;
	if (vconsole_current->R_SCROLLBACK(vconsole_current) == -1) {
		if (vconsole_current->T_SCROLLBACK(vconsole_current) == -1) {  
		}
	}
	return 0;
}

int
console_scrollforward ()
{
	if (vconsole_current == NULL)
		return 0;
	if (vconsole_current->R_SCROLLFORWARD(vconsole_current) == -1) {
		if (vconsole_current->T_SCROLLFORWARD(vconsole_current) == -1) {  
		}
	}
	return 0;
}

#if CHUQ

int
console_switchlast()
{
	return (physcon_switch(lastconsole));
}

#endif

int
physcon_switchdown()
{
	int start;
	int next = (vconsole_current->number);
	start=next;
	do {	
		next--;
		next = next&0xff;
		if (next == start) return 0;
	} while (physcon_switch(next));
        return 0;
}

int
physcon_switchup ()
{
	int start;
	int next = (vconsole_current->number);
	start=next;
	do {	
		next++;
		next = next&0xff;
		if (next == start) return 0;
	} while (physcon_switch(next));
	return 0;
}

void
console_switch(number)
	u_int number;
{
	physcon_switch(number);
}

/* switchto */
int 
physcon_switch(number)
	u_int number;
{
	register struct vconsole *vc;
        int s = spltty ();
        int ret;

	if (locked != 0) {
		ret=0;
		goto out;
        }

	if (printing) {
		want_switch = number;
		ret=0;
		goto out;
	}

	vc = find_vc(makedev(cdevsw_lookup_major(&physcon_cdevsw), number));

	if (vc == 0) {
		ret = 1;
		goto out;
	}

	if (vc == vconsole_current) {
		ret = 1;
		goto out;
 	}

	/* Point of no return */

	locked++;		/* We cannot reenter this routine now */

	/* De-activate the render engine functions */
	if ( vconsole_current->vtty==1 ) {
		vconsole_current->SLEEP(vconsole_current);
		vconsole_current->FLASH ( vc, 0 );
		vconsole_current->CURSOR_FLASH ( vc, 0 );
	}

	/* Swap in the new consoles state */

	lastconsole = vconsole_current->number;
	vconsole_current=vc;
	vconsole_current->R_SWAPIN ( vc );

	console_unblank();

	/* Re-activate the render engine functions */

	if ( vconsole_current->vtty==1 ) {
		vconsole_current->T_SWAPIN ( vc );
		vconsole_current->WAKE(vconsole_current);
		vconsole_current->FLASH ( vc, 1 );
		vconsole_current->CURSOR_FLASH ( vc, 1 );
	}

	locked--;

	/* Tell the process about the switch, like the X server */

	if ( vc->proc )
		psignal ( vc->proc, SIGIO );

	ret = 0;
out:
	(void)splx(s);
	return(ret);
}

char
rpcconsolecngetc(dev)
	dev_t dev;
{
	return( physcongetchar () );
}

void
rpcconsolecnpollc(dev, on)
	dev_t dev;
	int on;
{
	RPC_BUF_FLUSH
	cnpolling = on;
}

int rpcprobe(struct device *, struct cfdata *, void *);
void rpcattach(struct device *, struct device *, void *);

int
rpcprobe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return(1);
}

void
rpcattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	printf(": console driver %s using %s %s\n",
	    CONSOLE_VERSION, vconsole_master->terminal_emulator->name,
	    vconsole_master->render_engine->name);

	vconsole_master->T_ATTACH(vconsole_master, parent, self, aux);
	vconsole_master->R_ATTACH(vconsole_master, parent, self, aux);
}

CFATTACH_DECL(vt, sizeof(struct device),
    rpcprobe, rpcattach, NULL, NULL);

extern struct terminal_emulator vt220;

struct render_engine *render_engine_tab[] = {
        &vidcrender,
};

struct terminal_emulator *terminal_emulator_tab[] = {
        &vt220,
};
