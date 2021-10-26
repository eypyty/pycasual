#! /usr/bin/env bash

SCRIPT_PATH=$( dirname $0)
. $SCRIPT_PATH/build_common.sh

if (( $# < 5 ))
then
   usage
   exit -1
fi

export SOURCE_ROOT="$1"
export CASUAL_VERSION="$2"
export CASUAL_RELEASE="$3"
export BRANCH="$4"
export BUILD_TYPE="$5"


# OSX or LINUX
os=$( uname -s)

if [[ $os == 'Linux' ]];
then
    eval $( grep ^ID= /etc/os-release )

    case "$ID" in
        centos)
            source ${SCRIPT_PATH}/build_centos.sh
            ;;
        opensuse)
            source ${SCRIPT_PATH}/build_suse.sh
            ;;
        ubuntu)
            echo "ubuntu"
            source ${SCRIPT_PATH}/build_ubuntu.sh
            ;;

    esac

else
    source ${SCRIPT_PATH}/build_osx.sh
fi

export CASUAL_MAKE_BUILD_VERSION=$( total_version )

echo "CASUAL_VERSION="${CASUAL_VERSION}
echo "CASUAL_RELEASE="${CASUAL_RELEASE}
echo "BRANCH="${BRANCH}
echo "DISTRIBUTION="${DISTRIBUTION}
echo "CASUAL_MAKE_BUILD_VERSION="${CASUAL_MAKE_BUILD_VERSION}
echo "BUILD_TYPE="${BUILD_TYPE}

execute ${SOURCE_ROOT}