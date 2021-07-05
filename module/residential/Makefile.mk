pkglib_LTLIBRARIES += module/residential/residential.la

module_residential_residential_la_CPPFLAGS =
module_residential_residential_la_CPPFLAGS += $(AM_CPPFLAGS)

module_residential_residential_la_LDFLAGS =
module_residential_residential_la_LDFLAGS += $(AM_LDFLAGS)

module_residential_residential_la_LIBADD = -ldl -larmadillo

module_residential_residential_la_SOURCES = module/residential/init.cpp module/residential/main.cpp
module_residential_residential_la_SOURCES += module/residential/apartment.cpp module/residential/apartment.h
module_residential_residential_la_SOURCES += module/residential/appliance.cpp module/residential/appliance.h
module_residential_residential_la_SOURCES += module/residential/clotheswasher.cpp module/residential/clotheswasher.h
module_residential_residential_la_SOURCES += module/residential/dishwasher.cpp module/residential/dishwasher.h
module_residential_residential_la_SOURCES += module/residential/dryer.cpp module/residential/dryer.h
module_residential_residential_la_SOURCES += module/residential/evcharger.cpp module/residential/evcharger_det.cpp
module_residential_residential_la_SOURCES += module/residential/evcharger_det.h module/residential/evcharger.h
module_residential_residential_la_SOURCES += module/residential/freezer.cpp module/residential/freezer.h
module_residential_residential_la_SOURCES += module/residential/house_e.cpp module/residential/house_e.h
module_residential_residential_la_SOURCES += module/residential/lights.cpp module/residential/lights.h
module_residential_residential_la_SOURCES += module/residential/microwave.cpp module/residential/microwave.h
module_residential_residential_la_SOURCES += module/residential/occupantload.cpp module/residential/occupantload.h
module_residential_residential_la_SOURCES += module/residential/plugload.cpp module/residential/plugload.h
module_residential_residential_la_SOURCES += module/residential/range.cpp module/residential/range.h
module_residential_residential_la_SOURCES += module/residential/rbsa.cpp module/residential/rbsa.h
module_residential_residential_la_SOURCES += module/residential/refrigerator.cpp module/residential/refrigerator.h
module_residential_residential_la_SOURCES += module/residential/residential_enduse.cpp module/residential/residential_enduse.h
module_residential_residential_la_SOURCES += module/residential/solvers.cpp module/residential/solvers.h
module_residential_residential_la_SOURCES += module/residential/thermal_storage.cpp module/residential/thermal_storage.h
module_residential_residential_la_SOURCES += module/residential/waterheater.cpp module/residential/waterheater.h
module_residential_residential_la_SOURCES += module/residential/zipload.cpp module/residential/zipload.h
module_residential_residential_la_SOURCES += module/residential/residential.h
module_residential_residential_la_SOURCES += module/residential/rbsa2014.h
module_residential_residential_la_SOURCES += module/residential/elcap1990.h
module_residential_residential_la_SOURCES += module/residential/elcap2010.h

bin_SCRIPTS += module/residential/check_residential.py
