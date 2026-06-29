#!/bin/sh

if [ '!' -f "libcaj.a" -o '!' -f "libcaj.so" -o '!' -f "libcaj.so.1" ]; then
  echo "caj not made"
  exit 1
fi

PREFIX="$1"

if [ "a$PREFIX" = "a" ]; then
  PREFIX=/usr/local
fi

P="$PREFIX"
H="`hostname`"

if [ '!' -w "$P" ]; then
  echo "No write permissions to $P"
  exit 1
fi
if [ '!' -d "$P" ]; then
  echo "Not a valid directory: $P"
  exit 1
fi

instlib()
{
  if [ -e "$P/lib/$1" ]; then
    ln "$P/lib/$1" "$P/lib/.$1.cajinstold.$$.$H" || exit 1
  fi
  cp "$1" "$P/lib/.$1.cajinstnew.$$.$H" || exit 1
  mv "$P/lib/.$1.cajinstnew.$$.$H" "$P/lib/$1" || exit 1
  if [ -e "$P/lib/.$1.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/lib/.$1.cajinstold.$$.$H" || exit 1
  fi
}
instlib2()
{
  if [ -e "$P/lib/$1/$2" ]; then
    ln "$P/lib/$1/$2" "$P/lib/.$2.cajinstold.$$.$H" || exit 1
  fi
  cp "$1/$2" "$P/lib/.$2.cajinstnew.$$.$H" || exit 1
  mv "$P/lib/.$2.cajinstnew.$$.$H" "$P/lib/$2" || exit 1
  if [ -e "$P/lib/.$2.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/lib/.$2.cajinstold.$$.$H" || exit 1
  fi
}
instinc()
{
  if [ -e "$P/include/$1" ]; then
    ln "$P/include/$1" "$P/include/.$1.cajinstold.$$.$H" || exit 1
  fi
  cp "$1" "$P/include/.$1.cajinstnew.$$.$H" || exit 1
  mv "$P/include/.$1.cajinstnew.$$.$H" "$P/include/$1" || exit 1
  if [ -e "$P/include/.$1.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/include/.$1.cajinstold.$$.$H" || exit 1
  fi
}
instinc2()
{
  if [ -e "$P/include/$2" ]; then
    ln "$P/include/$2" "$P/include/.$2.cajinstold.$$.$H" || exit 1
  fi
  cp "$1/$2" "$P/include/.$2.cajinstnew.$$.$H" || exit 1
  mv "$P/include/.$2.cajinstnew.$$.$H" "$P/include/$2" || exit 1
  if [ -e "$P/include/.$2.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/include/.$2.cajinstold.$$.$H" || exit 1
  fi
}
instbin()
{
  if [ -e "$P/bin/$1" ]; then
    ln "$P/bin/$1" "$P/bin/.$1.cajinstold.$$.$H" || exit 1
  fi
  cp "$1" "$P/bin/.$1.cajinstnew.$$.$H" || exit 1
  mv "$P/bin/.$1.cajinstnew.$$.$H" "$P/bin/$1" || exit 1
  if [ -e "$P/bin/.$1.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/bin/.$1.cajinstold.$$.$H" || exit 1
  fi
}
instman()
{
  mkdir -p "$P/man/man$2" || exit 1
  cp "$1.$2" "$P/man/man$2/.$1.$2.cajinstnew.$$.$H" || exit 1
  mv "$P/man/man$2/.$1.$2.cajinstnew.$$.$H" "$P/man/man$2/$1.$2" || exit 1
}
instsym()
{
  if [ "`readlink "$P/lib/$1"`" != "libcaj.so.1" ]; then
    ln -s libcaj.so.1 "$P/lib/.$1.cajinstnew.$$.$H" || exit 1
    mv "$P/lib/.$1.cajinstnew.$$.$H" "$P/lib/$1" || exit 1
  fi
}
instsymcajun()
{
  if [ "`readlink "$P/lib/$1/$2"`" != "libcaj.so.1" ]; then
    ln -s libcajun.so.1 "$P/lib/.$1.cajinstnew.$$.$H" || exit 1
    mv "$P/lib/.$1.cajinstnew.$$.$H" "$P/lib/$1" || exit 1
  fi
}
instcajuninc()
{
  if [ -e "$P/include/cajun/$1" ]; then
    ln "$P/include/cajun/$1" "$P/include/cajun/.$1.cajinstold.$$.$H" || exit 1
  fi
  cp "cajun/$1" "$P/include/cajun/.$1.cajinstnew.$$.$H" || exit 1
  mv "$P/include/cajun/.$1.cajinstnew.$$.$H" "$P/include/cajun/$1" || exit 1
  if [ -e "$P/include/cajun/.$1.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/include/cajun/.$1.cajinstold.$$.$H" || exit 1
  fi
}
instexample3()
{
  if [ -e "$P/share/examples/caj/$2/$3" ]; then
    ln "$P/share/examples/caj/$2/$3" "$P/share/examples/caj/$2/.$3.cajinstold.$$.$H" || exit 1
  fi
  cp "$1/$3" "$P/share/examples/caj/$2/.$3.cajinstnew.$$.$H" || exit 1
  mv "$P/share/examples/caj/$2/.$3.cajinstnew.$$.$H" "$P/share/examples/caj/$2/$3" || exit 1
  if [ -e "$P/share/examples/caj/$2/.$3.cajinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/share/examples/caj/$2/.$3.cajinstold.$$.$H" || exit 1
  fi
}


mkdir -p "$P/bin"
instbin fast_json_pp
instman fast_json_pp 1
mkdir -p "$P/lib"
instlib libcaj.a
instlib libcaj.so.1
instlib2 cajun libcajun.a
instlib2 cajun libcajun.so.1
instsym libcaj.so
instsymcajun libcajun.so

mkdir -p "$P/include"
instinc caj.h
instinc pullcaj.h
instinc caj_out.h
if [ -e streamingatof/streamingatof.h ]; then
  instinc2 streamingatof streamingatof.h
fi
if [ -e prettyftoa/prettyftoa.h ]; then
  instinc2 prettyftoa prettyftoa.h
fi
mkdir -p "$P/include/cajun"
instcajuninc cajcontainerof.h
instcajuninc cajhdr.h
instcajuninc cajlikely.h
instcajuninc cajlinkedlist.h
instcajuninc cajmurmur.h
instcajuninc cajrbtree.h
instcajuninc cajunfrag.h
instcajuninc cajun.h

mkdir -p "$P/share/examples/caj"
mkdir -p "$P/share/examples/caj/caj"
mkdir -p "$P/share/examples/caj/pullcaj"
mkdir -p "$P/share/examples/caj/caj_out"
mkdir -p "$P/share/examples/caj/cajun"
mkdir -p "$P/share/examples/caj/cajunfrag"
instexample3 . caj main.c
echo "#!/bin/sh" > "$P/share/examples/caj/caj/build.sh"
echo 'cc `pkg-config caj --cflags` main.c `pkg-config caj --libs`' >> "$P/share/examples/caj/caj/build.sh"
chmod a+x "$P/share/examples/caj/caj/build.sh"
instexample3 . caj_out outmain.c
echo "#!/bin/sh" > "$P/share/examples/caj/caj_out/build.sh"
echo 'cc `pkg-config caj --cflags` outmain.c `pkg-config caj --libs`' >> "$P/share/examples/caj/caj_out/build.sh"
chmod a+x "$P/share/examples/caj/caj_out/build.sh"
instexample3 . pullcaj pullmain.c
echo "#!/bin/sh" > "$P/share/examples/caj/pullcaj/build.sh"
echo 'cc `pkg-config caj --cflags` pullmain.c `pkg-config caj --libs`' >> "$P/share/examples/caj/pullcaj/build.sh"
chmod a+x "$P/share/examples/caj/pullcaj/build.sh"
instexample3 cajun cajun cajuntest.c
echo "#!/bin/sh" > "$P/share/examples/caj/cajun/build.sh"
echo 'cc `pkg-config caj --cflags` cajuntest.c `pkg-config caj --libs`' >> "$P/share/examples/caj/cajun/build.sh"
chmod a+x "$P/share/examples/caj/cajun/build.sh"
instexample3 cajun cajunfrag cajunfragtest.c
echo "#!/bin/sh" > "$P/share/examples/caj/cajunfrag/build.sh"
echo 'cc `pkg-config caj --cflags` cajunfragtest.c `pkg-config caj --libs`' >> "$P/share/examples/caj/cajunfrag/build.sh"
chmod a+x "$P/share/examples/caj/cajunfrag/build.sh"

mkdir -p "$P/share/pkgconfig"
(echo "prefix=$P"; cat caj.pc|grep -v '^prefix=') > "$P/share/pkgconfig/caj.pc"

echo "All done, caj has been installed to $P"
