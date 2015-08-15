# Copyright (C) 2014-2015 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This testcase tests PR python/16699

import gdb

class CompleteFileInit(gdb.Command):
	def __init__(self):
		gdb.Command.__init__(self,'completefileinit',gdb.COMMAND_USER,gdb.COMPLETE_FILENAME)

	def invoke(self,argument,from_tty):
		raise gdb.GdbError('not implemented')

class CompleteFileMethod(gdb.Command):
	def __init__(self):
		gdb.Command.__init__(self,'completefilemethod',gdb.COMMAND_USER)

	def invoke(self,argument,from_tty):
		raise gdb.GdbError('not implemented')

	def complete(self,text,word):
		return gdb.COMPLETE_FILENAME

class CompleteFileCommandCond(gdb.Command):
	def __init__(self):
		gdb.Command.__init__(self,'completefilecommandcond',gdb.COMMAND_USER)

	def invoke(self,argument,from_tty):
		raise gdb.GdbError('not implemented')

	def complete(self,text,word):
		# This is a test made to know if the command
		# completion still works fine.  When the user asks to
		# complete something like "completefilecommandcond
		# /path/to/py-completion-t", it should not complete to
		# "/path/to/py-completion-test/", but instead just
		# wait for input.
		if "py-completion-t" in text:
			return gdb.COMPLETE_COMMAND
		else:
			return gdb.COMPLETE_FILENAME

CompleteFileInit()
CompleteFileMethod()
CompleteFileCommandCond()
