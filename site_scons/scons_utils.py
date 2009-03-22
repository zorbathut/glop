import SCons.Util
import os
import sys
import subprocess
import re
import platform

# Takes a name in the form 'foo/bar' or '/math/utils' and turns it into a valid global path for
# whatever operating system we're on
def TargetName(env, name):
  if name[0] == '/':
    full_path = os.path.join(env['PROJECT_ROOT'], reduce(os.path.join, name[1:].split('/')))
  else:
    full_path = os.path.join(os.getcwd(), reduce(os.path.join, name.split('/')))
    full_path = full_path.replace(env['BUILD_ROOT'], env['PROJECT_ROOT'])
  return full_path

# Takes a name in the form 'foo/bar' or '/math/utils' and turns it into a valid local path for
# whatever operating system we're on
def LocalTargetName(env, name):
  name = env.TargetName(name)
  return name[len(env['PROJECT_ROOT']):]

# Takes a name in the form 'foo/bar' or '/math/utils' and returns the path to the SConscript file
# that contains this target
def TargetSConscriptPath(env, name):
  name = env.TargetName(name)
  sconscript_path = os.path.join(os.path.dirname(name), 'SConscript')
  return sconscript_path

# Takes a name in the form 'foo/bar' or '/math/utils' and returns the varaint directory that the
# target output should be placed in
def TargetVariantDir(env, name):
  return os.path.dirname(env.TargetSConscriptPath(name).replace(env['PROJECT_ROOT'], env['BUILD_ROOT']))


# The sub-packages included with this one are loaded and bundled together so that tests can
# include the one package necessary and nothing else.
def AppendToPackage(env, pkg, sub_packages):
  for package_name in sub_packages:
    package = env.LoadPackage(package_name)
    for component in ['objects', 'headers']:
      pkg[component] += package[component]
      pkg[component] = [x.get_abspath() for x in pkg[component]]
      pkg[component] = SCons.Util.flatten(pkg[component])
      pkg[component] = SCons.Util.unique(pkg[component])
      pkg[component].sort()
      for i in range(len(pkg[component])):
        if os.path.isdir(pkg[component][i]):
          pkg[component][i] = env.Dir(pkg[component][i])
        else:
          pkg[component][i] = env.File(pkg[component][i])
    for component in ['libs', 'frameworks']:
      pkg[component] += package[component]
      pkg[component] = SCons.Util.unique(pkg[component])
      pkg[component].sort()
      

def BuildObjects(env, target_name, source, packages = [], frameworks = [], libs = [], headers = []):
  if env['PACKAGE_STACK'][-1] != 'all' and env['PACKAGE_STACK'][-1] != env.TargetName(target_name):
    return []

  for f in [x for x in frameworks if 'Glop' in x]:
    env.AppendUnique(FRAMEWORKPATH = [re.match('#?(.*)/(.*)\.framework.*', f).group(1)])
    env.AppendUnique(FRAMEWORKS = [re.match('(.*)/(.*)\.framework.*', f).group(2)])

  objects = env.Object(source)
  env.Alias(env.LocalTargetName(target_name), objects)
  pkg = {
    'objects' : objects,
    'frameworks' : frameworks,
    'libs' : libs,
    'headers' : [env.File(header) for header in headers],
  }

  env.AppendToPackage(pkg, packages)
  env['LOADED_PACKAGES_MAP'][env.TargetName(target_name)] = pkg
  return objects

# Calls the proto builder and creates a package out of the resulting files that can be included as
# a package by other packages, just like BuildObjects
def BuildProtos(env, target_name, source):
  if env['PACKAGE_STACK'][-1] != 'all' and env['PACKAGE_STACK'][-1] != env.TargetName(target_name):
    return []
  if not SCons.Util.is_List(source):
    source = [source]
  protos = []
  for s in source:
    protos += env.Proto(s)
  objects = env.Object([proto for proto in protos if str(proto).endswith('.pb.cc')])
  env.Alias(env.LocalTargetName(target_name), objects)
  pkg = {
    'objects' : objects,
    'frameworks' : [],
    'libs' : ['protobuf'],
    'headers' : [],
  }
  env['LOADED_PACKAGES_MAP'][env.TargetName(target_name)] = pkg
  return objects


# Loads the package named by package_path and returns the relevant data
# If the package has already been loaded once the data is already cached, and so isn't reloaded
def LoadPackage(env, package_path):
  if env['LOADED_PACKAGES_MAP'].has_key(env.TargetName(package_path)):
    return env['LOADED_PACKAGES_MAP'][env.TargetName(package_path)]
  package_directory, package_name = os.path.split(os.path.normpath(package_path))
  env.Append(PACKAGE_STACK = [env.TargetName(package_path)])

  sconscript_file    = env.TargetSConscriptPath(package_path)
  sconscript_variant = env.TargetVariantDir(package_path)
  env.SConscript(sconscript_file, variant_dir=sconscript_variant)
  loaded_package = env['LOADED_PACKAGES_MAP'][env.TargetName(package_path)]

  env['PACKAGE_STACK'] = env['PACKAGE_STACK'][:-1]
  return loaded_package


def RunTest(env, target, source):
  for test in source:
    exit_status = os.system(test.get_abspath())
    if exit_status != 0 and exit_status != 25:
      print 'Test %s exited with exit status %d.' % (test.get_abspath(), exit_status)
      env.Exit(1)

# Emits the name of a file that never gets created and doesn't already exist, so that it always gets
# 'built' and never conflicts with an existing target
def TestEmitter(target, source, env):
  return [str(x) + '.test' for x in target], source

def TestProgram(env, target_name, source, packages):
  if env.GetOption('buildtests') == None and env.GetOption('runtests') == None:
    return
  if target_name in env['TESTS']:
    return
  env['TESTS'].append(target_name)
  pkg = {
    'objects' : [],
    'frameworks' : [],
    'libs' : [],
    'headers' : [],
  }

  env.AppendToPackage(pkg, packages)
  # TODO(jwills): this is removing a global path and replacing it with a framework name.  This
  # shouldn't ever even have this global path as a framework
  for fw in [x for x in pkg['frameworks'] if 'Glop' in x]:
    pkg['frameworks'].remove(fw)
    pkg['frameworks'].append('Glop_test')

  test = env.Program(
      target_name,
      [source] + pkg['objects'],
      LIBS = ['gtest', 'gtest_main'] + pkg['libs'],
      FRAMEWORKS = pkg['frameworks'])
  assert len(test) == 1
  test = test[0]

  # Assume that any dylibs in our os lib path need to be copied to a location that the executable
  # will be able to find at runtime.
  dylib_names = ['lib' + lib + '.dylib' for lib in pkg['libs']]
  dylibs = [x for x in dylib_names if os.path.exists(os.path.join(env['LIBPATH'][-1], x))]
  test = \
      env.InstallNameTool(
          env.File(str(test) + '_installed'),
          test,
          env['LIBPATH'][-1],
          './',
          dylibs)

  if env.GetOption('runtests') != None:
    env.Alias(env.LocalTargetName(target_name), env.RunTest(test))
  else:
    env.Alias(env.LocalTargetName(target_name), test)
  return test

def ProtoEmitter(target, source, env):
  target = []
  for s in source:
    target.append(s.get_abspath()[:-6] + '.pb.cc')
    target.append(s.get_abspath()[:-6] + '.pb.h')
  return target, source

# TODO(jwills): Test this out on windows
def ProtoBuilder(target, source, env):
  assert len(target) == 2
  assert len(source) == 1
  target_path = target[0].get_abspath()
  target_directory = os.path.dirname(target_path)
  source_path = source[0].get_abspath()
  source_directory = os.path.dirname(source_path)
  cmd = 'protoc --cpp_out=%s --proto_path=%s %s' % \
      (target_directory, source_directory, source[0].get_abspath())
  try:
    os.makedirs('.build')
  except:
    pass
  temp_file = open('.build/tmp','w')
  cmd_array = [
    'protoc',
    '--cpp_out=%s' % target_directory,
    '--proto_path=%s' % source_directory,
    source[0].get_abspath(),
  ]
  p = subprocess.Popen(
    cmd_array,
    stdout = subprocess.PIPE,
    stderr = temp_file,
  )
  p.wait()
  temp_file.close()
  temp_file = open('.build/tmp','r')
  errors = temp_file.read()
  errors = errors.split('\n')
  errors = [os.path.join(os.path.dirname(str(source[0])), error) for error in errors]
  for error in errors:
    sys.stderr.write(error + '\n')
#  sys.stderr.write('\n'.join(errors))
#  sys.stderr.write('\n')

def AppendOsParams(env):
  # Adding in useful tools
  env.AddMethod(TargetName, 'TargetName')
  env.AddMethod(LocalTargetName, 'LocalTargetName')
  env.AddMethod(TargetSConscriptPath, 'TargetSConscriptPath')
  env.AddMethod(TargetVariantDir, 'TargetVariantDir')
  env.AddMethod(BuildObjects, 'BuildObjects')
  env.AddMethod(BuildProtos, 'BuildProtos')
  env.AddMethod(LoadPackage, 'LoadPackage')
  env.AddMethod(AppendToPackage, 'AppendToPackage')

  run_test = env.Builder(action = RunTest, emitter = TestEmitter)
  env.Append(BUILDERS = {'RunTest' : run_test})
  env.AddMethod(TestProgram, 'Test')

  proto_builder = env.Builder(action = ProtoBuilder, emitter = ProtoEmitter)
  env.Append(BUILDERS = {'Proto' : proto_builder})


  os_params = platform.uname()

  if os_params[0] == 'Darwin':
    import osx as operating_system
  elif os_params[0] == 'Windows':
    import win32 as operating_system
  elif os_params[0].startswith('CYGWIN'):
    import cygwin as operating_system
  elif os_params[0].startswith('Linux'):
    import linux as operating_system
  else:
    print 'Operating system "' + os_params[0] + '" was unrecognized.'
    Exit(1)

  operating_system.AppendOsParams(env)

  return env

