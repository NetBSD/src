/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)ppi.c	7.3 (Berkeley) 12/16/90
 *	$Id: par.c,v 1.1.1.1 1993/09/02 16:53:41 mw Exp $
 */

/*
 * parallel port interface

   XXXX NOT YET WORKING  XXXXX
 */

#include "ppi.h"
#if NPAR > 0

#include "sys/param.h"
#include "sys/errno.h"
#include "sys/uio.h"
#include "sys/malloc.h"

#include "device.h"
#include "parioctl.h"

int	parattach(), parstart(), partimo(), parintr();
struct	driver pardriver = {
	parattach, "par", parstart,
};

struct	par_softc {
	int	sc_flags;
	struct	amiga_device *sc_ad;
	struct	parparam sc_param;
#define sc_burst sc_param.burst
#define sc_timo  sc_param.timo
#define sc_delay sc_param.delay
} par_softc[NPPI];

/* sc_flags values */
#define	PARF_ALIVE	0x01	
#define	PARF_OPEN	0x02	
#define PARF_UIO	0x04
#define PARF_TIMO	0x08
#define PARF_DELAY	0x10
#define PARF_OREAD	0x40
#define PARF_OWRITE	0x80

#define UNIT(x)		minor(x)

#ifdef DEBUG
int	pardebug = 0x80;
#define PDB_FOLLOW	0x01
#define PDB_IO		0x02
#define PDB_NOCHECK	0x80
#endif

parattach(ad)
     register struct amiga_device *ad;
{
  register struct par_softc *sc = &par_softc[ad->amiga_unit];

#ifdef DEBUG
  if ((pardebug & PDB_NOCHECK) == 0)
#endif
    sc->sc_flags = PARF_ALIVE;
  sc->sc_ad = ad;
  return(1);
}

paropen(dev, flags)
     dev_t dev;
{
  register int unit = UNIT(dev);
  register struct par_softc *sc = &par_softc[unit];
  
  if (unit >= NPAR || (sc->sc_flags & PARF_ALIVE) == 0)
    return(ENXIO);
#ifdef DEBUG
  if (pardebug & PDB_FOLLOW)
    printf("paropen(%x, %x): flags %x\n",
	   dev, flags, sc->sc_flags);
#endif
  if (sc->sc_flags & PARF_OPEN)
    return(EBUSY);
  /* can either read or write, but not both */
  if ((flags & (FREAD|FWRITE)) == (FREAD|FWRITE))
    return EINVAL;
  
  sc->sc_flags |= PARF_OPEN;
  if (flags & FREAD)
    sc->sc_flags |= PARF_OREAD;
  else
    sc->sc_flags |= PARF_OWRITE;
  sc->sc_burst = PAR_BURST;
  sc->sc_timo = parmstohz(PAR_TIMO);
  sc->sc_delay = parmstohz(PAR_DELAY);
  /* enable interrupts for CIAA-FLG */
  ciaa.icr = CIA_ICR_IR_SC | CIA_ICR_FLG;
  return(0);
}

parclose(dev, flags)
     dev_t dev;
{
  register int unit = UNIT(dev);
  register struct par_softc *sc = &par_softc[unit];

#ifdef DEBUG
  if (pardebug & PDB_FOLLOW)
    printf("parclose(%x, %x): flags %x\n",
	   dev, flags, sc->sc_flags);
#endif
  sc->sc_flags &= ~(PARF_OPEN|PARF_OREAD|PARF_OWRITE);
  /* don't allow interrupts for CIAA-FLG any longer */
  ciaa.icr = CIA_ICR_FLG;
  return(0);
}

parstart(unit)
     int unit;
{
#ifdef DEBUG
  if (pardebug & PDB_FOLLOW)
    printf("parstart(%x)\n", unit);
#endif
  par_softc[unit].sc_flags &= ~PARF_DELAY;
  wakeup(&par_softc[unit]);
}

partimo(unit)
     int unit;
{
#ifdef DEBUG
  if (pardebug & PDB_FOLLOW)
    printf("partimo(%x)\n", unit);
#endif
  par_softc[unit].sc_flags &= ~(PARF_UIO|PARF_TIMO);
  wakeup(&par_softc[unit]);
}

parread(dev, uio)
     dev_t dev;
     struct uio *uio;
{

#ifdef DEBUG
  if (pardebug & PDB_FOLLOW)
    printf("parread(%x, %x)\n", dev, uio);
#endif
  return (parrw(dev, uio));
}

parwrite(dev, uio)
     dev_t dev;
     struct uio *uio;
{

#ifdef DEBUG
  if (pardebug & PDB_FOLLOW)
    printf("parwrite(%x, %x)\n", dev, uio);
#endif
  return (parrw(dev, uio));
}

parrw(dev, uio)
     dev_t dev;
     register struct uio *uio;
{
  int unit = UNIT(dev);
  register struct par_softc *sc = &par_softc[unit];
  register int s, len, cnt;
  register char *cp;
  int error = 0, gotdata = 0;
  int buflen;
  char *buf;

  if ((sc->sc_flags & PARF_OREAD) ^^ (uio->uio_rw == UIO_READ))
    return EINVAL;

  if (uio->uio_resid == 0)
    return(0);

#ifdef DEBUG
  if (pardebug & (PDB_FOLLOW|PDB_IO))
    printf("parrw(%x, %x, %c): burst %d, timo %d, resid %x\n",
	   dev, uio, uio->uio_rw == UIO_READ ? 'R' : 'W',
	   sc->sc_burst, sc->sc_timo, uio->uio_resid);
#endif
  buflen = MIN(sc->sc_burst, uio->uio_resid);
  buf = (char *)malloc(buflen, M_DEVBUF, M_WAITOK);
  sc->sc_flags |= PARF_UIO;
  if (sc->sc_timo > 0) {
    sc->sc_flags |= PARF_TIMO;
    timeout(partimo, unit, sc->sc_timo);
  }
  while (uio->uio_resid > 0) {
    len = MIN(buflen, uio->uio_resid);
    cp = buf;
    if (uio->uio_rw == UIO_WRITE) {
      error = uiomove(cp, len, uio);
      if (error)
	break;
    }
  again:
    s = splbio();
#if 0
    if ((sc->sc_flags & PARF_UIO) && hpibreq(&sc->sc_dq) == 0)
      sleep(sc, PRIBIO+1);
#endif
    /*
     * Check if we timed out during sleep or uiomove
     */
    (void) splsoftclock();
    if ((sc->sc_flags & PARF_UIO) == 0) {
#ifdef DEBUG
      if (pardebug & PDB_IO)
	printf("parrw: uiomove/sleep timo, flags %x\n",
	       sc->sc_flags);
#endif
      if (sc->sc_flags & PARF_TIMO) {
	untimeout(partimo, unit);
	sc->sc_flags &= ~PARF_TIMO;
      }
      splx(s);
      break;
    }
    splx(s);
    /*
     * Perform the operation
     */
    if (uio->uio_rw == UIO_WRITE)
      cnt = hpibsend(sc->sc_ad->amiga_ctlr, sc->sc_ad->amiga_slave,
		     sc->sc_sec, cp, len);
    else
      cnt = hpibrecv(sc->sc_ad->amiga_ctlr, sc->sc_ad->amiga_slave,
		     sc->sc_sec, cp, len);
    s = splbio();
    hpibfree(&sc->sc_dq);
#ifdef DEBUG
    if (pardebug & PDB_IO)
      printf("parrw: %s(%d, %d, %x, %x, %d) -> %d\n",
	     uio->uio_rw == UIO_READ ? "recv" : "send",
	     sc->sc_ad->amiga_ctlr, sc->sc_ad->amiga_slave,
	     sc->sc_sec, cp, len, cnt);
#endif
    splx(s);
    if (uio->uio_rw == UIO_READ) {
      if (cnt) {
	error = uiomove(cp, cnt, uio);
	if (error)
	  break;
	gotdata++;
      }
      /*
       * Didn't get anything this time, but did in the past.
       * Consider us done.
       */
      else if (gotdata)
	break;
    }
    s = splsoftclock();
    /*
     * Operation timeout (or non-blocking), quit now.
     */
    if ((sc->sc_flags & PARF_UIO) == 0) {
#ifdef DEBUG
      if (pardebug & PDB_IO)
	printf("parrw: timeout/done\n");
#endif
      splx(s);
      break;
    }
    /*
     * Implement inter-read delay
     */
    if (sc->sc_delay > 0) {
      sc->sc_flags |= PARF_DELAY;
      timeout(parstart, unit, sc->sc_delay);
      error = tsleep(sc, PCATCH|PZERO-1, "par-cdelay", 0);
      if (error) {
	splx(s);
	break;
      }
    }
    splx(s);
    /*
     * Must not call uiomove again til we've used all data
     * that we already grabbed.
     */
    if (uio->uio_rw == UIO_WRITE && cnt != len) {
      cp += cnt;
      len -= cnt;
      cnt = 0;
      goto again;
    }
  }
  s = splsoftclock();
  if (sc->sc_flags & PARF_TIMO) {
    untimeout(partimo, unit);
    sc->sc_flags &= ~PARF_TIMO;
  }
  if (sc->sc_flags & PARF_DELAY) {
    untimeout(parstart, unit);
    sc->sc_flags &= ~PARF_DELAY;
  }
  splx(s);
  /*
   * Adjust for those chars that we uiomove'ed but never wrote
   */
  if (uio->uio_rw == UIO_WRITE && cnt != len) {
    uio->uio_resid += (len - cnt);
#ifdef DEBUG
    if (pardebug & PDB_IO)
      printf("parrw: short write, adjust by %d\n",
	     len-cnt);
#endif
  }
  free(buf, M_DEVBUF);
#ifdef DEBUG
  if (pardebug & (PDB_FOLLOW|PDB_IO))
    printf("parrw: return %d, resid %d\n", error, uio->uio_resid);
#endif
  return (error);
}

parioctl(dev, cmd, data, flag)
dev_t dev;
int cmd;
caddr_t data;
int flag;
{
  struct par_softc *sc = &par_softc[UNIT(dev)];
  struct parparam *pp, *upp;
  int error = 0;

  switch (cmd) {
  case PARIOCGPARAM:
    pp = &sc->sc_param;
    upp = (struct parparam *)data;
    upp->burst = pp->burst;
    upp->timo = parhztoms(pp->timo);
    upp->delay = parhztoms(pp->delay);
    break;
  case PARIOCSPARAM:
    pp = &sc->sc_param;
    upp = (struct parparam *)data;
    if (upp->burst < PAR_BURST_MIN || upp->burst > PAR_BURST_MAX ||
	upp->delay < PAR_DELAY_MIN || upp->delay > PAR_DELAY_MAX)
      return(EINVAL);
    pp->burst = upp->burst;
    pp->timo = parmstohz(upp->timo);
    pp->delay = parmstohz(upp->delay);
    break;
  default:
    return(EINVAL);
  }
  return (error);
}

parhztoms(h)
int h;
{
  extern int hz;
  register int m = h;

  if (m > 0)
    m = m * 1000 / hz;
  return(m);
}

parmstohz(m)
int m;
{
  extern int hz;
  register int h = m;

  if (h > 0) {
    h = h * hz / 1000;
    if (h == 0)
      h = 1000 / hz;
  }
  return(h);
}

void
parintr (mask)
     int mask;
{
  wakeup (parintr);
}

int
parsend (buf, len)
     u_char *buf;
     int len;
{
  /* make sure I/O lines are setup right for output */

  /* control lines set to input */
  ciab.ddra &= ~(CIAB_PRA_SEL|CIAB_PRA_POUT|CIAB_PRA_BUSY);
  /* data lines to output */
  ciaa.ddrb = 0xff;
  
  while (len)
    {
      
    }
}






#endif
