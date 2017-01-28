# pylTestScript.py

def HelloWorld():
    print('Hello World from', __name__)

def NarrowToWide(narrowStr):
    if not(isinstance(narrowStr, bytes)):
        raise RuntimeError('Error: Byte string input needed!')
    return narrowStr.decode('utf8')

class Foo:
    def __init__(self):
        self.x = 0
        self.y = 0
    
    def SetX(self, x):
        self.x = x

    def SetY(self, y):
        self.y = y

    def Print(self):
        print('My values are', self.x, self.y)

fooInst = Foo()