/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/* #include "stand.h"  */
#include "via.h"

#define PRAM_SIZE	256 
#define PRAM_GET	256 
#define RTC_DATA	0x1 
#define RTC_CLOCK	0x2 
#define RTC_ENB		0x4

#define PRAMCMD_SEC0	0x01
#define PRAMCMD_SEC1	0x05
#define PRAMCMD_SEC2	0x09
#define PRAMCMD_SEC3	0x0D
#define PRAMCMD_TEST	0x31
#define PRAMCMD_WPRT	0x35
#define PRAMCMD_RA1x	0x21
#define	PRAMCMD_RA0x	0x41
#define	PRAMCMD_READ	0x80
#define PRAMBIT_WRIT	0xf0
#define PRAMADDR_SHF	2

#define PRAMCMD_SEL	0x38
#define PRAMCMD_EXT	0x0
#define PRAM_SECT_MASK	0xE0 /* 0x1C0 */
#define PRAM_BYTE_MASK	0x1F /* 0x3F */
#define PRAM_SECT_SHF	5
#define PRAM_BYTE_SHF	2

unsigned char pram_save[PRAM_SIZE];
int pram_chr = 0;

void pram_enable()
{
	/* enable serial function */
   via_reg(VIA1, vDirB) = RTC_ENB;
   via_reg(VIA1, vBufB) = RTC_ENB;	/* flush? */
   via_reg(VIA1, vBufB) = 0;
}


void pram_disable()
{
      /* disable serial function */
   via_reg(VIA1, vDirB) = RTC_ENB | vDirA_ADBState;
   via_reg(VIA1, vBufB) = RTC_ENB;
}


unsigned char pram_getbyte(void)
{
   int bitnum;
   unsigned char data;

   data = 0;

   via_reg(VIA1, vDirB) |= RTC_CLOCK;

   for(bitnum = 0; bitnum < 8; bitnum++){

         /* lower clock bit (please send data) */
      via_reg(VIA1, vBufB) = 0;

         /* get data bit */
      data = (data << 1) | (via_reg(VIA1, vBufB) & RTC_DATA);

         /* raise clock bit (please ready next data) */
      via_reg(VIA1, vBufB) = RTC_CLOCK;
   }

   via_reg(VIA1, vDirB) &= ~RTC_CLOCK;

   return(data);
}


void pram_putbyte(unsigned char data)
{
   int bitnum;

   via_reg(VIA1, vDirB) |= RTC_CLOCK | RTC_DATA; 

   for(bitnum = 0; bitnum < 8; bitnum++){

         /* lower clock bit (please accept data) */
      via_reg(VIA1, vBufB) = 0;

         /* send data bit */
      via_reg(VIA1, vBufB) = (data & (1 << (7 - bitnum))) ? RTC_DATA : 0;

         /* raise clock bit (please store the data) */
      via_reg(VIA1, vBufB) = RTC_CLOCK;
   }

   via_reg(VIA1, vDirB) &= ~(RTC_CLOCK | RTC_DATA); 
}


unsigned char pram_read(unsigned char cmd)
{
   unsigned char data=0;

   pram_enable();
   pram_putbyte(cmd | PRAMCMD_READ);
   data = pram_getbyte();
   pram_disable();

   return(data);
}


void pram_write(unsigned char cmd, unsigned char val)
{
   pram_enable();
   pram_putbyte(cmd);
   pram_putbyte(val);
   pram_disable();
}


unsigned char pram_ext_read(int addr)
{
   unsigned char data;
   int sect, byte;

   sect = (addr & PRAM_SECT_MASK) >> PRAM_SECT_SHF;
   byte = (addr & PRAM_BYTE_MASK) << PRAM_BYTE_SHF;
   pram_enable();
   pram_putbyte(PRAMCMD_READ | PRAMCMD_SEL | sect);
   pram_putbyte(PRAMCMD_EXT | byte);
   data = pram_getbyte();
   pram_disable();

   return(data);
}


void pram_ext_write(int addr, unsigned char val)
{
   int sect, byte;

   sect = (addr & PRAM_SECT_MASK) >> PRAM_SECT_SHF;
   byte = (addr & PRAM_BYTE_MASK) << PRAM_BYTE_SHF;
   pram_enable();
   pram_putbyte(PRAMCMD_SEL | sect);
   pram_putbyte(PRAMCMD_EXT | byte);
   pram_putbyte(val);
   pram_disable();
}


unsigned char *pram_get(void)
{
   int byte_get;

   printf("PRAM contents:\n");
   for(byte_get = 0; byte_get < PRAM_GET; byte_get++){

      pram_save[byte_get] = pram_ext_read(byte_get);

      if(byte_get % 16 == 0)
         printf("\n(%x)", byte_get);
      printf("%x%s", pram_save[byte_get], (byte_get == PRAM_GET - 1)?"":", "); 
   }
   printf("\n");
}

char *convtime(unsigned long t)
{
  static long daypmon[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
  static char *monstr[] = {"January","February","March","April","May","June",
    "July","August","September","October","November","December" };
  static char s[200];
  long year,month,day,hour,minute,seconds,i,dayperyear;

  year=1904;
  month=0;  /* Jan */
  day=1;
  hour=0;
  minute=0;
  seconds=0;

  if(t == 0xffffffff)
     return("<time value is -1>");

  while (t > 0)
  {
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    {
      dayperyear=366;
      daypmon[1]=29;
    }
    else
    {
      dayperyear=365;
      daypmon[1]=28;
    }
    i=dayperyear*60*60*24;
    if (t >= i)
    {
      t-=i;
      year++;
      continue;
    }
    i=daypmon[month]*60*60*24;
    if (t >= i)
    {
      t-=i;
      month++;
      continue;
    }
    i=60*60*24;
    if (t >= i)
    {
      t-=i;
      day++;
      continue;
    }
    i=60*60;
    if (t >= i)
    {
      t-=i;
      hour++;
      continue;
    }
    i=60;
    if (t >= i)
    {
      t-=i;
      minute++;
      continue;
    }
    seconds=t;
    t=0;
  }

  sprintf(s,"%s %d, %d   %d:%d:%d",monstr[month],day,year,hour,minute,seconds);

  return s;
}

unsigned long pram_readtime(void)
{
   unsigned long timedata,otim;

   timedata = 0;
   otim = 0xffffffff;
   while(otim != timedata){
      otim = timedata;

      timedata = pram_read(PRAMCMD_SEC3);
      timedata = (timedata << 8) | pram_read(PRAMCMD_SEC2);
      timedata = (timedata << 8) | pram_read(PRAMCMD_SEC1);
      timedata = (timedata << 8) | pram_read(PRAMCMD_SEC0);

      /* printf("time read from PRAM: %d\n", timedata); */
      /* printf("Date and time: %s\n",convtime(timedata)); */
   }

   pram_write(PRAMCMD_WPRT, 0xff);

   return(timedata);
}


int pram_settime(unsigned long time)
{
   unsigned char t1, t2, t3, t4, tmp;
   unsigned char ret;

   t1 = time & 0xff;
   t2 = (time >>= 8) & 0xff;
   t3 = (time >>= 8) & 0xff;
   t4 = (time >>= 8) & 0xff;

   pram_write(PRAMCMD_WPRT, 0);

   pram_write(PRAMCMD_SEC0, 0);
   tmp = pram_read(PRAMCMD_SEC1);
   pram_write(PRAMCMD_SEC1, (tmp + 1) & 0xff);
   if((ret=pram_read(PRAMCMD_SEC1)) != ((tmp + 1) & 0xff))
   {
printf("we read ret=%x, tmp=%x (tmp+1) &0xff= %x\n",ret,tmp,(tmp +1) & 0xff);
      return(0);
   }
   pram_write(PRAMCMD_SEC3, t4);
   pram_write(PRAMCMD_SEC2, t3);
   pram_write(PRAMCMD_SEC1, t2);
   pram_write(PRAMCMD_SEC0, t1);

   pram_write(PRAMCMD_WPRT, 0xff);

   return(-1);
}
