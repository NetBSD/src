/*	$NetBSD: blocksort.c,v 1.3 1999/07/02 15:55:41 simonb Exp $	*/

/*-------------------------------------------------------------*/
/*--- Block sorting machinery                               ---*/
/*---                                           blocksort.c ---*/
/*-------------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-1998 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software.  If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior written
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Guildford, Surrey, UK.
  jseward@acm.org
  bzip2/libbzip2 version 0.9.0 of 28 June 1998

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/


#include "bzlib_private.h"

/*---------------------------------------------*/
/*--
  Compare two strings in block.  We assume (see
  discussion above) that i1 and i2 have a max
  offset of 10 on entry, and that the first
  bytes of both block and quadrant have been
  copied into the "overshoot area", ie
  into the subscript range
  [nblock .. nblock+NUM_OVERSHOOT_BYTES-1].
--*/
static __inline__ Bool fullGtU ( UChar*  block,
                                 UInt16* quadrant,
                                 UInt32  nblock,
                                 Int32*  workDone,
                                 Int32   i1,
                                 Int32   i2
                               )
{
   Int32 k;
   UChar c1, c2;
   UInt16 s1, s2;

   AssertD ( i1 != i2, "fullGtU(1)" );

   c1 = block[i1];
   c2 = block[i2];
   if (c1 != c2) return (c1 > c2);
   i1++; i2++;

   c1 = block[i1];
   c2 = block[i2];
   if (c1 != c2) return (c1 > c2);
   i1++; i2++;

   c1 = block[i1];
   c2 = block[i2];
   if (c1 != c2) return (c1 > c2);
   i1++; i2++;

   c1 = block[i1];
   c2 = block[i2];
   if (c1 != c2) return (c1 > c2);
   i1++; i2++;

   c1 = block[i1];
   c2 = block[i2];
   if (c1 != c2) return (c1 > c2);
   i1++; i2++;

   c1 = block[i1];
   c2 = block[i2];
   if (c1 != c2) return (c1 > c2);
   i1++; i2++;

   k = nblock;

   do {

      c1 = block[i1];
      c2 = block[i2];
      if (c1 != c2) return (c1 > c2);
      s1 = quadrant[i1];
      s2 = quadrant[i2];
      if (s1 != s2) return (s1 > s2);
      i1++; i2++;

      c1 = block[i1];
      c2 = block[i2];
      if (c1 != c2) return (c1 > c2);
      s1 = quadrant[i1];
      s2 = quadrant[i2];
      if (s1 != s2) return (s1 > s2);
      i1++; i2++;

      c1 = block[i1];
      c2 = block[i2];
      if (c1 != c2) return (c1 > c2);
      s1 = quadrant[i1];
      s2 = quadrant[i2];
      if (s1 != s2) return (s1 > s2);
      i1++; i2++;

      c1 = block[i1];
      c2 = block[i2];
      if (c1 != c2) return (c1 > c2);
      s1 = quadrant[i1];
      s2 = quadrant[i2];
      if (s1 != s2) return (s1 > s2);
      i1++; i2++;

      if (i1 >= nblock) i1 -= nblock;
      if (i2 >= nblock) i2 -= nblock;

      k -= 4;
      (*workDone)++;
   }
      while (k >= 0);

   return False;
}

/*---------------------------------------------*/
/*--
   Knuth's increments seem to work better
   than Incerpi-Sedgewick here.  Possibly
   because the number of elems to sort is
   usually small, typically <= 20.
--*/
static Int32 incs[14] = { 1, 4, 13, 40, 121, 364, 1093, 3280,
                          9841, 29524, 88573, 265720,
                          797161, 2391484 };

static void simpleSort ( EState* s, Int32 lo, Int32 hi, Int32 d )
{
   Int32 i, j, h, bigN, hp;
   Int32 v;

   UChar*  block        = s->block;
   UInt32* zptr         = s->zptr;
   UInt16* quadrant     = s->quadrant;
   Int32*  workDone     = &(s->workDone);
   Int32   nblock       = s->nblock;
   Int32   workLimit    = s->workLimit;
   Bool    firstAttempt = s->firstAttempt;

   bigN = hi - lo + 1;
   if (bigN < 2) return;

   hp = 0;
   while (incs[hp] < bigN) hp++;
   hp--;

   for (; hp >= 0; hp--) {
      h = incs[hp];
      i = lo + h;
      while (True) {

         /*-- copy 1 --*/
         if (i > hi) break;
         v = zptr[i];
         j = i;
         while ( fullGtU ( block, quadrant, nblock, workDone,
                           zptr[j-h]+d, v+d ) ) {
            zptr[j] = zptr[j-h];
            j = j - h;
            if (j <= (lo + h - 1)) break;
         }
         zptr[j] = v;
         i++;

         /*-- copy 2 --*/
         if (i > hi) break;
         v = zptr[i];
         j = i;
         while ( fullGtU ( block, quadrant, nblock, workDone,
                 zptr[j-h]+d, v+d ) ) {
            zptr[j] = zptr[j-h];
            j = j - h;
            if (j <= (lo + h - 1)) break;
         }
         zptr[j] = v;
         i++;

         /*-- copy 3 --*/
         if (i > hi) break;
         v = zptr[i];
         j = i;
         while ( fullGtU ( block, quadrant, nblock, workDone,
                           zptr[j-h]+d, v+d ) ) {
            zptr[j] = zptr[j-h];
            j = j - h;
            if (j <= (lo + h - 1)) break;
         }
         zptr[j] = v;
         i++;

         if (*workDone > workLimit && firstAttempt) return;
      }
   }
}


/*---------------------------------------------*/
/*--
   The following is an implementation of
   an elegant 3-way quicksort for strings,
   described in a paper "Fast Algorithms for
   Sorting and Searching Strings", by Robert
   Sedgewick and Jon L. Bentley.
--*/

#define swap(lv1, lv2) \
   { Int32 tmp = lv1; lv1 = lv2; lv2 = tmp; }

static void vswap ( UInt32* zptr, Int32 p1, Int32 p2, Int32 n )
{
   while (n > 0) {
      swap(zptr[p1], zptr[p2]);
      p1++; p2++; n--;
   }
}

static UChar med3 ( UChar a, UChar b, UChar c )
{
   UChar t;
   if (a > b) { t = a; a = b; b = t; };
   if (b > c) { t = b; b = c; c = t; };
   if (a > b)          b = a;
   return b;
}


#define min(a,b) ((a) < (b)) ? (a) : (b)

typedef
   struct { Int32 ll; Int32 hh; Int32 dd; }
   StackElem;

#define push(lz,hz,dz) { stack[sp].ll = lz; \
                         stack[sp].hh = hz; \
                         stack[sp].dd = dz; \
                         sp++; }

#define pop(lz,hz,dz) { sp--;               \
                        lz = stack[sp].ll;  \
                        hz = stack[sp].hh;  \
                        dz = stack[sp].dd; }

#define SMALL_THRESH 20
#define DEPTH_THRESH 10

/*--
   If you are ever unlucky/improbable enough
   to get a stack overflow whilst sorting,
   increase the following constant and try
   again.  In practice I have never seen the
   stack go above 27 elems, so the following
   limit seems very generous.
--*/
#define QSORT_STACK_SIZE 1000


static void qSort3 ( EState* s, Int32 loSt, Int32 hiSt, Int32 dSt )
{
   Int32 unLo, unHi, ltLo, gtHi, med, n, m;
   Int32 sp, lo, hi, d;
   StackElem stack[QSORT_STACK_SIZE];

   UChar*  block        = s->block;
   UInt32* zptr         = s->zptr;
   Int32*  workDone     = &(s->workDone);
   Int32   workLimit    = s->workLimit;
   Bool    firstAttempt = s->firstAttempt;

   sp = 0;
   push ( loSt, hiSt, dSt );

   while (sp > 0) {

      AssertH ( sp < QSORT_STACK_SIZE, 1001 );

      pop ( lo, hi, d );

      if (hi - lo < SMALL_THRESH || d > DEPTH_THRESH) {
         simpleSort ( s, lo, hi, d );
         if (*workDone > workLimit && firstAttempt) return;
         continue;
      }

      med = med3 ( block[zptr[ lo         ]+d],
                   block[zptr[ hi         ]+d],
                   block[zptr[ (lo+hi)>>1 ]+d] );

      unLo = ltLo = lo;
      unHi = gtHi = hi;

      while (True) {
         while (True) {
            if (unLo > unHi) break;
            n = ((Int32)block[zptr[unLo]+d]) - med;
            if (n == 0) { swap(zptr[unLo], zptr[ltLo]); ltLo++; unLo++; continue; };
            if (n >  0) break;
            unLo++;
         }
         while (True) {
            if (unLo > unHi) break;
            n = ((Int32)block[zptr[unHi]+d]) - med;
            if (n == 0) { swap(zptr[unHi], zptr[gtHi]); gtHi--; unHi--; continue; };
            if (n <  0) break;
            unHi--;
         }
         if (unLo > unHi) break;
         swap(zptr[unLo], zptr[unHi]); unLo++; unHi--;
      }

      AssertD ( unHi == unLo-1, "bad termination in qSort3" );

      if (gtHi < ltLo) {
         push(lo, hi, d+1 );
         continue;
      }

      n = min(ltLo-lo, unLo-ltLo); vswap(zptr, lo, unLo-n, n);
      m = min(hi-gtHi, gtHi-unHi); vswap(zptr, unLo, hi-m+1, m);

      n = lo + unLo - ltLo - 1;
      m = hi - (gtHi - unHi) + 1;

      push ( lo, n, d );
      push ( n+1, m-1, d+1 );
      push ( m, hi, d );
   }
}


/*---------------------------------------------*/

#define BIGFREQ(b) (ftab[((b)+1) << 8] - ftab[(b) << 8])

#define SETMASK (1 << 21)
#define CLEARMASK (~(SETMASK))

static void sortMain ( EState* s )
{
   Int32 i, j, k, ss, sb;
   Int32 runningOrder[256];
   Int32 copy[256];
   Bool  bigDone[256];
   UChar c1, c2;
   Int32 numQSorted;

   UChar*  block        = s->block;
   UInt32* zptr         = s->zptr;
   UInt16* quadrant     = s->quadrant;
   Int32*  ftab         = s->ftab;
   Int32*  workDone     = &(s->workDone);
   Int32   nblock       = s->nblock;
   Int32   workLimit    = s->workLimit;
   Bool    firstAttempt = s->firstAttempt;

   /*--
      In the various block-sized structures, live data runs
      from 0 to last+NUM_OVERSHOOT_BYTES inclusive.  First,
      set up the overshoot area for block.
   --*/

   if (s->verbosity >= 4)
      VPrintf0( "        sort initialise ...\n" );

   for (i = 0; i < BZ_NUM_OVERSHOOT_BYTES; i++)
       block[nblock+i] = block[i % nblock];
   for (i = 0; i < nblock+BZ_NUM_OVERSHOOT_BYTES; i++)
       quadrant[i] = 0;


   if (nblock <= 4000) {

      /*--
         Use simpleSort(), since the full sorting mechanism
         has quite a large constant overhead.
      --*/
      if (s->verbosity >= 4) VPrintf0( "        simpleSort ...\n" );
      for (i = 0; i < nblock; i++) zptr[i] = i;
      firstAttempt = False;
      *workDone = workLimit = 0;
      simpleSort ( s, 0, nblock-1, 0 );
      if (s->verbosity >= 4) VPrintf0( "        simpleSort done.\n" );

   } else {

      numQSorted = 0;
      for (i = 0; i <= 255; i++) bigDone[i] = False;

      if (s->verbosity >= 4) VPrintf0( "        bucket sorting ...\n" );

      for (i = 0; i <= 65536; i++) ftab[i] = 0;

      c1 = block[nblock-1];
      for (i = 0; i < nblock; i++) {
         c2 = block[i];
         ftab[(c1 << 8) + c2]++;
         c1 = c2;
      }

      for (i = 1; i <= 65536; i++) ftab[i] += ftab[i-1];

      c1 = block[0];
      for (i = 0; i < nblock-1; i++) {
         c2 = block[i+1];
         j = (c1 << 8) + c2;
         c1 = c2;
         ftab[j]--;
         zptr[ftab[j]] = i;
      }
      j = (block[nblock-1] << 8) + block[0];
      ftab[j]--;
      zptr[ftab[j]] = nblock-1;

      /*--
         Now ftab contains the first loc of every small bucket.
         Calculate the running order, from smallest to largest
         big bucket.
      --*/

      for (i = 0; i <= 255; i++) runningOrder[i] = i;

      {
         Int32 vv;
         Int32 h = 1;
         do h = 3 * h + 1; while (h <= 256);
         do {
            h = h / 3;
            for (i = h; i <= 255; i++) {
               vv = runningOrder[i];
               j = i;
               while ( BIGFREQ(runningOrder[j-h]) > BIGFREQ(vv) ) {
                  runningOrder[j] = runningOrder[j-h];
                  j = j - h;
                  if (j <= (h - 1)) goto zero;
               }
               zero:
               runningOrder[j] = vv;
            }
         } while (h != 1);
      }

      /*--
         The main sorting loop.
      --*/

      for (i = 0; i <= 255; i++) {

         /*--
            Process big buckets, starting with the least full.
            Basically this is a 4-step process in which we call
            qSort3 to sort the small buckets [ss, j], but
            also make a big effort to avoid the calls if we can.
         --*/
         ss = runningOrder[i];

         /*--
            Step 1:
            Complete the big bucket [ss] by quicksorting
            any unsorted small buckets [ss, j], for j != ss.
            Hopefully previous pointer-scanning phases have already
            completed many of the small buckets [ss, j], so
            we don't have to sort them at all.
         --*/
         for (j = 0; j <= 255; j++) {
            if (j != ss) {
               sb = (ss << 8) + j;
               if ( ! (ftab[sb] & SETMASK) ) {
                  Int32 lo = ftab[sb]   & CLEARMASK;
                  Int32 hi = (ftab[sb+1] & CLEARMASK) - 1;
                  if (hi > lo) {
                     if (s->verbosity >= 4)
                        VPrintf4( "        qsort [0x%x, 0x%x]   done %d   this %d\n",
                                  ss, j, numQSorted, hi - lo + 1 );
                     qSort3 ( s, lo, hi, 2 );
                     numQSorted += ( hi - lo + 1 );
                     if (*workDone > workLimit && firstAttempt) return;
                  }
               }
               ftab[sb] |= SETMASK;
            }
         }

         /*--
            Step 2:
            Deal specially with case [ss, ss].  This establishes the
            sorted order for [ss, ss] without any comparisons.
            A clever trick, cryptically described as steps Q6b and Q6c
            in SRC-124 (aka BW94).  This makes it entirely practical to
            not use a preliminary run-length coder, but unfortunately
            we are now stuck with the .bz2 file format.
         --*/
         {
            Int32 put0, get0, put1, get1;
            Int32 sbn = (ss << 8) + ss;
            Int32 lo = ftab[sbn] & CLEARMASK;
            Int32 hi = (ftab[sbn+1] & CLEARMASK) - 1;
            UChar ssc = (UChar)ss;
            put0 = lo;
            get0 = ftab[ss << 8] & CLEARMASK;
            put1 = hi;
            get1 = (ftab[(ss+1) << 8] & CLEARMASK) - 1;
            while (get0 < put0) {
               j = zptr[get0]-1; if (j < 0) j += nblock;
               c1 = block[j];
               if (c1 == ssc) { zptr[put0] = j; put0++; };
               get0++;
            }
            while (get1 > put1) {
               j = zptr[get1]-1; if (j < 0) j += nblock;
               c1 = block[j];
               if (c1 == ssc) { zptr[put1] = j; put1--; };
               get1--;
            }
            ftab[sbn] |= SETMASK;
         }

         /*--
            Step 3:
            The [ss] big bucket is now done.  Record this fact,
            and update the quadrant descriptors.  Remember to
            update quadrants in the overshoot area too, if
            necessary.  The "if (i < 255)" test merely skips
            this updating for the last bucket processed, since
            updating for the last bucket is pointless.

            The quadrant array provides a way to incrementally
            cache sort orderings, as they appear, so as to
            make subsequent comparisons in fullGtU() complete
            faster.  For repetitive blocks this makes a big
            difference (but not big enough to be able to avoid
            randomisation for very repetitive data.)

            The precise meaning is: at all times:

               for 0 <= i < nblock and 0 <= j <= nblock

               if block[i] != block[j],

                  then the relative values of quadrant[i] and
                       quadrant[j] are meaningless.

                  else {
                     if quadrant[i] < quadrant[j]
                        then the string starting at i lexicographically
                        precedes the string starting at j

                     else if quadrant[i] > quadrant[j]
                        then the string starting at j lexicographically
                        precedes the string starting at i

                     else
                        the relative ordering of the strings starting
                        at i and j has not yet been determined.
                  }
         --*/
         bigDone[ss] = True;

         if (i < 255) {
            Int32 bbStart  = ftab[ss << 8] & CLEARMASK;
            Int32 bbSize   = (ftab[(ss+1) << 8] & CLEARMASK) - bbStart;
            Int32 shifts   = 0;

            while ((bbSize >> shifts) > 65534) shifts++;

            for (j = 0; j < bbSize; j++) {
               Int32 a2update     = zptr[bbStart + j];
               UInt16 qVal        = (UInt16)(j >> shifts);
               quadrant[a2update] = qVal;
               if (a2update < BZ_NUM_OVERSHOOT_BYTES)
                  quadrant[a2update + nblock] = qVal;
            }

            AssertH ( ( ((bbSize-1) >> shifts) <= 65535 ), 1002 );
         }

         /*--
            Step 4:
            Now scan this big bucket [ss] so as to synthesise the
            sorted order for small buckets [t, ss] for all t != ss.
            This will avoid doing Real Work in subsequent Step 1's.
         --*/
         for (j = 0; j <= 255; j++)
            copy[j] = ftab[(j << 8) + ss] & CLEARMASK;

         for (j = ftab[ss << 8] & CLEARMASK;
              j < (ftab[(ss+1) << 8] & CLEARMASK);
              j++) {
            k = zptr[j]-1; if (k < 0) k += nblock;
            c1 = block[k];
            if ( ! bigDone[c1] ) {
               zptr[copy[c1]] = k;
               copy[c1] ++;
            }
         }

         for (j = 0; j <= 255; j++) ftab[(j << 8) + ss] |= SETMASK;
      }
      if (s->verbosity >= 4)
         VPrintf3( "        %d pointers, %d sorted, %d scanned\n",
                   nblock, numQSorted, nblock - numQSorted );
   }
}


/*---------------------------------------------*/
static void randomiseBlock ( EState* s )
{
   Int32 i;
   BZ_RAND_INIT_MASK;
   for (i = 0; i < 256; i++) s->inUse[i] = False;

   for (i = 0; i < s->nblock; i++) {
      BZ_RAND_UPD_MASK;
      s->block[i] ^= BZ_RAND_MASK;
      s->inUse[s->block[i]] = True;
   }
}


/*---------------------------------------------*/
void blockSort ( EState* s )
{
   Int32 i;

   s->workLimit       = s->workFactor * (s->nblock - 1);
   s->workDone        = 0;
   s->blockRandomised = False;
   s->firstAttempt    = True;

   sortMain ( s );

   if (s->verbosity >= 3)
      VPrintf3( "      %d work, %d block, ratio %5.2f\n",
                s->workDone, s->nblock-1,
                (float)(s->workDone) / (float)(s->nblock-1) );

   if (s->workDone > s->workLimit && s->firstAttempt) {
      if (s->verbosity >= 2)
         VPrintf0( "    sorting aborted; randomising block\n" );
      randomiseBlock ( s );
      s->workLimit = s->workDone = 0;
      s->blockRandomised = True;
      s->firstAttempt = False;
      sortMain ( s );
      if (s->verbosity >= 3)
         VPrintf3( "      %d work, %d block, ratio %f\n",
                   s->workDone, s->nblock-1,
                   (float)(s->workDone) / (float)(s->nblock-1) );
   }

   s->origPtr = -1;
   for (i = 0; i < s->nblock; i++)
       if (s->zptr[i] == 0)
          { s->origPtr = i; break; };

   AssertH( s->origPtr != -1, 1003 );
}

/*-------------------------------------------------------------*/
/*--- end                                       blocksort.c ---*/
/*-------------------------------------------------------------*/
