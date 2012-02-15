#!/bin/sh
# $Id$

# Shell script for extracting subversion revision number and writing to specified header define file in specified folder.
# If folder is missing no file is written
# Author: Peter Nordin peter.nordin@liu.se
# Date:   2011-10-26
# For use in Hopsan, requires "subversion commandline" installed (apt-get install subversion)

filename="svnrevnum.h"
foldername="include"

writeFile()
{
  echo "//This file has been automatically generated by getSvnRevision.sh" > $1
  echo "#ifndef SVNREVNUM_H" >> $1
  echo "#define SVNREVNUM_H" >> $1
  if [ -z "$2" ]; then
    echo "//Revision information not found" >> $1
  else
    echo "#define SVNREVNUM \"$2\"" >> $1 
  fi
  echo "#endif" >> $1
}

# Get revision number from last change
if [ -x /usr/bin/svn ]; then
  rev=`svn info | grep "Last Changed Rev:" | cut -d" " -f4`
else
  rev=""
fi

#Check if dir exist, if so write the file
if [ -d $foldername ]; then
  writeFile "$foldername/$filename" $rev
fi 

# Determine what to report and what exit code to give
if [ -z "$rev" ]; then
  echo "Revision information not found"
  exit -1
else 
  echo $rev
  exit 0
fi

