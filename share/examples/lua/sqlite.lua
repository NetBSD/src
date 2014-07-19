-- $NetBSD: sqlite.lua,v 1.2 2014/07/19 18:38:34 lneto Exp $

local sqlite = require 'sqlite'

print(sqlite._VERSION .. ' - ' .. sqlite._DESCRIPTION)
print(sqlite._COPYRIGHT)
print()

print('initialize sqlite')
sqlite.initialize()

print('this is sqlite ' .. sqlite.libversion() .. ' (' ..
    sqlite.libversion_number() .. ')')
print('sourceid ' .. sqlite.sourceid())

db, state = sqlite.open('/tmp/db.sqlite',
    sqlite.OPEN_READWRITE + sqlite.OPEN_CREATE)

if state ~= sqlite.OK then
	print('db open failed')
else
	err = db:exec('create table test (name varchar(32))')

	if err ~= sqlite.OK then
		print('table creation failed')
		print('error code ' .. db:errcode() .. ' msg ' .. db:errmsg())
	end

	db:exec("insert into test values('Balmer')")
	print('last command changed ' .. db:changes() .. ' rows')

	stmt = db:prepare("insert into test values(:name)")

	print('statement has ' .. stmt:bind_parameter_count() .. ' parameters')
	print('param 1 name: ' .. stmt:bind_parameter_name(1))
	print('param name is at index ' .. stmt:bind_parameter_index('name'))

	stmt:bind(1, 'Hardmeier')
	stmt:step()
	stmt:reset()
	stmt:bind(1, 'Keller')
	stmt:step()
	stmt:finalize()

	s2 = db:prepare('select name from test')

	while s2:step() == sqlite.ROW do
		print('name = ' .. s2:column(1))
	end
	s2:finalize()

	stmt = db:prepare('drop table testx')
	stmt:step()
	stmt:finalize()
	db:close()
end

print('shutdown sqlite')
sqlite.shutdown()

