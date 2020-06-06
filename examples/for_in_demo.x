# for_in_demo.x

list l = [0, 1, 2, 3]

for i in l
	print "i =", i
print

for j in l
	if j == 1
		continue  # skip 1
	print "j =", j
print

for k in l
	if k == 2
		break  # stop before 2
	print "k =", k
print

for c in "abcdef"
	print -raw c, " "
print

for i in "abcdef"[2:]
	print -raw i, " "
print "\n"

for element in [1, 3.14, "abc", 'd', ["allo"]]
	print element, "=", type(element)
