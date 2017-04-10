/*	$NetBSD: libmath.b,v 1.1 2017/04/10 02:28:23 phil Exp $ */

/*
 * Copyright (C) 1991-1994, 1997, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names Philip A. Nelson and Free Software Foundation may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP A. NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP A. NELSON OR THE FREE SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* libmath.b for bc.  */

scale = 20

/* Uses the fact that e^x = (e^(x/2))^2
   When x is small enough, we use the series:
     e^x = 1 + x + x^2/2! + x^3/3! + ...
*/

define e(x) {
  auto  a, b, d, e, f, i, m, n, v, z

  /* a - holds x^y of x^y/y! */
  /* d - holds y! */
  /* e - is the value x^y/y! */
  /* v - is the sum of the e's */
  /* f - number of times x was divided by 2. */
  /* m - is 1 if x was minus. */
  /* i - iteration count. */
  /* n - the scale to compute the sum. */
  /* z - orignal scale. */
  /* b - holds the original ibase. */

  /* Non base 10 ibase? */
  if (ibase != A) {
     b = ibase;
     ibase = A;
     v = e(x);
     ibase = b;
     return (v);
  }

  /* Check the sign of x. */
  if (x<0) {
    m = 1
    x = -x
  } 

  /* Precondition x. */
  z = scale;
  n = 6 + z + .44*x;
  scale = scale(x)+1;
  while (x > 1) {
    f += 1;
    x /= 2;
    scale += 1;
  }

  /* Initialize the variables. */
  scale = n;
  v = 1+x
  a = x
  d = 1

  for (i=2; 1; i++) {
    e = (a *= x) / (d *= i)
    if (e == 0) {
      if (f>0) while (f--)  v = v*v;
      scale = z
      if (m) return (1/v);
      return (v/1);
    }
    v += e
  }
}

/* Natural log. Uses the fact that ln(x^2) = 2*ln(x)
    The series used is:
       ln(x) = 2(a+a^3/3+a^5/5+...) where a=(x-1)/(x+1)
*/

define l(x) {
  auto b, e, f, i, m, n, v, z

  /* Non base 10 ibase? */
  if (ibase != A) {
     b = ibase;
     ibase = A;
     v = l(x);
     ibase = b;
     return (v);
  }

  /* return something for the special case. */
  if (x <= 0) return ((1 - 10^scale)/1)

  /* Precondition x to make .5 < x < 2.0. */
  z = scale;
  scale = 6 + scale;
  f = 2;
  i=0
  while (x >= 2) {  /* for large numbers */
    f *= 2;
    x = sqrt(x);
  }
  while (x <= .5) {  /* for small numbers */
    f *= 2;
    x = sqrt(x);
  }

  /* Set up the loop. */
  v = n = (x-1)/(x+1)
  m = n*n

  /* Sum the series. */
  for (i=3; 1; i+=2) {
    e = (n *= m) / i
    if (e == 0) {
      v = f*v
      scale = z
      return (v/1)
    }
    v += e
  }
}

/* Sin(x)  uses the standard series:
   sin(x) = x - x^3/3! + x^5/5! - x^7/7! ... */

define s(x) {
  auto  b, e, i, m, n, s, v, z

  /* Non base 10 ibase? */
  if (ibase != A) {
     b = ibase;
     ibase = A;
     v = s(x);
     ibase = b;
     return (v);
  }

  /* precondition x. */
  z = scale 
  scale = 1.1*z + 2;
  v = a(1)
  if (x < 0) {
    m = 1;
    x = -x;
  }
  scale = 0
  n = (x / v + 2 )/4
  x = x - 4*n*v
  if (n%2) x = -x

  /* Do the loop. */
  scale = z + 2;
  v = e = x
  s = -x*x
  for (i=3; 1; i+=2) {
    e *= s/(i*(i-1))
    if (e == 0) {
      scale = z
      if (m) return (-v/1);
      return (v/1);
    }
    v += e
  }
}

/* Cosine : cos(x) = sin(x+pi/2) */
define c(x) {
  auto b, v, z;

  /* Non base 10 ibase? */
  if (ibase != A) {
     b = ibase;
     ibase = A;
     v = c(x);
     ibase = b;
     return (v);
  }

  z = scale;
  scale = scale*1.2;
  v = s(x+a(1)*2);
  scale = z;
  return (v/1);
}

/* Arctan: Using the formula:
     atan(x) = atan(c) + atan((x-c)/(1+xc)) for a small c (.2 here)
   For under .2, use the series:
     atan(x) = x - x^3/3 + x^5/5 - x^7/7 + ...   */

define a(x) {
  auto a, b, e, f, i, m, n, s, v, z

  /* a is the value of a(.2) if it is needed. */
  /* f is the value to multiply by a in the return. */
  /* e is the value of the current term in the series. */
  /* v is the accumulated value of the series. */
  /* m is 1 or -1 depending on x (-x -> -1).  results are divided by m. */
  /* i is the denominator value for series element. */
  /* n is the numerator value for the series element. */
  /* s is -x*x. */
  /* z is the saved user's scale. */

  /* Non base 10 ibase? */
  if (ibase != A) {
     b = ibase;
     ibase = A;
     v = a(x);
     ibase = b;
     return (v);
  }

  /* Negative x? */
  m = 1;
  if (x<0) {
    m = -1;
    x = -x;
  }

  /* Special case and for fast answers */
  if (x==1) {
    if (scale <= 25) return (.7853981633974483096156608/m)
    if (scale <= 40) return (.7853981633974483096156608458198757210492/m)
    if (scale <= 60) \
      return (.785398163397448309615660845819875721049292349843776455243736/m)
  }
  if (x==.2) {
    if (scale <= 25) return (.1973955598498807583700497/m)
    if (scale <= 40) return (.1973955598498807583700497651947902934475/m)
    if (scale <= 60) \
      return (.197395559849880758370049765194790293447585103787852101517688/m)
  }


  /* Save the scale. */
  z = scale;

  /* Note: a and f are known to be zero due to being auto vars. */
  /* Calculate atan of a known number. */ 
  if (x > .2)  {
    scale = z+5;
    a = a(.2);
  }
   
  /* Precondition x. */
  scale = z+3;
  while (x > .2) {
    f += 1;
    x = (x-.2) / (1+x*.2);
  }

  /* Initialize the series. */
  v = n = x;
  s = -x*x;

  /* Calculate the series. */
  for (i=3; 1; i+=2) {
    e = (n *= s) / i;
    if (e == 0) {
      scale = z;
      return ((f*a+v)/m);
    }
    v += e
  }
}


/* Bessel function of integer order.  Uses the following:
   j(-n,x) = (-1)^n*j(n,x) 
   j(n,x) = x^n/(2^n*n!) * (1 - x^2/(2^2*1!*(n+1)) + x^4/(2^4*2!*(n+1)*(n+2))
            - x^6/(2^6*3!*(n+1)*(n+2)*(n+3)) .... )
*/
define j(n,x) {
  auto a, b, d, e, f, i, m, s, v, z

  /* Non base 10 ibase? */
  if (ibase != A) {
     b = ibase;
     ibase = A;
     v = j(n,x);
     ibase = b;
     return (v);
  }

  /* Make n an integer and check for negative n. */
  z = scale;
  scale = 0;
  n = n/1;
  if (n<0) {
    n = -n;
    if (n%2 == 1) m = 1;
  }

  /* Compute the factor of x^n/(2^n*n!) */
  f = 1;
  for (i=2; i<=n; i++) f = f*i;
  scale = 1.5*z;
  f = x^n / 2^n / f;

  /* Initialize the loop .*/
  v = e = 1;
  s = -x*x/4
  scale = 1.5*z + length(f) - scale(f);

  /* The Loop.... */
  for (i=1; 1; i++) {
    e =  e * s / i / (n+i);
    if (e == 0) {
       scale = z
       if (m) return (-f*v/1);
       return (f*v/1);
    }
    v += e;
  }
}
