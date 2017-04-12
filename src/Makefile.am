AM_CPPFLAGS = $(CXX11_FLAGS) -I../protobufs
# AM_CXXFLAGS = $(PICKY_CXXFLAGS)
AM_CXXFLAGS = -Wall -Wpedantic -Wextra
LDADD = ../protobufs/libremyprotos.a -lm $(protobuf_LIBS)

bin_PROGRAMS = remy remy-poisson remy-lerp sender-runner sender-logger scoring-example configuration inspect-config inspect-simulationsdata lerptester

common_source = delay.hh evaluator.cc evaluator.hh                 \
	exponential.hh link.hh link-templates.cc stochastic-loss.hh                      \
	memory.cc memory.hh memoryrange.cc memoryrange.hh              \
	network.cc network.hh packet.hh poisson.hh                     \
	random.cc random.hh rat.cc rat.hh rat-templates.cc             \
	receiver.cc receiver.hh sendergang.cc sendergang.hh            \
	senderdatapoint.hh                                             \
	sendergangofgangs.cc sendergangofgangs.hh                  \
	utility.hh whisker.cc whisker.hh whiskertree.cc whiskertree.hh \
	poly.hh                                                 \
	aimd-templates.cc aimd.cc aimd.hh polysender-templates.cc polysender.cc polysender.hh                              \
	lerpsender-templates.cc lerpsender.cc lerpsender.hh linterp.h p2.cc p2.h \
	configrange.hh configrange.cc                              \
	simulationresults.hh simulationresults.cc                      \
    action.hh fin.hh fin.cc fintree.cc fintree.hh  	               \
    fish.hh fish.cc fish-templates.cc

noinst_LIBRARIES = libremycore.a
libremycore_a_SOURCES = $(common_source)

remy_SOURCES = $(common_source) remy.cc ratbreeder.cc ratbreeder.hh \
	breeder.cc breeder.hh

remy_poisson_SOURCES = $(common_source) remy-poisson.cc breeder.cc breeder.hh fishbreeder.cc fishbreeder.hh

remy_lerp_SOURCES = $(common_source) remy-lerp.cc lerpbreeder.cc lerpbreeder.hh

sender_runner_SOURCES = $(common_source) sender-runner.cc

sender_logger_SOURCES = $(common_source) sender-logger.cc

scoring_example_SOURCES = $(common_source) scoring-example.cc

configuration_SOURCES = input-configrange.cc

inspect_config_SOURCES = inspect-configrange.cc

inspect_simulationsdata_SOURCES = inspect-simulationsdata.cc

lerptester_SOURCES = $(common_source) lerptester.cc lerpbreeder.cc lerpbreeder.hh

