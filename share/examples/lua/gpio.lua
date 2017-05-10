-- $NetBSD: gpio.lua,v 1.4 2017/05/10 07:51:07 mbalmer Exp $

-- This example works only if all pins, starting from pin 0 up to the
-- number returned by gpio.info() are, readable.  It does _not_ work if
-- only part of the pins are configured.

local gpio = require 'gpio'

print(gpio._VERSION .. ' - ' .. gpio._DESCRIPTION)
print(gpio._COPYRIGHT)
print()

g = gpio.open('/dev/gpio0')

local npins = g:info()

print('gpio0 has ' .. npins .. ' pins.')

for n = 0, npins - 1 do
	print('pin ' .. n .. ': ' .. g:read(n))
end

local oldval = g:write(31, gpio.PIN_HIGH)
print('pin 31: ' .. oldval .. ' -> ' .. g:read(31))

oldval = g:toggle(31)
print('pin 31: ' .. oldval .. ' -> ' .. g:read(31))

g:write(31, gpio.PIN_LOW)

g:write(31, 5)

