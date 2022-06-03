#!python
import os
import sys
import subprocess

opts = Variables([], ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Try to detect the host platform automatically.
# This is used if no `platform` argument is passed
if sys.platform.startswith("linux"):
    host_platform = "linux"
elif sys.platform == "darwin":
    host_platform = "osx"
elif sys.platform == "win32" or sys.platform == "msys":
    host_platform = "windows"
else:
    raise ValueError("Could not detect platform automatically, please specify with platform=<platform>")

# Define our options
opts.Add(EnumVariable('target', "Compilation target", 'debug', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('platform', "Compilation platform", host_platform, ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", 'no'))
opts.Add(BoolVariable('use_mingw', "Use the MINGW compiler", 'no'))
opts.Add(PathVariable('target_path', 'The path where the lib is installed.', 'bin/'))
opts.Add(EnumVariable('bits', 'Register width of target platform.', '64', ['32', '64']))

# Local dependency paths, adapt them to your setup
libyuarel_path = "libyuarel"
godot_headers_path = "godot-headers"
base_lib_name = "gd-url-parser"
lib_name = base_lib_name

# Updates the environment with the option variables.
opts.Update(env)

# Process some arguments
if env['use_llvm']:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'

if env['use_mingw']:
    env['CC'] = 'gcc'
    env['CXX'] = 'g++'

if env['platform'] == '':
    print("No valid target platform selected.")
    quit();


if env['platform'] == "osx":
    env['target_path'] += 'osx/'
    lib_name += '.osx'
    env.Append(CCFLAGS=['-arch', 'x86_64'])
    env.Append(LINKFLAGS=['-arch', 'x86_64'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g', '-O2'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] in ('x11', 'linux'):
    env['target_path'] += 'x11/'
    lib_name += '.linux'
    env.Append(CCFLAGS=['-fPIC'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g3', '-Og'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] == "windows":
    if env['bits'] == '64':
        env['target_path'] += 'win64/'
    else:
        env['target_path'] += 'win32/'
    lib_name += '.windows'

    if not env['use_llvm'] and not env['use_mingw']:
        # This makes sure to keep the session environment variables on windows,
        # that way you can run scons in a vs 2017 prompt and it will find all the required tools
        env.Append(ENV=os.environ)

        env.Append(CPPDEFINES=['WIN32', '_WIN32', '_WINDOWS', '_CRT_SECURE_NO_WARNINGS'])
        env.Append(CCFLAGS=['-W3', '-GR'])
        if env['target'] in ('debug', 'd'):
            env.Append(CPPDEFINES=['_DEBUG'])
            env.Append(CCFLAGS=['-EHsc', '-MDd', '-ZI'])
            env.Append(LINKFLAGS=['-DEBUG'])
        else:
            env.Append(CPPDEFINES=['NDEBUG'])
            env.Append(CCFLAGS=['-O2', '-EHsc', '-MD'])
    else:
        env.Append(LINKFLAGS=['--static', '-static-libgcc']) # todo: Should we?
        env.Append(CCFLAGS=['-Wall', '-Wextra'])

        if env['target'] in ('debug', 'd'):
            env.Append(CPPDEFINES=['_DEBUG'])
            env.Append(CCFLAGS=['-O0', '-g3'])
        else:
            env.Append(CPPDEFINES=['NDEBUG'])
            env.Append(CCFLAGS=['-flto', '-O3', '-g0'])


if env['target'] in ('debug', 'd'):
    lib_name += '.debug'
else:
    lib_name += '.release'

lib_name += '.' + str(env['bits'])

env.Append(CPPPATH=[godot_headers_path, godot_headers_path + '/gdnative'])
env.Append(CPPPATH=['src/', libyuarel_path])
sources = Glob('src/*.c') + Glob(libyuarel_path + '/*.c')

library = env.SharedLibrary(target=env['target_path'] + base_lib_name, source=sources)

Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
