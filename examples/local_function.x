# local_function.x

int n = 1

def f()
	int n = 3

	# function g() is declared within an other function

	def g(n)
		n += 6
		return n

	print -raw n
	print -raw g(n)
	print -raw n  # n is unchanged (= 3)


f()

print -raw n  # n is unchanged (= 1)
