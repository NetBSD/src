<!doctype html public "-//W3C//DTD HTML 4.01 Transitional//EN"
        "http://www.w3.org/TR/html4/loose.dtd">

<html>

<head>

<title>Postfix Configuration Parameters </title>

<meta http-equiv="Content-Type" content="text/html; charset=us-ascii">

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

<li> <p> The expressions "$name", "${name}" or "$(name)" are
recursively replaced by the value of the named parameter. </p>

<li> <p> The expression "${name?value}" expands to "value" when
"$name" is non-empty. </p>

<li> <p> The expression "${name:value}" expands to "value" when
"$name" is empty. </p>

</ul>

<li> <p> When the same parameter is defined multiple times, only
the last instance is remembered. </p>

<li> <p> Otherwise, the order of main.cf parameter definitions does
not matter. </p>

</ul>

<p> The remainder of this document is a description of all Postfix
configuration parameters. Default values are shown after the
parameter name in parentheses, and can be looked up with the
<b>postconf -d</b> command. </p>

<p> Note: this is not an invitation to make changes to Postfix
configuration parameters. Unnecessary changes are likely to impair
the operation of the mail system.  </p>

<dl>
