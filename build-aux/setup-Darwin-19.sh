#!/bin/bash
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

brew update || ruby -e "$(curl -fsSL https://raw.githubusercontent.com/HomeBrew/install/master/install)"
brew doctor

# build tools
brew install autoconf automake libtool gnu-sed gawk armadillo
[ ! -L /usr/local/bin/sed -o ! "$(readlink /usr/local/bin/sed)" == "/usr/local/bin/gsed" ] && mv /usr/local/bin/sed /usr/local/bin/sed-old
[ ! -e /usr/local/bin/sed ] && ln -s /usr/local/bin/gsed /usr/local/bin/sed
[ ! -e /usr/local/bin/libtoolize ] && ln -s /usr/local/bin/glibtoolize /usr/local/bin/libtoolize

# install python3
brew install python3

# docs generators
brew install mono
brew install naturaldocs
ln -s /usr/local/bin/naturaldocs /usr/local/bin/natural_docs
brew install doxygen

# influxdb
brew install influxdb
brew services start influxdb

# subversion cli
brew install svn
