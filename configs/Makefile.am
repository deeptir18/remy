M_CPPFLAGS = -I../protobufs
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
LDADD = ../protobufs/libremyprotos.a -ltcmalloc -lm $(protobuf_LIBS)

bin_PROGRAMS = configs

common_source = setinputconfig.hh setinputconfig.cc

configs_SOURCES = $(common_source)
