--	$NetBSD: happy.lua,v 1.1.16.2 2017/12/03 11:38:53 jdolecek Exp $
--
-- Copyright (c) 2015 The NetBSD Foundation, Inc.
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
--
-- THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
-- ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
-- TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
-- PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
-- BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
-- POSSIBILITY OF SUCH DAMAGE.
--
--
-- Commentary:
-- A happy number is a number defined by the following process: Starting with
-- any positive integer, replace the number by the sum of the squares of its
-- digits, and repeat the process until the number equals 1 (where it will
-- stay), or it loops endlessly in a cycle which does not include 1. Those
-- numbers for which this process ends in 1 are happy numbers, while those that
-- do not end in 1 are unhappy numbers (or sad numbers).
--
-- For more information on happy numbers, and the algorithms, see
--      http://en.wikipedia.org/wiki/Happy_number
--
-- The happy number generator is here only to have something that the user
-- can read from our device.  Any other arbitrary data generator could
-- have been used.  The algorithm is not critical to the implementation
-- of the module.

local HAPPY_NUMBER = 1

-- If n is not happy then its sequence ends in the cycle:
-- 4, 16, 37, 58, 89, 145, 42, 20, 4, ...
local SAD_NUMBER = 4

-- This following algorithm is designed for numbers of the integer type.
-- Integer numbers are used by default in the NetBSD kernel, as there would be
-- need for additional overhead in context-switch with support for floats.

function dsum(n)
	local sum = 0
	while n > 0 do
		local x = n % 10
		sum = sum + (x * x)
		n = n / 10
	end
	return sum
end

function is_happy(n)
	while true do
		local total = dsum(n)

		if total == HAPPY_NUMBER then
			return 1
		end
		if total == SAD_NUMBER then
			return 0
		end

		n = total
	end
end
