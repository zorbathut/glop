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

global_env = Environment()
global_env.Replace(GLOBAL_ROOT = os.getcwd() + os.sep)
global_env = scons_utils.AppendOsParams(global_env)

# Compile all projects.  The only requirement here is that Glop be first in the list, since the
# other projects reference it.
for project in ['Glop', 'Tests']: #, 'CloneChu', 'Maze']:# 'FactoryFun']:
  variant = os.path.join(os.getcwd(), 'build-%s-%s' % (GetOption('compile-mode'), project) + os.sep)
  env = global_env.Clone()
  Export('env')
  env.Replace(PROJECT_ROOT = os.path.join(os.getcwd(), project + os.sep))
  env.Replace(BUILD_ROOT = variant)
  env.Replace(PACKAGE_STACK = ['all'])
  env.Replace(TESTS = [])
  env.Replace(LOADED_PACKAGES_MAP = {})
  env.Append(CPPPATH = [env['PROJECT_ROOT']])
  env.Append(CPPPATH = [env['BUILD_ROOT']])
  #if project != 'Glop':
    #env.Append(FRAMEWORKPATH = [os.path.dirname(global_env['Glop'])])
  global_env[project] = \
      env.SConscript(
          os.path.join(project, 'SConscript'),
          variant_dir=variant,
          duplicate=0)
