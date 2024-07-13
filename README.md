# qalculate-helper
qalculate-helper is a small program that allows other
software to easily and securely integrate the amazing
[libqalculate](https://github.com/Qalculate/libqalculate) library. It
was primarily created for [LiBot](https://libot.eu.org/), but may be
integrated into other software.

The first part of the version (`X.Y.Z`) matches the minimum LiBot
version. When this part of the version changes, breaking changes may be
introduced to the input/output format.

The second part of the version (`-N`) denotes the qalculate-helper update
number. No breaking changes are introduced to the input/output format
when this number changes, but output content may change.

## Running
qalculate-helper takes three arguments:
```
./qalculate-helper [expression] [mode] [base]
```

* **`expression`** is the expression string, for example `1 + 1`.

	Note that while libqalculate handles `to <unit>` syntax, it
	does not handle `to <base>` - this must be passed into the
	`base` arguments.

* **`mode`** is a bit field of modes to enable
	* `precision` (`1 << 0`) sets precision to 900
	* `exact` (`1 << 1`) enables exact results (for example, `2/6`
	will evaluate to `1/3` rather than the decimal representation)
	* `nocolor` (`1 << 2`) disables ANSI color escapes in the output

* **`base`** determines which base the output should be expressed in. A
list of supported bases is available on
https://qalculate.github.io/reference/includes_8h.html - base constants
are prefixed with `BASE_`. Note that some bases, for example
`BASE_CUSTOM`, are not yet supported.

### Output
qalculate-helper will print multiple outputs, separated by a NUL byte
(`\x00`). The first byte in every output determines its type:

* **`TYPE_MESSAGE`** (**`\x01`**) denotes messages emitted by the
	calculator. The first denotes its severity:
	* `LEVEL_INFO` (`\x01`)
	* `LEVEL_WARNING` (`\x02`)
	* `LEVEL_ERROR` (`\x03`)
	* `LEVEL_UNKNOWN` (`\x04`)

	The message content is stored from the second byte until the
	next separator as a string.
* **`TYPE_RESULT`** (**`\x02`**) denotes the evaluation result. The
	result content is stored from the first byte until the next separator
	as a string.

The order of outputs is not guaranteed. Every invocation of
`qalculate-helper` will print exactly one result, but may print any
number of messages.

### Exit codes
* 1 - invalid arguments
* 102 - evaluation has timed out
* 103 - failed to update the exchange rates (only returned by
`exchange-rate-updater`)
* 139 - segmentation fault (report a bug)
* 159 - bad system call / seccomp violation (report a bug)

## Compiling
Ensure you have the following prerequisites before compiling:
- [Meson](https://mesonbuild.com/)
- A C++ compiler ([g++](https://gcc.gnu.org/) or
  [clang++](https://clang.llvm.org/) should work)
- libqalculate
- libseccomp (if `seccomp` is enabled)
- libcap-ng (if `setuid` is enabled)

qalculate-helper uses Meson as its build
system. To compile with default build options, run the following:
```
$ git clone git://zajc.eu.org/libot/qalculate-helper.git
$ cd qalculate-helper
$ meson setup build
$ meson compile -C build
```

### Compiling against a custom libqalculate
If you would like to compile qalculate-helper against a custom
libqalculate build, as opposed to the one provided by the system, set the
`PKG_CONFIG_PATH` environment variable to the `meson setup` command:

```
PKG_CONFIG_PATH='/path/to/libqalculate/lib/pkgconfig/ meson setup
```

If you would like prevent `qalculate-helper` from building against
the (potentially insecure) system-provided libqalculate, set the
`expect_custom_libqalculate` parameter to `true`:

```
PKG_CONFIG_PATH='/path/to/libqalculate/lib/pkgconfig/ meson -Dexpect_custom_libqalculate=true setup
```

This will abort the build if libqalculate from `/usr` is to be used.

### Build options
qalculate-helper provides many build options to tune its behavior and
security

Build options are provided to the `meson setup` command in the following
manner:
```
$ meson setup -Doption=value -Doption=value ... build/
```

You can run `meson configure` to view all available options.

* **`-Dbuildtype` (default: `release`)

	Builds the project in a different mode. If `debug` or
	`debugoptimized` is used, certain security features will be
	disabled or relaxed to make the project easier to debug.

* **`-Dexchange_rate_updater` (default: `enabled`)

	Builds `exchange-rate-updater`, a separate binary used to update
	monetary exchange rates.

* **`-Dseccomp`** (default: `enabled`)

	[seccomp](https://en.wikipedia.org/wiki/seccomp) is a Linux security
	feature that limits a process' ability to use certain system
	cals. In the case of qalculate-helper, this prevents any potential
	exploit from opening new files, using the network, executing
	new programs, and many others.

	Issues with seccomp manifest as "Bad system call" errors (exit
	code 159). In case this happens, find the offending system call
	in the kernel's ring buffer (run `dmesg`) and add it to the
	whitelist in `src/security_util.cpp`. Please also report the
	bug to `marko@zajc.eu.org`.

	Disabling this degrades security.

* **`-Dsetuid`** (default: `enabled`)
	* **`-Dsetuid_uid`** (default: `2000`)
	* **`-Dsetuid_gid`** (default: `2000`)

	Controls setuid/setid behavior of qalculate-helper, and requires
	some setup. You need to create a user and a group, get their
	uid and gid, chown the executable to the uid:gid, add suid and
	sgid mode, and set setgid and setpcap capabilities. For example:
	1. Open /etc/passwd and add the following line (pick a different
	uid and gid if 2000 is already used)
		```
		qalc:x:2000:2000::/opt/qalculate-helper:/bin/false
		```
	2. Open /etc/group and add the following line
		```
		qalc:x:2000:
		```
	3. (optional) Run `pwck` and `grpck` to correct shadow and
	gshadow databases
	3. Run the following as root:
		```
		# chown qalc:qalc build/qalculate-helper build/exchange-rate-updater
		# chmod 6755 build/qalculate-helper build/exchange-rate-updater
		# setcap cap_setgid+ep cap_setpcap+ep build/qalculate-helper build/exchange-rate-updater
		```

	Disabling this degrades security.

* **`-Dlibqalculate_static_link` (default: `false`)

	Link libqalculate statically. This removes the risk of using
	the wrong libqalculate at runtime and makes qalculate-helper
	easier to move to a different machine, but increases the binary
	size and requires recompilation when updating libqalculate.

* **`-Dexpect_custom_libqalculate` (default: `false`)

	Aborts the build if libqalculate from `/usr` is to be used. 

* **`-Ddefang_calculator` (default: `true`)

	Removes dangerous functions from `Calculator` at runtime. This
	has no effect if libqalculate was compiled with
	`--disable-insecure` and `--without-gnuplot-call` (you pass
	these flags to `./autogen.sh` when building it).

	Enabling `expect_custom_libqalculate` and
	`libqalculate_static_link` is recommended when setting this to
	`false` to reduce the risk of using the wrong libqalculate

* **`-preload_datasets` (default: `true`)

	Preloads datasets for dataset commands (`atom` and `planet`)
	to fix them when `seccomp` is enabled. This is not necessary if
	libqalculate was compiled with `--enable-compiled-definitions`
	(you pass this flag to `./autogen.sh` when building it).

	Enabling `expect_custom_libqalculate` and
	`libqalculate_static_link` is recommended when setting this to
	`false` to reduce the risk of using the wrong libqalculate
