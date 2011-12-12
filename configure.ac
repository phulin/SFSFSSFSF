
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([sfsfssfsf], [0.1], [sfsfssfsf-bugs@mit.edu])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CXX

AC_C_BIGENDIAN

# Checks for programs.
AC_CHECK_PROG([FFMPEG], [ffmpeg], [ffmpeg])
AC_ARG_VAR([FFMPEG], [ffmpeg to use])

# Checks for libraries.
AS_IF([ test ! -d ./resources/boost], [echo "Unpacking boost (1st time use)..."
 	unzip -q resources/tarballs/boost_1_48_0.zip -d resources
	mv resources/boost_1_48_0 resources/boost])
#if [ ! -d ./resources/boost ]; then
#	pushd `dirname $0`
#	echo "Unpacking boost (1st time use)..."
#	unzip -q resources/tarballs/boost_1_48_0.zip -d resources
#	mv resources/boost_1_48_0 resources/boost
#	popd
#fi


PKG_CHECK_MODULES([fuse], [fuse])

PKG_CHECK_MODULES([libavcodec], [libavcodec])
PKG_CHECK_MODULES([libavformat], [libavformat])

# Checks for header files.
AC_CHECK_HEADERS([endian.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT([Makefile src/Makefile])
