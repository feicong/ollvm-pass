#!/bin/bash

s=1
use_ll_in_xcode=1
use_ll_in_clang=0
new_args=()
CONF=Release
while [[ $# -gt 0 ]]; do
    case "$1" in
        -target) 
            TARGET="$2"
            new_args+=("$1" "$2")
            shift 2
            ;;
        -isysroot)
            SYSROOT="$2"
            new_args+=("$1" "$2")
            shift 2
            ;;
        -c)
            if [ $use_ll_in_xcode -eq 1 ]; then 
                new_args+=("-emit-llvm" "-S")
            else
                new_args+=("-c")
            fi
            shift 1
            ;;
        -o)
            OUTPUT="$2"
            new_args+=("$1" "$2")
            shift 2
            ;;
        -O*)
            OPTLEVEL="$1"
            new_args+=("$1")
            shift 1
            ;;
        *) 
            new_args+=("$1")
            shift 1
            ;;
    esac
done
set -- "${new_args[@]}"

function log_run() {
    local cmd="$*"
    echo $cmd
    eval $cmd
    r=$?
    if [ $r -ne 0 ]; then
        exit -1
    fi
    return $r
}

XCODE_CLANG=$SYSROOT/../../../../../Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
echo XCODE_CLANG $XCODE_CLANG
OPT=~/data/ollvm/build_llvm19/bin/opt # TODO: set to desired value
SLLVM=~/Documents/Tool/sllvm_r1/build19/sllvm19.dylib # TODO: set to desired value

echo CONF $CONF
echo target $TARGET
echo sysroot $SYSROOT
echo clang $XCODE_CLANG
echo output $OUTPUT

if [ $use_ll_in_xcode -eq 1 ]; then
    out_tmp_1=$OUTPUT.1.ll
else
    out_tmp_1=$OUTPUT.1.bc
fi
if [ $use_ll_in_clang -eq 1 ]; then
    out_tmp_2=$OUTPUT.2.ll
    out_ext_2="-S"
else
    out_tmp_2=$OUTPUT.2.bc
fi

log_run $XCODE_CLANG $@
log_run cp -f $OUTPUT $out_tmp_1    
if [ ! x$OPTLEVEL = x-O0 ]; then
    out_ext_2="$out_ext_2 -load-pass-plugin $SLLVM -passes all"
fi
log_run $OPT $out_ext_2 -o $out_tmp_2 $out_tmp_1
log_run $XCODE_CLANG -target $TARGET -c $OPTLEVEL -o $OUTPUT $out_tmp_2

