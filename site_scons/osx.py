import SCons.Util
import SCons.Defaults
import os
import re
import shutil

def ListAllChildren(path):
  try:
    os.readlink(path)
    return [path]
  except OSError, e:
    pass
  try:
    kids = os.listdir(path)
  except OSError, e:
    kids = []
  if len(kids) == 0:
    return [path]
  ret = []
  for kid in kids:
    ret += ListAllChildren(os.path.join(path, kid))
  return ret

# WARNING!!!  MAJOR HACK ALERT!!!
def SymlinkBuilder(env, target, source):
  assert len(target) == 1 and len(source) == 1
  target = str(target[0])
  source = str(source[0])[len(os.path.dirname(target)) + 1:]
  try:
    os.makedirs(os.path.dirname(target))
  except OSError, e:
    pass
  try:
    os.remove(target)
  except OSError, e:
    pass
  try:
    os.symlink(source, target)
  except OSError, e:
    pass

def CopyDirectory(env, target, source):
  base_dir = os.path.join(env['GLOBAL_ROOT'], source)
  source = ListAllChildren(base_dir)
  for s in [str(x) for x in source]:
    source_path = s[len(base_dir) + 1:]
    if os.path.islink(s):
      target_dir = env.Dir(os.path.join(target, source_path))
      source_dir = env.Dir(os.path.join(os.path.dirname(str(target_dir)), os.readlink(s)))
      env.Symlink(target_dir, source_dir)
    elif not os.path.isdir(s):
      env.Command(
          os.path.join(target, source_path),
          s,
          SCons.Defaults.Copy('$TARGET', '$SOURCE'))
  return [env.Dir(target)]

# Copies a binary source to target, and for every library in libs, changes the binary's expected
# path for that library from source_lib_path to target_lib_path.
def InstallNameTool(env, target, source, target_lib_path, source_lib_path, libs):
  if SCons.Util.is_List(target):
    assert len(target) == 1
    target = target[0]
  if SCons.Util.is_List(source):
    assert len(source) == 1
    source = source[0]
  cmd = [
    'install_name_tool -change %s %s %s' % (
      source_lib_path + name,
      target_lib_path + name,
      target.get_abspath()
    )
    for name in libs
  ]
  for lib in libs:
    env.Command(
        os.path.join(os.path.dirname(target.get_abspath()), lib),
        os.path.join('OSX', 'lib', lib),
        SCons.Defaults.Copy('$TARGET', '$SOURCE'))
  env.Command(target, source, [SCons.Defaults.Copy('$TARGET', '$SOURCE')] + cmd)
  return [target]

# Note: This is obviously hacked to work for a framework named Glop, and nothing else
def CompileFramework(env, objs, headers, libs, framework_structure):
  framework_name = 'Glop'
  framework_env = env.Clone()
  framework_env.Append(LINKFLAGS =
      '-dynamiclib ' +
      '-arch i386 ' +
      '-install_name @executable_path/../Frameworks/' + framework_name +
      '.framework/Versions/A/' + framework_name + ' ' +
      '-Wl,-single_module ' +
      '-compatibility_version 1 ' +
      '-current_version 1 ' +
      '-mmacosx-version-min=10.5 ' +
      '-headerpad_max_install_names'
  )
  framework_env.Append(LIBS = libs)
  Glop = framework_env.Program(framework_name, objs)
  assert len(Glop) == 1
  Glop = Glop[0]
  framework_path = Glop.get_abspath() + '.framework'
  binary = os.path.join(framework_path, 'Versions', 'A', 'Glop_raw')

  # Assume that any dylibs in our os lib path need to be copied to a location that the executable
  # will be able to find at runtime.
  dylib_names = ['lib' + lib + '.dylib' for lib in libs]
  dylibs = [x for x in dylib_names if os.path.exists(os.path.join(env['LIBPATH'][-1], x))]
  framework_binary = \
      framework_env.InstallNameTool(
          env.File(os.path.join(framework_path, 'Versions', 'A', 'Glop')),
          Glop,
          '@executable_path/../Frameworks/Glop.Framework/Versions/A/',
          './',
          dylibs)
  # TODO(jwills): checking if the path begins with './' might not always work?

  framework_directory = env.CopyDirectory(framework_path, framework_structure)
  env.Symlink(
      os.path.join(str(framework_directory[0]), os.path.basename(str(framework_binary[0]))),
      str(framework_binary[0]))

  headers_directory = os.path.join(framework_path, 'Versions', 'A', 'Headers')
  for header in headers:
    header_directory = os.path.join(framework_path, 'Versions', 'A', 'Headers')
    header_path = os.path.join(header_directory, header.get_abspath()[len(env['BUILD_ROOT']):])
    env.Command(header_path, header, SCons.Defaults.Copy('$TARGET', '$SOURCE'))

  return framework_path
#  return framework_env.OrganizeFramework(Glop)
  

def Application(env, target, source, resources = [], frameworks = [], packages = []):
  app_env = env.Clone()
  target_directory = target + '.app/'

  if not SCons.Util.is_List(source):
    source = [source]

  if not SCons.Util.is_List(frameworks):
    frameworks = [frameworks]

  for s in source:
    for f in frameworks:
      env.Depends(s, f)

  for framework in frameworks:
    framework_name = re.findall('[^/]+\.framework', framework)[0]
    target_framework = target_directory + 'Contents/Frameworks/' + framework_name
    # Execute(Delete), Command(Copy) is a cludge to force SCons to copy a directory
    env.Execute(SCons.Defaults.Delete(target_framework))
    env.Command(target_framework, framework, SCons.Defaults.Copy("$TARGET", "$SOURCE"))

  if not SCons.Util.is_List(resources):
    resources = [resources]
  for resource in resources:
    resource_name = os.path.basename(os.path.normpath(resource))
    target_resource = target_directory + 'Contents/Resources/' + resource_name
    # Execute(Delete), Command(Copy) is a cludge to force SCons to copy a directory
    if os.path.isdir(resource):
      env.Execute(SCons.Defaults.Delete(target_resource))
    env.Command(target_resource, resource, SCons.Defaults.Copy("$TARGET", "$SOURCE"))

  app_env.Append(LINKFLAGS = ' -arch i386 ')
  for framework in frameworks:
    app_env.AppendUnique(FRAMEWORKPATH = [re.match('#?(.*)/(.*)\.framework.*', framework).group(1)])
    app_env.AppendUnique(FRAMEWORKS = [re.match('(.*)/(.*)\.framework.*', framework).group(2)])

  objects = []
  for package in packages:
    # TODO(jwills): Not entirely sure why we have to load the packages into env and not app_env, but
    # if we don't we get problems with packages not being loaded
    pkg = env.LoadPackage(package)
    app_env.Append(LIBS = pkg['libs'])
    objects.append(pkg['objects'])
  app = app_env.Program(target, source + objects)
  cmd = SCons.Defaults.Copy("$TARGET", "$SOURCE")
  app_env.Command(target_directory + 'Contents/MacOS/' + target, app, cmd)
  return app


def AppendOsParams(env):
  env.Append(LIBPATH = [os.path.join(env['GLOBAL_ROOT'], 'Glop', 'OSX', 'lib')])
#  build_framework = env.Builder(action = RunTest, emitter = TestEmitter)
#  env.Append(BUILDERS = {'RunTest' : run_test})

  symlink_builder = env.Builder(action = SymlinkBuilder)
  env.Append(BUILDERS = {'Symlink' : symlink_builder})

  env.AddMethod(InstallNameTool, 'InstallNameTool')
  env.AddMethod(CompileFramework, 'CompleteLibrary')  
  env.AddMethod(Application, 'Application')
  env.AddMethod(CopyDirectory, 'CopyDirectory')

  env.AppendUnique(CPPDEFINES = ['MACOSX'])
  env.AppendUnique(FRAMEWORKPATH = ['/System/Library/Frameworks'])
  env.AppendUnique(FRAMEWORKS = [
      'Carbon',
      'AGL',
      'OpenGL',
      'ApplicationServices',
      'IOKit'])
  if env.GetOption('compile-mode') == 'dbg':
    env.AppendUnique(CPPDEFINES = ['_DEBUG'])
#      env.AppendUnique(FRAMEWORKS = ['OpenGL'])
  if env.GetOption('compile-mode') == 'opt':
    env.AppendUnique(CPPDEFINES = ['_RELEASE'])
    env.Append(CCFLAGS = ['-O2'])
