# module_1.x
#
# example of import of a module which itself also imports modules

print "** now in module_1.x **"

import "module_2.x"

print "back in module_1.x after import of module_2.x"

fm2(10)
fm2(3.14)
fm2(name)
fm3()
fm4()

# import "module_2.x"  # has not effect

print "** leaving module_1.x **"
