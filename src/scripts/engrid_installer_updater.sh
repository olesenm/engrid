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
# This script allows:
# -downloading, building and installing the dependencies for engrid
# -downloading, building and installing engrid
# -updating netgen
# -updating engrid
# -rebuilding engrid
# -generating an environment setup script that can then be sourced by ~/.bashrc
# -generating a startup script that sets up the environment and then starts engrid
#
# USAGE:
# First of all change the configuration file engrid_installer_updater.cfg according to your needs.
# Then you can run this script and choose the actions you wish to execute. Multiple actions can be run at once. They will be run in the order of the checklist.
# Note 1: Altough it should be enough to run create_bash_engrid once, it's recommended to run it every time to make sure the other actions use the correct environment.
#
# EXAMPLES
# Engrid installation:
# [X] create_bash_engrid
# [X] install_QT
# [X] install_VTK
# [X] install_CGNS
# [X] build_engrid
# [ ] update_netgen
# [ ] update_engrid
# [ ] rebuild_engrid
# [X] create_start_engrid
#
# Engrid update:
# [X] create_bash_engrid
# [ ] install_QT
# [ ] install_VTK
# [ ] install_CGNS
# [ ] build_engrid
# [X] update_netgen
# [X] update_engrid
# [ ] rebuild_engrid
# [ ] create_start_engrid
#
# Engrid rebuild:
# [X] create_bash_engrid
# [ ] install_QT
# [ ] install_VTK
# [ ] install_CGNS
# [ ] build_engrid
# [ ] update_netgen
# [ ] update_engrid
# [X] rebuild_engrid
# [ ] create_start_engrid

#for debugging
set -eux

. "${0%/*}/engrid_installer_updater.cfg"

VTKINCDIR=$VTKPREFIX/include/vtk-$VTKVERSION
VTKLIBDIR=$VTKPREFIX/lib/vtk-$VTKVERSION

CGNSINCDIR=$CGNSPREFIX/include
CGNSLIBDIR=$CGNSPREFIX/lib

create_bash_engrid()
{
  echo "Create bash_engrid"
  mkdir -p $BINPREFIX

  echo "#!/usr/bin/env bash" > $BINPREFIX/$ENV_SETUP
  echo "export VTKINCDIR=$VTKINCDIR" >> $BINPREFIX/$ENV_SETUP
  echo "export VTKLIBDIR=$VTKLIBDIR" >> $BINPREFIX/$ENV_SETUP
  echo "export LD_LIBRARY_PATH=$VTKLIBDIR:\$LD_LIBRARY_PATH" >> $BINPREFIX/$ENV_SETUP

  echo "export CGNSINCDIR=$CGNSINCDIR" >> $BINPREFIX/$ENV_SETUP
  echo "export CGNSLIBDIR=$CGNSLIBDIR" >> $BINPREFIX/$ENV_SETUP
  echo "export LD_LIBRARY_PATH=$CGNSLIBDIR:\$LD_LIBRARY_PATH" >> $BINPREFIX/$ENV_SETUP

  echo "export PATH=$QTPREFIX/bin:\$PATH" >> $BINPREFIX/$ENV_SETUP
  echo "export QTDIR=$QTPREFIX" >> $BINPREFIX/$ENV_SETUP
  echo "export LD_LIBRARY_PATH=$QTPREFIX/lib:\$LD_LIBRARY_PATH" >> $BINPREFIX/$ENV_SETUP

  chmod 755 $BINPREFIX/$ENV_SETUP
}

create_start_engrid()
{
  echo "Create start_engrid"
  mkdir -p $BINPREFIX

  echo "#!/usr/bin/env bash" > $BINPREFIX/$START_ENGRID
  echo ". $BINPREFIX/$ENV_SETUP" >> $BINPREFIX/$START_ENGRID
  echo "$SRCPREFIX/engrid/src/engrid" >> $BINPREFIX/$START_ENGRID

  chmod 755 $BINPREFIX/$START_ENGRID
}

install_QT()
{
  echo "Install QT"
  if [ $DOWNLOAD_QT = 1 ]; then wget $URL_QT; fi
  tar -xzvf ./$ARCHIVE_QT
  (
    cd ${ARCHIVE_QT%.tar.gz} || exit 1
    mkdir -p $QTPREFIX
    # echo yes | ./configure --prefix=$QTPREFIX -opensource
    echo yes | ./configure --prefix=$QTPREFIX -opensource -nomake examples -nomake demos -nomake docs -no-webkit -no-phonon -no-phonon-backend -no-qt3support -no-accessibility -silent

    make && make install
  )
}

install_VTK()
{
  echo "Install VTK"
  if [ $DOWNLOAD_VTK = 1 ]; then wget $URL_VTK; fi
  tar -xzvf ./$ARCHIVE_VTK
  (
    cd VTK
    mkdir -p $VTKPREFIX
    cmake -DCMAKE_INSTALL_PREFIX:PATH=$VTKPREFIX -DBUILD_SHARED_LIBS:BOOL=ON -DVTK_USE_GUISUPPORT:BOOL=ON -DVTK_USE_QVTK:BOOL=ON -DDESIRED_QT_VERSION:STRING=4  .
    chmod 644 Utilities/vtktiff/tif_fax3sm.c
    make && make install
  )
}

install_CGNS()
{
  echo "Install CGNS"
  if [ $DOWNLOAD_CGNS = 1 ]; then wget $URL_CGNS; fi
  tar -xzvf ./$ARCHIVE_CGNS
  (
    cd cgnslib_2.5/
    mkdir -p $CGNSPREFIX/include
    mkdir -p $CGNSPREFIX/lib
    ./configure --prefix=$CGNSPREFIX && make && make install
  )
}

build_engrid()
{
  (
    mkdir -p $SRCPREFIX
    cd $SRCPREFIX
    git clone $URL_ENGRID
    cd engrid/src
    if [ $BRANCH != "master" ]
    then
        git checkout -b $BRANCH origin/$BRANCH
    fi;
    echo "Build netgen"
    ./scripts/build-nglib.sh
    echo "Build enGrid"
    qmake $PROJECT_FILE && make $MAKEOPTIONS
  )
}

update_netgen()
{
  (
    cd $SRCPREFIX/engrid/src
    echo "Update netgen"
    ./scripts/build-nglib.sh
  )
}

update_engrid()
{
  (
    cd $SRCPREFIX/engrid/src
    echo "Update enGrid"
    git pull
    qmake $PROJECT_FILE && make $MAKEOPTIONS
  )
}

rebuild_engrid()
{
  (
    cd $SRCPREFIX/engrid/src
    qmake && make distclean && qmake $PROJECT_FILE && make $MAKEOPTIONS
  )
}


if type zenity >/dev/null 2>&1
then
    # using zenity for this script
    select=$(
    zenity \
    --height=350 --list --text "Which actions should be executed?" \
    --checklist --column "Run" --column "Actions" \
        FALSE "create_bash_engrid" \
        FALSE "install_QT" \
        FALSE "install_VTK" \
        FALSE "install_CGNS" \
        FALSE "build_engrid" \
        FALSE "update_netgen" \
        FALSE "update_engrid" \
        FALSE "rebuild_engrid" \
        FALSE "create_start_engrid" \
        --separator=":"
    )
    exit 1
elif type kdialog >/dev/null 2>&1
then
    # using kdialog for this script
    select=$(
    kdialog \
    --title "engrid installer" \
    --checklist "Which actions should be executed?" \
        create_bash_engrid  create_bash_engrid  off \
        install_QT          install_QT          off \
        install_VTK         install_VTK         off \
        install_CGNS        install_CGNS        off \
        build_engrid        build_engrid        off \
        update_netgen       update_netgen       off \
        update_engrid       update_engrid       off \
        rebuild_engrid      rebuild_engrid      off \
        create_start_engrid create_start_engrid off \
        ;
    )
else
    echo "need zenity or kdialog for this script"
    exit 1
fi

echo $select

if ( echo $select | grep -w create_bash_engrid ) then create_bash_engrid; fi

set +u
. $BINPREFIX/$ENV_SETUP
set -u

if ( echo $select | grep -w install_QT ) then install_QT; fi;
if ( echo $select | grep -w install_VTK ) then install_VTK; fi;
if ( echo $select | grep -w install_CGNS ) then install_CGNS; fi;
if ( echo $select | grep -w build_engrid ) then build_engrid; fi;
if ( echo $select | grep -w update_netgen ) then update_netgen; fi;
if ( echo $select | grep -w update_engrid ) then update_engrid; fi;
if ( echo $select | grep -w rebuild_engrid ) then rebuild_engrid; fi;
if ( echo $select | grep -w create_start_engrid ) then create_start_engrid; fi;

echo Done
exit 0

# ----------------------------------------------------------------- end-of-file
