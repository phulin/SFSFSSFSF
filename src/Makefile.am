# what flags you want to pass to the C compiler & linker
AM_CFLAGS = --pedantic -Wall -std=c99 -O2 @fuse_CFLAGS@
AM_CXXFLAGS = --pedantic -Wall -O2 @fuse_CFLAGS@

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = sfsfssfsf flip_lowbits flip_lowbits_cpp #foo

sfsfssfsf_SOURCES = sfsfssfsf.cc sfsfssfsf.h pstat.c pstat.h
sfsfssfsf_LDADD = @fuse_LIBS@

flip_lowbits_SOURCES = flip_lowbits.c pstat.c pstat.h

#flip_lowbits_cpp_LDADD = -lz -lm -lbz2 ../resources/ffmpeg/build/lib/libavcodec.a ../resources/ffmpeg/build/lib/libavformat.a ../resources/ffmpeg/build/lib/libavdevice.a ../resources/ffmpeg/build/lib/libavfilter.a ../resources/ffmpeg/build/lib/libavutil.a ../resources/ffmpeg/build/lib/libswscale.a
flip_lowbits_cpp_SOURCES = flip_lowbits_cpp.cpp flip_lowbits_cpp.hpp
flip_lowbits_cpp_LDADD = @libavformat_LIBS@ @libavcodec_LIBS@

#flip_lowbits_cpp_CPPFLAGS = -I$(top_srcdir)/resources/boost -I$(top_srcdir)/resources/ffmpeg/libavcodec -I$(top_srcdir)/resources/ffmpeg/libavformat -I$(top_srcdir)/resources/ffmpeg
flip_lowbits_cpp_CPPFLAGS = -I$(top_srcdir)/resources/boost

