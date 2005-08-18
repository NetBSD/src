.TH POSTCONF 5 
.SH NAME
postconf
\-
Postfix configuration parameters
.SH SYNOPSIS
.na
.nf
\fBpostconf\fR \fIparameter\fR ...

\fBpostconf -e\fR "\fIparameter=value\fR" ...
.SH DESCRIPTION
.ad
.fi
The Postfix main.cf configuration file specifies a small subset
of all the parameters that control the operation of the Postfix
mail system. Parameters not specified in main.cf are left at their
default values.
.PP
The general format of the main.cf file is as follows:
.IP \(bu
Each logical line has the form "parameter = value".
Whitespace around the "=" is ignored, as is whitespace at the
end of a logical line.
.IP \(bu
Empty lines and whitespace-only lines are ignored, as are lines
whose first non-whitespace character is a `#'.
.IP \(bu
A logical line starts with non-whitespace text. A line that starts
with whitespace continues a logical line.
.IP \(bu
A parameter value may refer to other parameters.
.RS
.IP \(bu 
The expressions "$name", "${name}" or "$(name)" are
recursively replaced by the value of the named parameter.
.IP \(bu
The expression "${name?value}" expands to "value" when
"$name" is non-empty. This form is supported with Postfix
version 2.2 and later.
.IP \(bu
The expression "${name:value}" expands to "value" when
"$name" is empty. This form is supported with Postfix
version 2.2 and later.
.RE
.IP \(bu
When the same parameter is defined multiple times, only the last
instance is remembered.
.IP \(bu
Otherwise, the order of main.cf parameter definitions does not matter.
.PP
The remainder of this document is a description of all Postfix
configuration parameters. Default values are shown after the 
parameter name in parentheses, and can be looked up with the
"\fBpostconf -d\fR" command. 
.PP
Note: this is not an invitation to make changes to Postfix
configuration parameters. Unnecessary changes can impair the
operation of the mail system.
