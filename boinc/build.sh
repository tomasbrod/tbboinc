test -x configure || ./_autosetup
./configure -C --disable-client --disable-fcgi --disable-manager
make -j4
