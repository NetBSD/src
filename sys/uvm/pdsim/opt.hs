{-
/*	$NetBSD: opt.hs,v 1.1 2006/09/30 08:50:32 yamt Exp $	*/

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
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
import qualified Data.Set as Set

ff s [] = head $ Set.elems s
ff s (y:ys) =
	if Set.size s == 1 then head $ Set.elems s else
	if Set.member y s then
		ff (Set.delete y s) ys
	else
		ff s ys

do_opt1 npg n q [] = (reverse n, q)
do_opt1 npg n q rs@(r:rs2) =
	if Set.member r q then
		do_opt1 npg n q rs2
	else if Set.size q < npg then
		do_opt1 npg (r:n) (Set.insert r q) rs2
	else
		let
			c = ff q rs2
			nq = Set.delete c q
		in
		do_opt1 npg (r:n) (Set.insert r nq) rs2

do_opt npg rs = fst $ do_opt1 npg [] Set.empty rs
do_opt_dbg npg rs = do_opt1 npg [] Set.empty rs

main = do
	xs <- getContents
	args <- getArgs
	let
		ls = lines xs
		npgs::Int
		npgs = read $ args !! 0
		pgs::[Int]
		pgs = map read ls
	mapM_ print $ do_opt npgs pgs
