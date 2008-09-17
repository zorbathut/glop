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

def CopyBuilder(env, target, source):
  print "asdfASDFASDF"
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
    shutil.copy(target, source)
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
#      env.Command(os.path.join(target, source_path), os.readlink(s), source_path, Symlink)
    elif not os.path.isdir(s):
#      env.MyCopy(env.File(os.path.join(target, source_path)), env.File(s))
      env.Command(
          os.path.join(target, source_path),
          s,
          SCons.Defaults.Copy('$TARGET', '$SOURCE'))
  return [env.Dir(target)]
#  for p in l:
#    print 'target: ', p
#  l = ListAllChildren(source)
#  for p in l:
#    print 'source: ', p

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
      '-mmacosx-version-min=10.5 '
  )
  framework_env.Append(LIBS = libs)
  Glop = framework_env.Program(framework_name, objs)
  assert len(Glop) == 1
  Glop = Glop[0]
  framework_path = Glop.get_abspath() + '.framework'
  binary = os.path.join(framework_path, 'Versions', 'A', 'Glop')

  a = env.Dir(framework_path)
  b = env.Dir(framework_structure)

  print 'copy dir: ', framework_path, framework_structure
  framework_directory = env.CopyDirectory(framework_path, framework_structure)
  framework_binary = env.Command(binary, Glop, SCons.Defaults.Copy('$TARGET', '$SOURCE'))

  env.Symlink(
      os.path.join(str(framework_directory[0]), os.path.basename(str(framework_binary[0]))),
      str(framework_binary[0]))

  headers_directory = os.path.join(framework_path, 'Versions', 'A', 'Headers')
  for header in headers:
    header_directory = os.path.join(framework_path, 'Versions', 'A', 'Headers')
    header_path = os.path.join(header_directory, header.get_abspath()[len(env['PROJECT_ROOT']):])
    env.Command(header_path, header, SCons.Defaults.Copy('$TARGET', '$SOURCE'))

  return framework_path
#  return framework_env.OrganizeFramework(Glop)
  

def Application(env, target, source, resources = [], frameworks = []):
  app_env = env.Clone()
  target_directory = target + '.app/'

  if not SCons.Util.is_List(frameworks):
    frameworks = [frameworks]
  for framework in frameworks:
    framework_name = re.findall('[^/]+\.framework', framework)[0]
    target_framework = target_directory + 'Contents/Frameworks/' + framework_name
    # Execute(Delete), Command(Copy) is a cludge to force SCons to copy a directory
    env.Execute(SCons.Defaults.Delete(target_framework))
#    print 'copy dir: ', target_framework, framework
#    env.CopyDirectory(os.path.abspath(target_framework), framework)
    env.Command(target_framework, framework, SCons.Defaults.Copy("$TARGET", "$SOURCE"))

  # TODO(jwills): This probably isn't being tested at all right now
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

  app = app_env.Program(target, source)
  cmd = SCons.Defaults.Copy("$TARGET", "$SOURCE")
  app_env.Command(target_directory + 'Contents/MacOS/' + target, app, cmd)
  return app


def AppendOsParams(env):
  env.Append(LIBPATH = ['#Glop/OSX/lib'])
#  build_framework = env.Builder(action = RunTest, emitter = TestEmitter)
#  env.Append(BUILDERS = {'RunTest' : run_test})

  symlink_builder = env.Builder(action = SymlinkBuilder)
  env.Append(BUILDERS = {'Symlink' : symlink_builder})

  copy_builder = env.Builder(action = CopyBuilder)
  env.Append(BUILDERS = {'MyCopy' : copy_builder})

  env.AddMethod(CompileFramework, 'CompleteLibrary')  
  env.AddMethod(Application, 'Application')
  env.AddMethod(CopyDirectory, 'CopyDirectory')

  if env.GetOption('compile-mode') == 'dbg':
    pass
  if env.GetOption('compile-mode') == 'opt':
    env.Append(CCFLAGS = ['-O2'])
