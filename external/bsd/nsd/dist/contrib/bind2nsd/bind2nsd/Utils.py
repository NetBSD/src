#!/usr/bin/env python
# Copyright (c) 2007, Secure64 Software Corporation
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
#
#
#	utility functions used throughout the package
#
#

import popen2
import sys

VERBOSE = False

def set_verbosity(flag):
   global VERBOSE
   VERBOSE = flag
   return

def isVerbose():
   global VERBOSE
   return VERBOSE

def report_error(somestuff):
   if len(somestuff) > 0:
      print >> sys.stderr, somestuff
      sys.stderr.flush()
   return

def report_info(somestuff):
   global VERBOSE
   if VERBOSE and len(somestuff) > 0:
      print >> sys.stdout, somestuff
      sys.stdout.flush()
   return

def bail(somestuff):
   report_error(somestuff)
   sys.exit(1)
   return

def run_cmd_capture(cmd):
   (cout, cin, cerr) = popen2.popen3(cmd)

   cin.close()

   output = cout.readlines()
   cout.close()

   errors = cerr.readlines()
   cerr.close()
   
   return (''.join(output), ''.join(errors))

def run_cmd(cmd, desc):
   #-- run a command using our own particular idiom
   global VERBOSE

   result = True		# T => ran without error reports, F otherwise

   if VERBOSE and len(desc) > 0:
      report_info('=> ' + desc)

   (cout, cin, cerr) = popen2.popen3(cmd)

   cin.close()

   output = cout.readlines()
   cout.close()
   if VERBOSE:
      report_info(''.join(output))

   errors = cerr.readlines()
   cerr.close()
   if len(errors) > 0:
      result = False
      report_error(''.join(errors))

   return result

