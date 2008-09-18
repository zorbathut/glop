import re
import os
import scons_utils

AddOption(
    '--buildtests',
    dest='buildtests',
    nargs=0,
    help='whether or not to build the tests')

AddOption(
    '--runtests',
    dest='runtests',
    nargs=0,
    help='whether or not to run the tests')

AddOption(
    '--compile-mode',
    dest='compile-mode',
    type='choice',
    choices=['dbg', 'opt'],
    nargs=1,
    help='dbg = debug mode, opt = release mode',
    default='dbg',
    metavar='COMPILE_MODE')

global_env = scons_utils.AppendOsParams(Environment())
global_env.Replace(GLOBAL_ROOT = os.getcwd() + os.sep)
Export('global_env')

# Compile all projects.  The only requirement here is that Glop be first in the list, since the
# other projects reference it.
for project in ['Glop', 'Tests']:# 'FactoryFun']:
  global_env[project] = \
      global_env.SConscript(
          os.path.join(project, 'SConscript'),
          variant_dir='build-%s-%s' % (GetOption('compile-mode'), project),
          duplicate=1)
