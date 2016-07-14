/* $NetBSD: gcc-softfloat.c,v 1.1 2016/07/14 01:59:18 matt Exp $ */
/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * This contain the softfloat primitives used by GCC.
 * It can be used to verify the functions invoked by tGCC to do softfloat.
 * It can also be used to what FP instructions GCC generates to implement the
 * various primitives.  Your arch-dependent code should provide all of these
 * that are easy to implement.  
 */

long long
xfixdfdi(double a)
{
	return (long long)a;
}

int
xfixdfsi(double a)
{
	return (int)a;
}

unsigned long long
xfixunsdfdi(double a)
{
	return (unsigned long long)a;
}

unsigned int
xfixunsdfsi(double a)
{
	return (unsigned int)a;
}

double
xfloatundidf(unsigned long long a)
{
	return (double) a;
}

double
xfloatunsidf(unsigned int a)
{
	return (double) a;
}

double
xfloatdidf(long long a)
{
	return (double) a;
}

double
xfloatsidf(int a)
{
	return (double) a;
}

double
xextendsfdf2(float a)
{
	return (double) a;
}

int
xeqdf2(double a, double b)
{
	return a == b;
}

int
xnedf2(double a, double b)
{
	return a != b;
}

int
xledf2(double a, double b)
{
	return a <= b;
}

int
xgtdf2(double a, double b)
{
	return a > b;
}

int
xltdf2(double a, double b)
{
	return a < b;
}

int
xgedf2(double a, double b)
{
	return a >= b;
}

long long
xfixsfdi(float a)
{
	return (long long)a;
}

int
xfixsfsi(float a)
{
	return (int)a;
}

unsigned long long
xfixunssfdi(float a)
{
	return (unsigned long long)a;
}

unsigned int
xfixunssfsi(float a)
{
	return (unsigned int)a;
}

float
xfloatundisf(unsigned long long a)
{
	return (float) a;
}

float
xfloatunsisf(unsigned int a)
{
	return (float) a;
}

float
xfloatdisf(long long a)
{
	return (float) a;
}

float
xfloatsisf(int a)
{
	return (float) a;
}

float
xtruncdfsf2(double a)
{
	return (float) a;
}

int
xeqsf2(float a, float b)
{
	return a == b;
}

int
xnesf2(float a, float b)
{
	return a != b;
}

int
xlesf2(float a, float b)
{
	return a <= b;
}

int
xgtsf2(float a, float b)
{
	return a > b;
}

int
xltsf2(float a, float b)
{
	return a < b;
}

int
xgesf2(float a, float b)
{
	return a >= b;
}
