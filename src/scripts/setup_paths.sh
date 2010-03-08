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
# This is a convenience script to set up environment variables for
# compiling engrid on different system configurations.

# Check if all parameters are present
# If no, exit
if [ $# -ne 2 ]
then
    echo "usage :"
    echo "source ${0##*/} CONFIGURATION ENABLE_CGNS"
    echo "CONFIGURATION = engits / debian / ubuntu / opensuse / debian_etch"
    echo "ENABLE_CGNS = yes / no"
    exit 0
fi

case "$1" in
debian)
    export VTKLIBDIR=/usr/lib/
    export VTKINCDIR=/usr/include/vtk-5.0/
    ;;

debian_etch | etch)
    export PATH=/usr/local/Trolltech/Qt-4.5.1/bin:$PATH
    export QTDIR=/usr/local/Trolltech/Qt-4.5.1/
    export LD_LIBRARY_PATH=/usr/local/Trolltech/Qt-4.5.1/lib:$LD_LIBRARY_PATH
    export VTKLIBDIR=/usr/lib/
    export VTKINCDIR=/usr/include/vtk-5.0/
    ;;

ubuntu)
    export VTKLIBDIR=/usr/lib/
    export VTKINCDIR=/usr/include/vtk-5.0/
    ;;

opensuse | suse)
    export VTKLIBDIR=/usr/lib64
    export VTKINCDIR=/usr/include/vtk
fi

engits)    #enGits local system
    export QTDIR=/opt/shared/Qt/4.5.1/debug
    export VTKLIBDIR=/opt/shared/VTK/lib/vtk-5.4/
    export VTKINCDIR=/opt/shared/VTK/include/vtk-5.4/
    if [ "$2" = yes ]
    then
        export CGNSINCDIR=/opt/shared/cgns/include/
        export CGNSLIBDIR=/opt/shared/cgns/lib/
    fi
    # compiler and other build tools
    export PATH=/usr/bin:/opt/shared/Qt/4.5.1/debug/bin:$PATH
    export PATH=/opt/shared/OpenFOAM/ThirdParty/gcc-4.3.1/platforms/linux64/bin:$PATH
    export LD_LIBRARY_PATH=/opt/shared/OpenFOAM/ThirdParty/gcc-4.3.1/platforms/linux64/lib:$LD_LIBRARY_PATH
    ;;

*)
    echo "unknown system specified: '$1'"
    exit 1
    ;;
esac

export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$VTKLIBDIR:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$CGNSLIBDIR:$LD_LIBRARY_PATH

# ----------------------------------------------------------------- end-of-file
