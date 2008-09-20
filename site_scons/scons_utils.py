import SCons.Util
import os
import re
import platform

# Takes a name in the form 'foo/bar' or '/math/utils' and turns it into a valid global path for
# whatever operating system we're on
def TargetName(env, name):
  if name[0] == '/':
    full_path = os.path.join(env['PROJECT_ROOT'], reduce(os.path.join, name[1:].split('/')))
  else:
    full_path = os.path.join(os.getcwd(), reduce(os.path.join, name.split('/')))
  return full_path

# Takes a name in the form 'foo/bar' or '/math/utils' and turns it into a valid local path for
# whatever operating system we're on
def LocalTargetName(env, name):
  name = env.TargetName(name)
  return name[len(env['PROJECT_ROOT']):]

# The sub-packages included with this one are loaded and bundled together so that tests can
# include the one package necessary and nothing else.
def AppendToPackage(env, pkg, sub_packages):
  for package_name in sub_packages:
    package = env.LoadPackage(package_name)
    for component in ['objects', 'headers']:
      pkg[component] += package[component]
      pkg[component] = [x.get_abspath() for x in pkg[component]]
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
  if env['PACKAGE_STACK'][-1] != 'all' and env['PACKAGE_STACK'] != target_name:
    print 'Error loading packages.  Ignoring...'
    return []
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


# Loads the package named by package_path and returns the relevant data
# If the package has already been loaded once the data is already cached, and so isn't reloaded
def LoadPackage(env, package_path):
  if env['LOADED_PACKAGES_MAP'].has_key(env.TargetName(package_path)):
    return env['LOADED_PACKAGES_MAP'][env.TargetName(package_path)]
  package_directory, package_name = os.path.split(os.path.normpath(package_path))
  temp_env = env.Clone()
  temp_env.Append(PACKAGE_STACK = package_path)

  sconscript_file = os.path.join(temp_env['PROJECT_ROOT'], package_directory, 'SConscript')
  temp_env.SConscript(sconscript_file)

  return env['LOADED_PACKAGES_MAP'][env.TargetName(package_path)]



def RunTest(env, target, source):
  old_cwd = os.getcwd()
  for test in source:
    os.chdir(os.path.dirname(test.get_abspath()))
    exit_status = os.system(test.get_abspath())
    if exit_status != 0 and exit_status != 25:
      print 'Test %s exited with exit status %d.' % (test.get_abspath(), exit_status)
      env.Exit(1)
  os.chdir(old_cwd)

# Emits the name of a file that never gets created and doesn't already exist, so that it always gets
# 'built' and never conflicts with an existing target
def TestEmitter(target, source, env):
  return [str(x) + '.test' for x in target], source

def TestProgram(env, test_name, test_file, packages):
  if env.GetOption('buildtests') == None and env.GetOption('runtests') == None:
    return
  pkg = {
    'objects' : [],
    'frameworks' : [],
    'libs' : [],
    'headers' : [],
  }

  env.AppendToPackage(pkg, packages)

  test = env.Program(
      test_name,
      [test_file] + pkg['objects'],
      LIBS = ['gtest', 'gtest_main'] + pkg['libs'],
      FRAMEWORKS = pkg['frameworks'])

  # Must copy dylibs to the same directory as the test binaries.  Will also have to chdir to those
  # directories to run the binaries.
  # TODO(jwills): Maybe it would be better make the path to the dylib relative to the root directory
  # since that is where people will normally be?
  for lib in pkg['libs']:
    dylib_path = os.path.join(env['LIBPATH'][-1], 'lib' + lib + '.dylib')
    if os.path.exists(dylib_path):
      env.Command(
          os.path.join(os.path.dirname(str(test)), 'lib' + lib + '.dylib'),
          dylib_path,
          SCons.Defaults.Copy('$TARGET', '$SOURCE'))

  if env.GetOption('runtests') != None:
    env.Alias(env.LocalTargetName(test_name), env.RunTest(test))
  else:
    env.Alias(env.LocalTargetName(test_name), test)
  return test

def AppendOsParams(env):
  # Adding in useful tools
  env.AddMethod(TargetName, 'TargetName')
  env.AddMethod(LocalTargetName, 'LocalTargetName')
  env.AddMethod(BuildObjects, 'BuildObjects')
  env.AddMethod(LoadPackage, 'LoadPackage')
  env.AddMethod(AppendToPackage, 'AppendToPackage')

  run_test = env.Builder(action = RunTest, emitter = TestEmitter)
  env.Append(BUILDERS = {'RunTest' : run_test})
  env.AddMethod(TestProgram, 'Test')

  os_params = platform.uname()
  if os_params[0] not in ['Darwin', 'Windows']:
    print 'Operating system "' + os_params[0] + '" was unrecognized.'
    Exit(1)

  if os_params[0] == 'Darwin':
    import osx as operating_system
  if os_params[0] == 'Windows':
    import win32 as operating_system

  operating_system.AppendOsParams(env)

  return env

