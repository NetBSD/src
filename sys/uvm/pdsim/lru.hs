{-
/*	$NetBSD: lru.hs,v 1.1 2006/09/30 09:00:00 yamt Exp $	*/

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
import System.IO
import List
import Maybe

contain x xs = isJust $ find (== x) xs

do_lru1 npg n q qlen [] = (reverse n, q)
do_lru1 npg n q qlen rs@(r:rs2) =
	if contain r q then
		let
			nq = delete r q
		in
		do_lru1 npg n (r:nq) qlen rs2
	else if qlen < npg then
		do_lru1 npg (r:n) (r:q) (qlen+1) rs2
	else
		let
			nq = take (qlen-1) q
		in
		do_lru1 npg (r:n) (r:nq) qlen rs2

do_lru npg rs = fst $ do_lru1 npg [] [] 0 rs

main = do
	xs <- getContents
	args <- getArgs
	let
		ls = lines xs
		npgs::Int
		npgs = read $ args !! 0
		pgs::[Int]
		pgs = map read ls
	mapM_ print $ do_lru npgs pgs
