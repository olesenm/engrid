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
# This script compiles engrid with the OpenFOAM paraview libraries.

usage() {
    while [ "$#" -ge 1 ]; do echo "$1"; shift; done
    cat<<USAGE

usage: ${0##*/} [OPTION]
options:
  -help

* compile engrid with the OpenFOAM paraview libraries

The engridFoam script (provided with OpenFOAM) can be used to invoke
engrid with paraview libraries from OpenFOAM ThirdParty

USAGE
    exit 1
}

#------------------------------------------------------------------------------

# parse options
while [ "$#" -gt 0 ]
do
    case "$1" in
    -h | -help)
        usage
        ;;
    *)
        usage "unknown option/argument: '$*'"
        ;;
    esac
done

# run from the src/ directory which is the parent of this directory
cd "${0%/*}/.." || exit 1

#------------------------------------------------------------------------------

# if needed, set MAJOR version to correspond to VERSION
# ParaView_MAJOR is "<digits>.<digits>" from ParaView_VERSION
case "${ParaView_VERSION:-unknown}" in
"${ParaView_MAJOR}.*" )
    # version and major appear to correspond
    ;;

*)
    ParaView_MAJOR=$(echo $ParaView_VERSION | sed -e 's/^\([0-9][0-9]*\.[0-9][0-9]*\).*$/\1/')
    ;;
esac
export ParaView_MAJOR


bindir="$ParaView_DIR/bin"
libdir="$ParaView_DIR/lib/paraview-$ParaView_MAJOR"

[ -d "$ParaView_DIR" ] || usage "ParaView_DIR not found ($ParaView_DIR)"
[ -d $bindir ] || echo "paraview binary not found - could mean something is wrong"
[ -d $libdir ] || usage "paraview libraries not found"

export LD_LIBRARY_PATH=$libdir:$LD_LIBRARY_PATH

# report what is happening
echo "==================================================="
echo "compile engrid with the OpenFOAM paraview libraries"
echo "==================================================="
echo
set -x
qmake engrid.pro.OpenFOAM-paraview && make && make install

# ----------------------------------------------------------------- end-of-file
