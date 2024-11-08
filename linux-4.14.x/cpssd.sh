#
export DEBUG_INFO=D_ON
export APP_DEMO_ONLY=YES
#
./build_cpss.sh INTEL64 DX_ALL CUST NOLUA UTF_NO
#
cd        compilation_root
rm  -f    cpssd    cpssd.* libcpss.*
#
find .    -name *.o | grep -v libs | xargs ar q libcpss.a
ranlib                     libcpss.a
nm  -g -A --defined-only   libcpss.a >libcpss.nm
#
gcc -g -o cpssd ../cpssd.c libcpss.a $WG_CPSSD_STATIC -lm -lpthread -lrt -ldl -Xlinker -Map -Xlinker cpssd.map
