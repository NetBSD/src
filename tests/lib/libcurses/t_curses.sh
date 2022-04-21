h_run()
{
	TEST_LOCALE=en_US.UTF-8

	file=$1
	locale=`locale -a | grep -i ${TEST_LOCALE}`
	if [ -z "${locale}" ]; then
		atf_fail "test ${file} failed because locale ${locale} not available"
	else
		# export the locale and shift the parameters by two and pass the rest
		export LC_ALL=$locale
		shift
		r_run $file $@
	fi
}

r_run()
{
	file="$(atf_get_srcdir)/tests/${1}"
	export COLUMNS=80
	export LINES=24
	$(atf_get_srcdir)/director $2 \
		-T $(atf_get_srcdir) \
		-t atf \
		-C $(atf_get_srcdir)/check_files \
		-s $(atf_get_srcdir)/slave $file || atf_fail "test ${file} failed"
}

##########################################
# testframe utility functions
##########################################

atf_test_case startup
startup_head()
{
	atf_set "descr" "Checks curses initialisation sequence"
}
startup_body()
{
	h_run start
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

atf_test_case start_slk
start_slk_head()
{
	atf_set "descr" "Checks curses initialisation sequence with soft key labels"
}
start_slk_body()
{
	h_run start_slk
}

atf_test_case window_hierarchy
window_hierarchy_head()
{
	atf_set "descr" "Checks creating a hierarchy of windows"
}
window_hierarchy_body()
{
	h_run window_hierarchy
}

atf_test_case two_window
two_window_head()
{
	atf_set "descr" "Checks creating 2 windows"
}
two_window_body()
{
	h_run two_window
}

atf_test_case varcheck
varcheck_head()
{
	atf_set "descr" "Checks if the testframe CHECK command works"
}
varcheck_body()
{
	h_run varcheck
}

##########################################
# curses add characters to window routines
##########################################

atf_test_case addbytes
addbytes_head()
{
	atf_set "descr" "Tests adding bytes to stdscr"
}
addbytes_body()
{
	h_run addbytes
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

atf_test_case waddch
waddch_head()
{
    atf_set "descr" "Tests adding a chtype to window - tests mvwaddch too"
}
waddch_body()
{
    h_run waddch
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

atf_test_case addchstr
addchstr_head()
{
	atf_set "descr" "Tests adding a chtype string to stdscr"
}
addchstr_body()
{
	h_run addchstr
}

atf_test_case waddchstr
waddchstr_head()
{
    atf_set "descr" "Tests adding a chtype string to window"
}
waddchstr_body()
{
    h_run waddchstr
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

atf_test_case waddchnstr
waddchnstr_head()
{
    atf_set "descr" "Tests adding bytes from a chtype string to window"
}
waddchnstr_body()
{
    h_run waddchnstr
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

atf_test_case mvwaddchstr
mvwaddchstr_head()
{
    atf_set "descr" "Move the cursor and add a ch string to window"
}
mvwaddchstr_body()
{
    h_run mvwaddchstr
}

atf_test_case mvaddchnstr
mvaddchnstr_head()
{
	atf_set "descr" "Move the cursor and add a limited ch string to stdscr"
}
mvaddchnstr_body()
{
	h_run mvaddchnstr
}

atf_test_case mvwaddchnstr
mvwaddchnstr_head()
{
    atf_set "descr" "Move the cursor and add a limited ch string to window"
}
mvwaddchnstr_body()
{
    h_run mvwaddchnstr
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

atf_test_case addwstr
addwstr_head()
{
    atf_set "descr" "Tests adding wide character string to stdscr"
}
addwstr_body()
{
    h_run addwstr
}

atf_test_case waddstr
waddstr_head()
{
    atf_set "descr" "Tests adding bytes from a string to window"
}
waddstr_body()
{
    h_run waddstr
}

atf_test_case waddwstr
waddwstr_head()
{
    atf_set "descr" "Tests adding wide character string to window"
}
waddwstr_body()
{
    h_run waddwstr
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

atf_test_case addnwstr
addnwstr_head()
{
    atf_set "descr" "Tests adding wide characters from string to stdscr"
}
addnwstr_body()
{
    h_run addnwstr
}

atf_test_case waddnstr
waddnstr_head()
{
    atf_set "descr" "Tests adding wide characters from string to window"
}
waddnstr_body()
{
    h_run waddnstr
}

atf_test_case waddnwstr
waddnwstr_head()
{
    atf_set "descr" "Move the cursor and add wide characters from string to stdscr"
}
waddnwstr_body()
{
    h_run waddnwstr
}

atf_test_case mvwaddnwstr
mvwaddnwstr_head()
{
    atf_set "descr" "Move the cursor and add wide characters from string to stdscr"
}
mvwaddnwstr_body()
{
    h_run mvwaddnwstr
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

atf_test_case mvaddwstr
mvaddwstr_head()
{
    atf_set "descr" "Move the cursor and add wide character string to stdscr"
}
mvaddwstr_body()
{
    h_run mvaddwstr
}

atf_test_case mvwaddwstr
mvwaddwstr_head()
{
    atf_set "descr" "Move the cursor and add wide character string to window"
}
mvwaddwstr_body()
{
    h_run mvwaddwstr
}

atf_test_case mvwaddstr
mvwaddstr_head()
{
    atf_set "descr" "Move the cursor and add a string to window"
}
mvwaddstr_body()
{
    h_run mvwaddstr
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

atf_test_case mvaddnwstr
mvaddnwstr_head()
{
    atf_set "descr" "Move the cursor and add wide characters from string to stdscr"
}
mvaddnwstr_body()
{
    h_run mvaddnwstr
}

atf_test_case mvwaddnstr
mvwaddnstr_head()
{
    atf_set "descr" "Move the cursor and add wide characters from string to window"
}
mvwaddnstr_body()
{
    h_run mvwaddnstr
}

atf_test_case add_wch
add_wch_head()
{
	atf_set "descr" "Test adding complex character to stdscr"
}
add_wch_body()
{
	h_run add_wch
}

atf_test_case wadd_wch
wadd_wch_head()
{
    atf_set "descr" "Test adding complex character to window"
}
wadd_wch_body()
{
    h_run wadd_wch
}

##########################################
# curses input stream routines
##########################################

atf_test_case getch
getch_head()
{
	atf_set "descr" "Checks reading a character input - tests mvgetch also"
}
getch_body()
{
	h_run getch
}

atf_test_case wgetch
wgetch_head()
{
	atf_set "descr" "Checks reading a character input from window - tests mvwgetch also"
}
wgetch_body()
{
	h_run wgetch
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

atf_test_case keyok
keyok_head()
{
	atf_set "descr" "Check the ability to disable interpretation of a multichar key sequence"
}
keyok_body()
{
	h_run keyok
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

atf_test_case wgetnstr
wgetnstr_head()
{
    atf_set "descr" "Check getting a string on window input with a limit"
}
wgetnstr_body()
{
    h_run wgetnstr
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

atf_test_case mvwgetnstr
mvwgetnstr_head()
{
    atf_set "descr" "Move the cursor and get a limited number of characters on window input"
}
mvwgetnstr_body()
{
    h_run mvwgetnstr
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

atf_test_case wgetstr
wgetstr_head()
{
    atf_set "descr" "Check getting a string from window input"
}
wgetstr_body()
{
    h_run wgetstr
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

atf_test_case mvwgetstr
mvwgetstr_head()
{
    atf_set "descr" "Move the cursor and get characters on window input"
}
mvwgetstr_body()
{
    h_run mvwgetstr
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

atf_test_case key_name
key_name_head()
{
    atf_set "descr" "Convert integers into printable key names"
}
key_name_body()
{
    h_run key_name
}

atf_test_case keypad
keypad_head()
{
    atf_set "descr" "Checks enable/disable abbreviation of function keys - tests is_keypad also"
}
keypad_body()
{
    h_run keypad
}

atf_test_case notimeout
notimeout_head()
{
    atf_set "descr" "Checks notimeout when reading a character"
}
notimeout_body()
{
    h_run notimeout
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

atf_test_case wtimeout
wtimeout_head()
{
    atf_set "descr" "Checks timeout when reading a character on window"
}
wtimeout_body()
{
    h_run wtimeout
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

atf_test_case unget_wch
unget_wch_head()
{
    atf_set "descr" "Checks pushing of character into input queue - tests ungetch also"
}
unget_wch_body()
{
    h_run unget_wch
}

atf_test_case getn_wstr
getn_wstr_head()
{
    atf_set "descr" "Checks getting limited characters from wide string through queue"
}
getn_wstr_body()
{
    h_run getn_wstr
}

atf_test_case wgetn_wstr
wgetn_wstr_head()
{
    atf_set "descr" "Checks getting limited characters from wide string on window through queue"
}
wgetn_wstr_body()
{
    h_run wgetn_wstr
}

atf_test_case get_wstr
get_wstr_head()
{
    atf_set "descr" "Checks getting characters from wide string through queue"
}
get_wstr_body()
{
    h_run get_wstr
}

atf_test_case wget_wstr
wget_wstr_head()
{
    atf_set "descr" "Checks getting characters from wide string on window through queue"
}
wget_wstr_body()
{
    h_run wget_wstr
}

atf_test_case mvgetn_wstr
mvgetn_wstr_head()
{
    atf_set "descr" "Move the cursor and get limited characters from wide string through queue"
}
mvgetn_wstr_body()
{
    h_run mvgetn_wstr
}

atf_test_case mvwgetn_wstr
mvwgetn_wstr_head()
{
    atf_set "descr" "Move the cursor and get limited characters from wide string on window through queue"
}
mvwgetn_wstr_body()
{
    h_run mvwgetn_wstr
}

atf_test_case mvget_wstr
mvget_wstr_head()
{
    atf_set "descr" "Move the cursor and get characters from wide string through queue"
}
mvget_wstr_body()
{
    h_run mvget_wstr
}

atf_test_case mvwget_wstr
mvwget_wstr_head()
{
    atf_set "descr" "Move the cursor and get characters from wide string on window through queue"
}
mvwget_wstr_body()
{
    h_run mvwget_wstr
}

atf_test_case get_wch
get_wch_head()
{
	atf_set "descr" "Checks reading a complex character through input queue"
}
get_wch_body()
{
	h_run get_wch
}

##########################################
# curses read screen contents routines
##########################################

atf_test_case inch
inch_head()
{
	atf_set "descr" "Get the character under the cursor on stdscr"
}
inch_body()
{
	h_run inch
}

atf_test_case winch
winch_head()
{
    atf_set "descr" "Get the character under the cursor on window"
}
winch_body()
{
    h_run winch
}

atf_test_case mvinch
mvinch_head()
{
	atf_set "descr" "Move the cursor and get the character under the cursor on stdscr"
}
mvinch_body()
{
	h_run mvinch
}

atf_test_case mvwinch
mvwinch_head()
{
    atf_set "descr" "Move the cursor and get the character under the cursor on window"
}
mvwinch_body()
{
    h_run mvwinch
}

atf_test_case inchnstr
inchnstr_head()
{
	atf_set "descr" "Get a limited chtype string from the stdscr - tests inchstr too"
}
inchnstr_body()
{
	h_run inchnstr
}

atf_test_case winchnstr
winchnstr_head()
{
    atf_set "descr" "Get a limited chtype string from the window - tests winchstr too"
}
winchnstr_body()
{
    h_run winchnstr
}

atf_test_case mvinchnstr
mvinchnstr_head()
{
	atf_set "descr" "Move the cursor read characters from stdscr - tests both mvinchstr and mvinchnstr"
}
mvinchnstr_body()
{
	h_run mvinchnstr
}

atf_test_case mvwinchnstr
mvwinchnstr_head()
{
    atf_set "descr" "Move the cursor read characters from window - tests both mvinchstr and mvinchnstr"
}
mvwinchnstr_body()
{
    h_run mvwinchnstr
}

atf_test_case innstr
innstr_head()
{
	atf_set "descr" "Get a limited string starting at the cursor from stdscr - tests instr also"
}
innstr_body()
{
	h_run innstr
}

atf_test_case winnstr
winnstr_head()
{
    atf_set "descr" "Get a limited string starting at the cursor from window - tests instr also"
}
winnstr_body()
{
    h_run winnstr
}

atf_test_case mvinnstr
mvinnstr_head()
{
    atf_set "descr" "Move the cursor read limited characters from stdscr - tests mvinstr also"
}
mvinnstr_body()
{
    h_run mvinnstr
}

atf_test_case mvwinnstr
mvwinnstr_head()
{
    atf_set "descr" "Move the cursor read limited characters from window - tests mvwinstr also"
}
mvwinnstr_body()
{
    h_run mvwinnstr
}

atf_test_case in_wch
in_wch_head()
{
    atf_set "descr" "Read the complex character from stdscr - tests mvin_wch too"
}
in_wch_body()
{
    h_run in_wch
}

atf_test_case win_wch
win_wch_head()
{
    atf_set "descr" "Read the complex character from window - tests mvwin_wch too"
}
win_wch_body()
{
    h_run win_wch
}

atf_test_case innwstr
innwstr_head()
{
    atf_set "descr" "Get a limited wide string starting at the cursor from stdscr"
}
innwstr_body()
{
    h_run innwstr
}

atf_test_case winnwstr
winnwstr_head()
{
    atf_set "descr" "Get a limited wide string starting at the cursor from window"
}
winnwstr_body()
{
    h_run winnwstr
}

atf_test_case inwstr
inwstr_head()
{
    atf_set "descr" "Get a wide string starting at the cursor from stdscr"
}
inwstr_body()
{
    h_run inwstr
}

atf_test_case winwstr
winwstr_head()
{
    atf_set "descr" "Get a wide string starting at the cursor from window"
}
winwstr_body()
{
    h_run winwstr
}

atf_test_case mvinnwstr
mvinnwstr_head()
{
    atf_set "descr" "Move the cursor and get a limited wide string starting at the cursor from stdscr"
}
mvinnwstr_body()
{
    h_run mvinnwstr
}

atf_test_case mvwinnwstr
mvwinnwstr_head()
{
    atf_set "descr" "Move the cursor and get a limited wide string starting at the cursor from window"
}
mvwinnwstr_body()
{
    h_run mvwinnwstr
}

atf_test_case mvinwstr
mvinwstr_head()
{
    atf_set "descr" "Move the cursor and get a wide string starting at the cursor from stdscr"
}
mvinwstr_body()
{
    h_run mvinwstr
}

atf_test_case mvwinwstr
mvwinwstr_head()
{
    atf_set "descr" "Move the cursor and get a limited wide string starting at the cursor from window"
}
mvwinwstr_body()
{
    h_run mvwinwstr
}

##########################################
# curses insert character to window routines
##########################################

atf_test_case insch
insch_head()
{
    atf_set "descr" "Tests inserting a chtype to stdscr"
}
insch_body()
{
    h_run insch
}

atf_test_case winsch
winsch_head()
{
    atf_set "descr" "Tests inserting a chtype to window"
}
winsch_body()
{
    h_run winsch
}

atf_test_case mvinsch
mvinsch_head()
{
    atf_set "descr" "Move the cursor and insert a chtype to stdscr"
}
mvinsch_body()
{
    h_run mvinsch
}

atf_test_case mvwinsch
mvwinsch_head()
{
    atf_set "descr" "Move the cursor and insert a chtype to window"
}
mvwinsch_body()
{
    h_run mvwinsch
}

atf_test_case ins_wch
ins_wch_head()
{
    atf_set "descr" "Tests inserting complex character to stdscr"
}
ins_wch_body()
{
    h_run ins_wch
}

atf_test_case wins_wch
wins_wch_head()
{
    atf_set "descr" "Tests inserting complex character to window"
}
wins_wch_body()
{
    h_run wins_wch
}

atf_test_case mvins_wch
mvins_wch_head()
{
    atf_set "descr" "Move the cursor and insert complex character to stdscr"
}
mvins_wch_body()
{
    h_run mvins_wch
}

atf_test_case mvwins_wch
mvwins_wch_head()
{
    atf_set "descr" "Move the cursor and insert complex character to window"
}
mvwins_wch_body()
{
    h_run mvwins_wch
}

atf_test_case ins_nwstr
ins_nwstr_head()
{
    atf_set "descr" "Tests inserting a limited wide character string to stdscr"
}
ins_nwstr_body()
{
    h_run ins_nwstr
}

atf_test_case wins_nwstr
wins_nwstr_head()
{
    atf_set "descr" "Tests inserting a limited wide character string to window"
}
wins_nwstr_body()
{
    h_run wins_nwstr
}

atf_test_case ins_wstr
ins_wstr_head()
{
    atf_set "descr" "Tests inserting a wide character string to stdscr"
}
ins_wstr_body()
{
    h_run ins_wstr
}

atf_test_case wins_wstr
wins_wstr_head()
{
    atf_set "descr" "Tests inserting a wide character string to window"
}
wins_wstr_body()
{
    h_run wins_wstr
}

atf_test_case mvins_nwstr
mvins_nwstr_head()
{
    atf_set "descr" "Move the cursor and insert a limited wide character string to stdscr"
}
mvins_nwstr_body()
{
    h_run mvins_nwstr
}

atf_test_case mvwins_nwstr
mvwins_nwstr_head()
{
    atf_set "descr" "Move the cursor and insert a limited wide character string to window"
}
mvwins_nwstr_body()
{
    h_run mvwins_nwstr
}

atf_test_case mvins_wstr
mvins_wstr_head()
{
    atf_set "descr" "Move the cursor and insert a wide character string to stdscr"
}
mvins_wstr_body()
{
    h_run mvins_wstr
}

atf_test_case mvwins_wstr
mvwins_wstr_head()
{
    atf_set "descr" "Move the cursor and insert a wide character string to window"
}
mvwins_wstr_body()
{
    h_run mvwins_wstr
}

##########################################
# curses delete characters routines
##########################################

atf_test_case delch
delch_head()
{
    atf_set "descr" "Tests deleting a character from stdscr and window both"
}
delch_body()
{
    h_run delch
}

atf_test_case mvdelch
mvdelch_head()
{
    atf_set "descr" "Move the cursor, deletes the character from stdscr and window"
}
mvdelch_body()
{
    h_run mvdelch
}

##########################################
# curses terminal manipulation routines
##########################################

atf_test_case beep
beep_head()
{
	atf_set "descr" "Check sending a beep"
}
beep_body()
{
	h_run beep
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

atf_test_case curs_set
curs_set_head()
{
	atf_set "descr" "Check setting the cursor visibility works"
}
curs_set_body()
{
	h_run curs_set
}

atf_test_case delay_output
delay_output_head()
{
    atf_set "descr" "Tests pausing the output"
}
delay_output_body()
{
    h_run delay_output
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

atf_test_case erasewchar
erasewchar_head()
{
    atf_set "descr" "Validate erase wide char can be retrieved"
}
erasewchar_body()
{
    h_run erasewchar
}

atf_test_case echochar
echochar_head()
{
    atf_set "descr" "echo single-byte character and rendition to a stdscr/window and refresh"
}
echochar_body()
{
    h_run echochar
}

atf_test_case echo_wchar
echo_wchar_head()
{
    atf_set "descr" "echo wide character and rendition to a stdscr and refresh"
}
echo_wchar_body()
{
    h_run echo_wchar
}

atf_test_case wecho_wchar
wecho_wchar_head()
{
    atf_set "descr" "echo wide character and rendition to a window and refresh"
}
wecho_wchar_body()
{
    h_run wecho_wchar
}

atf_test_case halfdelay
halfdelay_head()
{
    atf_set "descr" "Tests setting the input mode to half delay"
}
halfdelay_body()
{
    h_run halfdelay
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

atf_test_case killchar
killchar_head()
{
	atf_set "descr" "Get the value of the terminals kill character"
}
killchar_body()
{
	h_run killchar
}

atf_test_case killwchar
killwchar_head()
{
    atf_set "descr" "Get the value of the terminals wide kill character"
}
killwchar_body()
{
    h_run killwchar
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

atf_test_case cbreak
cbreak_head()
{
	atf_set "descr" "Check cbreak mode works"
}
cbreak_body()
{
	h_run cbreak
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

##########################################
# curses general attribute manipulation routines
##########################################

atf_test_case attributes
attributes_head()
{
	atf_set "descr" "Check setting, clearing and getting of attributes of stdscr"
}
attributes_body()
{
	h_run attributes
}

atf_test_case wattributes
wattributes_head()
{
    atf_set "descr" "Check setting, clearing and getting of attributes of window"
}
wattributes_body()
{
    h_run wattributes
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

atf_test_case color_set
color_set_head()
{
    atf_set "descr" "Validate curses can set the color pair for stdscr"
}
color_set_body()
{
    h_run color_set
}

atf_test_case wcolor_set
wcolor_set_head()
{
    atf_set "descr" "Validate curses can set the color pair for window"
}
wcolor_set_body()
{
    h_run wcolor_set
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

##########################################
# curses on-screen attribute manipulation routines
##########################################

atf_test_case chgat
chgat_head()
{
	atf_set "descr" "Check changing attributes works on stdscr"
}
chgat_body()
{
	h_run chgat
}

atf_test_case wchgat
wchgat_head()
{
    atf_set "descr" "Check changing attributes works on window"
}
wchgat_body()
{
    h_run wchgat
}

atf_test_case mvchgat
mvchgat_head()
{
	atf_set "descr" "Move the cursor and change the attributes on the screen"
}
mvchgat_body()
{
	h_run mvchgat
}

atf_test_case mvwchgat
mvwchgat_head()
{
    atf_set "descr" "Move the cursor and change the attributes on the window"
}
mvwchgat_body()
{
    h_run mvwchgat
}

##########################################
# curses standout attribute manipulation routines
##########################################

atf_test_case standout
standout_head()
{
    atf_set "descr" "Checks tuning on/off of standard attribute on stdscr"
}
standout_body()
{
    h_run standout
}

atf_test_case wstandout
wstandout_head()
{
    atf_set "descr" "Checks tuning on/off of standard attribute on window"
}
wstandout_body()
{
    h_run wstandout
}

##########################################
# curses color manipulation routines
##########################################

atf_test_case has_colors
has_colors_head()
{
	atf_set "descr" "Check if the terminal can support colours"
}
has_colors_body()
{
	h_run has_colors
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

atf_test_case start_color
start_color_head()
{
    atf_set "descr" "Check if curses can enable use of colours"
}
start_color_body()
{
    h_run start_color
}

atf_test_case pair_content
pair_content_head()
{
    atf_set "descr" "Checks color pair initialization and retrieval"
}
pair_content_body()
{
    h_run pair_content
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

atf_test_case color_content
color_content_head()
{
    atf_set "descr" "Check if curses can extract the color intensity from colors"
}
color_content_body()
{
    h_run color_content
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

##########################################
# curses clear window routines
##########################################

atf_test_case clear
clear_head()
{
	atf_set "descr" "Check clear,erase,clrtobot,clrtoeol work - tests window routines too"
}
clear_body()
{
	h_run clear
}

atf_test_case clearok
clearok_head()
{
    atf_set "descr" "Check clearing of screen during a refresh if correspnding flag is set"
}
clearok_body()
{
    h_run clearok
}

##########################################
# curses terminal update routines
##########################################

atf_test_case doupdate
doupdate_head()
{
	atf_set "descr" "Check doupdate performs an update - test wnoutrefresh too"
}
doupdate_body()
{
	h_run doupdate
}

atf_test_case immedok
immedok_head()
{
    atf_set "descr" "Checks if the screen is refreshed whenever window is changed"
}
immedok_body()
{
    h_run immedok
}

atf_test_case leaveok
leaveok_head()
{
    atf_set "descr" "Checks cursor positioning from refresh operations - tests is_leaveok too"
}
leaveok_body()
{
    h_run leaveok
}

##########################################
# curses window scrolling routines
##########################################

atf_test_case wscrl
wscrl_head()
{
	atf_set "descr" "Check window scrolling"
}
wscrl_body()
{
	h_run wscrl
}

atf_test_case scroll
scroll_head()
{
    atf_set "descr" "Checks scrolling"
}
scroll_body()
{
    h_run scroll
}

atf_test_case setscrreg
setscrreg_head()
{
    atf_set "descr" "Checks if setting the scrolling region works for stdscr"
}
setscrreg_body()
{
    h_run setscrreg
}

atf_test_case wsetscrreg
wsetscrreg_head()
{
    atf_set "descr" "Checks if setting the scrolling region works for window"
}
wsetscrreg_body()
{
    h_run wsetscrreg
}

##########################################
# curses window modification routines
##########################################

atf_test_case touchline
touchline_head()
{
    atf_set "descr" "Checks touchline to touch lines"
}
touchline_body()
{
    h_run touchline
}

atf_test_case touchoverlap
touchoverlap_head()
{
    atf_set "descr" "Check touching of partial and full overlap of windows"
}
touchoverlap_body()
{
    h_run touchoverlap
}

atf_test_case touchwin
touchwin_head()
{
    atf_set "descr" "Tests touching of window to completely refresh it"
}
touchwin_body()
{
    h_run touchwin
}

atf_test_case untouchwin
untouchwin_head()
{
    atf_set "descr" "Tests untouching the changes made to window so they are not reflected in refresh"
}
untouchwin_body()
{
    h_run untouchwin
}

atf_test_case wtouchln
wtouchln_head()
{
    atf_set "descr" "Tests touching of multiple lines in window"
}
wtouchln_body()
{
    h_run wtouchln
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

atf_test_case redrawwin
redrawwin_head()
{
    atf_set "descr" "Tests marking whole window as touched and redraw it"
}
redrawwin_body()
{
    h_run redrawwin
}

atf_test_case wredrawln
wredrawln_head()
{
    atf_set "descr" "Tests marking line in window as touched and redraw it"
}
wredrawln_body()
{
    h_run wredrawln
}

##########################################
# curses soft label key routines
##########################################

atf_test_case slk
slk_head()
{
    atf_set "descr" "Tests routines related to soft key labels"
}
slk_body()
{
    h_run slk
}

##########################################
# curses draw lines on windows routines
##########################################

atf_test_case hline
hline_head()
{
	atf_set "descr" "Draw a horizontal line on stdscr"
}
hline_body()
{
	h_run hline
}

atf_test_case whline
whline_head()
{
    atf_set "descr" "Draw a horizontal line on window - tests mvwhline too"
}
whline_body()
{
    h_run whline
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

atf_test_case wvline
wvline_head()
{
    atf_set "descr" "Draw a vertical line on window - tests mvwvline too"
}
wvline_body()
{
    h_run wvline
}

atf_test_case mvvline
mvvline_head()
{
	atf_set "descr" "Move the cursor and draw a vertical line - tests vline too"
}
mvvline_body()
{
	h_run mvvline
}

atf_test_case hline_set
hline_set_head()
{
    atf_set "descr" "Draws a horizontal line on stdscr using complex character"
}
hline_set_body()
{
    h_run hline_set
}

atf_test_case whline_set
whline_set_head()
{
    atf_set "descr" "Draws a horizontal line on window using complex character"
}
whline_set_body()
{
    h_run whline_set
}

atf_test_case vline_set
vline_set_head()
{
    atf_set "descr" "Draws a vertical line on stdscr using complex character"
}
vline_set_body()
{
    h_run vline_set
}

atf_test_case wvline_set
wvline_set_head()
{
    atf_set "descr" "Draws a vertical line on window using complex character"
}
wvline_set_body()
{
    h_run wvline_set
}

##########################################
# curses pad routines
##########################################

atf_test_case pad
pad_head()
{
	atf_set "descr" "Test the newpad, subpad, pnoutrefresh and prefresh functions"
}
pad_body()
{
	h_run pad
}

atf_test_case pechochar
pechochar_head()
{
    atf_set "descr" "Tests pechochar and pecho_wchar functions"
}
pechochar_body()
{
    h_run pechochar
}

##########################################
# curses cursor and window location and positioning routines
##########################################

atf_test_case cursor
cursor_head()
{
    atf_set "descr" "Tests cursor positioning and window location routines"
}
cursor_body()
{
    h_run cursor
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

atf_test_case getparx
getparx_head()
{
	atf_set "descr" "Check getting the location of a window relative to its parent"
}
getparx_body()
{
	h_run getparx
}

atf_test_case getbegy
getbegy_head()
{
	atf_set "descr" "Validate curses getting the maximum y value of a window"
}
getbegy_body()
{
	h_run getbegy
}

atf_test_case getbegx
getbegx_head()
{
	atf_set "descr" "Validate curses getting the maximum y value of a window"
}
getbegx_body()
{
	h_run getbegx
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


##########################################
# curses window routines
##########################################

atf_test_case copywin
copywin_head()
{
	atf_set "descr" "Check all the modes of copying a window work"
}
copywin_body()
{
	h_run copywin
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

atf_test_case delwin
delwin_head()
{
    atf_set "descr" "Tests deleting a window"
}
delwin_body()
{
    h_run delwin
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

atf_test_case mvwin
mvwin_head()
{
	atf_set "descr" "Check moving a window"
}
mvwin_body()
{
	h_run mvwin
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

atf_test_case newwin
newwin_head()
{
    atf_set "descr" "Check creating a new window"
}
newwin_body()
{
    h_run newwin
}

atf_test_case overlay
overlay_head()
{
    atf_set "descr" "Checks overlaying the overlapping portion of two windows"
}
overlay_body()
{
    h_run overlay
}

atf_test_case overwrite
overwrite_head()
{
    atf_set "descr" "Checks overwriting the overlapping portion of two windows"
}
overwrite_body()
{
    h_run overwrite
}

atf_test_case getwin
getwin_head()
{
    atf_set "descr" "Tests dumping window to, and reloading window from, a file"
}
getwin_body()
{
    h_run getwin
}

##########################################
# curses background attribute manipulation routines
##########################################

atf_test_case background
background_head()
{
	atf_set "descr" "Check setting background character and attributes for both stdscr and a window."
}
background_body()
{
	h_run background
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

atf_test_case bkgrndset
bkgrndset_head()
{
	atf_set "descr" "Validate curses sets the background character using a complex char on stdscr"
}
bkgrndset_body()
{
	h_run bkgrndset
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

##########################################
# curses border drawing routines
##########################################

atf_test_case box
box_head()
{
	atf_set "descr" "Checks drawing a box around a window"
}
box_body()
{
	h_run box
}

atf_test_case box_set
box_set_head()
{
    atf_set "descr" "Checks drawing the box from complex character"
}
box_set_body()
{
    h_run box_set
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

atf_test_case border_set
border_set_head()
{
    atf_set "descr" "Checks drawing borders from complex characters and renditions on stdscr"
}
border_set_body()
{
    h_run border_set
}

atf_test_case wborder_set
wborder_set_head()
{
    atf_set "descr" "Checks drawing borders from complex characters and renditions on window"
}
wborder_set_body()
{
    h_run wborder_set
}

##########################################
# curses insert or delete lines routines
##########################################

atf_test_case deleteln
deleteln_head()
{
    atf_set "descr" "Checks curses can delete lines from stdscr and window both"
}
deleteln_body()
{
    h_run deleteln
}

atf_test_case insertln
insertln_head()
{
    atf_set "descr" "Checks curses can insert lines from stdscr and window both"
}
insertln_body()
{
    h_run insertln
}

atf_test_case insdelln
insdelln_head()
{
    atf_set "descr" "Checks curses can insert/delete lines from stdscr and window both based on argument"
}
insdelln_body()
{
    h_run insdelln
}

##########################################
# curses print formatted strings on windows routines
##########################################

atf_test_case wprintw
wprintw_head()
{
	atf_set "descr" "Checks printing to a window"
}
wprintw_body()
{
	h_run wprintw
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

##########################################
# curses underscore attribute manipulation routines
##########################################

atf_test_case underscore
underscore_head()
{
	atf_set "descr" "Manipulate underscore attribute on stdscr"
}
underscore_body()
{
	h_run underscore
}

atf_test_case wunderscore
wunderscore_head()
{
	atf_set "descr" "Manipulate underscore attribute on window"
}
wunderscore_body()
{
	h_run wunderscore
}

atf_init_test_cases()
{
	# testframe utility functions
	atf_add_test_case startup
	atf_add_test_case window
	atf_add_test_case start_slk
	atf_add_test_case window_hierarchy
	atf_add_test_case two_window
	atf_add_test_case varcheck

	# curses add characters to window routines
	atf_add_test_case addbytes
	atf_add_test_case addch
	atf_add_test_case waddch
	atf_add_test_case mvaddch
	atf_add_test_case addchstr
	atf_add_test_case waddchstr
	atf_add_test_case addchnstr
	atf_add_test_case waddchnstr
	atf_add_test_case mvaddchstr
	atf_add_test_case mvwaddchstr
	atf_add_test_case mvaddchnstr
	atf_add_test_case mvwaddchnstr
	atf_add_test_case addstr
	atf_add_test_case addwstr
	atf_add_test_case waddstr
	atf_add_test_case waddwstr
	atf_add_test_case addnstr
	atf_add_test_case addnwstr
	atf_add_test_case waddnstr
	atf_add_test_case waddnwstr
	atf_add_test_case mvwaddnwstr
	atf_add_test_case mvaddstr
	atf_add_test_case mvaddwstr
	atf_add_test_case mvwaddwstr
	atf_add_test_case mvwaddstr
	atf_add_test_case mvaddnstr
	atf_add_test_case mvaddnwstr
	atf_add_test_case mvwaddnstr
	atf_add_test_case add_wch
	atf_add_test_case wadd_wch

	# curses input stream routines
	atf_add_test_case getch
	#atf_add_test_case wgetch [test is missing]
	atf_add_test_case define_key
	atf_add_test_case keyok
	atf_add_test_case getnstr
	atf_add_test_case wgetnstr
	atf_add_test_case mvgetnstr
	atf_add_test_case mvwgetnstr
	atf_add_test_case getstr
	atf_add_test_case wgetstr
	atf_add_test_case mvgetstr
	atf_add_test_case mvwgetstr
	atf_add_test_case keyname
	atf_add_test_case key_name
	atf_add_test_case keypad
	atf_add_test_case notimeout
	atf_add_test_case timeout
	atf_add_test_case wtimeout
	atf_add_test_case nodelay
	atf_add_test_case unget_wch
	atf_add_test_case getn_wstr
	atf_add_test_case wgetn_wstr
	atf_add_test_case get_wstr
	atf_add_test_case wget_wstr
	atf_add_test_case mvgetn_wstr
	atf_add_test_case mvwgetn_wstr
	atf_add_test_case mvget_wstr
	atf_add_test_case mvwget_wstr
	atf_add_test_case get_wch

	# curses read screen contents routines
	atf_add_test_case inch
	atf_add_test_case winch
	atf_add_test_case mvinch
	atf_add_test_case mvwinch
	atf_add_test_case inchnstr
	atf_add_test_case winchnstr
	atf_add_test_case mvinchnstr
	atf_add_test_case mvwinchnstr
	atf_add_test_case innstr
	atf_add_test_case winnstr
	atf_add_test_case mvinnstr
	atf_add_test_case mvwinnstr
	atf_add_test_case in_wch
	atf_add_test_case win_wch
	atf_add_test_case innwstr
	atf_add_test_case winnwstr
	atf_add_test_case inwstr
	atf_add_test_case winwstr
	atf_add_test_case mvinnwstr
	atf_add_test_case mvwinnwstr
	atf_add_test_case mvinwstr
	atf_add_test_case mvwinwstr

	# curses insert character to window routines
	atf_add_test_case insch
	atf_add_test_case winsch
	atf_add_test_case mvinsch
	atf_add_test_case mvwinsch
	atf_add_test_case ins_wch
	atf_add_test_case wins_wch
	atf_add_test_case mvins_wch
	atf_add_test_case mvwins_wch
	atf_add_test_case ins_nwstr
	atf_add_test_case wins_nwstr
	atf_add_test_case ins_wstr
	atf_add_test_case wins_wstr
	atf_add_test_case mvins_nwstr
	atf_add_test_case mvwins_nwstr
	atf_add_test_case mvins_wstr
	atf_add_test_case mvwins_wstr

	# curses delete characters routines
	atf_add_test_case delch
	atf_add_test_case mvdelch

	# curses terminal manipulation routines
	atf_add_test_case beep
	atf_add_test_case flash
	atf_add_test_case curs_set
	# atf_add_test_case delay_output [WORKS CORRECTLY BUT FAILS IN TESTFRAME]
	atf_add_test_case erasechar
	atf_add_test_case erasewchar
	atf_add_test_case echochar
	atf_add_test_case echo_wchar
	atf_add_test_case wecho_wchar
	atf_add_test_case halfdelay
	atf_add_test_case has_ic
	atf_add_test_case killchar
	atf_add_test_case killwchar
	atf_add_test_case meta
	atf_add_test_case cbreak
	atf_add_test_case nocbreak

	# curses general attribute manipulation routines
	atf_add_test_case attributes
	atf_add_test_case wattributes
	atf_add_test_case getattrs
	atf_add_test_case color_set
	atf_add_test_case wcolor_set
	atf_add_test_case termattrs

	# curses on-screen attribute manipulation routines
	atf_add_test_case chgat
	atf_add_test_case wchgat
	atf_add_test_case mvchgat
	atf_add_test_case mvwchgat

	# curses standout attribute manipulation routines
	atf_add_test_case standout
	atf_add_test_case wstandout

	# curses color manipulation routines
	atf_add_test_case has_colors
	atf_add_test_case can_change_color
	atf_add_test_case start_color
	atf_add_test_case pair_content
	atf_add_test_case init_color
	atf_add_test_case color_content
	atf_add_test_case assume_default_colors

	# curses clear window routines
	atf_add_test_case clear
	atf_add_test_case clearok

	# curses terminal update routines
	atf_add_test_case doupdate
	atf_add_test_case immedok
	atf_add_test_case leaveok

	# curses window scrolling routines
	atf_add_test_case wscrl
	atf_add_test_case scroll
	atf_add_test_case setscrreg
	atf_add_test_case wsetscrreg

	# curses window modification routines
	atf_add_test_case touchline
	atf_add_test_case touchoverlap
	atf_add_test_case touchwin
	atf_add_test_case untouchwin
	atf_add_test_case wtouchln
	atf_add_test_case is_linetouched
	atf_add_test_case is_wintouched
	atf_add_test_case redrawwin
	atf_add_test_case wredrawln

	# curses soft label key routines
	atf_add_test_case slk

	# curses draw lines on windows routines
	atf_add_test_case hline
	atf_add_test_case whline
	atf_add_test_case mvhline
	atf_add_test_case wvline
	atf_add_test_case mvvline
	atf_add_test_case hline_set
	atf_add_test_case whline_set
	atf_add_test_case vline_set
	atf_add_test_case wvline_set

	# curses pad routines
	atf_add_test_case pad
	atf_add_test_case pechochar

	# curses cursor and window location and positioning routines
	atf_add_test_case cursor
	atf_add_test_case getcurx
	atf_add_test_case getmaxx
	atf_add_test_case getmaxy
	atf_add_test_case getparx
	atf_add_test_case getbegy
	atf_add_test_case getbegx
	atf_add_test_case mvcur

	# curses window routines
	atf_add_test_case copywin
	atf_add_test_case dupwin
	# atf_add_test_case delwin [FAILING]
	atf_add_test_case derwin
	atf_add_test_case mvwin
	atf_add_test_case mvderwin
	atf_add_test_case newwin
	atf_add_test_case overlay
	atf_add_test_case overwrite
	atf_add_test_case getwin

	# curses background attribute manipulation routines
	atf_add_test_case background
	atf_add_test_case bkgdset
	atf_add_test_case bkgrndset
	atf_add_test_case getbkgd

	# curses border drawing routines
	atf_add_test_case box
	atf_add_test_case box_set
	atf_add_test_case wborder
	atf_add_test_case border_set
	atf_add_test_case wborder_set

	# curses insert or delete lines routines
	atf_add_test_case deleteln
	atf_add_test_case insertln
	atf_add_test_case insdelln

	# curses print formatted strings on windows routines
	atf_add_test_case wprintw
	atf_add_test_case mvprintw
	atf_add_test_case mvscanw

	# curses underscore attribute manipulation routines
	atf_add_test_case underscore
	atf_add_test_case wunderscore
}
