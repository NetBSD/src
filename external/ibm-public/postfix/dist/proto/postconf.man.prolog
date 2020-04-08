.TH POSTCONF 5 
.SH NAME
postconf
\-
Postfix configuration parameters
.SH SYNOPSIS
.na
.nf
\fBpostconf\fR \fIparameter\fR ...

\fBpostconf \-e\fR "\fIparameter=value\fR" ...
.SH DESCRIPTION
.ad
.fi
The Postfix main.cf configuration file specifies parameters that
control the operation of the Postfix mail system. Typically the
file contains only a small subset of all parameters; parameters
not specified are left at their default values.
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
The expressions "$name" and "${name}" are recursively replaced with
the value of the named parameter. The parameter name must contain
only characters from the set [a-zA-Z0-9_]. An undefined parameter
value is replaced with the empty value.
.IP \(bu
The expressions "${name?value}" and "${name?{value}}" are replaced
with "value" when "$name" is non-empty. The parameter name must
contain only characters from the set [a-zA-Z0-9_]. These forms are
supported with Postfix versions >= 2.2 and >= 3.0, respectively.
.IP \(bu
The expressions "${name:value}" and "${name:{value}}" are replaced
with "value" when "$name" is empty. The parameter name must contain
only characters from the set [a-zA-Z0-9_]. These forms are supported
with Postfix versions >= 2.2 and >= 3.0, respectively.
.IP \(bu
The expression "${name?{value1}:{value2}}" is replaced with "value1"
when "$name" is non-empty, and with "value2" when "$name" is empty.
The "{}" is required for "value1", optional for "value2". The
parameter name must contain only characters from the set [a-zA-Z0-9_].
This form is supported with Postfix versions >= 3.0.
.IP \(bu
The first item inside "${...}" may be a relational expression of the
form: "{value3} == {value4}". Besides the "==" (equality) operator
Postfix supports "!=" (inequality), "<", "<=", ">=", and ">". The
comparison is numerical when both operands are all digits, otherwise
the comparison is lexicographical. These forms are supported with
Postfix versions >= 3.0.
.IP \(bu
Each "value" is subject to recursive named parameter and relational
expression evaluation, except where noted.
.IP \(bu
Whitespace before or after each "{value}" is ignored.
.IP \(bu
Specify "$$" to produce a single "$" character.
.IP \(bu
The legacy form "$(...)" is equivalent to the preferred form "${...}".
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
"\fBpostconf \-d\fR" command. 
.PP
Note: this is not an invitation to make changes to Postfix
configuration parameters. Unnecessary changes can impair the
operation of the mail system.
