/* Copyright (C) 2023 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "gdbsupport/gdb_assert.h"
#include "gdbsupport/selftest.h"
#include "test-target.h"
#include "scoped-mock-context.h"
#include "break-cond-parse.h"
#include "tid-parse.h"
#include "ada-lang.h"
#include "exceptions.h"

/* When parsing tokens from a string, which direction are we parsing?

   Given the following string and pointer 'ptr':

	ABC DEF GHI JKL
	       ^
	       ptr

   Parsing 'forward' will return the token 'GHI' and update 'ptr' to point
   between GHI and JKL.  Parsing 'backward' will return the token 'DEF' and
   update 'ptr' to point between ABC and DEF.
*/

enum class parse_direction
{
  /* Parse the next token forwards.  */
  forward,

  /* Parse the previous token backwards.  */
  backward
};

/* Find the next token in DIRECTION from *CURR.  */

static std::string_view
find_next_token (const char **curr, parse_direction direction)
{
  const char *tok_start, *tok_end;

  gdb_assert (**curr != '\0');

  if (direction == parse_direction::forward)
    {
      *curr = skip_spaces (*curr);
      tok_start = *curr;
      *curr = skip_to_space (*curr);
      tok_end = *curr - 1;
    }
  else
    {
      gdb_assert (direction == parse_direction::backward);

      while (isspace (**curr))
	--(*curr);

      tok_end = *curr;

      while (!isspace (**curr))
	--(*curr);

      tok_start = (*curr) + 1;
    }

  return std::string_view (tok_start, tok_end - tok_start + 1);
}

/* A class that represents a complete parsed token.  Each token has a type
   and a std::string_view into the original breakpoint condition string.  */

struct token
{
  /* The types a token might take.  */
  enum class type
  {
    /* These are the token types for the 'if', 'thread', 'inferior', and
       'task' keywords.  The m_content for these token types is the value
       passed to the keyword, not the keyword itself.  */
    CONDITION,
    THREAD,
    INFERIOR,
    TASK,

    /* This is the token used when we find unknown content, the m_content
       for this token is the rest of the input string.  */
    REST,

    /* This is the token for the -force-condition token, the m_content for
       this token contains the keyword itself.  */
    FORCE
  };

  token (enum type type, std::string_view content)
    : m_type (type),
      m_content (std::move (content))
  {
    /* Nothing.  */
  }

  /* Return a string representing this token.  Only used for debug.  */
  std::string to_string () const
  {
    switch (m_type)
      {
      case type::CONDITION:
	return string_printf ("{ CONDITION: \"%s\" }",
			      std::string (m_content).c_str ());
      case type::THREAD:
	return string_printf ("{ THREAD: \"%s\" }",
			      std::string (m_content).c_str ());
      case type::INFERIOR:
	return string_printf ("{ INFERIOR: \"%s\" }",
			      std::string (m_content).c_str ());
      case type::TASK:
	return string_printf ("{ TASK: \"%s\" }",
			      std::string (m_content).c_str ());
      case type::REST:
	return string_printf ("{ REST: \"%s\" }",
			      std::string (m_content).c_str ());
      case type::FORCE:
	return string_printf ("{ FORCE }");
      default:
	return "** unknown **";
      }
  }

  /* The type of this token.  */
  const type &get_type () const
  {
    return m_type;
  }

  /* Return the value of this token.  */
  const std::string_view &get_value () const
  {
    gdb_assert (m_content.size () > 0);
    return m_content;
  }

  /* Extend this token with the contents of OTHER.  This only makes sense
     if OTHER is the next token after this one in the original string,
     however, enforcing that restriction is left to the caller of this
     function.

     When OTHER is a keyword/value token, e.g. 'thread 1', the m_content
     for OTHER will only point to the '1'.  However, as the m_content is a
     std::string_view, then when we merge the m_content of OTHER into this
     token we automatically merge in the 'thread' part too, as it
     naturally sits between this token and OTHER.  */

  void
  extend (const token &other)
  {
    m_content = std::string_view (this->m_content.data (),
				  (other.m_content.data ()
				   - this->m_content.data ()
				   + other.m_content.size ()));
  }

private:
  /* The type of this token.  */
  type m_type;

  /* The important content part of this token.  The extend member function
     depends on this being a std::string_view.  */
  std::string_view m_content;
};

/* Split STR, a breakpoint condition string, into a vector of tokens where
   each token represents a component of the condition.  Tokens are first
   parsed from the front of STR until we encounter an 'if' token.  At this
   point tokens are parsed from the end of STR until we encounter an
   unknown token, which we assume is the other end of the 'if' condition.
   If when scanning forward we encounter an unknown token then the
   remainder of STR is placed into a 'rest' token (the rest of the
   string), and no backward scan is performed.  */

static std::vector<token>
parse_all_tokens (const char *str)
{
  gdb_assert (str != nullptr);

  std::vector<token> forward_results;
  std::vector<token> backward_results;

  const char *cond_start = nullptr;
  const char *cond_end = nullptr;
  parse_direction direction = parse_direction::forward;
  std::vector<token> *curr_results = &forward_results;
  while (*str != '\0')
    {
      /* Find the next token.  If moving backward and this token starts at
	 the same location as the condition then we must have found the
	 other end of the condition string -- we're done.  */
      std::string_view t = find_next_token (&str, direction);
      if (direction == parse_direction::backward && t.data () <= cond_start)
	{
	  cond_end = &t.back ();
	  break;
	}

      /* We only have a single flag option to check for.  All the other
	 options take a value so require an additional token to be found.
	 Additionally, we require that this flag be at least '-f', we
	 don't allow it to be abbreviated to '-'.  */
      if (t.length () > 1 && startswith ("-force-condition", t))
	{
	  curr_results->emplace_back (token::type::FORCE, t);
	  continue;
	}

      /* Maybe the first token was the last token in the string.  If this
	 is the case then we definitely can't try to extract a value
	 token.  This also means that the token T is meaningless.  Reset
	 TOK to point at the start of the unknown content and break out of
	 the loop.  We'll record the unknown part of the string outside of
	 the scanning loop (below).  */
      if (direction == parse_direction::forward && *str == '\0')
	{
	  str = t.data ();
	  break;
	}

      /* As before, find the next token and, if we are scanning backwards,
	 check that we have not reached the start of the condition string.  */
      std::string_view v = find_next_token (&str, direction);
      if (direction == parse_direction::backward && v.data () <= cond_start)
	{
	  /* Use token T here as that must also be part of the condition
	     string.  */
	  cond_end = &t.back ();
	  break;
	}

      /* When moving backward we will first parse the value token then the
	 keyword token, so swap them now.  */
      if (direction == parse_direction::backward)
	std::swap (t, v);

      /* Check for valid option in token T.  If we find a valid option then
	 parse the value from the token V.  Except for 'if', that's handled
	 differently.

	 For the 'if' token we need to capture the entire condition
	 string, so record the start of the condition string and then
	 start scanning backwards looking for the end of the condition
	 string.

	 The order of these checks is important, at least the check for
	 'thread' must occur before the check for 'task'.  We accept
	 abbreviations of these token names, and 't' should resolve to
	 'thread', which will only happen if we check 'thread' first.  */
      if (direction == parse_direction::forward && startswith ("if", t))
	{
	  cond_start = v.data ();
	  str = str + strlen (str);
	  gdb_assert (*str == '\0');
	  --str;
	  direction = parse_direction::backward;
	  curr_results = &backward_results;
	  continue;
	}
      else if (startswith ("thread", t))
	curr_results->emplace_back (token::type::THREAD, v);
      else if (startswith ("inferior", t))
	curr_results->emplace_back (token::type::INFERIOR, v);
      else if (startswith ("task", t))
	curr_results->emplace_back (token::type::TASK, v);
      else
	{
	  /* An unknown token.  If we are scanning forward then reset TOK
	     to point at the start of the unknown content, we record this
	     outside of the scanning loop (below).

	     If we are scanning backward then unknown content is assumed to
	     be the other end of the condition string, obviously, this is
	     just a heuristic, we could be looking at a mistyped command
	     line, but this will be spotted when the condition is
	     eventually evaluated.

	     Either way, no more scanning is required after this.  */
	  if (direction == parse_direction::forward)
	    str = t.data ();
	  else
	    {
	      gdb_assert (direction == parse_direction::backward);
	      cond_end = &v.back ();
	    }
	  break;
	}
    }

  if (cond_start != nullptr)
    {
      /* If we found the start of a condition string then we should have
	 switched to backward scan mode, and found the end of the condition
	 string.  Capture the whole condition string into COND_STRING
	 now.  */
      gdb_assert (direction == parse_direction::backward);
      gdb_assert (cond_end != nullptr);

      std::string_view v (cond_start, cond_end - cond_start + 1);

      forward_results.emplace_back (token::type::CONDITION, v);
    }
  else if (*str != '\0')
    {
      /* If we didn't have a condition start pointer then we should still
	 be in forward scanning mode.  If we didn't reach the end of the
	 input string (TOK is not at the null character) then the rest of
	 the input string is garbage that we didn't understand.

	 Record the unknown content into REST.  The caller of this function
	 will report this as an error later on.  We could report the error
	 here, but we prefer to allow the caller to run other checks, and
	 prioritise other errors before reporting this problem.  */
      gdb_assert (direction == parse_direction::forward);
      gdb_assert (cond_end == nullptr);

      std::string_view v (str, strlen (str));

      forward_results.emplace_back (token::type::REST, v);
    }

  /* If we have tokens in the BACKWARD_RESULTS vector then this means that
     we found an 'if' condition (which will be the last thing in the
     FORWARD_RESULTS vector), and then we started a backward scan.

     The last tokens from the input string (those after the 'if' condition)
     will be the first tokens added to the BACKWARD_RESULTS vector, so the
     last items in the BACKWARD_RESULTS vector are those next to the 'if'
     condition.

     Check the tokens in the BACKWARD_RESULTS vector from back to front.
     If the tokens look invalid then we assume that they are actually part
     of the 'if' condition, and merge the token with the 'if' condition.
     If it turns out that this was incorrect and that instead the user just
     messed up entering the token value, then this will show as an error
     when parsing the 'if' condition.

     Doing this allows us to handle things like:

       break function if ( variable == thread )

     Where 'thread' is a local variable within 'function'.  When parsing
     this we will initially see 'thread )' as a thread token with ')' as
     the value.  However, the following code will spot that ')' is not a
     valid thread-id, and so we merge 'thread )' into the 'if' condition
     string.

     This code also handles the special treatment for '-force-condition',
     which exists for backwards compatibility reasons.  Traditionally this
     flag, if it occurred immediately after the 'if' condition, would be
     treated as part of the 'if' condition.  When the breakpoint condition
     parsing code was rewritten, this behavior was retained.  */
  gdb_assert (backward_results.empty ()
	      || (forward_results.back ().get_type ()
		  == token::type::CONDITION));
  while (!backward_results.empty ())
    {
      token &t = backward_results.back ();

      if (t.get_type () == token::type::FORCE)
	forward_results.back ().extend (std::move (t));
      else if (t.get_type () == token::type::THREAD)
	{
	  const char *end;
	  std::string v (t.get_value ());
	  if (is_thread_id (v.c_str (), &end) && *end == '\0')
	    break;
	  forward_results.back ().extend (std::move (t));
	}
      else if (t.get_type () == token::type::INFERIOR
	       || t.get_type () == token::type::TASK)
	{
	  /* Place the token's value into a null-terminated string, parse
	     the string as a number and check that the entire string was
	     parsed.  If this is true then this looks like a valid inferior
	     or task number, otherwise, assume an invalid id, and merge
	     this token with the 'if' token.  */
	  char *end;
	  std::string v (t.get_value ());
	  (void) strtol (v.c_str (), &end, 0);
	  if (end > v.c_str () && *end == '\0')
	    break;
	  forward_results.back ().extend (std::move (t));
	}
      else
	gdb_assert_not_reached ("unexpected token type");

      /* If we found an actual valid token above then we will have broken
	 out of the loop.  We only get here if the token was merged with
	 the 'if' condition, in which case we can discard the last token
	 and then check the token before that.  */
      backward_results.pop_back ();
    }

  /* If after the above checks we still have some tokens in the
     BACKWARD_RESULTS vector, then these need to be appended to the
     FORWARD_RESULTS vector.  However, we first reverse the order so that
     FORWARD_RESULTS retains the tokens in the order they appeared in the
     input string.  */
  if (!backward_results.empty ())
    forward_results.insert (forward_results.end (),
			    backward_results.rbegin (),
			    backward_results.rend ());

  return forward_results;
}

/* Called when the global debug_breakpoint is true.  Prints VEC to the
   debug output stream.  */

static void
dump_condition_tokens (const std::vector<token> &vec)
{
  gdb_assert (debug_breakpoint);

  bool first = true;
  std::string str = "Tokens: ";
  for (const token &t : vec)
    {
      if (!first)
	str += " ";
      first = false;
      str += t.to_string ();
    }
  breakpoint_debug_printf ("%s", str.c_str ());
}

/* See break-cond-parse.h.  */

void
create_breakpoint_parse_arg_string
  (const char *str, gdb::unique_xmalloc_ptr<char> *cond_string_ptr,
   int *thread_ptr, int *inferior_ptr, int *task_ptr,
   gdb::unique_xmalloc_ptr<char> *rest_ptr, bool *force_ptr)
{
  /* Set up the defaults.  */
  cond_string_ptr->reset ();
  rest_ptr->reset ();
  *thread_ptr = -1;
  *inferior_ptr = -1;
  *task_ptr = -1;
  *force_ptr = false;

  if (str == nullptr)
    return;

  /* Split STR into a series of tokens.  */
  std::vector<token> tokens = parse_all_tokens (str);
  if (debug_breakpoint)
    dump_condition_tokens (tokens);

  /* Temporary variables.  Initialised to the default state, then updated
     as we parse TOKENS.  If all of TOKENS is parsed successfully then the
     state from these variables is copied into the output arguments before
     the function returns.  */
  int thread = -1, inferior = -1, task = -1;
  bool force = false;
  gdb::unique_xmalloc_ptr<char> cond_string, rest;

  for (const token &t : tokens)
    {
      std::string tok_value (t.get_value ());
      switch (t.get_type ())
	{
	case token::type::FORCE:
	  force = true;
	  break;
	case token::type::THREAD:
	  {
	    if (thread != -1)
	      error ("You can specify only one thread.");
	    if (task != -1 || inferior != -1)
	      error ("You can specify only one of thread, inferior, or task.");
	    const char *tmptok;
	    thread_info *thr = parse_thread_id (tok_value.c_str (), &tmptok);
	    gdb_assert (*tmptok == '\0');
	    thread = thr->global_num;
	  }
	  break;
	case token::type::INFERIOR:
	  {
	    if (inferior != -1)
	      error ("You can specify only one inferior.");
	    if (task != -1 || thread != -1)
	      error ("You can specify only one of thread, inferior, or task.");
	    char *tmptok;
	    long inferior_id = strtol (tok_value.c_str (), &tmptok, 0);
	    if (*tmptok != '\0')
	      error (_("Junk '%s' after inferior keyword."), tmptok);
	    if (inferior_id > INT_MAX)
	      error (_("No inferior number '%ld'"), inferior_id);
	    inferior = static_cast<int> (inferior_id);
	    struct inferior *inf = find_inferior_id (inferior);
	    if (inf == nullptr)
	      error (_("No inferior number '%d'"), inferior);
	  }
	  break;
	case token::type::TASK:
	  {
	    if (task != -1)
	      error ("You can specify only one task.");
	    if (inferior != -1 || thread != -1)
	      error ("You can specify only one of thread, inferior, or task.");
	    char *tmptok;
	    long task_id = strtol (tok_value.c_str (), &tmptok, 0);
	    if (*tmptok != '\0')
	      error (_("Junk '%s' after task keyword."), tmptok);
	    if (task_id > INT_MAX)
	      error (_("Unknown task %ld"), task_id);
	    task = static_cast<int> (task_id);
	    if (!valid_task_id (task))
	      error (_("Unknown task %d."), task);
	  }
	  break;
	case token::type::CONDITION:
	  cond_string.reset (savestring (t.get_value ().data (),
					 t.get_value ().size ()));
	  break;
	case token::type::REST:
	  rest.reset (savestring (t.get_value ().data (),
				  t.get_value ().size ()));
	  break;
	}
    }

  /* Move results into the output locations.  */
  *force_ptr = force;
  *thread_ptr = thread;
  *inferior_ptr = inferior;
  *task_ptr = task;
  rest_ptr->reset (rest.release ());
  cond_string_ptr->reset (cond_string.release ());
}

#if GDB_SELF_TEST

namespace selftests {

/* Run a single test of the create_breakpoint_parse_arg_string function.
   INPUT is passed to create_breakpoint_parse_arg_string while all other
   arguments are the expected output from
   create_breakpoint_parse_arg_string.  */

static void
test (const char *input, const char *condition, int thread = -1,
      int inferior = -1, int task = -1, bool force = false,
      const char *rest = nullptr, const char *error_msg = nullptr)
{
  gdb::unique_xmalloc_ptr<char> extracted_condition;
  gdb::unique_xmalloc_ptr<char> extracted_rest;
  int extracted_thread, extracted_inferior, extracted_task;
  bool extracted_force_condition;
  std::string exception_msg, error_str;

  if (error_msg != nullptr)
    error_str = std::string (error_msg) + "\n";

  try
    {
      create_breakpoint_parse_arg_string (input, &extracted_condition,
					  &extracted_thread,
					  &extracted_inferior,
					  &extracted_task, &extracted_rest,
					  &extracted_force_condition);
    }
  catch (const gdb_exception_error &ex)
    {
      string_file buf;

      exception_print (&buf, ex);
      exception_msg = buf.release ();
    }

  if ((condition == nullptr) != (extracted_condition.get () == nullptr)
      || (condition != nullptr
	  && strcmp (condition, extracted_condition.get ()) != 0)
      || (rest == nullptr) != (extracted_rest.get () == nullptr)
      || (rest != nullptr && strcmp (rest, extracted_rest.get ()) != 0)
      || thread != extracted_thread
      || inferior != extracted_inferior
      || task != extracted_task
      || force != extracted_force_condition
      || exception_msg != error_str)
    {
      if (run_verbose ())
	{
	  debug_printf ("input: '%s'\n", input);
	  debug_printf ("condition: '%s'\n", extracted_condition.get ());
	  debug_printf ("rest: '%s'\n", extracted_rest.get ());
	  debug_printf ("thread: %d\n", extracted_thread);
	  debug_printf ("inferior: %d\n", extracted_inferior);
	  debug_printf ("task: %d\n", extracted_task);
	  debug_printf ("forced: %s\n",
			extracted_force_condition ? "true" : "false");
	  debug_printf ("exception: '%s'\n", exception_msg.c_str ());
	}

      /* Report the failure.  */
      SELF_CHECK (false);
    }
}

/* Wrapper for test function.  Pass through the default values for all
   parameters, except the last parameter, which indicates that we expect
   INPUT to trigger an error.  */

static void
test_error (const char *input, const char *error_msg)
{
  test (input, nullptr, -1, -1, -1, false, nullptr, error_msg);
}

/* Test the create_breakpoint_parse_arg_string function.  Just wraps
   multiple calls to the test function above.  */

static void
create_breakpoint_parse_arg_string_tests ()
{
  gdbarch *arch = current_inferior ()->arch ();
  scoped_restore_current_pspace_and_thread restore;
  scoped_mock_context<test_target_ops> mock_target (arch);

  int global_thread_num = mock_target.mock_thread.global_num;

  /* Test parsing valid breakpoint condition strings.  */
  test ("  if blah  ", "blah");
  test (" if blah thread 1", "blah", global_thread_num);
  test (" if blah inferior 1", "blah", -1, 1);
  test (" if blah thread 1  ", "blah", global_thread_num);
  test ("thread 1 woof", nullptr, global_thread_num, -1, -1, false, "woof");
  test ("thread 1 X", nullptr, global_thread_num, -1, -1, false, "X");
  test (" if blah thread 1 -force-condition", "blah", global_thread_num,
	-1, -1, true);
  test (" -force-condition if blah thread 1", "blah", global_thread_num,
	-1, -1, true);
  test (" -force-condition if blah thread 1  ", "blah", global_thread_num,
	-1, -1, true);
  test ("thread 1 -force-condition if blah", "blah", global_thread_num,
	-1, -1, true);
  test ("if (A::outer::func ())", "(A::outer::func ())");
  test ("if ( foo == thread )", "( foo == thread )");
  test ("if ( foo == thread ) inferior 1", "( foo == thread )", -1, 1);
  test ("if ( foo == thread ) thread 1", "( foo == thread )",
	global_thread_num);
  test ("if foo == thread", "foo == thread");
  test ("if foo == thread 1", "foo ==", global_thread_num);

  /* Test parsing some invalid breakpoint condition strings.  */
  test_error ("thread 1 if foo == 123 thread 1",
	      "You can specify only one thread.");
  test_error ("thread 1 if foo == 123 inferior 1",
	      "You can specify only one of thread, inferior, or task.");
  test_error ("thread 1 if foo == 123 task 1",
	      "You can specify only one of thread, inferior, or task.");
  test_error ("inferior 1 if foo == 123 inferior 1",
	      "You can specify only one inferior.");
  test_error ("inferior 1 if foo == 123 thread 1",
	      "You can specify only one of thread, inferior, or task.");
  test_error ("inferior 1 if foo == 123 task 1",
	      "You can specify only one of thread, inferior, or task.");
  test_error ("thread 1.2.3", "Invalid thread ID: 1.2.3");
  test_error ("thread 1/2", "Invalid thread ID: 1/2");
  test_error ("thread 1xxx", "Invalid thread ID: 1xxx");
  test_error ("inferior 1xxx", "Junk 'xxx' after inferior keyword.");
  test_error ("task 1xxx", "Junk 'xxx' after task keyword.");
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

void _initialize_break_cond_parse ();
void
_initialize_break_cond_parse ()
{
#if GDB_SELF_TEST
  selftests::register_test
    ("create_breakpoint_parse_arg_string",
     selftests::create_breakpoint_parse_arg_string_tests);
#endif
}
