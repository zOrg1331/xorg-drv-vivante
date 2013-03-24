#! /bin/sh

export PREFIX=/usr
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig
export ACLOCAL="aclocal -I $PREFIX/share/aclocal"
export LD_LIBRARY_PATH=$PREFIX/lib
export PATH=$PREFIX/bin:$PATH

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
autoreconf --force --install --verbose "$srcdir"
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
