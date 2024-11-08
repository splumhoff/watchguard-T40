#
cd $(dirname $0)
cd ../..
ROOT=$(pwd)
#
if [ -z "$USE_PLATFORM" ]; then
USE_PLATFORM="spokane"
fi
#
fake() {
#
while            [ "$1" ]; do
#
mkdir  -p $(dirname $1)
#
if [ ! -e          "$1" ]; then
echo "touch         $1"
      touch         $1
fi
#
if [   -s          "$1" ]; then
echo "exist         $1"
fi
#
shift
done
}
#
# Create dummy .ko files needed by wg_linux
#
cd  $ROOT/components/kernel/exports/$USE_PLATFORM/modules
#
# fake net/dsa/wg_dsa.ko
