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
 * $Id: nubus.c,v 1.7 1994/06/26 12:59:19 briggs Exp $
 *
 */

/* 
	MF 8-24-93 first hack at a real nubus driver
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/cpu.h>

#include <sys/device.h>
#include "nubus.h"


/* TODO:
	be able to do "memcpy"s from the board, then i won't
	have to fill in structures by hand.

	get rid of extra crap calls that are useless

	I think the directory stuff is bogus, I need to find
        that other magic resource that tells me fancy stuff,
        I could be wrong.


*/


struct dir *getRsrcByNum(struct slot *Slot,struct dir *p,int num,struct dir *out,int max);
int print_rsrcinfo(struct slot *Slot,struct dir *p);
char *GetStringInfo(struct slot *Slot,unsigned char *data1,char *space,int len);
int printTree(struct slot *Slot,struct dir *root);
long GetLongInfo(struct slot *,unsigned char *data);
int FindMagic(unsigned long *data);
int GetHeader(struct slot *Slot,unsigned long pos);
unsigned char *IncPtr(struct slot *Slot,unsigned char *p,int size);
char GetByteInfo(struct slot *Slot,unsigned char *data);
int GetRsrcs(struct slot *Slot,unsigned char *p,struct dir *Dir,int maxdir);
unsigned char *getDataAtRsrc(struct slot *Slot,struct dir *p,int num);
short GetShortInfo(struct slot *Slot,unsigned char *data);

/* this is the main I used in macos to get stuff out */
#if 0
main()
{

	unsigned long *rawImage;
	struct imagedata image;
	struct dir *Dir;
	struct dir dirSpace[15];
	struct dir dirSpace2[10];
	unsigned long length;
	unsigned long offset;
	unsigned long nextoffset;
	int pos,i;
	unsigned char *data;
	struct slot Slot;

//	firstHeader=(char *)0xbfffff-40;

	data=(char *)SLOTADDR-100;
	
	for(i=0;i<100;i++)
		if (Slot.size=FindMagic( (long *)(data+i) ) )
		{
			/* printf("magic found at i=%d\n",i); */
			GetHeader(&Slot,SLOTADDR-100+i);
			break;
		}
	


	
	printf("main directory\n");
	print_rsrcinfo(&Slot,Slot.mainDir );

	Dir=getRsrcByNum(&Slot,Slot.mainDir,128,dirSpace,15);
	
	printf("image directory\n"); 
	print_rsrcinfo(&Slot,Dir); 
	
	printTree(&Slot,Slot.mainDir);
	
// get image param stuff	
	Dir=getRsrcByNum(&Slot,Dir,128,dirSpace2,10);	

	/* hopefully video mode params */
	rawImage=getDataAtRsrc(&Slot,Dir,1);
	
	image.whatTheHellIsThis=GetLongInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,4);
	
	image.offset=GetLongInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,4);
	
	image.rowbytes=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);
	
	image.top=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.left=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.bottom=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.right=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.version=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.packType=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.packSize=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.hRes=GetLongInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,4);

	image.vRes=GetLongInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,4);

	image.pixelType=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);

	image.pixelSize=GetShortInfo(&Slot,rawImage);
	rawImage=IncPtr(&Slot,rawImage,2);


}
#endif /* main */

int GetHeader(struct slot *Slot,unsigned long pos)
{
	/* the pos passed in is the pos that the magic testvalue was found at */
	unsigned char *p;
	unsigned char *dirBase;
	
	switch (Slot->size)
	{
		case 1:	/* char */
			pos-=14;
			break;
		case 2:
			pos-=28;
			break;
		case 4:
			pos-=56;
			break;
	}

	p=(unsigned char *)pos;
	
	Slot->head.offset=(0xff000000 | (unsigned long) GetLongInfo(Slot,p));
	p=IncPtr(Slot,p,4);
	
	Slot->head.length=GetLongInfo(Slot,p);
	p=IncPtr(Slot,p,4);
	
	Slot->head.crc=GetLongInfo(Slot,p);
	p=IncPtr(Slot,p,4);
	
	Slot->head.romrev=GetByteInfo(Slot,p);
	p=IncPtr(Slot,p,1);

	Slot->head.format=GetByteInfo(Slot,p);
	p=IncPtr(Slot,p,1);

	Slot->head.tst=GetLongInfo(Slot,p);
	p=IncPtr(Slot,p,4);

	Slot->head.reserved=GetByteInfo(Slot,p);
	p=IncPtr(Slot,p,1);

/* byte lanes should be used instead of size, this hasn't bitten me yet */
	Slot->head.bytelane=GetByteInfo(Slot,p);
	p=IncPtr(Slot,p,1);
	
	dirBase=(unsigned char *)(pos+Slot->head.offset*Slot->size);
	GetRsrcs(Slot,dirBase,Slot->mainDir,15);
	return 0;
}


printTree(struct slot *Slot,struct dir *root)
{
	struct dir *b;
	unsigned char *c;
	struct dir *d;
	unsigned char *e;
	struct dir *f;
	unsigned char *g;
	struct dir *h;
	unsigned char *i;
	unsigned char *j;
	unsigned char *k;
	struct dir bSpace[15];
	struct dir cSpace[15];
	struct dir dSpace[15];
	struct dir fSpace[15];
	struct dir hSpace[15];
	char space[40];
/* to get a good idea of what is happening here you should get the
"slots" program from apple dts, it is so cool.  Its the next 
best thing to actual docs, which i didn't have...
*/
	
	b=getRsrcByNum(Slot,root,1,bSpace,15);
	c=getDataAtRsrc(Slot,b,2);
	d=getRsrcByNum(Slot,b,0x24,dSpace,15);
	
	e=getDataAtRsrc(Slot,d,1);

	f=getRsrcByNum(Slot,root,0x80,fSpace,15);
	g=getDataAtRsrc(Slot,f,2);
	j=getDataAtRsrc(Slot,f,0x0a);
	k=getDataAtRsrc(Slot,f,0x0b);
	
	h=getRsrcByNum(Slot,root,0xa0,hSpace,15);
	i=getDataAtRsrc(Slot,h,2);

	printf("A\n");
	print_rsrcinfo(Slot,root);

	printf("B\n");
	print_rsrcinfo(Slot,b);
	
	printf("C\n");
	printf("%s\n",GetStringInfo(Slot,c,space,40));

	printf("D\n");
	print_rsrcinfo(Slot,d);
	
	printf("E\n");
	printf("%s\n",GetStringInfo(Slot,e,space,40));
	
	printf("F\n");
	print_rsrcinfo(Slot,f);
	
	
	printf("g\n");
	printf("%s\n",GetStringInfo(Slot,g,space,40));
	printf("Video RAM Base %lx\n", GetLongInfo(Slot,j) );
	printf("Video RAM Length %lx\n", GetLongInfo(Slot,k) );

	printf("H\n");
	print_rsrcinfo(Slot,h);

	printf("I\n");
	printf("%s\n",GetStringInfo(Slot,i,space,40));
	
}


print_rsrcinfo(struct slot *Slot,struct dir *p)
{
	int i=0;
	int failsafe=20;
	
	if (p==NULL) return 1;
	while(failsafe--)
	{
		printf("RSRC %02x :%06lx\n",p[i].rsrc,p[i].offset);
		if (p[i].rsrc == 0xff) break;
		i++;
	}

}

struct dir *getRsrcByNum(struct slot *Slot,struct dir *p,int num,struct dir *out,int max)
{
	int i=0;
	int failsafe=20;
	long nextoffset=0;
	unsigned char *base;
	
	if (p==NULL) return NULL;

	
	base=getDataAtRsrc(Slot,p,num);

	if (NULL==base) return NULL;
	
	GetRsrcs(Slot,base,out,max);
	
	return out;
}

char *GetStringInfo(struct slot *Slot,unsigned char *data,char *space,int len)
{
	int i;
	char *p=space;
	
	if (NULL==data) return "";
	
	for(i=0;(i<len) && *data;i++,p++)
	{
		*p=GetByteInfo(Slot,data);
		data=IncPtr(Slot,data,1);
	}
	*p='\0';
	return space;
}

long GetLongInfo(struct slot *Slot,unsigned char *data)
{
	long ret=0;
	
	switch (Slot->size)
	{
		case 1:
			ret= (unsigned long)data[0]<<24  | (unsigned long)data[1]<<16 | (unsigned long)data[2]<<8 |data[3] ;
			break;
		case 2:
			ret= (unsigned long)data[0]<<24  | (unsigned long)data[2]<<16 | (unsigned long)data[4]<<8 |data[6] ;
			break;
		case 4:
			ret= (unsigned long)data[0]<<24  | (unsigned long)data[4]<<16 | (unsigned long)data[8]<<8 |data[12] ;
			break;
	}
	
	return ret;
}

short GetShortInfo(struct slot *Slot,unsigned char *data)
{
	short ret;
	
	switch (Slot->size)
	{
		case 1:
			ret=  (unsigned long)data[0]<<8 |data[1] ;
			break;
		case 2:
			ret=  (unsigned long)data[0]<<8 |data[2] ;
			break;
		case 4:
			ret=  (unsigned long)data[0]<<8 |data[4] ;
			break;
	}
	
	return ret;
}

char GetByteInfo(struct slot *Slot,unsigned char *data)
{
	/* boring  .... */	
	return data[0];
}


int FindMagic(unsigned long data[])
{
	unsigned short *data2=(unsigned short *)data;
	unsigned char *data3=(unsigned char *)data;
	

	/* char data */
	if (((data3[0] )== 0x5a) &&
		((data3[1] )== 0x93) &&
		((data3[2] )== 0x2b) &&
		((data3[3] )== 0xc7) )
		return 1;
	
	/* short data */
	
	if (((data3[0] )== 0x5a) &&
		((data3[2] )== 0x93) &&
		((data3[4] )== 0x2b) &&
		((data3[6] )== 0xc7) )
		return 2;
		
	/* long data */
	if (((data3[0] )== 0x5a) &&
		((data3[4] )== 0x93) &&
		((data3[8] )== 0x2b) &&
		((data3[12] )== 0xc7) )
		return 4;
		

	return 0;
}

unsigned char *IncPtr(struct slot *Slot,unsigned char *p,int size)
{
/* MF make this a macro someday */

	unsigned char *tmp=p;
	
	tmp=tmp+size*Slot->size;
	
	return tmp;
	
}

int GetRsrcs(struct slot *Slot,unsigned char *p,struct dir *Dir,int maxdir)
{
	int i=0;
/* MF if you alias memory here you will be fucked. */
	
	if (p==NULL) return 1;
	
	while(maxdir--)
	{
		long entry;
		
		entry=GetLongInfo(Slot,p);
		
		Dir[i].rsrc=(entry 	& 0xff000000) >> 24;
		Dir[i].offset=entry & 0x00ffffff;
		Dir[i].base=(unsigned long)p;
		p=IncPtr(Slot,p,4);
		if (Dir[i].rsrc==0xff)
			break;
		i++;
	}

	return 0;
}

unsigned char *getDataAtRsrc(struct slot *Slot,struct dir *p,int num)
{
	int i=0;
	int failsafe=num;
	long nextoffset=0;
	unsigned char *base;
	
	if (p==NULL) return NULL;
	
	while(failsafe--)
	{
		if (p[i].rsrc==num)
		{
			base= (unsigned char *)( (unsigned long)Slot->size*p[i].offset+
				(unsigned long)p[i].base );
			return base;
		}
		if (p[i].rsrc==0xff) return NULL;
		i++;
	}


	
	return NULL;
}


int InitNubusSlot(unsigned long slotaddr,struct slot *newSlot)
{
	int i=0;
	struct slot Slot;
	struct dir *b;
	struct dir bSpace[5];
	struct dir *d;
	struct dir dSpace[5];
	struct dir *f;
	struct dir fSpace[5];
	unsigned char *c;
	unsigned char *e;
	unsigned char *g;

	unsigned long slotend;

	

	slotend=slotaddr+NBMEMSIZE-1;

	for(i=5;i<100;i++)
	{
/* lets be quite clear here, if magic is not on the card, then
   we will quite likely bus error, because we will read a long
   word when there are only 3 bytes left on the card, unless
   there is a card in the next slot that has readable memory starting
   at 0, so more than likely we crash, ohh well.

   The other day I heard (read) that the directory and the rest of
   the card can be in different formats, to this I say FUCK!  So
   for the time being I will just assume the whole card is in the
   same format, and let it crash most heiniously on bizzare cards
   from hell, which I hear has lots of luke warm fresca on tap.
*/
		if (Slot.size=FindMagic( (unsigned long *)(slotend-i) ) )
		{
			GetHeader(&Slot,slotend-i);
			break;
		}
	}
	
/* ohh ohh Mexico, i've never really been, but I'd sure like to go */
	if (Slot.size)
	{
		b=getRsrcByNum(&Slot,Slot.mainDir,1,bSpace,5);
		c=getDataAtRsrc(&Slot,b,2);
		d=getRsrcByNum(&Slot,b,0x24,dSpace,5);
		e=getDataAtRsrc(&Slot,d,1);
		f=getRsrcByNum(&Slot,Slot.mainDir,0x80,fSpace,5);
		g=getDataAtRsrc(&Slot,f,1);
		GetStringInfo(&Slot,c,Slot.name,40);	
/* printf("card is %s, ",Slot.name); */
		GetStringInfo(&Slot,e,Slot.manufacturer,40);	
/* printf("%s\n",Slot.manufacturer); */
/* info here is two long words (cat,type,drvrsw,drvrhw) */
		Slot.type=(GetLongInfo(&Slot,g) & 0xffff0000) >> 16;
/* printf("type is %x\n",Slot.type); */

/* sounds so simple with the sun sinking low */
		
	}
	else
		return 1;
/* this comment intentionally left meaningless */

	*newSlot=Slot;
	return 0;
}

struct imagedata *NUBUS_GetImageData(struct slot *Slot,
			struct imagedata *Rimage)
{
	struct imagedata image;
	struct dir *Dir;
	struct dir dirSpace[10];
	struct dir dirSpace2[10];
	unsigned char *rawImage;


	Dir=getRsrcByNum(Slot,Slot->mainDir,128,dirSpace,10);
	Dir=getRsrcByNum(Slot,Dir,128,dirSpace2,10);	

	rawImage=getDataAtRsrc(Slot,Dir,1);
	
/* this is self documenting code, WHAT THE HELL IS THIS? */
	image.whatTheHellIsThis=GetLongInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,4);
	
	image.offset=GetLongInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,4);
	
	image.rowbytes=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);
	
	image.top=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.left=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.bottom=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.right=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.version=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.packType=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.packSize=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.hRes=GetLongInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,4);

	image.vRes=GetLongInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,4);

	image.pixelType=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	image.pixelSize=GetShortInfo(Slot,rawImage);
	rawImage=IncPtr(Slot,rawImage,2);

	*Rimage=image;

	return Rimage;
}

struct	nubus_hw nubus_table[NUBUS_MAXSLOTS];

extern int
nubus_addr_to_slot(caddr_t addr)
{
	int nubus_num;

	for (nubus_num = 0 ; nubus_num < NUBUS_MAXSLOTS ; nubus_num++)
		if (nubus_table[nubus_num].addr == addr)
			return nubus_num;
	return -1;
}

static void
find_nubus(void)
{
   /* This functions sets up the array "nubus_table" which contains the
   basic information about each card in the Nubus slot.  When device
   drivers are initialized later, they can look through this array to
   see if their hardware is present and claim it. */

   extern unsigned long	NuBusBase;
   register struct nubus_hw *nu;
   int nubus_num;

   for (nubus_num = 0; nubus_num < NUBUS_MAXSLOTS; nubus_num++)
     nubus_table[nubus_num].found = 0; /* Empty */

   /* LAK: For now we can only check 9..F because that's all we map
   in locore.s.  Eventually (i.e. near future) we should put THIS
   function in locore.s before enabling the MMU and only map the
   slots that have a card in them.  Also, the next loop should go from
   1 to 0xF inclusive (0 is "reserved") to cover all possible hardware.
   Even if the MacII only has 9..F, it won't hurt us to probe 1..8 also. */
   for (nubus_num = 0; nubus_num < 6; nubus_num++)
   {
      nu = nubus_table + nubus_num + 9;
      nu->addr = (caddr_t)(NuBusBase + nubus_num * NBMEMSIZE);
      nu->rom = nu->addr + NBROMOFFSET;

      if(!badbaddr(nu->addr+NBMEMSIZE-1))
      {
	 InitNubusSlot((unsigned long) nu->addr, &(nu->Slot));

         nu->found = 1;
         nu->claimed = 0; /* No driver has claimed this slot yet */

      }
   }
}

static int
nubus_print(aux, name)
	void	*aux;
	char	*name;
{
	struct nubus_hw	*nu = (struct nubus_hw *) aux;
      	int		i;

	if (name) {
		i = nu - nubus_table;
		printf ("%s: s:%d t:%d \"",
			 name, i, nu->Slot.type);
		printf ("%s, ",nu->Slot.name);
		printf ("%s\"",nu->Slot.manufacturer);
	}
	return(UNCONF);
}

static void
nubus_attach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	struct cfdriver		 *cf;
	register struct nubus_hw *nu;
	int			 i;

	printf("\n");

	find_nubus();

	for (i = 0; i < 6; i++) {
		nu = nubus_table + i + 9;

		if (!nu->found)
			continue;

		if (config_found(dev, nu, nubus_print))
			nu->claimed = 1;
	}

}

extern int matchbyname();

struct cfdriver nubuscd =
      { NULL, "nubus", matchbyname, nubus_attach,
	DV_DULL, sizeof(struct device), NULL, 0 };
