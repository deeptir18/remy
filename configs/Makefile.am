M_CPPFLAGS = -I../protobufs
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
LDADD = ../protobufs/libremyprotos.a -ltcmalloc -lm $(protobuf_LIBS)

bin_PROGRAMS = configs test_config

common_source = setinputconfig.hh setinputconfig.cc 

configs_SOURCES = $(common_source)

test_config_SOURCES = settestconfig.cc
