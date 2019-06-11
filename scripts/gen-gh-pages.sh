#!/bin/bash

#=============================================================================
# Copyright 2014-2019 Istituto Italiano di Tecnologia (IIT)
#   Authors: Daniele E. Domenichelli <daniele.domenichelli@iit.it>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of YCM, substitute the full
#  License text for the above reference.)


if [ $# -gt 0 ]; then
    echo "Usage: $(basename $0)"
fi

if [ ! -f CMakeLists.txt ]; then
    echo "You must run this script in icub-tests main dir"
    exit 1
fi

grep -q "project(iCub-Tests)" CMakeLists.txt
if [ $? -ne 0 ]; then
    echo "You must run this script in icub-tests main dir"
    exit 1
fi

git branch -q -f gh-pages
git checkout -q gh-pages
rm -Rf doxygen/doc
(cd doxygen && doxygen Doxyfile)

git add doxygen/doc
git commit -q -m "Generate documentation"
git checkout -q - || exit 1

echo
echo "Finished. You can now push with"
echo
echo "     git push --force origin gh-pages"
echo
