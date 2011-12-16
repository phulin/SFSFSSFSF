AM_CFLAGS = --pedantic -Wall -std=c99 @fuse_CFLAGS@ @libssl_CFLAGS@
AM_CXXFLAGS = --pedantic -Wall @fuse_CFLAGS@ @libssl_CFLAGS@ -I../src

check_PROGRAMS = encode_test decode_test
TESTS = encode_test decode_test

encode_test_SOURCES = encode_test.cc ../src/sfsfssfsf.h ../src/sfsfssfsf_file.cc
decode_test_SOURCES = decode_test.cc ../src/sfsfssfsf.h ../src/sfsfssfsf_file.cc
