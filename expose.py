# a test script for PyLiaison

# PyLiaison is importable anywhere
from PyLiaison import *

# A simple function that invokes
# another function from PyLiaison
def SayHello():
	print('Hello, friend')
	print(testArgs(1,3))

# This will be exposed
# Note that you don't
# actually have to declare
# g_Foo beforehand, but
# it's probably a good habit
c_Foo = None

# Verify that we exposed foo
def checkFoo():
	if (c_Foo == None):
		print('Error: g_Foo object not exposed!')
		return False
	else:
		c_Foo.setVec([1.,2.,3.])
		print(c_Foo.getVec())
		return True

# This will be modified and
# then retrieved as a Vector3
# in the host code
g_Vec3 = None

def SetVec(x, y, z):
	g_Vec3 = [x, y, z]
	print(g_Vec3)

# You can also construct
# C++ objects on the fly
def FooTest(fPtr):
	f = Foo(fPtr)
	f.setVec(g_Vec3)
	print('We got a foo object at ' + str(f()))
	print('Its vector is ' + str(f.getVec()))
