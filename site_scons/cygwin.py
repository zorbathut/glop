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
  app_env.Append(LIBS = libs, LIBPATH = ['Glop/cygwin/lib'])
  lib = app_env.Library('GlopDbg.a', objs)
  header_directory = os.path.join(os.path.dirname(lib[0].get_abspath()[len(env['PROJECT_ROOT']):]), 'Glop')
  
  app_env.Append(CPPFLAGS = ['-mno-cygwin'])
  app_env.Append(LINKFLAGS = ['-mno-cygwin'])
  app_env.Append(CXXFLAGS = ['-mno-cygwin'])
  
  app_env.Append(CXXFLAGS = ['-g'])
  app_env.Append(LINKFLAGS = ['-g'])
  
  for header in headers:
    h = os.path.join(header_directory, header.get_abspath()[len(env['PROJECT_ROOT']):])
    app_env.Command(h, header, SCons.Defaults.Copy('$TARGET','$SOURCE'))
  return lib

def Application(env, target, source, resources = [], frameworks = []):
  app_env = env.Clone()
  print frameworks
  Glop = frameworks[0]
  print Glop.get_abspath()
  
  app_env.Append(CPPFLAGS = ['-mno-cygwin'])
  app_env.Append(LINKFLAGS = ['-mno-cygwin'])
  app_env.Append(CXXFLAGS = ['-mno-cygwin'])
  
  l = ['kernel32','user32','gdi32','winspool','comdlg32','advapi32','shell32','ole32','oleaut32','uuid','odbc32','odbccp32', 'opengl32', 'freetype', 'jpeg', 'glu32', 'msvcrt', 'dinput', 'dxguid', 'fmodex', 'winmm']
  app_env.Append(LIBS = ["GlopDbg"] + l)
  app_env.Append(LIBPATH = [os.path.dirname(Glop.get_abspath())])
  app_env.Append(LIBPATH = ["../Glop/cygwin/lib"])
  app_env.Append(LIBPATH = ["/lib/mingw"])
  app_env.Append(CPPPATH = [os.path.dirname(Glop.get_abspath())])
  app_env.Append(CPPPATH = ['.'])
  
  app_env.Append(CXXFLAGS = ['-g'])
  app_env.Append(LINKFLAGS = ['-g'])
  
  app = app_env.Program(source)

#  resource_directory = os.path.dirname(app[0].get_abspath()[len(env['PROJECT_ROOT']):])
 # print 'rd',resource_directory
  for resource in resources:
    app_env.Depends(app, resource)
#    r = os.path.basename(os.path.normpath(resource))
#    rpath = os.path.join(os.path.dirname(app[0].get_abspath()), r)
#    app_env.Command(rpath, resource, SCons.Defaults.Copy('$TARGET','$SOURCE'))
    
def AppendOsParams(env):
  env.AddMethod(CompileFramework, 'CompleteLibrary')  
  env.AddMethod(Application, 'Application')
  env.Append(CPPDEFINES = ['WIN32', '_LIB', '_MBCS'])
  env.Append(CPPFLAGS = ['-mno-cygwin'])
  env.Append(LINKFLAGS = ['-mno-cygwin'])
  env.Append(CXXFLAGS = ['-mno-cygwin'])
  
  env.Append(CPPPATH = ['../../Glop/cygwin/include'])
  env.Append(CPPPATH = ['..'])
  
  env.Append(CXXFLAGS = ['-g'])
  env.Append(LINKFLAGS = ['-g'])
  """
  env.Append(LIBPATH = [os.path.join(env['GLOBAL_ROOT'], 'Glop', 'Win32', 'lib')])
  env.Append(CPPFLAGS = ['/W3', '/Od', '/Ob2', '/Gm', '/EHsc', '/RTC1', '/MD', '/GS', '/c' , '/Wp64', '/Zi'])
  if env.GetOption('compile-mode') == 'dbg':
    env.Append(CPPDEFINES = ['_DEBUG'])
  if env.GetOption('compile-mode') == 'opt':
    env.Append(CPPDEFINES = ['_RELEASE'])
    env.Append(CCFLAGS = ['-O2'])"""
