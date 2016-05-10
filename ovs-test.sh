#! /bin/sh
# Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
# All Rights Reserved.
#

if ! test -f vswitchd/vswitch.ovsschema; then
    echo "============================================="
    echo "Schema files are needed to compile this "
    echo "module and run the tests."
    echo "Please build ops-openvswitch first"
    echo "============================================="
    exit 1
fi
if ! test -d mybuild; then
	mkdir mybuild
fi
cd mybuild
if test -f Makefile; then
    echo "Cleanning previous makes"
    sleep 5
    make clean
fi
autoreconf --install --force ../configure.ac
../configure --build=x86_64-linux --host=x86_64-linux --target=x86_64-linux --prefix=/usr --exec_prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --libexecdir=/usr/lib/ops-openvswitch --datadir=/usr/share --sysconfdir=/etc --sharedstatedir=/com --localstatedir=/var --libdir=/usr/lib --includedir=/usr/include --oldincludedir=/usr/include --infodir=/usr/share/info --mandir=/usr/share/man --disable-silent-rules --disable-dependency-tracking --with-libtool-sysroot=/ws/lutza/ops-build/build/tmp/sysroots/genericx86-64 TARGET_PYTHON=/usr/bin/python TARGET_PERL=/usr/bin/perl --disable-static --enable-shared LIBS=-ljemalloc --enable-simulator-provider
make check TESTSUITEFLAGS='-j'$(nproc)
cd ../
rm -rf libltdl/
