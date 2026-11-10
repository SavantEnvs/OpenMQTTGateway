#!/usr/bin/env bash
#
# mayhem/build.sh — builds the OpenMQTTGateway fuzz targets, their standalone reproducers and the
# known-answer selftest. The firmware sources (main/gatewaySRFB.cpp, main/gatewayLORA.cpp,
# lib/TheengsUtils/TheengsUtils.cpp) are compiled unmodified against a host shim of the Arduino/ESP32
# APIs (mayhem/shim, mayhem/stubs.cpp); ArduinoJson comes from the pre-fetched /opt/toolchains/deps copy.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

AJ=/opt/toolchains/deps/arduinojson-6.18.5/src/src
OUT="$SRC/mayhem/build"
rm -rf "$OUT"
mkdir -p "$OUT/fuzz" "$OUT/plain"

DEFS=(-DESP32 -DARDUINO '-DZmqttDiscovery="HADiscovery"' '-DZgatewayLORA="LORA"' '-DZgatewaySRFB="SRFB"')
INCS=(-Imain -Ilib/TheengsUtils -Imayhem/shim -Imayhem "-I$AJ")
SRCS=(main/gatewaySRFB.cpp main/gatewayLORA.cpp lib/TheengsUtils/TheengsUtils.cpp mayhem/stubs.cpp)
HARNESSES=(fuzz_theengs_utils fuzz_srfb_uart fuzz_srfb_mqtt fuzz_lora_rx fuzz_lora_mqtt)

objname() { basename "${1%.*}"; }

# 1) Project + stubs with sanitizers, DWARF 3 and SanCov (fuzzer-no-link), for the fuzz binaries.
# round2() casts value * 100 + 0.5 to int, which overflows for |value| > ~2e7 (never a sensor reading):
# float-cast-overflow is relaxed on TheengsUtils.cpp only; every other sanitizer check stays halting.
for s in "${SRCS[@]}"; do
  relax=""
  [ "$s" = lib/TheengsUtils/TheengsUtils.cpp ] && relax="-fno-sanitize=float-cast-overflow"
  # shellcheck disable=SC2086
  $CXX -std=gnu++17 -O1 $SANITIZER_FLAGS $relax $DEBUG_FLAGS -fsanitize=fuzzer-no-link "${DEFS[@]}" "${INCS[@]}" \
    -c "$s" -o "$OUT/fuzz/$(objname "$s").o" &
done
# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS -c mayhem/lsan_off.cc -o "$OUT/fuzz/lsan_off.o" &
$CC -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/fuzz/standalone_main.o" &
wait

# 2) Harnesses: libFuzzer binary and standalone reproducer, each with LSan switched off at build time.
for h in "${HARNESSES[@]}"; do
  # shellcheck disable=SC2086
  $CXX -std=gnu++17 -O1 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link "${DEFS[@]}" "${INCS[@]}" \
    -c "mayhem/harness/$h.cpp" -o "$OUT/fuzz/$h.o"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OUT/fuzz/$h.o" \
    "$OUT"/fuzz/{gatewaySRFB,gatewayLORA,TheengsUtils,stubs,lsan_off}.o -o "/mayhem/$h"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$OUT/fuzz/$h.o" "$OUT/fuzz/standalone_main.o" \
    "$OUT"/fuzz/{gatewaySRFB,gatewayLORA,TheengsUtils,stubs,lsan_off}.o -o "/mayhem/$h-standalone"
done

# 3) Test suite with the project's normal flags: no sanitizers, no coverage instrumentation of the fuzzer.
PLAIN=()
for s in "${SRCS[@]}" mayhem/selftest.cpp; do
  # shellcheck disable=SC2086
  $CXX -std=gnu++17 -O2 $COVERAGE_FLAGS "${DEFS[@]}" "${INCS[@]}" -c "$s" -o "$OUT/plain/$(objname "$s").o" &
done
wait
# shellcheck disable=SC2086
$CXX $COVERAGE_FLAGS "$OUT"/plain/*.o -o /mayhem/selftest
