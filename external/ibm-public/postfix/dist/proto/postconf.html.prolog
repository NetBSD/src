<!doctype html public "-//W3C//DTD HTML 4.01 Transitional//EN"
        "http://www.w3.org/TR/html4/loose.dtd">

<html>

<head>

<title>Postfix Configuration Parameters </title>

<meta http-equiv="Content-Type" content="text/html; charset=utf-8">

</head>

<body>

<h1><img src="postfix-logo.jpg" width="203" height="98" alt="">Postfix Configuration Parameters </h1>

<hr>

<h2> Postfix main.cf file format </h2>

<p> The Postfix main.cf configuration file specifies a very small
subset of all the parameters that control the operation of the
Postfix mail system. Parameters not explicitly specified are left
at their default values. </p>

<p> The general format of the main.cf file is as follows: </p>

<ul>

<li> <p> Each logical line is in the form "parameter = value".
Whitespace around the "=" is ignored, as is whitespace at the end
of a logical line. </p>

<li> <p> Empty lines and whitespace-only lines are ignored, as are
lines whose first non-whitespace character is a `#'. </p>

<li> <p> A logical line starts with non-whitespace text. A line
that starts with whitespace continues a logical line. </p>

<li> <p> A parameter value may refer to other parameters. </p>

<ul>

<li> <p> The expressions "$name" and "${name}" are recursively
replaced with the value of the named parameter. The parameter name
must contain only characters from the set [a-zA-Z0-9_].
An undefined parameter value is replaced with the empty value.  </p>

<li> <p> The expressions "${name?value}" and "${name?{value}}" are
replaced with "value" when "$name" is non-empty. The parameter name
must contain only characters from the set [a-zA-Z0-9_]. These forms are
supported with Postfix versions &ge; 2.2 and &ge; 3.0, respectively.
</p>

<li> <p> The expressions "${name:value}" and "${name:{value}}" are
replaced with "value" when "$name" is empty. The parameter name must
contain only characters from the set [a-zA-Z0-9_]. These forms are
supported with Postfix versions &ge; 2.2 and &ge; 3.0, respectively.
</p>

<li> <p> The expression "${name?{value1}:{value2}}" is replaced
with "value1" when "$name" is non-empty, and with "value2" when
"$name" is empty.  The "{}" is required for "value1", optional for
"value2".  The parameter name must contain only characters from the
set [a-zA-Z0-9_].  This form is supported with Postfix versions
&ge; 3.0.  </p>

<li> <p> The first item inside "${...}" may be a relational expression
of the form: "{value3} == {value4}". Besides the "==" (equality)
operator Postfix supports "!=" (inequality), "&lt;", "&le;", "&ge;",
and "&gt;". The comparison is numerical when both operands are all
digits, otherwise the comparison is lexicographical. These forms
are supported with Postfix versions &ge; 3.0. </p>

<li> <p> Each "value" is subject to recursive named parameter and
relational expression evaluation, except where noted.  </p>

<li> <p> Whitespace before or after each "{value}" is ignored. </p>

<li> <p> Specify "$$" to produce a single "$" character. </p>

<li> <p> The legacy form "$(...)" is equivalent to the preferred
form "${...}". </p>

</ul>

<li> <p> When the same parameter is defined multiple times, only
the last instance is remembered. </p>

<li> <p> Otherwise, the order of main.cf parameter definitions does
not matter. </p>

</ul>

<p> The remainder of this document is a description of all Postfix
configuration parameters. Default values are shown after the
parameter name in parentheses, and can be looked up with the
"<b>postconf -d</b>" command. </p>

<p> Note: this is not an invitation to make changes to Postfix
configuration parameters. Unnecessary changes are likely to impair
the operation of the mail system.  </p>

<dl>
