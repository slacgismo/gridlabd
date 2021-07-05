export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

brew update || ruby -e "$(curl -fsSL https://raw.githubusercontent.com/HomeBrew/install/master/install)"
brew doctor

# build tools
brew install autoconf automake libtool gnu-sed gawk
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

# armadillo9 library
cd /tmp
curl -sL "http://sourceforge.net/projects/arma/files/armadillo-9.900.5.tar.xz" >armadillo-9.900.5.tar.xz
gunzip armadillo-9.900.5.tar.xz
tar xf armadillo-9.900.5.tar
cd armadillo-9.900.5
cmake .
make install
