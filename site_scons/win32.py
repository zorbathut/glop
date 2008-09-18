import SCons.Util
import SCons.Defaults
import os
import re
import shutil
import platform


# Note: This is obviously hacked to work for a framework named Glop, and nothing else
def CompileFramework(env, objs, headers, libs, framework_structure):
  app_env = env.Clone()
  libs = ['opengl32','glu32','dinput','dxguid','winmm','freetype235','jpeg6b']
  app_env.Append(ARFLAGS = [x + '.lib' for x in libs])
  app_env.Append(ARFLAGS = ['/LIBPATH:Glop/Win32/lib'])
  lib = app_env.Library('GlopDbg.lib', objs)
  header_directory = os.path.join(os.path.dirname(lib[0].get_abspath()[len(env['PROJECT_ROOT']):]), 'Glop')
  for header in headers:
    h = os.path.join(header_directory, header.get_abspath()[len(env['PROJECT_ROOT']):])
    app_env.Command(h, header, SCons.Defaults.Copy('$TARGET','$SOURCE'))
  return lib

def Application(env, target, source, resources = [], frameworks = []):
  app_env = env.Clone()
  print frameworks
  Glop = frameworks[0]
  print Glop.get_abspath()
  l = ['kernel32','user32','gdi32','winspool','comdlg32','advapi32','shell32','ole32','oleaut32','uuid','odbc32','odbccp32']
  l = [x + '.lib' for x in l]
  app_env.Append(LIBS = [Glop.get_abspath()] + l)
  app_env.Append(LIBPATH = [os.path.dirname(Glop.get_abspath())])
  app_env.Append(CPPPATH = [os.path.dirname(Glop.get_abspath())])
  app = app_env.Program(source)

#  resource_directory = os.path.dirname(app[0].get_abspath()[len(env['PROJECT_ROOT']):])
 # print 'rd',resource_directory
  for resource in resources:
    app_env.Depends(app, resource)
#    r = os.path.basename(os.path.normpath(resource))
#    rpath = os.path.join(os.path.dirname(app[0].get_abspath()), r)
#    app_env.Command(rpath, resource, SCons.Defaults.Copy('$TARGET','$SOURCE'))
    
def AppendOsParams(env):
  env.Append(LIBPATH = ['#Glop/Win32/lib'])
  env.AddMethod(CompileFramework, 'CompleteLibrary')  
  env.AddMethod(Application, 'Application')
  env.Append(CPPFLAGS = ['/W3', '/Od', '/Ob2', '/Gm', '/EHsc', '/RTC1', '/MD', '/GS', '/c' , '/Wp64', '/Zi'])
  env.Append(CPPDEFINES = ['WIN32', '_DEBUG', '_LIB', '_MBCS'])

  if env.GetOption('compile-mode') == 'dbg':
    pass
  if env.GetOption('compile-mode') == 'opt':
    env.Append(CCFLAGS = ['-O2'])
