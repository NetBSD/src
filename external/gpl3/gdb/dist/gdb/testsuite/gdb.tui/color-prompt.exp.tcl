# Copyright 2025 Free Software Foundation, Inc.

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

# Check using a prompt with color in TUI ($tui == 0) or CLI ($tui == 0).

set csi [string cat {\033} "\["]
set rl_prompt_start_ignore {\001}
set rl_prompt_end_ignore {\002}

foreach_with_prefix rl_prompt_start_end_ignore { 0 1 } {
    set color_on [string cat $csi 31m]
    set color_off [string cat $csi 0m]

    if { $rl_prompt_start_end_ignore } {
	set color_on \
	    [string cat \
		 $rl_prompt_start_ignore \
		 $color_on \
		 $rl_prompt_end_ignore]
	set color_off \
	    [string cat \
		 $rl_prompt_start_ignore \
		 $color_off \
		 $rl_prompt_end_ignore]
    }

    # Set prompt with color.
    set prompt "${color_on}(gdb) $color_off"
    Term::command "set prompt $prompt"

    # Check the color.
    set line [Term::get_line_with_attrs $Term::_cur_row]
    gdb_assert { [regexp "^<fg:red>$gdb_prompt <fg:default> *$" $line] } \
	"prompt with color"

    # Type a string.
    set cmd "some long command"
    send_gdb $cmd
    Term::wait_for_line ^[string_to_regexp "(gdb) $cmd"] 23

    # Send ^A, aka C-a, trigger beginning-of-line.
    send_gdb "\001"
    if { $tui || $rl_prompt_start_end_ignore } {
	Term::wait_for_line ^[string_to_regexp "(gdb) $cmd"] 6
    } else {
	# Without the markers, readline may get the cursor position wrong, so
	# match less strict.
	Term::wait_for_line ^[string_to_regexp "(gdb) $cmd"]
    }
    Term::dump_screen

    # Type something else to flush out the effect of the ^A.
    set prefix "A"
    send_gdb $prefix
    if { $tui || $rl_prompt_start_end_ignore } {
	Term::wait_for_line ^[string_to_regexp "(gdb) $prefix$cmd"] 7
    } else {
	# Without the markers, readline may get the cursor position wrong, so
	# match less strict.
	Term::wait_for_line [string_to_regexp "$prefix"]
    }

    # Abort command line editing, and regenerate prompt.
    send_gdb "\003"

    # Reset prompt to default prompt.
    Term::command "set prompt (gdb) "
}
