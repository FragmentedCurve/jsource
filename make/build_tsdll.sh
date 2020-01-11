#!/bin/bash
# $1 is j32 or j64
# jvars.sh exports CC as gcc or clang

cd ~

macmin="-mmacosx-version-min=10.6"

if [ $CC = "gcc" ] ; then
# gcc
common="-Werror -fPIC -O2 -fwrapv -fno-strict-aliasing -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-clobbered -Wno-empty-body -Wno-unused-value -Wno-pointer-sign -Wno-parentheses -Wno-type-limits"
GNUC_MAJOR=$(echo __GNUC__ | $CC -E -x c - | tail -n 1)
GNUC_MINOR=$(echo __GNUC_MINOR__ | $CC -E -x c - | tail -n 1)
if [ $GNUC_MAJOR -ge 5 ] ; then
common="$common -Wno-maybe-uninitialized"
else
common="$common -DC_NOMULTINTRINSIC -Wno-uninitialized"
fi
if [ $GNUC_MAJOR -ge 6 ] ; then
common="$common -Wno-shift-negative-value"
fi
# alternatively, add comment /* fall through */
if [ $GNUC_MAJOR -ge 7 ] ; then
common="$common -Wno-implicit-fallthrough"
fi
if [ $GNUC_MAJOR -ge 8 ] ; then
common="$common -Wno-cast-function-type"
fi
else
# clang 3.4
common="-Werror -fPIC -O2 -fwrapv -fno-strict-aliasing -Wextra -Wno-consumed -Wno-uninitialized -Wno-unused-parameter -Wno-sign-compare -Wno-empty-body -Wno-unused-value -Wno-pointer-sign -Wno-parentheses -Wno-unsequenced -Wno-string-plus-int -Wno-tautological-constant-out-of-range-compare"
# clang 3.8
CLANG_MAJOR=$(echo __clang_major__ | $CC -E -x c - | tail -n 1)
CLANG_MINOR=$(echo __clang_minor__ | $CC -E -x c - | tail -n 1)
if [ $CLANG_MAJOR -eq 3 ] && [ $CLANG_MINOR -ge 8 ] ; then
common="$common -Wno-pass-failed"
else
if [ $CLANG_MAJOR -ge 4 ] ; then
common="$common -Wno-pass-failed"
fi
fi
# clang 10
if [ $CLANG_MAJOR -ge 10 ] ; then
common="$common -Wno-implicit-float-conversion"
fi
fi
darwin="-fPIC -O2 -fwrapv -fno-strict-aliasing -Wno-string-plus-int -Wno-empty-body -Wno-unsequenced -Wno-unused-value -Wno-pointer-sign -Wno-parentheses -Wno-return-type -Wno-constant-logical-operand -Wno-comment -Wno-unsequenced -Wno-pass-failed"

case $jplatform\_$1 in

linux_j32)
TARGET=libtsdll.so
COMPILE="$common -m32 "
LINK=" -shared -Wl,-soname,libtsdll.so  -m32 -o libtsdll.so -lm "
;;
linux_j64)
TARGET=libtsdll.so
COMPILE="$common "
LINK=" -shared -Wl,-soname,libtsdll.so -o libtsdll.so -lm "
;;
raspberry_j32)
TARGET=libtsdll.so
COMPILE="$common -marm -march=armv6 -mfloat-abi=hard -mfpu=vfp"
LINK=" -shared -Wl,-soname,libtsdll.so -o libtsdll.so -lm "
;;
raspberry_j64)
TARGET=libtsdll.so
COMPILE="$common -march=armv8-a+crc "
LINK=" -shared -Wl,-soname,libtsdll.so -o libtsdll.so -lm "
;;
darwin_j32)
TARGET=libtsdll.dylib
COMPILE="$darwin -m32 $macmin"
LINK=" -m32 $macmin -dynamiclib -o libtsdll.dylib -lm "
;;
darwin_j64)
TARGET=libtsdll.dylib
COMPILE="$common $macmin"
LINK=" $macmin -dynamiclib -o libtsdll.dylib -lm "
;;
*)
echo no case for those parameters
exit
esac

OBJS="tsdll.o "
export OBJS COMPILE LINK TARGET
$jmake/domake.sh $1

