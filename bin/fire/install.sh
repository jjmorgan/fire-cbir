#!/bin/bash

DSTPATH=$1
IMGPATH=$2


ARCH=`uname -p`

# -------------------- settings --------------------
FIREFILE=firev2.1.0.tgz
FIREURL=http://www-i6.informatik.rwth-aachen.de/~deselaers/file/$FIREFILE
APACHEFILE=httpd-2.0.54.tar.gz
APACHEURL=http://apache.mirrorplus.org/httpd/$APACHEFILE


# -------------------- DESTINATION --------------------
mkdir $DSTPATH

cd $DSTPATH
wget $FIREURL

tar -xvzf $FIREFILE


wget $APACHEURL
tar -xvzf $APACHEFILE

# -------------------- BUILD APACHE --------------------

cd httpd-2.0.54
./configure --prefix=$DSTPATH/apache


# -------------------- BUILD FIRE --------------------
