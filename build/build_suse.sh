echo "suse"
export DISTRIBUTION="suse"

performCoverageMeasuring()
{
   echo "Perform suse coverage measuring"
   # cd /git/casual && \
   # mkdir -p /git/casual/coverage && \
   # gcovr -r . --html --html-details --gcov-exclude='.*thirdparty.*|.*test_.*' -o coverage/index.html
}

