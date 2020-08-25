-- $NetBSD: printenv.lua,v 1.4 2020/08/25 20:02:33 leot Exp $

-- this small Lua script demonstrates the use of Lua in (bozo)httpd
-- it will simply output the "environment"

-- Keep in mind that bozohttpd forks for each request when started in
-- daemon mode, you can set global variables here, but they will have
-- the same value on each invocation.  You can not keep state between
-- two calls.

-- You can test this example by running the following command:
-- /usr/libexec/httpd -b -f -I 8080 -L test printenv.lua .
-- and then navigate to: http://127.0.0.1:8080/test/printenv

local httpd = require 'httpd'

function printenv(env, headers, query)

	-- we get the "environment" in the env table, the values are more
	-- or less the same as the variable for a CGI program

	-- output headers using httpd.write()
	-- httpd.write() will not append newlines
	httpd.write("HTTP/1.1 200 Ok\r\n")
	httpd.write("Content-Type: text/html\r\n\r\n")

	-- output html using httpd.print()
	-- you can also use print() and io.write() but they will not work with SSL
	httpd.print([[
		<html>
			<head>
				<title>Bozotic Lua Environment</title>
			</head>
			<body>
				<h1>Bozotic Lua Environment</h1>
	]])

	httpd.print('module version: ' .. httpd._VERSION .. '<br>')

	httpd.print('<h2>Server Environment</h2>')
	-- print the list of "environment" variables
	for k, v in pairs(env) do
		httpd.print(k .. '=' .. v .. '<br/>')
	end

	httpd.print('<h2>Request Headers</h2>')
	for k, v in pairs(headers) do
		httpd.print(k .. '=' .. v .. '<br/>')
	end

	if query ~= nil then
		httpd.print('<h2>Query Variables</h2>')
		for k, v in pairs(query) do
			httpd.print(k .. '=' .. v .. '<br/>')
		end
	end

	httpd.print('<h2>Form Test</h2>')

	httpd.print([[
	<form method="POST" action="form?sender=me">
	<input type="text" name="a_value">
	<input type="submit">
	</form>
	]])
	-- output a footer
	httpd.print([[
		</body>
	</html>
	]])
end

function form(env, header, query)

	httpd.write("HTTP/1.1 200 Ok\r\n")
	httpd.write("Content-Type: text/html\r\n\r\n")

	if query ~= nil then
		httpd.print('<h2>Form Variables</h2>')

		if env.CONTENT_TYPE ~= nil then
			httpd.print('Content-type: ' .. env.CONTENT_TYPE .. '<br>')
		end

		for k, v in pairs(query) do
			httpd.print(k .. '=' .. v .. '<br/>')
		end
	else
		httpd.print('No values')
	end
end

-- register this handler for http://<hostname>/<prefix>/printenv
httpd.register_handler('printenv', printenv)
httpd.register_handler('form', form)
