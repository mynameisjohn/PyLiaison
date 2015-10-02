# a test script for PyLiaison

# PyLiaison is importable anywhere
from PyLiaison import *

# A simple function that invokes
# another function from PyLiaison
def SayHello():
	print 'Hello, friend'
	print testArgs(1,3)

# This will be exposed
# Note that you don't
# actually have to declare
# g_Foo beforehand, but
# it's probably a good habit
g_Foo = None

# Verify that we exposed foo
def checkFoo():
	if (g_Foo == None):
		print('Error: g_Foo object not exposed!')
	else:
		g_Foo.setVec([1.,2.,3.])

# This will be retrieved as a Vector3
g_Vec3 = [1., 0., 0.]

# You can also construct
# C++ objects on the fly
def FooTest(fPtr):
	f = Foo(fPtr)
	f.setFloat(1.0)
	print('We got a foo object at ' + str(f()))
	print('It\'s vector is ' + str(Foo.getVec()))
