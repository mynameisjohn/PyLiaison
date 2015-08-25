PyLiaison

A mix of pywrapper and EcsPython. pywrapper made it very easy to return data from python, and Ecs Python had a good system of exposing C++ functionality. I took the overall pattern of EcsPython, replaced the macros with C++11 variadic templates, and this is the result.

It's very much a work in progress, but when it's done it should be able to

  1) Expose C++ functions, classes, and class member functions to Python
  
      This is done by creating a custom module (currently called "spam", I believe) defined by a buffer
      of PyMethodDef objects referencing the functions you'd like to expose. Class definitions are dynamically
      generated scripts (I'm not sure if you can declare classes in a module), and any class member functions
      simply call into functions defined in that module.
      All classes are defined to have the () operator return a PyCObject, which provides a void * back to the 
      original object (which implies exposed object instances cannot move in memory)
  
      
  2) Return Python data to C++
  
      This is where pywrapper shines, Python data (including lists, tuples, and dicts) can be converted to their
      C++ C Standard Lib counterparts. I had to add some functions to deal with object pointers and floats, and I
      I think more will be needed down the road, but a lot of this code is from pywrapper
