# -*- mode: python; -*-

import os

Env = Environment()
import os
Env['ENV']['TERM'] = os.environ['TERM']
Env.Replace(CXX="clang++", CXXFLAGS="-g -std=c++14 -Wall")
Env.Program(('test.cpp', "vec-io.cpp"))
