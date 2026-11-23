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

##############################
#        Build RDKLogger     #
##############################
echo "======================================================================================"
echo "buliding RDKLogger for coverity"
export

cd ${GITHUB_WORKSPACE}
aclocal -I cfg
libtoolize --automake
autoheader
automake --foreign --add-missing
CONFIGURE_TMP="/tmp/rdklogger-configure.$$"
autoconf -f -o "${CONFIGURE_TMP}" configure.ac
test -s "${CONFIGURE_TMP}"
cp "${CONFIGURE_TMP}" ./configure
rm -f "${CONFIGURE_TMP}"
export ac_cv_func_malloc_0_nonnull=yes
export ac_cv_func_memset=yes
sh ./configure "--prefix=${INSTALL_PREFIX}"
make
make install
echo "======================================================================================"
exit 0
