systm = require 'systm'

systm.print("hello, kernel world!\n")

function onClose()
	systm.print('I am about to be closed\n')
end

