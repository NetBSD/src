# From lole@epost.de Wed Sep  4 09:54:19 IDT 2002
# Article: 14288 of comp.lang.awk
# Path: iad-read.news.verio.net!dfw-artgen!iad-peer.news.verio.net!news.verio.net!news.maxwell.syr.edu!fu-berlin.de!uni-berlin.de!213.70.124.113!not-for-mail
# From: LorenzAtWork <familie.lenhardt@epost.de>
# Newsgroups: comp.lang.awk
# Subject: bug in gawk 3.1.1?
# Date: Wed, 28 Aug 2002 10:34:50 +0200
# Lines: 45
# Message-ID: <7g1pmukv07c56ep3qav3uebnipdaohqh2l@4ax.com>
# Reply-To: lole@epost.de
# NNTP-Posting-Host: 213.70.124.113
# Mime-Version: 1.0
# Content-Type: text/plain; charset=us-ascii
# Content-Transfer-Encoding: 7bit
# X-Trace: fu-berlin.de 1030523788 53278293 213.70.124.113 (16 [68559])
# X-Newsreader: Forte Agent 1.91/32.564
# Xref: dfw-artgen comp.lang.awk:14288
# 
# hello all,
# 
# I'm using the following script
# 
BEGIN {
	RS="ti1\n(dwv,)?"
	s = 0
	i = 0
}
{
	if ($1 != "")
		s = $1
	print ++i, s
}
# 
# to extract values from a file of the form
# 
# ti1
# dwv,98.22
# ti1
# dwv,103.08
# ti1
# ti1
# dwv,196.25
# ti1
# dwv,210.62
# ti1
# dwv,223.53
# 
# The desired result for this example looks like
# 
# 1 0
# 2 98.22
# 3 103.08
# 4 103.08
# 5 196.25
# 6 210.62
# 7 223.53
# 
# The script work fine the most time, but when run on the attached file
# (sorry for the size, but the error would not appear with less data) I
# get some (three with the attached file) lines that look like
# 
# 1262 dwv,212.97
# 1277 dwv,174.33
# 1279 dwv,151.79
# 
# I can't think of a other reason for this than a bug in gawk!
# 
# I'm running gawk 3.1.1 on winnt 4.0
# 
# best regards
#      Lorenz
# 
# 
