echo "centos"
export DISTRIBUTION="centos"
source scl_source enable devtoolset-9

performCoverageMeasuring()
{
   echo "Perform centos coverage measuring"
   # cd /git/casual && \
   # mkdir -p /git/casual/coverage && \
   # gcovr -r . --html --html-details --gcov-exclude='.*thirdparty.*|.*test_.*' -o coverage/index.html
}

