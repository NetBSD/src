        .option nopic
	.text
	.align  1
	.globl  func1
	.type   func1, @function
func1:
	jal	func2
	jr      ra
	.size   func1, .-func1
