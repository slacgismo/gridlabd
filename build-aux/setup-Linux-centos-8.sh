#!/bin/bash

## US English ##
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8
export LC_COLLATE=C
export LC_CTYPE=en_US.UTF-8

# Install needed system tools
yum -q update -y ; 
yum -q clean all
yum -q groupinstall "Development Tools" -y
yum -q install cmake -y 
yum -q install ncurses-devel -y
yum -q install epel-release -y
yum -q install curl-devel -y
yum -q install which -y
yum -q install svn -y

# Install armadillo
yum install -q epel-release -y 
yum install -q 'dnf-command(config-manager)' -y
yum config-manager --set-enabled PowerTools
yum install -q armadillo-devel -y

# python3 support needed as of 4.2
if [ ! -x /usr/local/bin/python3 -o $(/usr/local/bin/python3 --version) != "Python 3.9.0" ]; then
	yum -q install openssl-devel bzip2-devel libffi-devel zlib-devel -y
	cd /usr/local/src
	curl https://www.python.org/ftp/python/3.9.0/Python-3.9.0.tgz | tar xz
	cd Python-3.9.0
	./configure --prefix=/usr/local --enable-optimizations --with-system-ffi --with-computed-gotos --enable-loadable-sqlite-extensions CFLAGS="-fPIC"
	make -j $(nproc)
	make altinstall
	ln -sf /usr/local/bin/python3.9 /usr/local/bin/python3
	ln -sf /usr/local/bin/python3.9-config /usr/local/bin/python3-config
	ln -sf /usr/local/bin/pydoc3.9 /usr/local/bin/pydoc
	ln -sf /usr/local/bin/idle3.9 /usr/local/bin/idle
	ln -sf /usr/local/bin/pip3.9 /usr/local/bin/pip3
	/usr/local/bin/python3 pip -m install mysql-connector matplotlib numpy pandas Pillow
fi

# latex
if [ ! -x /usr/bin/tex ]; then
	yum -q install tex -y
fi

# doxygen
if [ ! -x /usr/bin/doxygen ]; then
	if [ ! -d /usr/local/src/doxygen ]; then
		git clone https://github.com/doxygen/doxygen.git /usr/local/src/doxygen --depth 1
	fi
	if [ ! -d /usr/local/src/doxygen/build ]; then
		mkdir /usr/local/src/doxygen/build
	fi
	cd /usr/local/src/doxygen/build
	cmake -G "Unix Makefiles" ..
	make
	make install
fi

# mono
if [ ! -f /usr/bin/mono ]; then
	rpmkeys --import "http://pool.sks-keyservers.net/pks/lookup?op=get&search=0x3fa7e0328081bff6a14da29aa6a19b38d3d831ef"
	curl -s https://download.mono-project.com/repo/centos8-stable.repo | tee /etc/yum.repos.d/mono-centos8-stable.repo
	yum -q install mono-devel -y
fi

# natural_docs
if [ ! -x /usr/local/bin/natural_docs ]; then
	cd /usr/local
	curl -s https://www.naturaldocs.org/download/natural_docs/2.0.2/Natural_Docs_2.0.2.zip > natural_docs.zip
	unzip -qq natural_docs
	rm -f natural_docs.zip
	mv Natural\ Docs natural_docs
	echo '#!/bin/bash
mono /usr/local/natural_docs/NaturalDocs.exe \$*' > /usr/local/bin/natural_docs
	chmod a+x /usr/local/bin/natural_docs
fi

