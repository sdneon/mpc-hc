#!/bin/sh

echo "$(pwd)" | grep -q '[[:blank:]]' &&
  echo "Out of tree builds are impossible with whitespace in source path." && exit 1

if [ "${4}" == "VS2015" ]; then
  bin_folder=bin15
else
  bin_folder=bin
fi

if [ "${1}" == "x64" ]; then
  arch=x86_64
  archdir=x64
  cross_prefix=x86_64-w64-mingw32-
  lav_folder=LAVFilters64
  mpc_hc_folder=mpc-hc_x64
else
  arch=x86
  archdir=Win32
  cross_prefix=
  lav_folder=LAVFilters
  mpc_hc_folder=mpc-hc_x86
fi

if [ "${2}" == "Debug" ]; then
  FFMPEG_DLL_PATH=$(readlink -f ../../..)/${bin_folder}/${mpc_hc_folder}_Debug/${lav_folder}
  BASEDIR=$(pwd)/src/bin_${archdir}d
  cross_prefix=
  COMPILER=MSVC
else
  FFMPEG_DLL_PATH=$(readlink -f ../../..)/${bin_folder}/${mpc_hc_folder}/${lav_folder}
  BASEDIR=$(pwd)/src/bin_${archdir}
  COMPILER=GCC
fi

THIRDPARTYPREFIX=${BASEDIR}/thirdparty
FFMPEG_BUILD_PATH=${THIRDPARTYPREFIX}/ffmpeg
FFMPEG_LIB_PATH=${BASEDIR}/lib
NUMBER_OF_PROCESSORS=4

make_dirs() {
  mkdir -p ${FFMPEG_LIB_PATH}
  mkdir -p ${FFMPEG_BUILD_PATH}
  mkdir -p ${FFMPEG_DLL_PATH}
}

CV2PDB_PATH=$(readlink -f ../../..)/build/cv2pdb.exe

copy_libs() {
  # install -s --strip-program=${cross_prefix}strip lib*/*-lav-*.dll ${FFMPEG_DLL_PATH}
  cp lib*/*-lav-*.dll ${FFMPEG_DLL_PATH}
  if [ "${COMPILER}" == "GCC" ]; then
    #${cross_prefix}strip ${FFMPEG_DLL_PATH}/*-lav-*.dll
    ${CV2PDB_PATH} ${FFMPEG_DLL_PATH}/avcodec-lav-58.dll
    ${CV2PDB_PATH} ${FFMPEG_DLL_PATH}/avfilter-lav-7.dll
    ${CV2PDB_PATH} ${FFMPEG_DLL_PATH}/avformat-lav-58.dll
    ${CV2PDB_PATH} ${FFMPEG_DLL_PATH}/avresample-lav-4.dll
    ${CV2PDB_PATH} ${FFMPEG_DLL_PATH}/avutil-lav-56.dll
    ${CV2PDB_PATH} ${FFMPEG_DLL_PATH}/swscale-lav-5.dll
  fi
  cp -u lib*/*.lib ${FFMPEG_LIB_PATH}
}

clean() {
  cd ${FFMPEG_BUILD_PATH}
  echo Cleaning...
  if [ -f ffbuild/config.mak ]; then
    rm -r ${FFMPEG_BUILD_PATH}
  fi
  cd ${BASEDIR}
}

configure() {
  OPTIONS="
    --enable-shared                 \
    --disable-static                \
    --enable-gpl                    \
    --enable-version3               \
    --enable-w32threads             \
    --disable-demuxer=matroska      \
    --disable-filters               \
    --enable-filter=scale,yadif,w3fdif \
    --disable-protocol=async,cache,concat,httpproxy,icecast,md5,subfile \
    --disable-muxers                \
    --enable-muxer=spdif            \
    --disable-bsfs                  \
    --enable-bsf=extract_extradata,vp9_superframe_split \
    --disable-cuda                  \
    --disable-cuda-llvm             \
    --disable-cuvid                 \
    --disable-nvenc                 \
    --disable-mediafoundation       \
    --enable-avresample             \
    --enable-avisynth               \
    --disable-avdevice              \
    --disable-postproc              \
    --disable-swresample            \
    --disable-encoders              \
    --disable-devices               \
    --disable-programs              \
    --disable-doc                   \
    --build-suffix=-lav             \
    --arch=${arch}"

  if [ "${COMPILER}" == "GCC" ]; then
    OPTIONS="${OPTIONS}             \
    --enable-libdav1d               \
    --enable-libspeex               \
    --enable-libopencore-amrnb      \
    --enable-libopencore-amrwb      \
    --disable-debug                 \
    --disable-schannel              \
    --enable-gnutls                 \
    --enable-libxml2                \
    --disable-stripping             \
    --enable-gmp"
  fi
  
  EXTRA_LDFLAGS=""
  PKG_CONFIG_PREFIX_DIR=""
  TOOLCHAIN=""
  if [ "${arch}" == "x86_64" ]; then
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:../../../thirdparty/64/lib/pkgconfig/"
    if [ "${COMPILER}" == "MSVC" ]; then
      OPTIONS="${OPTIONS} --enable-debug"
      EXTRA_CFLAGS="-D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -Zo -GS-"
      EXTRA_CFLAGS="${EXTRA_CFLAGS} -I../../../thirdparty/64/include -I../../../../../../thirdparty/zlib -MDd"
      EXTRA_LDFLAGS="${EXTRA_LDFLAGS} -LIBPATH:../../../thirdparty/64/lib -LIBPATH:../../../../../../../bin/lib/Debug_x64 -NODEFAULTLIB:libcmt"
      TOOLCHAIN="--toolchain=msvc"
    else
      OPTIONS="${OPTIONS} --enable-cross-compile --cross-prefix=${cross_prefix} --target-os=mingw32 --pkg-config=pkg-config"
      EXTRA_CFLAGS="-fno-tree-vectorize -D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -gdwarf-2 -fno-omit-frame-pointer"
      EXTRA_CFLAGS="${EXTRA_CFLAGS} -I../../../thirdparty/64/include"
      EXTRA_LDFLAGS="${EXTRA_LDFLAGS} -L../../../thirdparty/64/lib"
    fi
    PKG_CONFIG_PREFIX_DIR="--define-variable=prefix=../../../thirdparty/64"
  else
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:../../../thirdparty/32/lib/pkgconfig/"
    if [ "${COMPILER}" == "MSVC" ]; then
      OPTIONS="${OPTIONS} --enable-debug"
      EXTRA_CFLAGS="-D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -Zo -GS-"
      EXTRA_CFLAGS="${EXTRA_CFLAGS} -I../../../thirdparty/32/include -I../../../../../../thirdparty/zlib -MDd"
      EXTRA_LDFLAGS="${EXTRA_LDFLAGS} -LIBPATH:../../../thirdparty/32/lib -LIBPATH:../../../../../../../bin/lib/Debug_Win32 -NODEFAULTLIB:libcmt"
      TOOLCHAIN="--toolchain=msvc"
    else
      OPTIONS="${OPTIONS} --cpu=i686 --target-os=mingw32"
      EXTRA_CFLAGS="-fno-tree-vectorize -D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -gdwarf-2 -fno-omit-frame-pointer"
      EXTRA_CFLAGS="${EXTRA_CFLAGS} -I../../../thirdparty/32/include -mmmx -msse -msse2 -mfpmath=sse -mstackrealign"
      EXTRA_LDFLAGS="${EXTRA_LDFLAGS} -L../../../thirdparty/32/lib"
    fi
    PKG_CONFIG_PREFIX_DIR="--define-variable=prefix=../../../thirdparty/32"
  fi

  echo tc=${TOOLCHAIN}
  sh ../../../ffmpeg/configure ${TOOLCHAIN} --x86asmexe=yasm --extra-ldflags="${EXTRA_LDFLAGS}" --extra-cflags="${EXTRA_CFLAGS}" --pkg-config-flags="--static ${PKG_CONFIG_PREFIX_DIR}" ${OPTIONS}
}

build() {
  echo Building...
  make -j$NUMBER_OF_PROCESSORS 2>&1 | tee make.log
  ## Check the return status and the log to detect possible errors
  [ ${PIPESTATUS[0]} -eq 0 ] && ! grep -q -F "rerun configure" make.log
}

configureAndBuild() {
  cd ${FFMPEG_BUILD_PATH}
  ## Don't run configure again if it was previously run
  if [ ../../../ffmpeg/configure -ot ffbuild/config.mak ] &&
     [ ../../../../build_ffmpeg.sh -ot ffbuild/config.mak ]; then
    echo Skipping configure...
  else
    echo Configuring...

    ## In case of out-of-tree build this directory is created too late
    mkdir -p ffbuild
    ## run configure, redirect to file because of a msys bug
    configure > ffbuild/config.out 2>&1
    CONFIGRETVAL=$?

    ## show configure output
    cat ffbuild/config.out
  fi

  ## Only if configure succeeded, actually build
  if [ ${CONFIGRETVAL} -eq 0 ]; then
    build &&
    copy_libs
    CONFIGRETVAL=$?
  fi
  cd ${BASEDIR}
}

echo Building ffmpeg in ${COMPILER} ${arch} ${2} config...

make_dirs

CONFIGRETVAL=0

if [ "${3}" == "Clean" ]; then
  clean
  CONFIGRETVAL=$?
else
  ## Check if configure was previously run
  if [ -f ffbuild/config.mak ]; then
    CLEANBUILD=0
  else
    CLEANBUILD=1
  fi

  configureAndBuild

  ## In case of error and only if we didn't start with a clean build,
  ## we try to rebuild from scratch including a full reconfigure
  if [ ! ${CONFIGRETVAL} -eq 0 ] && [ ${CLEANBUILD} -eq 0 ]; then
    echo Trying again with forced reconfigure...
    clean && configureAndBuild
  fi
fi

exit ${CONFIGRETVAL}
