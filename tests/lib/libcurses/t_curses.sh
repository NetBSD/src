
h_run()
{
	file="$(atf_get_srcdir)/tests/${1}"

	export COLUMNS=80
	export LINES=24
	$(atf_get_srcdir)/director $2 \
	    -T $(atf_get_srcdir) \
	    -t atf \
	    -I $(atf_get_srcdir)/tests \
	    -C $(atf_get_srcdir)/check_files \
	    -s $(atf_get_srcdir)/slave $file || atf_fail "test ${file} failed"
}

atf_test_case startup
startup_head()
{
	atf_set "descr" "Checks curses initialisation sequence"
}
startup_body()
{
	h_run start
}

atf_test_case addch
addch_head()
{
	atf_set "descr" "Tests adding a chtype to stdscr"
}
addch_body()
{
	h_run addch
}

atf_test_case addchstr
addchstr_head()
{
	atf_set "descr" "Tests adding a chtype string to stdscr"
}
addchstr_body()
{
	h_run addchstr
}

atf_test_case addchnstr
addchnstr_head()
{
	atf_set "descr" "Tests adding bytes from a chtype string to stdscr"
}
addchnstr_body()
{
	h_run addchnstr
}

atf_test_case addstr
addstr_head()
{
	atf_set "descr" "Tests adding bytes from a string to stdscr"
}
addstr_body()
{
	h_run addstr
}

atf_test_case addnstr
addnstr_head()
{
	atf_set "descr" "Tests adding bytes from a string to stdscr"
}
addnstr_body()
{
	h_run addnstr
}

atf_test_case getch
getch_head()
{
	atf_set "descr" "Checks reading a character input"
}
getch_body()
{
	h_run getch
}

atf_test_case timeout
timeout_head()
{
	atf_set "descr" "Checks timeout when reading a character"
}
timeout_body()
{
	h_run timeout
}

atf_test_case window
window_head()
{
	atf_set "descr" "Checks window creation"
}
window_body()
{
	h_run window
}

atf_test_case wborder
wborder_head()
{
	atf_set "descr" "Checks drawing a border around a window"
}
wborder_body()
{
	h_run wborder
}

atf_test_case box
box_head()
{
	atf_set "descr" "Checks drawing a box around a window"
}
box_body()
{
	h_run box
}

atf_test_case wprintw
wprintw_head()
{
	atf_set "descr" "Checks printing to a window"
}
wprintw_body()
{
	h_run wprintw
}

atf_test_case wscrl
wscrl_head()
{
	atf_set "descr" "Check window scrolling"
}
wscrl_body()
{
	h_run wscrl
}

atf_test_case mvwin
mvwin_head()
{
	atf_set "descr" "Check moving a window"
}
mvwin_body()
{
	h_run mvwin
}

atf_test_case getstr
getstr_head()
{
	atf_set "descr" "Check getting a string from input"
}
getstr_body()
{
	h_run getstr
}

atf_test_case termattrs
termattrs_head()
{
	atf_set "descr" "Check the terminal attributes"
}
termattrs_body()
{
	h_run termattrs
}

atf_test_case assume_default_colors
assume_default_colors_head()
{
	atf_set "descr" "Check setting the default color pair"
}
assume_default_colors_body()
{
	h_run assume_default_colors
}

atf_test_case attributes
attributes_head()
{
	atf_set "descr" "Check setting, clearing and getting of attributes"
}
attributes_body()
{
	h_run attributes
}

atf_test_case beep
beep_head()
{
	atf_set "descr" "Check sending a beep"
}
beep_body()
{
	h_run beep
}

atf_test_case background
background_head()
{
	atf_set "descr" "Check setting background character and attributes for both stdscr and a window."
}
background_body()
{
	h_run background
}

atf_test_case can_change_color
can_change_color_head()
{
	atf_set "descr" "Check if the terminal can change colours"
}
can_change_color_body()
{
	h_run can_change_color
}

atf_test_case cbreak
cbreak_head()
{
	atf_set "descr" "Check cbreak mode works"
}
cbreak_body()
{
	h_run cbreak
}

atf_test_case chgat
chgat_head()
{
	atf_set "descr" "Check changing attributes works"
}
chgat_body()
{
	h_run chgat
}

atf_test_case clear
clear_head()
{
	atf_set "descr" "Check clear and erase work"
}
clear_body()
{
	h_run clear
}

atf_test_case copywin
copywin_head()
{
	atf_set "descr" "Check all the modes of copying a window work"
}
copywin_body()
{
	h_run copywin
}

atf_test_case curs_set
curs_set_head()
{
	atf_set "descr" "Check setting the cursor visibility works"
}
curs_set_body()
{
	h_run curs_set
}

atf_test_case define_key
define_key_head()
{
	atf_set "descr" "Check defining a key and removing the definition works"
}
define_key_body()
{
	h_run define_key
}

atf_test_case delay_output
delay_output_head()
{
	atf_set "descr" "Check that padding is inserted when delaying output"
}
delay_output_body()
{
	h_run delay_output -v
}

atf_test_case derwin
derwin_head()
{
	atf_set "descr" "Check derived subwindow creation behaves correctly."
}
derwin_body()
{
	h_run derwin
}

atf_test_case doupdate
doupdate_head()
{
	atf_set "descr" "Check doupdate performs an update"
}
doupdate_body()
{
	h_run doupdate
}

atf_test_case dupwin
dupwin_head()
{
	atf_set "descr" "Check duplicating a window works"
}
dupwin_body()
{
	h_run dupwin
}

atf_test_case erasechar
erasechar_head()
{
	atf_set "descr" "Validate erase char can be retrieved"
}
erasechar_body()
{
	h_run erasechar
}

atf_test_case flash
flash_head()
{
	atf_set "descr" "Validate curses can flash the screen"
}
flash_body()
{
	h_run flash
}

atf_test_case getattrs
getattrs_head()
{
	atf_set "descr" "Validate curses can get and set attributes on a window"
}
getattrs_body()
{
	h_run getattrs
}

atf_test_case bkgdset
bkgdset_head()
{
	atf_set "descr" "Validate curses set the background attributes on stdscr"
}
bkgdset_body()
{
	h_run bkgdset
}

atf_test_case getbkgd
getbkgd_head()
{
	atf_set "descr" "Validate curses getting the background attributes on stdscr"
}
getbkgd_body()
{
	h_run getbkgd
}

atf_test_case getcurx
getcurx_head()
{
	atf_set "descr" "Validate curses getting cursor locations in a window"
}
getcurx_body()
{
	h_run getcurx
}

atf_test_case getmaxx
getmaxx_head()
{
	atf_set "descr" "Validate curses getting the maximum x value of a window"
}
getmaxx_body()
{
	h_run getmaxx
}

atf_test_case getmaxy
getmaxy_head()
{
	atf_set "descr" "Validate curses getting the maximum y value of a window"
}
getmaxy_body()
{
	h_run getmaxy
}

atf_test_case getnstr
getnstr_head()
{
	atf_set "descr" "Check getting a string with a limit"
}
getnstr_body()
{
	h_run getnstr
}

atf_test_case getparx
getparx_head()
{
	atf_set "descr" "Check getting the location of a window relative to its parent"
}
getparx_body()
{
	h_run getparx
}

atf_test_case has_colors
has_colors_head()
{
	atf_set "descr" "Check if the terminal can support colours"
}
has_colors_body()
{
	h_run has_colors
}

atf_test_case has_ic
has_ic_head()
{
	atf_set "descr" "Check if the terminal can insert characters and lines"
}
has_ic_body()
{
	h_run has_ic
}

atf_test_case hline
hline_head()
{
	atf_set "descr" "Draw a horizontal line"
}
hline_body()
{
	h_run hline
}

atf_test_case inch
inch_head()
{
	atf_set "descr" "Get the character under the cursor on stdscr"
}
inch_body()
{
	h_run inch
}

atf_test_case inchnstr
inchnstr_head()
{
	atf_set "descr" "Get a limited chtype string from the screen"
}
inchnstr_body()
{
	h_run inchnstr
}

atf_test_case init_color
init_color_head()
{
	atf_set "descr" "Set a custom color entry"
}
init_color_body()
{
	h_run init_color
}

atf_test_case innstr
innstr_head()
{
	atf_set "descr" "Get a limited string starting at the cursor"
}
innstr_body()
{
	h_run innstr
}

atf_test_case is_linetouched
is_linetouched_head()
{
	atf_set "descr" "Check if a line has been modified in a window"
}
is_linetouched_body()
{
	h_run is_linetouched
}

atf_test_case is_wintouched
is_wintouched_head()
{
	atf_set "descr" "Check if a window has been modified"
}
is_wintouched_body()
{
	h_run is_wintouched
}

atf_test_case keyname
keyname_head()
{
	atf_set "descr" "Convert integers into printable key names"
}
keyname_body()
{
	h_run keyname
}

atf_test_case keyok
keyok_head()
{
	atf_set "descr" "Check the ability to disable interpretation of a multichar key sequence"
}
keyok_body()
{
	h_run keyok
}

atf_test_case killchar
killchar_head()
{
	atf_set "descr" "Get the value of the terminals kill character"
}
killchar_body()
{
	h_run killchar
}

atf_test_case meta
meta_head()
{
	atf_set "descr" "Check setting and clearing the meta flag on a window"
}
meta_body()
{
	h_run meta
}

atf_test_case mvaddch
mvaddch_head()
{
	atf_set "descr" "Move the cursor and add a character to stdscr"
}
mvaddch_body()
{
	h_run mvaddch
}

atf_test_case mvaddchnstr
mvaddchnstr_head()
{
	atf_set "descr" "Move the cursor and add a character string to stdscr"
}
mvaddchnstr_body()
{
	h_run mvaddchnstr
}

atf_test_case mvaddchstr
mvaddchstr_head()
{
	atf_set "descr" "Move the cursor and add a ch string to stdscr"
}
mvaddchstr_body()
{
	h_run mvaddchstr
}

atf_test_case mvaddnstr
mvaddnstr_head()
{
	atf_set "descr" "Move the cursor and add a limited string to stdscr"
}
mvaddnstr_body()
{
	h_run mvaddnstr
}

atf_test_case mvaddstr
mvaddstr_head()
{
	atf_set "descr" "Move the cursor and add a string to stdscr"
}
mvaddstr_body()
{
	h_run mvaddstr
}

atf_test_case mvchgat
mvchgat_head()
{
	atf_set "descr" "Change the attributes on the screen"
}
mvchgat_body()
{
	h_run mvchgat
}

atf_test_case mvcur
mvcur_head()
{
	atf_set "descr" "Move the cursor on the screen"
}
mvcur_body()
{
	h_run mvcur
}

atf_test_case mvderwin
mvderwin_head()
{
	atf_set "descr" "Move the mapping of a region relative to the parent"
}
mvderwin_body()
{
	h_run mvderwin
}

atf_test_case mvgetnstr
mvgetnstr_head()
{
	atf_set "descr" "Move the cursor and get a limited number of characters"
}
mvgetnstr_body()
{
	h_run mvgetnstr
}

atf_test_case mvgetstr
mvgetstr_head()
{
	atf_set "descr" "Move the cursor and get characters"
}
mvgetstr_body()
{
	h_run mvgetstr
}

atf_test_case mvhline
mvhline_head()
{
	atf_set "descr" "Move the cursor and draw a horizontal line"
}
mvhline_body()
{
	h_run mvhline
}

atf_test_case mvinchnstr
mvinchnstr_head()
{
	atf_set "descr" "Move the cursor read characters - tests both mvinchstr and mvinchnstr"
}
mvinchnstr_body()
{
	h_run mvinchnstr
}

atf_test_case mvprintw
mvprintw_head()
{
	atf_set "descr" "Move the cursor and print a string"
}
mvprintw_body()
{
	h_run mvprintw
}

atf_test_case mvscanw
mvscanw_head()
{
	atf_set "descr" "Move the cursor and scan for input patterns"
}
mvscanw_body()
{
	h_run mvscanw
}

atf_test_case mvvline
mvvline_head()
{
	atf_set "descr" "Move the cursor and draw a vertical line"
}
mvvline_body()
{
	h_run mvvline
}

atf_test_case pad
pad_head()
{
	atf_set "descr" "Test the newpad, subpad, pnoutrefresh and prefresh functions"
}
pad_body()
{
	h_run pad
}

atf_test_case nocbreak
nocbreak_head()
{
	atf_set "descr" "Test that the nocbreak call returns the terminal to canonical character processing"
}
nocbreak_body()
{
	h_run nocbreak
}

atf_test_case nodelay
nodelay_head()
{
	atf_set "descr" "Test that the nodelay call causes wget to not block"
}
nodelay_body()
{
	h_run nodelay
}

atf_init_test_cases()
{
	atf_add_test_case startup
	atf_add_test_case addch
	atf_add_test_case addchstr
	atf_add_test_case addchnstr
	atf_add_test_case addstr
	atf_add_test_case addnstr
	atf_add_test_case getch
	atf_add_test_case timeout
	atf_add_test_case window
	atf_add_test_case wborder
	atf_add_test_case box
	atf_add_test_case wprintw
	atf_add_test_case wscrl
	atf_add_test_case mvwin
	atf_add_test_case getstr
	atf_add_test_case termattrs
	atf_add_test_case can_change_color
	atf_add_test_case assume_default_colors
	atf_add_test_case attributes
	atf_add_test_case beep
	atf_add_test_case background
	atf_add_test_case cbreak
	atf_add_test_case chgat
	atf_add_test_case clear
	atf_add_test_case copywin
	atf_add_test_case curs_set
	atf_add_test_case define_key
#	atf_add_test_case delay_output # not working
	atf_add_test_case derwin
	atf_add_test_case doupdate
	atf_add_test_case dupwin
	atf_add_test_case erasechar
	atf_add_test_case flash
	atf_add_test_case getattrs
	atf_add_test_case bkgdset
	atf_add_test_case getbkgd
	atf_add_test_case getcurx
	atf_add_test_case getmaxx
	atf_add_test_case getmaxy
	atf_add_test_case getnstr
	atf_add_test_case getparx
	atf_add_test_case has_colors
	atf_add_test_case has_ic
	atf_add_test_case hline
	atf_add_test_case inch
	atf_add_test_case inchnstr
	atf_add_test_case init_color
	atf_add_test_case innstr
	atf_add_test_case is_linetouched
	atf_add_test_case is_wintouched
	atf_add_test_case keyname
	atf_add_test_case keyok
	atf_add_test_case killchar
	atf_add_test_case meta
	atf_add_test_case mvaddch
	atf_add_test_case mvaddchnstr
	atf_add_test_case mvaddchstr
	atf_add_test_case mvaddnstr
	atf_add_test_case mvaddstr
	atf_add_test_case mvchgat
	atf_add_test_case mvcur
	atf_add_test_case mvderwin
	atf_add_test_case mvgetnstr
	atf_add_test_case mvgetstr
	atf_add_test_case mvhline
	atf_add_test_case mvinchnstr
	atf_add_test_case mvprintw
	atf_add_test_case mvscanw
	atf_add_test_case mvvline
	atf_add_test_case pad
	atf_add_test_case nocbreak
	atf_add_test_case nodelay
}

