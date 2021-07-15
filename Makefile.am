# File: Makefile.am
#
# GridLAB-D 4.0
# Copyright (C) 2008 Battelle Memorial Institute
# This file is distributed under the same terms as GridLAB-D.
#
# HiPAS GridLAB-D 4.2
#   Copyright (C) 2020 Regents of the Leland Stanford Junior University
#
# Authors:
#   DP Chassin (dchassin@slac.stanford.edu)

ACLOCAL_AMFLAGS = -I m4

CC = $(PTHREAD_CC)
CXX = $(PTHREAD_CXX)

AM_CPPFLAGS = -Wall -fPIC
AM_CPPFLAGS += -I$(top_srcdir)/gldcore -I$(top_srcdir)/build-dir/gldcore
AM_CPPFLAGS += $(GLD_CPPFLAGS)

AM_CFLAGS = -Wall
AM_CFLAGS +=  -I$(top_srcdir)/gldcore -I$(top_srcdir)/build-dir/gldcore
AM_CFLAGS += $(PTHREAD_CFLAGS)
AM_CFLAGS += $(GLD_CFLAGS)  -I$(top_srcdir)/build-dir/gldcore

AM_CXXFLAGS = -Wall -fPIC
AM_CXXFLAGS += $(PTHREAD_CFLAGS)
AM_CXXFLAGS += $(GLD_CXXFLAGS) -I$(top_srcdir)/build-dir/gldcore

AM_LDFLAGS =
AM_LDFLAGS += -no-undefined
AM_LDFLAGS += -module
AM_LDFLAGS += -avoid-version
AM_LDFLAGS += -export-dynamic
AM_LDFLAGS += $(GLD_LDFLAGS)

# makefile fragments populate these variables
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
dist_pkgdata_DATA =
pkgdata_DATA =
noinst_LTLIBRARIES =
pkglib_LTLIBRARIES =
bin_PROGRAMS =
bin_SCRIPTS =
dist_doc_DATA =

python_requirements = $(top_srcdir)/requirements.txt

docs_targets =

# required modules
include third_party/Makefile.mk
include gldcore/Makefile.mk
include python_extras/Makefile.mk
include cloud/Makefile.mk

# approved modules
include modules.mk

# custom modules (see ./customize)
include custom.mk

# for doxygen commands
include aminclude.mk

dist_doc_DATA += COPYRIGHT
dist_doc_DATA += LICENSE

EXTRA_DIST += $(top_srcdir)/gldcore/gridlabd.htm
EXTRA_DIST += $(top_srcdir)/utilities/build_number

# if MISSING_XERCES
# all-local:
# 	@echo
# 	@echo "  Missing dependency: libxerces-c"
# 	@echo ""
# 	@echo "    libxerces-c was not found during configuration and is a required dependency."
# 	@echo "    Install the libxerces-c development libraries and re-run ./configure or run"
# 	@echo "    ./configure with the --with-xerces option.  Try './configure --help' for"
# 	@echo "    more information."
# 	@echo
# 	@false
# endif

help:
	@echo "Configuration targets:"
	@echo "  reconfigure       - Reconfigures a release build from a clean start"
	@echo "  reconfigure-debug - Reconfigures a debugging build from a clean start"
	@echo ""
	@echo "Cleaning targets:"
	@echo "  clean          - Erase from the build tree the files built by 'make all'"
	@echo "  distclean      - Same as 'make clean' plus erase anything ./configure created"
	@echo "  mostlyclean    - Erase intermediate files (such as .o files)"
	@echo ""
	@echo "Building targets:"
	@echo "  all            - Build $(PACKAGE) (same as 'make')"
	@echo "  install        - Install $(PACKAGE) to $(prefix)"
	@echo "  system         - Build and install $(PACKAGE) to the system folders"
	@echo "  install-strip  - Same as install, but follow by stripping debug symbols"
	@echo "  install-clean  - Same as install-system, but runs distclean, autoreconf, and configure first"
	@echo "  install-system - Same as install, but links target to /usr/local for all system users"
	@echo "  uninstall      - Erase files installed by 'make install'"
	@echo ""
	@echo "Cleaning targets:"
	@echo "  clean        - Erase from the build tree the files built by 'make all'"
	@echo "  distclean    - Same as 'make clean' plus erase anything ./configure created"
	@echo "  mostlyclean  - Erase intermediate files (such as .o files)"
	@echo ""
	@echo "Testing targets:"
	@echo "  validate  - Run the test/validation suite (requires Python)"
	@echo ""
	@echo "Packaging targets:"
	@echo "  dist          - same as 'make dist-gzip'"
	@echo "  dist-bzip2    - Generate a bzip2 tar archive of the distribution"
	@echo "  dist-deb      - Create Debian binary and source deb packages"
	@echo "  dist-deb-bin  - Create Debian binary-only deb package"
	@echo "  dist-deb-src  - Create Debian source-only deb package"
	@echo "  dist-gzip     - Generate a gzip tar archive of the distribution"
	@echo "  dist-lzma     - Generate an lzma tar archive of the distribution"
	@echo "  dist-osx      - Create Mac OS X package installer (OS X w/ XCode only)"
	@echo "  dist-rpm      - Create binary-only RPM package"
	@echo "  dist-shar     - Generate a shar archive of the distribution"
	@echo "  dist-tarZ     - Generate a compressed tar archive of the distribution"
	@echo "  dist-xz       - Generate an xz tar archive of the distribution"
	@echo "  dist-zip      - Generate a zip archive of the distribution"
	@echo "  rpm-prep      - Create \$$HOME/.rpmmacros suitable for building RPMs in"
	@echo "                  \$$HOME/rpmbuild as non-root user"
	@echo ""
	@echo "Documentation targets:"
	@echo "  html                 - Generate all html documentation"
	@echo "  module-html          - Generate module documentation (only if doxygen was found"
	@echo "                         by ./configure)"
	@echo "  developer-html       - Generate developer documentation (only if natural-docs was"
	@echo "                         found by ./configure)"
	@echo "  troubleshooting-html - Generate troubleshooting documentation (only if gawk was"
	@echo "                         was found by ./configure)"
	@echo ""
	@echo "Data targets:"
	@echo "  index        - Download all indexes from data archives"
	@echo "  weather      - Download the weather data index from the weather archive"
	@echo "  library      - Download the library index from the library archive"
	@echo "  template     - Download the template index from the template archive"
	@echo ""
	@echo "Developer targets (may require special tools):"
	@echo "  clean-wc   - Recursively remove all files and directories not under"
	@echo "               revision control (will prompt to confirm first)"
	@echo "  ctags      - Generate vi-style tags files"
	@echo "  tags       - Generate emacs-style TAGS files"
	@echo "  maintainer-clean"

over: clean all

# Recursively remove all files and directories not under revision control
clean-wc:
	@echo -e "\nThis removes all unversioned files and directories in the working copy."
	@unset REPLY && read -t 60 -p "Clean working copy (type 'yes' to proceed)? " && test "`echo "$$REPLY" | tr '[:upper:]' '[:lower:]'`" = "yes"
	. utilities/cleanwc

install-validate: install validate

check-local validate:
	@(export LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH:-${libdir}}; ${bindir}/gridlabd -D keep_progress=TRUE --validate -D keep_progress=TRUE || (utilities/save_validation_output;exit 1))

scratchdir = scratch
# XERCES_TARNAME = xerces-c-3.1.1
# XERCESCROOT = $(CURDIR)/$(scratchdir)/$(XERCES_TARNAME)

$(scratchdir)/.xerces.stamp: third_party/$(XERCES_TARNAME).tar.gz
	-rm -f $@
	test ! -e $(@D)/$(XERCES_TARNAME) || rm -rf $(@D)/$(XERCES_TARNAME)
	test -d $(@D) || mkdir -p $(@D)
	tar -xzf $< -C $(@D)
	export XERCESCROOT=$(CURDIR)/$(@D)/$(XERCES_TARNAME) && \
		cd $(@D)/$(XERCES_TARNAME) && \
		autoreconf -isf && ./configure && make
	touch $@

$(scratchdir)/pkg: distdir # $(scratchdir)/.xerces.stamp
	test ! -e $@ || rm -rf $@
	test -d $@/gldcore || mkdir -p $@/gldcore
# 	cd $(distdir) && ./configure --with-xerces=$(CURDIR)/$(scratchdir)/$(XERCES_TARNAME) && make install DESTDIR=$(CURDIR)/$@/gldcore
	cd $(distdir) && ./configure && make install DESTDIR=$(CURDIR)/$@/gldcore
	cd $@ && mkdir -p climate-data/usr/local/lib/gridlabd devel/usr/local/lib/gridlabd # libxerces-c/usr/lib
#	mv $@/gldcore/usr/lib/gridlabd/*.zip $@/climate-data/usr/lib/gridlabd/
	mv $@/gldcore/usr/local/include $@/devel/usr/local/include
	mv $@/gldcore/usr/local/lib/gridlabd/*.la $@/devel/usr/local/lib/gridlabd/
#	cp $(scratchdir)/$(XERCES_TARNAME)/src/.libs/libxerces-c.* $@/libxerces-c/usr/lib/
	$(am__remove_distdir)

$(scratchdir)/$(PACKAGE_TARNAME).mpkg: $(scratchdir)/pkg
	env PATH=$$PATH:/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS \
	PackageMaker --doc Resources/GridLAB-D.pmdoc --version "$(PACKAGE_VERSION)" --title "$(PACKAGE_NAME)" --out $@

$(scratchdir)/$(PACKAGE_TARNAME).dmg: $(scratchdir)/$(PACKAGE_TARNAME).mpkg
	test ! -e $@ || rm -f $@
	hdiutil create -size $$((`du -sm $< | cut -f 1` + 3))m -fs HFS+ -volname "$(PACKAGE_STRING)" $@
	hdiutil attach -mountpoint $(@D)/mnt $@
	mv $< $(@D)/mnt/
	hdiutil detach $(@D)/mnt

$(distdir).dmg: $(scratchdir)/$(PACKAGE_TARNAME).dmg
	test ! -e $@ || rm -rf $@
	hdiutil convert -format UDZO -o $@ $<
	rm -f $<
	-rmdir $(scratchdir)

clean-dist-osx:
	-test ! -d $(scratchdir)/mnt || hdiutil detach $(scratchdir)/mnt
#	-rm -rf $(scratchdir)

# Build a .mpkg installer package for Mac OS X
dist-osx:
	trap '$(MAKE) clean-dist-osx' EXIT && \
	$(MAKE) $(distdir).dmg

# Build deb (binary and source) packages
dist-deb: distdir
	echo -e "gridlabd ($(VERSION)-1) unstable; urgency=low\n\n  * Version $(VERSION) release.\n  * View recent changes at: https://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Special:RecentChanges\n\n -- GridLAB-D Team <gridlabd@pnl.gov> `date -R`\n" > $(distdir)/debian/changelog
	ln -s $(distdir) $(distdir)-1
	(cd $(distdir)-1 && dpkg-buildpackage -tc -us -uc $(DEBFLAGS))
	rm -f $(distdir)-1
	$(am__remove_distdir)

# Build Debian deb binary package
dist-deb-bin:
	$(MAKE) dist-deb DEBFLAGS=-b

# Build Debian deb source package
dist-deb-src:
	$(MAKE) dist-deb DEBFLAGS=-S

# Build RPM binary package
dist-rpm: dist-gzip
	rpmbuild -tb $(distdir).tar.gz

RPMBUILDDIR = $(HOME)/rpmbuild

# Create $(HOME)/.rpmmacros file suitable for building RPMs in $(RPMBUILDDIR) as non-root user
rpm-prep:
	@DIR="$(RPMBUILDDIR)" && if [ "$${DIR:0:1}" != "/" ]; then \
		echo >&2 "ERROR: RPMBUILDDIR must be an absolute path"; exit 1; \
	fi
	@if [ -f "$$HOME/.rpmmacros" ]; then \
		sed -i 's/^\(%_topdir\)/#\1/' $$HOME/.rpmmacros; \
		echo >&2 "WARNING: $$HOME/.rpmmacros already exists; commenting out _topdir macros"; \
	fi
	@echo "%_topdir $(RPMBUILDDIR)" >> $$HOME/.rpmmacros
	@echo "RPMs will be built in $(RPMBUILDDIR)"
	@for DIR in $(RPMBUILDDIR)/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}; do \
		test -d "$$DIR" || mkdir -p "$$DIR"; \
	done

clean-local: python-clean
	@rm -rf $(top_srcdir)/documents/html
	@rm -rf $(top_srcdir)/validate.txt
	@rm -f $(top_srcdir)/gridlabd.spec
	@rm -f $(top_srcdir)/gridlabd.bin origin.txt
	@rm -rf $(top_srcdir)/build
	@(cd $(top_srcdir) ; build-aux/clean_dirs.sh)

distclean-local: python-clean
	@(cd $(top_srcdir) ; build-aux/clean_dirs.sh --depends)
	@rm -rf $(top_srcdir)/autom4te.cache
	@rm -f $(top_srcdir)/aclocal.m4
	@rm -f $(top_srcdir)/autoscan.log
	@rm -f $(top_srcdir)/m4/libtool.m4
	@rm -f $(top_srcdir)/ltoptions.m4
	@rm -f $(top_srcdir)/ltsugar.m4
	@rm -f $(top_srcdir)/ltversion.m4
	@rm -f $(top_srcdir)/lt~obsolete.m4
	@rm -f $(top_srcdir)/configure
	@rm -f $(top_srcdir)/config.log
	@rm -f $(top_srcdir)/config.status
	@rm -f $(top_srcdir)/gridlabd.spec
	@rm -f $(top_srcdir)/libtool
	@rm -f $(top_srcdir)/Makefile
	@rm -f $(top_srcdir)/Makefile.in
	@rm -f $(top_srcdir)/build-aux/ar-lib
	@rm -f $(top_srcdir)/build-aux/compile
	@rm -f $(top_srcdir)/build-aux/config.log
	@rm -f $(top_srcdir)/build-aux/config.guess
	@rm -f $(top_srcdir)/build-aux/config.sub
	@rm -f $(top_srcdir)/build-aux/install-sh
	@rm -f $(top_srcdir)/build-aux/ltmain.sh
	@rm -f $(top_srcdir)/build-aux/missing
	@rm -f $(top_srcdir)/build-aux/depcomp
	@rm -f $(top_srcdir)/gridlabd.spec
	@rm -f $(top_srcdir)/gridlabd.bin origin.txt
	@rm -rf $(top_srcdir)/build
	@rm -rf $(top_srcdir)/documents/html
	@rm -f $(top_srcdir)/validate.txt

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool > /dev/null

install-exec-hook: index python-requirements python-install docs
	@cp ${top_srcdir}/{COPYRIGHT,LICENSE} $(prefix)
	@echo ""
	@echo "Install complete. Here are some useful commands now:"
	@echo ""
	@echo "To include this version in your command path:"
	@echo "  export PATH=$(DESTDIR)$(bindir):$$PATH"
	@echo ""
	@echo "To run this version directly without using the command path:"
	@echo "  $(DESTDIR)$(bindir)/gridlabd"
	@echo ""
	@echo "To make this version the default for all users on this system:"
	@echo "  $(DESTDIR)$(bindir)/gridlabd version set"
	@echo ""

python-requirements: $(python_requirements)
	cat $(python_requirements) | python3 -m pip install -q -r /dev/stdin

reconfigure: distclean
	autoreconf -isf && ./configure --quiet

reconfigure-debug: distclean
	autoreconf -isf && ./configure --quiet 'CXXFLAGS=-O0 -g $(CXXFLAGS)'

docker: $(PREFIX)/docker.img

docker-debug: docker/Dockerfile Makefile
	docker build -f docker/Dockerfile -t $(PACKAGE)/$(PACKAGE_VERSION)-$(PACKAGE_BRANCH):latest --build-arg BRANCH=$(PACKAGE_ORIGIN) --build-arg RUN_VALIDATION=no docker
	@mkdir -p $(prefix)
	docker save $(PACKAGE)/$(PACKAGE_VERSION)-$(PACKAGE_BRANCH):latest -o $(prefix)/docker.img

$(PREFIX)/docker.img: docker/Dockerfile Makefile
	docker build -f docker/Dockerfile -t $(PACKAGE)/$(PACKAGE_VERSION)-$(PACKAGE_BRANCH):latest --build-arg BRANCH=$(PACKAGE_ORIGIN) --build-arg RUN_VALIDATION=yes docker
	@mkdir -p $(prefix)
	docker save $(PACKAGE)/$(PACKAGE_VERSION)-$(PACKAGE_BRANCH):latest -o $(prefix)/docker.img

system: install
	@echo "Setting $$(./build-aux/version.sh --install) to the current system version"
	@$(DESTDIR)$(bindir)/gridlabd version set
	@if [ ! "$$(gridlabd --version=install)" == "$$(./build-aux/version.sh --install)" ]; then \
		echo 'WARNING : build and install versions differ; run `make reconfigure` to fix this problem'; \
	fi

index: weather library template

html-local: module-html developer-html troubleshooting-html
	@mkdir -p $(prefix)/share/doc/developer
	@cp $(top_srcdir)/documents/index.html $(prefix)/share/doc/index.html
	@cp -R $(top_srcdir)/documents/html/* $(prefix)/share/doc/
	@cp -R $(top_srcdir)/gldcore/html/* $(prefix)/share/doc/developer/

module-html:
	@mkdir -p $(top_srcdir)/documents/html/modules
	@doxygen $(top_srcdir)/doxygen/gridlabd.conf

developer-html: # TODO
	@(cd $(top_srcdir)/gldcore/html ; natural_docs . ; cd - > /dev/null )

troubleshooting-html:
	@mkdir -p $(top_srcdir)/documents/html/troubleshooting/
	@(cd $(top_srcdir) ; gawk -f utilities/troubleshooting.awk */*.cpp > $(top_srcdir)/documents/html/troubleshooting/index.html )

documents/gridlabd.pdf: documents/gridlabd.glm
	(cd documents ; gridlabd gridlabd.glm)
	pandoc -V geometry:landscape documents/gridlabd.md -o documents/gridlabd.pdf

docs: $(docs_targets)

docs_toc:
	utilities/regen_toc >'docs/User manual/0 - Cover/1 - Contents.md'
