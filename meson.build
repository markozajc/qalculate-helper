project('qalculate-helper', 'cpp', version: '5.0.6', default_options: [
	'warning_level=3',
	'cpp_std=c++17',
	'buildtype=release',
])
add_project_arguments([], language: 'cpp')
cpp = meson.get_compiler('cpp')

dependencies = []
flags = []
link_flags = []

libqalculate = dependency('libqalculate')
if get_option('libqalculate_static_link')
	dependencies += cpp.find_library('qalculate', dirs: libqalculate.get_variable('libdir'), static: true)
	dependencies += libqalculate.partial_dependency(compile_args: true, includes: true, sources: true)
	dependencies += dependency('mpfr')
	dependencies += dependency('icu-uc')
	dependencies += dependency('libxml-2.0')
	dependencies += dependency('libcurl')
else
	dependencies += libqalculate
endif

if get_option('prefer_static') or get_option('libqalculate_static_link')
	link_flags += '-Wl,-zmuldefs'
endif


libqalculate_path = libqalculate.get_variable('prefix')
if get_option('expect_custom_libqalculate') and libqalculate_path in ['/usr']
	error('libqalculate_expect_custom_prefix is set to true, but libqalculate was found in',
		libqalculate_path,
		'(hint: run with PKG_CONFIG_PATH=/path/to/prefix/lib/pkgconfig/)')
endif

seccomp = dependency('libseccomp', required: get_option('seccomp'))
if seccomp.found()
	dependencies += seccomp
	flags += '-DSECCOMP'
else
	warning('Building without SECCOMP support, do not use in production!')
endif

capng = dependency('libcap-ng', required: get_option('setuid'))
if capng.found()
	dependencies += capng
	uid = get_option('setuid_uid')
	gid = get_option('setuid_gid')
	flags += '-DSETUID'
	flags += f'-DSETUID_UID=@uid@'
	flags += f'-DSETUID_GID=@gid@'
else
	warning('Building without SETUID support, do not use in production!')
endif

if get_option('preload_datasets')
	flags += '-DLIBQALCULATE_PRELOAD_DATASETS'
endif

if get_option('defang_calculator')
	flags += '-DLIBQALCULATE_DEFANG'
endif

if get_option('debug')
	flags += '-DENABLE_DEBUG'
else
	flags += '-D_FORTIFY_SOURCE=2'
	flags += '-fstack-protector-strong'
	flags += '-march=native'
	link_flags += '-Wl,-znow'
endif

add_project_arguments(flags, language: 'cpp')
add_project_link_arguments(link_flags, language: 'cpp')

if get_option('exchange_rate_updater').allowed()
	executable('exchange-rate-updater', [
		'src/exchange_rate_updater.cpp',
		'src/exchange_update_exception.cpp',
		'src/qalculate_exception.cpp',
		'src/security_util.cpp',
	], include_directories: [
		include_directories('include')
	], dependencies: dependencies)
endif

executable('qalculate-helper', [
	'src/security_util.cpp',
	'src/exchange_update_exception.cpp',
	'src/qalculate_exception.cpp',
	'src/qalculate_helper.cpp',
	'src/timeout_exception.cpp',
], include_directories: [
	include_directories('include')
], dependencies: dependencies)

message('Building with libqalculate from', libqalculate.get_variable(pkgconfig: 'prefix'))
