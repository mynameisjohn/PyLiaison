# pylTestScript.py

def HelloWorld():
    print('Hello World from', __name__)

def narrow2wide(narrowStr):
    if not(isinstance(narrowStr, bytes)):
        raise RuntimeError('Error: Byte string input needed!')
    return narrowStr.decode('utf8')

def delimit(str, d):
    return str.split(d)