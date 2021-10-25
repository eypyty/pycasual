#! /usr/bin/env bash

SCRIPT_PATH=$( dirname $0)
. $SCRIPT_PATH/build_common.sh

if (( $# < 4 ))
then
   usage
   exit -1
fi

export CASUAL_VERSION="$1"
export CASUAL_RELEASE="$2"
export BRANCH="$3"
export BUILD_TYPE="$4"


# OSX or LINUX
os=$( uname -s)

if [[ $os == 'Linux' ]];
then
    eval $( grep ^ID= /etc/os-release )

    case "$ID" in
        centos)
            echo "centos"
            DISTRIBUTION="centos"
            ;;
        opensuse)
            echo "opensuse"
            DISTRIBUTION="opensuse"
            ;;
    esac

else
    DISTRIBUTION="darwin"
fi

export CASUAL_MAKE_BUILD_VERSION=$( total_version )

echo "CASUAL_VERSION="${CASUAL_VERSION}
echo "CASUAL_RELEASE="${CASUAL_RELEASE}
echo "BRANCH="${BRANCH}
echo "DISTRIBUTION="${DISTRIBUTION}
echo "CASUAL_MAKE_BUILD_VERSION="${CASUAL_MAKE_BUILD_VERSION}
echo "BUILD_TYPE="${BUILD_TYPE}


performCoverageMeasuring()
{
   echo "Perform no coverage measuring"
   # cd /git/casual && \
   # mkdir -p /git/casual/coverage && \
   # gcovr -r . --html --html-details --gcov-exclude='.*thirdparty.*|.*test_.*' -o coverage/index.html
}

#source scl_source enable devtoolset-9


execute /Users/hbergk/repo