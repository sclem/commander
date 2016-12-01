ROOTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $ROOTDIR

make clean
make all

sudo su -c "source $ROOTDIR/dev/commander.env; $ROOTDIR/bin/commander"
