include gldcore/link/engine/Makefile.mk
include gldcore/link/matlab/Makefile.mk
include gldcore/link/python/Makefile.mk
include gldcore/rt/Makefile.mk
include gldcore/converters/Makefile.mk
include gldcore/solvers/Makefile.mk
include gldcore/scripts/Makefile.mk
include gldcore/geodata/Makefile.mk

dist_pkgdata_DATA += gldcore/tzinfo.txt
dist_pkgdata_DATA += gldcore/unitfile.txt
dist_pkgdata_DATA += gldcore/gridlabd.cnf

GLD_SOURCES_PLACE_HOLDER = 
GLD_SOURCES_PLACE_HOLDER += gldcore/aggregate.cpp gldcore/aggregate.h
GLD_SOURCES_PLACE_HOLDER += gldcore/class.cpp gldcore/class.h
GLD_SOURCES_PLACE_HOLDER += gldcore/cmdarg.cpp gldcore/cmdarg.h
GLD_SOURCES_PLACE_HOLDER += gldcore/compare.cpp gldcore/compare.h
GLD_SOURCES_PLACE_HOLDER += gldcore/complex.h
GLD_SOURCES_PLACE_HOLDER += gldcore/console.h
GLD_SOURCES_PLACE_HOLDER += gldcore/convert.cpp gldcore/convert.h
GLD_SOURCES_PLACE_HOLDER += gldcore/curl.cpp gldcore/curl.h
GLD_SOURCES_PLACE_HOLDER += gldcore/daemon.cpp gldcore/daemon.h
GLD_SOURCES_PLACE_HOLDER += gldcore/debug.cpp gldcore/debug.h
GLD_SOURCES_PLACE_HOLDER += gldcore/deltamode.cpp gldcore/deltamode.h
GLD_SOURCES_PLACE_HOLDER += gldcore/enduse.cpp gldcore/enduse.h
GLD_SOURCES_PLACE_HOLDER += gldcore/environment.cpp gldcore/environment.h
GLD_SOURCES_PLACE_HOLDER += gldcore/exception.cpp gldcore/exception.h
GLD_SOURCES_PLACE_HOLDER += gldcore/exec.cpp gldcore/exec.h
GLD_SOURCES_PLACE_HOLDER += gldcore/find.cpp gldcore/find.h
GLD_SOURCES_PLACE_HOLDER += gldcore/gld_sock.h
GLD_SOURCES_PLACE_HOLDER += gldcore/globals.cpp gldcore/globals.h
GLD_SOURCES_PLACE_HOLDER += gldcore/gridlabd.h
GLD_SOURCES_PLACE_HOLDER += gldcore/gui.cpp gldcore/gui.h
GLD_SOURCES_PLACE_HOLDER += gldcore/http_client.cpp gldcore/http_client.h
GLD_SOURCES_PLACE_HOLDER += gldcore/index.cpp gldcore/index.h
GLD_SOURCES_PLACE_HOLDER += gldcore/instance.cpp gldcore/instance.h
GLD_SOURCES_PLACE_HOLDER += gldcore/instance_cnx.cpp gldcore/instance_cnx.h
GLD_SOURCES_PLACE_HOLDER += gldcore/instance_slave.cpp gldcore/instance_slave.h
GLD_SOURCES_PLACE_HOLDER += gldcore/interpolate.cpp gldcore/interpolate.h
GLD_SOURCES_PLACE_HOLDER += gldcore/job.cpp gldcore/job.h
GLD_SOURCES_PLACE_HOLDER += gldcore/json.cpp gldcore/json.h
GLD_SOURCES_PLACE_HOLDER += gldcore/kill.cpp gldcore/kill.h
GLD_SOURCES_PLACE_HOLDER += gldcore/kml.cpp gldcore/kml.h
GLD_SOURCES_PLACE_HOLDER += gldcore/legal.cpp gldcore/legal.h
GLD_SOURCES_PLACE_HOLDER += gldcore/linkage.cpp gldcore/linkage.h
GLD_SOURCES_PLACE_HOLDER += gldcore/link.cpp gldcore/link.h
GLD_SOURCES_PLACE_HOLDER += gldcore/list.cpp gldcore/list.h
GLD_SOURCES_PLACE_HOLDER += gldcore/load.cpp gldcore/load.h
GLD_SOURCES_PLACE_HOLDER += gldcore/loadshape.cpp gldcore/loadshape.h
GLD_SOURCES_PLACE_HOLDER += gldcore/local.cpp gldcore/local.h
GLD_SOURCES_PLACE_HOLDER += gldcore/lock.cpp gldcore/lock.h
GLD_SOURCES_PLACE_HOLDER += gldcore/main.cpp gldcore/main.h
GLD_SOURCES_PLACE_HOLDER += gldcore/match.cpp gldcore/match.h
GLD_SOURCES_PLACE_HOLDER += gldcore/matlab.cpp gldcore/matlab.h
GLD_SOURCES_PLACE_HOLDER += gldcore/module.cpp gldcore/module.h
GLD_SOURCES_PLACE_HOLDER += gldcore/object.cpp gldcore/object.h
GLD_SOURCES_PLACE_HOLDER += gldcore/output.cpp gldcore/output.h
GLD_SOURCES_PLACE_HOLDER += gldcore/platform.h
GLD_SOURCES_PLACE_HOLDER += gldcore/property.cpp gldcore/property.h
GLD_SOURCES_PLACE_HOLDER += gldcore/python_embed.cpp gldcore/python_embed.h
GLD_SOURCES_PLACE_HOLDER += gldcore/python_property.cpp gldcore/python_property.h
GLD_SOURCES_PLACE_HOLDER += gldcore/random.cpp gldcore/random.h
GLD_SOURCES_PLACE_HOLDER += gldcore/realtime.cpp gldcore/realtime.h
GLD_SOURCES_PLACE_HOLDER += gldcore/sanitize.cpp gldcore/sanitize.h
GLD_SOURCES_PLACE_HOLDER += gldcore/save.cpp gldcore/save.h
GLD_SOURCES_PLACE_HOLDER += gldcore/schedule.cpp gldcore/schedule.h
GLD_SOURCES_PLACE_HOLDER += gldcore/server.cpp gldcore/server.h
GLD_SOURCES_PLACE_HOLDER += gldcore/setup.cpp gldcore/setup.h
GLD_SOURCES_PLACE_HOLDER += gldcore/stream.cpp gldcore/stream.h
GLD_SOURCES_PLACE_HOLDER += gldcore/stream_type.h
GLD_SOURCES_PLACE_HOLDER += gldcore/test.cpp gldcore/test.h
GLD_SOURCES_PLACE_HOLDER += gldcore/threadpool.cpp gldcore/threadpool.h
GLD_SOURCES_PLACE_HOLDER += gldcore/timestamp.cpp gldcore/timestamp.h
GLD_SOURCES_PLACE_HOLDER += gldcore/transform.cpp gldcore/transform.h
GLD_SOURCES_PLACE_HOLDER += gldcore/unit.cpp gldcore/unit.h
GLD_SOURCES_PLACE_HOLDER += gldcore/validate.cpp gldcore/validate.h
GLD_SOURCES_PLACE_HOLDER += gldcore/version.cpp gldcore/version.h
GLD_SOURCES_PLACE_HOLDER += gldcore/link/python/python.cpp

GLD_SOURCES_EXTRA_PLACE_HOLDER =
GLD_SOURCES_EXTRA_PLACE_HOLDER += gldcore/cmex.c gldcore/cmex.h
GLD_SOURCES_EXTRA_PLACE_HOLDER += gldcore/ufile.c gldcore/ufile.h
GLD_SOURCES_EXTRA_PLACE_HOLDER += gldcore/xcore.cpp gldcore/xcore.h

bin_PROGRAMS += gridlabd.bin

gridlabd_bin_CPPFLAGS = -DMAIN_PYTHON -fPIC
gridlabd_bin_CPPFLAGS += $(XERCES_CPPFLAGS)
gridlabd_bin_CPPFLAGS += $(AM_CPPFLAGS)

gridlabd_bin_LDFLAGS =
gridlabd_bin_LDFLAGS += $(XERCES_LDFLAGS)
gridlabd_bin_LDFLAGS += $(AM_LDFLAGS)

gridlabd_bin_LDADD =
gridlabd_bin_LDADD += $(XERCES_LIB)
gridlabd_bin_LDADD += $(CURSES_LIB)
gridlabd_bin_LDADD += -ldl -lcurl

gridlabd_bin_SOURCES =
gridlabd_bin_SOURCES += $(GLD_SOURCES_PLACE_HOLDER)

EXTRA_gridlabd_bin_SOURCES =
EXTRA_gridlabd_bin_SOURCES += $(GLD_SOURCES_EXTRA_PLACE_HOLDER)

bin_SCRIPTS += gldcore/gridlabd 

GLD_SOURCES_PLACE_HOLDER += gldcore/build.h
BUILT_SOURCES += gldcore/build.h
CLEANFILES += gldcore/build.h origin.txt

pkginclude_HEADERS =
pkginclude_HEADERS += gldcore/build.h
pkginclude_HEADERS += gldcore/class.h
pkginclude_HEADERS += gldcore/complex.h
pkginclude_HEADERS += gldcore/debug.h
pkginclude_HEADERS += gldcore/enduse.h
pkginclude_HEADERS += gldcore/exception.h
pkginclude_HEADERS += gldcore/loadshape.h
pkginclude_HEADERS += gldcore/lock.h
pkginclude_HEADERS += gldcore/module.h
pkginclude_HEADERS += gldcore/object.h
pkginclude_HEADERS += gldcore/property.h
pkginclude_HEADERS += gldcore/schedule.h
pkginclude_HEADERS += gldcore/test.h
pkginclude_HEADERS += gldcore/version.h

gridlabddir = $(prefix)/share/gridlabd
gridlabd_DATA = origin.txt

gldcore/gridlabd.in: gldcore/gridlabd.m4sh
	@autoreconf -isf
	@autom4te -l m4sh $< > $@

gldcore/build.h: buildnum

buildnum: utilities/build_number
	@/bin/bash -c "source $(top_srcdir)/utilities/build_number $(top_srcdir) gldcore/build.h"
	@/bin/bash -c "source utilities/update_origin.sh" > origin.txt

weather:
	@(echo "Updating weather data index" && export GLD_ETC=$(prefix)/share/gridlabd && $(bindir)/gridlabd-weather fetch_index)

template: # TODO

library: # TODO	
