# Atto Emacs Change Log

## Atto v1.22 06 Dec 2020
* fixed some color issues when running on Arch, needed to set color in modeline and msg line

## Atto v1.21 27 Aug 2020
* small fix, reframe on insert if point is sitting on EOF and have scrolled off end of window

## Atto v1.20 30 Mar 2018
* very small enhancement to previous fix.  When we go to the end of the file we do not reframe if the end of file is already displayed in the window.

## Atto v1.19 9 Mar 2018
* fixed issue with display reframe when last line is displayed and text has been cut above it. On scrolling to end of buffer there was a nasty jump to frame the screen with the last line anchored to the end of the screen. This issue was present in the original Anthonys Editor.

## Atto v1.18 1 Feb 2018
* fixed bug with opening file ~/.xxx where filename completion has not been used. If ~ is detected we force a call as if the user had selected filename completion.

## Atto v1.17 20 Jan 2018
* fixed bug with lnend() when at end of buffer, fixed killtoeol() behaviour

## Atto V1.16 25 Nov 2017
* merged in file descriptor leak from and code reductions in complete.c from "The Infinnovation team"
* small changes in complete.c to reduce line count

## Atto v1.15.1 25 Nov 2017
* small code optimisation to reduce line count

## Atto v1.15 29 July 2017
* Merged in fixes for multi-byte/wide chars, backspace and cursor position provided by Matt Fielding (Magnetic Realms)
* Added simplified chinese test file to docs

## Atto v1.14 14 April 2017
* added alternative home and end key bindings (some systems differ in setup)
* removed some non emacs bindings 
* re-order definition of keys

## Atto v1.13 13 April 2017
* fixed problem with set_parse_state.  To be sure of setting hilite state we parse the text up to point at the top of the screen.
* fixed problem with complete.c not initialising response buffer
* added resize-terminal

## Atto v1.12 12 April 2017
* added UTF8 support

## Atto v1.11 11 April 2017
* reduced some functions to 1 liners in command.c to make space for future UTF8 fixes

## Atto v1.10 9 March 2017
* discard unbound keystrokes, compacted 1 line functions in command.c

## Atto v1.9 9 March 2017
* Fixed cut, copy code to avoid corruption as pointer p should be set after call to movegap()

## Atto v1.8 22 December 2016
* Added generic syntax highlighting, removed the basic undo command to keep code count under 2000 lines.
* made searchtext start from empty string (as per GNU emacs)
* modified search_forward() and search_backwards() so that they can be called on any buffer (not just current buf)
* fixed failed search when first char in buffer should match.
* some comments reduced to 1 line to save precious line count
* added flag to getinput() to clear the response if required

## Atto v1.7,  12 October 2016
* Added filename completion
NOTE: If this creates a problem on windows the change can be backed out by uncommenting the line in command.c readfile() and using the getinput() instead of getfilename() functions.

## Atto v1.6,  29 December 2015
* Fixed display problem when editing same buffer in multiple windows.
* Fixed overflow of filename variable when a large filename is supplied at the command line
* Reformatted comments at top of source files to claw back 30 more lines for defect fixes

## Atto v1.5,  20 December 2015
* Added INS = toggle-overwrite-mode
* flushed keyboard on detection of Esc in search and replace, which means if you touch the arrow keys you exit cleanly without writing the rest of an escape sequence into the buffer.
* Added entries for escape key binding (eg esc-v and esc-V) to handle when CAPSLOCK is active.
* Handled CAPSLOCK when prompted y/n to exit.
* Line count is 1987.

## Atto v1.4.3, 15 December 2015
* fixed bug with display of last line

## Atto v1.4.2, 13 December 2015
* Fixed crash bug with free_windows on MS windows, thanks to Ed Davies for reporting
* went through all files and ensure correct indentation and tabstops, ensured if (foo) { opening brace style
* line count is 1955

## Atto v1.4.1, 12 December 2015
* Added esc-@ as alternative to C-space for set-mark

## Atto v1.4 8 December 2015
* Working Atto that supports multiple windows. It all fits in 1969 lines of C !
* Fixed bug that meant that scrolling to end of file in other than the first window jumped off the screen.
* Removed previous-buffer which was not going to get used.
* Removed redundant definitions of FIRST_LINE
* Corrected PgDn and PgUp to work with variable size windows.

## Atto V1.3.1 5 December 2015
* Fixed bug with count_buffers causing kill-buffer to core dump

## Atto V1.3 1 December 2015
* Added M-r search and replace
* Added M-g goto-line
* Updated show-pos with line/total_lines count
* fixed bug introduced in 1.2 where last modified buffer would not trigger the prompt to save.
* code footprint is 1775 lines !

## Atto v1.2 29 November 2015
* Implemented multibuffer support, added approx 180 lines of code
* First buffer created is called *scratch*
* Creates *scratch* buffer if you want to delete last buffer
* Added C-x C-n next-buffer
* Added C-x C-p prev-buffer
* Added C-x k   delete-buffer
* Consumed key.h into header.h
* code footprint is 1586 lines !

## Atto v1.1 26 November 2015
* Reduced code footprint by simplification of key code and the definition of keymap. This means I dont have to edit more than one map to add a function.
* Added forward and reverse text search.
* Code footprint is less than 1400 lines !

## Atto v1.0 2015

