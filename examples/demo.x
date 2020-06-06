# demo.x

str newline = "\n"


print "Create list which includes another list", newline

def list_with_two_dimensions()
    list outer, inner
    int i = 0, j

    while i != 3
        j = 0
        inner = []
        while j < 4
            inner.append((i + 1) * 10 + j)
            j += 1
        outer.append(inner)
        print "inner =", inner
        i = i + 1

    print "outer =", outer, "\n"

    print "Print list which includes another list:"

    i = 0
    do
        j = 0
        while j < outer[i].len()
            print -raw outer[i][j], " "
            j += 1
        i += 1
    while i < outer.len()

    print

list_with_two_dimensions()
print


print "Multiply two values (also works for strings and list):"

def multiply(x, y)
    print -raw x, " * ", y, " = "
    return x * y

float a = multiply(1.1, 2.1)
print a

int b = multiply(2, 3)
print b

str s = multiply("abc", 4)
print s

list t = multiply([1.2, "ab", 'c'], 2)
print t, "\n"


print "Add two values (numbers, strings and lists):"

def add(x, y)
    print -raw x, " + ", y, " = "
    return x + y

print add(1.1, 2.2)		# two floats
print add("abc", "def")	# two strings
print add([1,2], [3,4])	# two lists
print add("abc", 'd')	# string and char
print add("abc", 123)	# string and int
print add("abc", 3.14)	# string and float
print


print "Nested if " + "then else:"  # string concatenation via +

# Test if value is less then, equal if greater then zero
#
def test_for_zero(v)
    if v < 0
        print -raw "v < 0, v = ", v, "\n"
    else
        if v == 0
            print "v = 0"
        else
            print -raw "v > 0, v = ", v, "\n"

test_for_zero(-2)
test_for_zero(0)
test_for_zero(2)
print


print "while loop:"

def while_loop(v)
    while (v += 1) != 5
        print "v =", v

while_loop(0)
print


print "do while loop:"

def do_loop(v)
    do
        print "v =", v += 1
    while v < 5

do_loop(0)
print


print -raw "Cascaded assignment (a = b = c = d = 3 * 3): "

int c
int d

a = b = c = d = 3 * 3

if a == b and c == d and d == 3 * 3
    print a, "=" , b, "=", c, "=", d, newline


print -raw "Recursive function: "

def func(local)
    if local > 0
        func(local - 1)
    print -raw local, " "

func(9)
print newline


print -raw "Function which returns string f1: "

def f1()
    return "f1"

print f1(), newline


print "Function which returns a new list of i elements:"

def create_list(i)
    list l

    while i
        l.insert(0, i)  # insert i before element nr 0
        i -= 1

    return l

print "create_list(6)      = ", create_list(6)
print "create_list(6)[1]   = ", create_list(6)[1]
print "create_list(6)[-1]  = ", create_list(6)[-1]  # -1 is first element from end
print "create_list(6)[:2]  = ", create_list(6)[:2]
print "create_list(6)[2:]  = ", create_list(6)[2:]
print "create_list(6)[2:4] = ", create_list(6)[2:4]
print "create_list(6)[2:-2]= ", create_list(6)[2:-2], "\n"


# Create Fibonacci sequence for n elements, return as list
#
def fibonacci(n)
    list fib
    int f0 = 0, f1 = 1, fn, i

    while i < n
        if i <= 1
            fn = i
        else
            fn = f0 + f1
            f0 = f1, f1 = fn
        fib.append(fn)
        i += 1

    return fib

int n = 10
print "Fibonacci sequence for", n, "elements:", fibonacci(n), "\n"


# Direct use of function return value
#
def power(g, e)
    float x, r
    int negative = 0

    if e == 0
        return 1

    if e < 0
        negative = 1
        e = 0 - e

    x = g
    while  e > 1
        x *= g
        e -= 1

    if negative
        x = 1 / x

    return x

print -raw "Direct use of function return value: "

print "2^3 + 1.1 =", power(2, 3) + 1.1, newline


print -raw "Scientific notation of float constants: 2.1E2 = "
print 2.1E2, newline


print "This script will now exit with a return value of", 1 + 2 * 3
return 1 + 2 * 3
