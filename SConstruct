# Setup
import os
env = Environment(ENV = os.environ)
try:
  env.Tool('config', toolpath = [os.environ.get('CBANG_HOME')])
except Exception as e:
  raise Exception('CBANG_HOME not set?\n' + str(e))

env.CBLoadTools('compiler cbang dist build_info packager')

# Override variables
env.CBAddVariables(('cxxstd', 'Set C++ language standard', 'c++11'))

conf = env.CBConfigure()

# Settings
import json
with open('package.json', 'r') as f: pkg_cnf = json.load(f)
name = pkg_cnf['name']

# Version
version = pkg_cnf['version']
major, minor, revision = version.split('.')

# Config vars
env.Replace(PACKAGE_VERSION = version)
env.Replace(BUILD_INFO_NS = 'JmpAPI::BuildInfo')

if not env.GetOption('clean') and not 'package' in COMMAND_LINE_TARGETS:
  conf.CBConfig('compiler')

  conf.CBConfig('cbang')
  env.CBDefine('USING_CBANG') # Using CBANG macro namespace

  if not env.CBConfigEnabled('event'):
    raise Exception('libevent support in C! is required')

  if not env.CBConfigEnabled('openssl'):
    raise Exception('OpenSSL support in C! is required')

  env.Append(PREFER_DYNAMIC = 'rt tcmalloc Xss m'.split())

conf.Finish()

# Fronend
if not ('package' in COMMAND_LINE_TARGETS or 'dist' in COMMAND_LINE_TARGETS):
  from subprocess import check_call
  check_call(['npm', 'install'])
  check_call(['npm', 'run', 'build'])

# Program
Export('env name')
prog, lib = SConscript(
  'src/%s.scons' % name, variant_dir = 'build', duplicate = 0)
Default(prog)

# Clean
Clean(prog, ['build', 'config.log'])

# Dist
docs = ['README.md'] + list(map(lambda x: (str(x), str(x)), Glob('api/*.yaml')))
tar = env.TarBZ2Dist(name, docs + [prog])
Alias('dist', tar)
AlwaysBuild(tar)

if 'package' in COMMAND_LINE_TARGETS:
  pkg = env.Packager(
    name,
    version = version,
    maintainer = 'Joseph Coffland <joseph@cauldrondevelopment.com>',
    vendor = 'Cauldron Development LLC',
    url = 'https://github.com/CauldronDevelopmentLLC/jmpapi',
    license = 'copyright',
    bug_url = 'https://github.com/CauldronDevelopmentLLC/jmpapi/issues',
    summary = pkg_cnf['description'],
    description = pkg_cnf['description'],
    prefix = '/usr',

    documents = docs,
    programs = [str(prog[0]), 'scripts/jmpapi-run'],
    changelog = 'ChangeLog',
    platform_independent = [
      'http', 'scripts/jmpapi.service', 'scripts/jmpapi@.service'] + \
      list(map(str, Glob('src/sql/*.sql'))),

    deb_directory = 'debian',
    deb_section = 'science',
    deb_depends = 'debconf | debconf-2.0, libc6, bzip2, zlib1g',
    deb_pre_depends = 'adduser, ssl-cert',
    deb_priority = 'optional',
    )

  AlwaysBuild(pkg)
  env.Alias('package', pkg)

  f = None
  try:
    f = open('package-description.txt', 'w')
    f.write(pkg_cnf['description'])
  finally:
    if f is not None: f.close()
