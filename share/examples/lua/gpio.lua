-- $NetBSD: gpio.lua,v 1.1 2011/10/15 12:58:43 mbalmer Exp $

require 'gpio'

print(gpio._VERSION .. ' - ' .. gpio._DESCRIPTION)
print(gpio._COPYRIGHT)
print()

g = gpio.open('/dev/gpio0')

local npins = g:info()

print('gpio0 has ' .. npins .. ' pins.')

for n = 1, npins do
	print('pin ' .. n .. ': ' .. g:read(n))
end

local oldval = g:write(32, gpio.PIN_HIGH)
print('pin 32: ' .. oldval .. ' -> ' .. g:read(32))

oldval = g:toggle(32)
print('pin 32: ' .. oldval .. ' -> ' .. g:read(32))

g:pulse(32, 1, 50)
g:write(1, gpio.PIN_LOW)

g:write(32, gpio.PIN_LOW)

g:write(32, 5)

