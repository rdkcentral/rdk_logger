#!/bin/bash
set -e
set -x

##############################
#      Setup WorkSpace       #
##############################
GITHUB_WORKSPACE="${PWD}"
export INSTALL_PREFIX="${PWD}/install"
export LD_LIBRARY_PATH="${PWD}/install/lib"
export PKG_CONFIG_PATH="${PWD}/install/lib/pkgconfig"
ls -la ${GITHUB_WORKSPACE}
cd ${GITHUB_WORKSPACE}

#######################################
#  Install Dependencies and packages  #
#######################################
apt update && apt install -y libcurl4-openssl-dev libgtest-dev lcov gcovr libmsgpack* build-essential

wget https://sourceforge.net/projects/log4c/files/log4c/1.2.3/log4c-1.2.3.tar.gz
tar -xzf log4c-1.2.3.tar.gz
cd "log4c-1.2.3"
./configure "--prefix=${INSTALL_PREFIX}"
make
make install

