
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([sfsfssfsf], [0.1], [sfsfssfsf-bugs@mit.edu])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

AC_C_BIGENDIAN

# Checks for programs.
AC_CHECK_PROG([FFMPEG], [ffmpeg], [ffmpeg])
AC_ARG_VAR([FFMPEG], [ffmpeg to use])

# Checks for libraries.
PKG_CHECK_MODULES([fuse], [fuse])
# PKG_CHECK_MODULES([libavcodec], libavcodec)
# PKG_CHECK_MODULES([libavformat], libavformat)

AC_CHECK_LIB([avcodec], [avcodec_version])
AC_CHECK_LIB([avformat], [avforamt_version])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT([Makefile src/Makefile])
