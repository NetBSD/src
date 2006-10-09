{-
/*	$NetBSD: lfu.hs,v 1.1 2006/10/09 12:32:46 yamt Exp $	*/

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
import Monad

pgmatch idx pg = idx == fst pg

pglookup :: Eq a => a -> [(a,b)] -> Maybe (a,b)
pglookup x = find (pgmatch x)

pgdequeue :: Eq a => a -> [a] -> [a]
pgdequeue = delete

victim :: Ord b => [(a,b)] -> (a,b)
victim = minimumBy p where
	p x y = compare (snd x) (snd y)

do_lfu1 npg n q qlen [] = (reverse n, q)
do_lfu1 npg n q qlen rs@(r:rs2) =
	let
		p = pglookup r q
	in
	if isJust p then
		let
			opg = fromJust p
			nq = pgdequeue opg q
			pg = (fst opg, snd opg + 1)
		in
		do_lfu1 npg n (pg:nq) qlen rs2
	else if qlen < npg then
		do_lfu1 npg (r:n) ((r,1):q) (qlen+1) rs2
	else
		let
			opg = victim q
			nq = pgdequeue opg q
		in
		do_lfu1 npg (r:n) ((r,1):nq) qlen rs2

do_lfu npg rs = fst $ do_lfu1 npg [] [] 0 rs

main = do
	xs <- getContents
	args <- getArgs
	let
		ls = lines xs
		npgs::Int
		npgs = read $ args !! 0
		pgs::[Int]
		pgs = map read ls
	mapM_ print $ do_lfu npgs pgs
