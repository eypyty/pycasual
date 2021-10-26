
if ! type realpath > /dev/null 2>&1
then
    realpath()
    {
        local repo_root="$1"
        local working_dir=$(pwd)

        if echo $repo_root | grep ^/ > /dev/null
        then
            echo -n $repo_root
        else
            echo -n $working_dir/$repo_root
        fi
    }
fi

usage()
{
    echo "usage: $0 source_root version release branch build_type"
}

isRelease()
{
    if [[ ${BRANCH} == release/* ]]
    then
        return 0
    else
        return -1
    fi
}

selectExtendedVersion()
{
    if ( isRelease)
    then
      echo -n ""
    fi
    echo -n "-"${CASUAL_RELEASE}
}


total_version()
{
    echo -n ${CASUAL_VERSION}$( selectExtendedVersion)
}

execute()
{
    local SOURCE_ROOT="$( dirname $( realpath $1))"
    echo "SOURCE_ROOT="$SOURCE_ROOT
    export CASUAL_REPOSITORY_ROOT=${SOURCE_ROOT}/casual
    export CASUAL_TOOLS_HOME=$CASUAL_REPOSITORY_ROOT
    export CASUAL_MAKE_HOME=$CASUAL_REPOSITORY_ROOT/../casual-make/source
    export PYTHONPATH=$CASUAL_MAKE_HOME:$CASUAL_REPOSITORY_ROOT/make
    export CASUAL_BUILD_HOME=$CASUAL_REPOSITORY_ROOT
    export CASUAL_MAKE_OPTIONAL_INCLUDE_PATHS="${CASUAL_THIRDPARTY}/googletest/include"
    export CASUAL_MAKE_OPTIONAL_LIBRARY_PATHS="${CASUAL_THIRDPARTY}/googletest/bin"
    export CASUAL_HOME=/opt/casual
    export PATH=$CASUAL_MAKE_HOME:$CASUAL_BUILD_HOME/middleware/tools/bin:$CASUAL_TOOLS_HOME/bin:$CASUAL_HOME/bin:$PATH
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CASUAL_HOME/lib:$CASUAL_BUILD_HOME/middleware/buffer/bin:$CASUAL_BUILD_HOME/middleware/configuration/bin:$CASUAL_BUILD_HOME/middleware/domain/bin:$CASUAL_BUILD_HOME/middleware/event/bin:$CASUAL_BUILD_HOME/middleware/service/bin:$CASUAL_BUILD_HOME/middleware/common/bin:$CASUAL_BUILD_HOME/middleware/serviceframework/bin:$CASUAL_BUILD_HOME/middleware/xatmi/bin:$CASUAL_BUILD_HOME/middleware/queue/bin:$CASUAL_BUILD_HOME/middleware/transaction/bin:/usr/local/lib

    if [[ ${BUILD_TYPE} == "docker" ]]
    then
        umask 000
        echo "running docker build type"
        find $CASUAL_REPOSITORY_ROOT -name "*.gc*" -print | xargs rm > /dev/null
        find $CASUAL_REPOSITORY_ROOT -name "report.xml" -print | xargs rm > /dev/null
    fi

    rm $CASUAL_REPOSITORY_ROOT/middleware/.casual/unittest/.singleton/.domain-singleton > /dev/null
    cd $CASUAL_REPOSITORY_ROOT && \
        casual-make --stat --no-colors clean && \
        casual-make --stat --quiet --no-colors compile && \
        casual-make --stat --no-colors link && \
        casual-make --no-colors test --gtest_color=no --gtest_output='xml:report.xml' && \
        mkdir -p $CASUAL_REPOSITORY_ROOT/coverage && \
        casual-make --no-colors clean && \
        casual-make --no-colors --quiet compile && \
        casual-make --no-colors link && \
        casual-make --no-colors install && \
        casual-make nginx && \
        cp $CASUAL_REPOSITORY_ROOT/package/casual-middleware.spec /root/rpmbuild/SPECS/. && \
        rpmbuild -bb --noclean --define "casual_version $CASUAL_VERSION" --define "casual_release $CASUAL_RELEASE" --define "distribution $DISTRIBUTION" /root/rpmbuild/SPECS/casual-middleware.spec  && \
        cp /root/rpmbuild/RPMS/x86_64/casual-middleware*.rpm $CASUAL_REPOSITORY_ROOT/. && \
        performCoverageMeasuring && \
        cd $CASUAL_REPOSITORY_ROOT && \
        casual-make --no-colors clean
    returncode=$?

    if [[ ${BUILD_TYPE} == "docker" ]]
    then
        cd ${SOURCE_ROOT}/casual/
        find $CASUAL_REPOSITORY_ROOT/.. -user root | xargs chmod 777
    fi

    return $returncode
}