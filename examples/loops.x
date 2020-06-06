# loops.x

int i

i = 100 * 1000

while (i -= 1)
	pass


i = 100 * 1000

do
    pass
while (i -= 1)


def loop_while(i)
    while i
		if i % 1000 == 0
			print i
		i -= 1

loop_while(10 * 1000)


def loop_do_while(i)
    do
		if i % 1000 == 0
			print i
    while i -= 1

loop_do_while(10 * 1000)


print "done"
