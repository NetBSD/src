/*	Id: sinh.c,v 1.2 2008/02/26 19:54:41 ragge Exp 	*/	
/*	$NetBSD: sinh.c,v 1.1.1.2 2010/06/03 18:58:10 plunky Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
	sinh(arg) returns the hyperbolic sign of its floating-
	point argument.

	The exponential function is called for arguments
	greater in magnitude than 0.5.
	The result overflows and 'huge' is returned for
	arguments greater than somewhat.

	A series is used for arguments smaller in magnitude than 0.5.
	The coeffieients are #2029 from Hart & Cheney. (20.36D)

	cosh(arg) is computed from the exponential function for
	all arguments.
*/

double exp();

static double p0 -0.6307673640497716991184787251e+6;
static double p1 -0.8991272022039509355398013511e+5;
static double p2 -0.2894211355989563807284660366e+4;
static double p3 -0.2630563213397497062819489e+2;
static double q0 -0.6307673640497716991212077277e+6;
static double q1  0.1521517378790019070696485176e+5;
static double q2 -0.173678953558233699533450911e+3;
static double q3  1.0;

double
sinh(arg) double arg; {

	double sign, temp, argsq;

	sign = 1;
	if(arg < 0){
		arg = - arg;
		sign = -1;
	}

	if(arg > 21.){
		temp = exp(arg)/2;
		return(sign*temp);
	}

	if(arg > 0.5) {
		temp = (exp(arg) - exp(-arg))/2;
		return(sign*temp);
	}

	argsq = arg*arg;
	temp = (((p3*argsq+p2)*argsq+p1)*argsq+p0)*arg;
	temp = temp/(((q3*argsq+q2)*argsq+q1)*argsq+q0);
	return(sign*temp);

}

double
cosh(arg) double arg; {

	double temp;

	if(arg < 0)
		arg = - arg;

	if(arg > 21.){
		temp = exp(arg)/2;
		return(temp);
	}

	temp = (exp(arg) + exp(-arg))/2;
	return(temp);
}
