# what flags you want to pass to the C compiler & linker
AM_CFLAGS = --pedantic -Wall -std=c99 -O2 -D_FILE_OFFSET_BITS=64

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = sfsfssfsf flip_lowbits #foo

sfsfssfsf_SOURCES = hellofs.c
sfsfssfsf_CFLAGS = @fuse_CFLAGS@
sfsfssfsf_LDADD = @fuse_LIBS@

flip_lowbits_SOURCES = flip_lowbits.c
flip_lowbits_CFLAGS = @libavcodec_CFLAGS@ @libavformat_CFLAGS@
flip_lowbits_LDADD = @libavcodec_LIBS@ @libavformat_LIBS@
#foo_SOURCES = foo.c bar.h ...
