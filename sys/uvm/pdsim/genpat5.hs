{-
/*	$NetBSD: genpat5.hs,v 1.2 2006/09/30 15:52:39 yamt Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
-}

import System.Environment
import List
import Numeric
import Random

tagai (x:xs) (y:ys) = x:y:tagai xs ys

acc0 _ r1 = cycle [start..(start+n-1)] where
	(start, n) = r1

acc1 seed r1 = map f rands where
	(start, n) = r1
	f x = start + x `mod` n
	rands = unfoldr (Just . next) $ mkStdGen seed

acc2 seed r1 = map f rands where
	(start, n) = r1
	f x = start + intsqrt (x `mod` n ^ 2)
	rands = unfoldr (Just . next) $ mkStdGen seed where
	intsqrt = truncate . (sqrt . ((flip encodeFloat) 0)) . toInteger

acc3 seed r1 = map f rands where
	(start, n) = r1
	f x = start + g (mod x 1024) n
	g x n = x * x * n `div` (1024 * 1024)
	rands = unfoldr (Just . next) $ mkStdGen seed where

acc4 seed r1 = map g $ zip [1..] $ map f rands where
	f x = apply r1 x where d = x `mod` 2
	g (x,y) = x+y
	apply r x = fst r + intsqrt (x `mod` (snd r) ^ 2)
	intsqrt = truncate . (sqrt . ((flip encodeFloat) 0)) . toInteger
	rands = unfoldr (Just . next) $ mkStdGen seed

patterns = [acc0, acc1, acc2, acc3, acc4]

mklist t a b = take t $ tagai a b

main = getArgs >>= \ args ->
	let
		pat = read (args !! 0)
		t = read (args !! 1)
		start = read (args !! 2)
		n = read (args !! 3)
		seed = 0
	in
	mapM_ print $ take t $ (patterns !! pat) seed (start, n)
