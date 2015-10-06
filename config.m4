PHP_ARG_ENABLE(sass, [whether to enable sass extension],
[  --enable-sass      Enable sass extension], yes, yes)

if test "$PHP_SASS" != "no"; then
  PHP_ADD_LIBRARY_WITH_PATH(sass, lib/libexec, LIBSASS_SHARED_LIBADD)

  LDFLAGS="-lsass -L"`pwd`"/lib/libsass/lib/ -lstdc++"

  # manually extract sass version as the non-autotool'ed
  # sass_version.h that comes with libsass doesn't contain
  # the version
  sass_version=$(cd lib/libsass && ./version.sh)

  if test "x$sass_version" != "x" ; then
      # escape hell. This is what we need to pass to the
      #  compiler: -DLIBSASS_VERSION="\"'3.2.5\""
      CFLAGS="-DLIBSASS_VERSION=\"\\\"$sass_version\\\"\" $CFLAGS"
      CXXFLAGS="-DLIBSASS_VERSION=\"\\\"$sass_version\\\"\" $CXXFLAGS"
  fi

  PHP_NEW_EXTENSION(sass, src/sass.c src/utilities.c, $ext_shared)
fi
