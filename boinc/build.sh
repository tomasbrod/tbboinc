test -x configure || ./_autosetup
./configure -C --disable-client --enable-fcgi --disable-manager
make -j4
