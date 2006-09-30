{-
/*	$NetBSD: nbsd.hs,v 1.1 2006/09/30 08:50:32 yamt Exp $	*/

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
import qualified Data.Map as Map
import Control.Exception
import Data.Queue

type PageId = Int
data Page = Pg { pgid :: PageId, referenced :: Bool }
data Pageq = Pgq { active, inactive :: PageList }

{-
data PageList = Pgl Int [Int]
pglenqueue x (Pgl n xs) = Pgl (n+1) (xs++[x])
pgldequeue (Pgl n (x:xs)) = (x, Pgl (n-1) xs)
pglsize (Pgl n _) = n
pglempty = Pgl 0 []
-}
data PageList = Pgl Int (Queue Int)
pglenqueue x (Pgl n q) = Pgl (n+1) (addToQueue q x)
pgldequeue (Pgl n q) = (x, Pgl (n-1) nq) where
	Just (x,nq) = deQueue q
pglsize (Pgl n _) = n
pglempty = Pgl 0 emptyQueue

{-
instance Show Page where
	show pg = "(" ++ (show $ pgid pg) ++ "," ++ (show $ referenced pg) ++ ")"
instance Show Pageq where
	show q = "(act=" ++ (show $ active q) ++ ",inact=" ++ (show $ inactive q) ++ ")"
-}

pglookup idx m = Map.lookup idx m

emptyq = Pgq { active = pglempty, inactive = pglempty }

clrref pg = pg { referenced = False }
markref pg = pg { referenced = True }

clrrefm x m = Map.update (Just . clrref) x m

reactivate :: (Pageq,Map.Map Int Page) -> (Pageq,Map.Map Int Page)
reactivate (q,m) = (nq,nm) where
	nq = q { active = pglenqueue x $ active q, inactive = niaq }
	nm = clrrefm x m
	(x,niaq) = pgldequeue $ inactive q
reactivate_act (q,m) = (nq,nm) where
	nq = q { active = pglenqueue x $ naq }
	nm = clrrefm x m
	(x,naq) = pgldequeue $ active q
deactivate_act (q,m) = (nq,nm) where
	nq = q { active = naq, inactive = pglenqueue x $ inactive q }
	nm = clrrefm x m
	(x,naq) = pgldequeue $ active q

reclaim :: Int -> (Pageq,Map.Map Int Page)->(Pageq,Map.Map Int Page)
reclaim pct (q0,m0) =
	if referenced p then
		reclaim pct $ reactivate (q,m)
	else
		(q { inactive = npgl },Map.delete x m)
	where
		(q,m) = fillinact pct (q0,m0)
		(x,npgl) = pgldequeue $ inactive q
		Just p = Map.lookup x m0

fillinact inactpct (q,m) =
	if inactlen >= inacttarg then (q,m) else
#if defined(LINUX)
	if referenced p then
	fillinact inactpct $ reactivate_act (q,m) else
#endif
	fillinact inactpct $ deactivate_act (q,m)
	where
		Just p = Map.lookup x m
		(x,_) = pgldequeue $ active q
		inactlen = pglsize $ inactive q
		inacttarg = div (Map.size m * inactpct) 100

pgref :: Int->Map.Map Int Page -> Map.Map Int Page
pgref idx m = Map.update f idx m where
	f = Just . markref

do_nbsd1 npg pct n q m [] = (reverse n, q)
do_nbsd1 npg pct n q m rs@(r:rs2) =
	let
		p = pglookup r m
	in
	if isJust p then
		do_nbsd1 npg pct n q (pgref r m) rs2
	else if Map.size m < npg then
		do_nbsd1 npg pct (r:n) (enqueue r q) (pgenqueue r m) rs2
	else
		let
			(nq, nm) = reclaim pct (q,m)
		in
		do_nbsd1 npg pct (r:n) (enqueue r nq) (pgenqueue r nm) rs2
	where
		newpg i = Pg {pgid = i, referenced = True}
		pgenqueue i m = Map.insert i (newpg i) m
#if defined(LINUX)
		enqueue i q = q { inactive = pglenqueue i $ inactive q }
#else
		enqueue i q = q { active = pglenqueue i $ active q }
#endif

do_nbsd npg pct rs = fst $ do_nbsd1 npg pct [] emptyq Map.empty rs
do_nbsd_dbg npg pct rs = do_nbsd1 npg pct [] emptyq Map.empty rs

main = do
	xs <- getContents
	args <- getArgs
	let
		ls = lines xs
		npgs::Int
		npgs = read $ args !! 0
		pct = read $ args !! 1
		pgs::[Int]
		pgs = map read ls
	mapM_ print $ do_nbsd npgs pct pgs
