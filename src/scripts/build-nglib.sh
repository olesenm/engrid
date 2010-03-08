#!/usr/bin/env bash
#
# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# +                                                                      +
# + This file is part of enGrid.                                         +
# +                                                                      +
# + Copyright 2008,2009 Oliver Gloth                                     +
# +                                                                      +
# + enGrid is free software: you can redistribute it and/or modify       +
# + it under the terms of the GNU General Public License as published by +
# + the Free Software Foundation, either version 3 of the License, or    +
# + (at your option) any later version.                                  +
# +                                                                      +
# + enGrid is distributed in the hope that it will be useful,            +
# + but WITHOUT ANY WARRANTY; without even the implied warranty of       +
# + MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        +
# + GNU General Public License for more details.                         +
# +                                                                      +
# + You should have received a copy of the GNU General Public License    +
# + along with enGrid. If not, see <http:#www.gnu.org/licenses/>.        +
# +                                                                      +
# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# DESCRIPTION:
# This script checks out or updates the netgen source code and creates
# the static netgen library.

# run from the src/ directory which is the parent of this directory
cd "${0%/*}/.." || exit 1

package=netgen-mesher
(
    echo "Working directory = $PWD"
    cd netgen_svn || exit 1

    if [ -d netgen-mesher/.svn ]
    then
        echo "updating NETGEN from SVN repository (sourceforge.net) -- please wait"
        svn up $package
    else
        echo "downloading NETGEN from SVN repository (sourceforge.net) -- please wait"
        svn co https://netgen-mesher.svn.sourceforge.net/svnroot/$package $package
    fi

    if [ "$1" = win ]
    then
        echo
        echo "adapting nglib.h for static library on windows"
        echo
        sed -i \
            -e 's/__declspec(dllexport)//' \
            -e 's/__declspec(dllimport)//' \
            ./netgen-mesher/netgen/nglib/nglib.h
    fi

    echo
    echo "starting qmake for $package"
    echo

    qmake || exit 1

    echo
    echo "making $package"
    echo

    make || exit 1
)

# ----------------------------------------------------------------- end-of-file
