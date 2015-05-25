#!/bin/bash


if [[ $# -eq 0 || "$1" = "-h" ]] ; then
  cat <<EOF
USAGE: `basename $0` [options]  (--filelist <filelist>|| --images image1 [image2 [image3 [image4 .. ] ] ] )

OPTIONS: 
  --32x32
  --Xx32
  --suffix <suffix>
  
EOF
exit
fi

i=0
while [ $i -le $# ] ; do
i=$((i+1))
  case $1 in
    --suffix)
       SUFFIX=$2
       shift 2
       ;;
    --32x32)
        OPT="-resize 32x32!"
        shift
        ;;
    --Xx32)
        OPT="-resize x32"
        shift
        ;;
    --filelist)
        FILELIST=$2
        shift 2
        ;;
    --images)
        FILELIST=/tmp/tmpfilelist
        rm -f  $FILELIST
        while [ $i -le $# ] ; do
          eval echo \$$i >> $FILELIST
          i=$((i+1))
        done
        ;;
    *)
        echo "unknown option: $1"
        exit
  esac
done


set -x
cat $FILELIST | (while read filename ; do convert $OPT $filename $filename.$SUFFIX ; done)
