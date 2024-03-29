# EXIN-AST - An Interpreter using an Abstract Syntax Tree

EXIN-AST is an exercise in defining a programming language and writing an interpreter for it. Both are fairly simple to make it easy to understand how the interpreter code is structured and to be able to add new features. It is a further development of [EXIN](https://github.com/erikdelange/EXIN-The-Experimental-Interpreter), but now with clear separation between parsing and execution by using an Abstract Syntax Tree (AST), hence the repository's name EXIN-AST. The language contains traces of (as far as I know) Basic, C and Python. This is an example of EXIN-AST code:
```
# Reverse string s
#
def reverse(s)
    str new
    int index = s.len()

    while index
        index -= 1
        new += s[index]

    return new

# First a short demo
#
str s2, s1 = "abracadabra"

s2 = reverse(s1)

print -raw s1, " reversed is ", s2, 2 * "\n"

# Now let the user enter strings
#
do
    input "Enter a string to reverse (empty line to end): " s1
    if s1 == ""
        break
    print s1, "reversed is", reverse(s1)
while 1
```
The interpreter is written in C (version C11). The details of the language and the interpreter are explained in the documents listed below. EXIN is designed for fun and education (at least mine :) and you can do with it whatever you like under the GPL-3.0-or-later license.

- [EXIN language reference](EXIN%20language%20reference.md)
- [EXIN software architecture](EXIN%20software%20architecture.md)
- [EXIN syntax diagram](EXIN%20syntax%20diagram.pdf)
- [Testing EXIN](https://github.com/erikdelange/EXIN-Test-Suite-Management)
