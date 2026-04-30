#!/bin/bash
set -euo pipefail

# Compare local compile time vs distributed client compile time.
# Requires server already running.

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT" || exit 1

NUM_FILES=${NUM_FILES:-20}
RUNS=${RUNS:-3}
FUNC_COUNT=${FUNC_COUNT:-3000}
SWITCH_CASES=${SWITCH_CASES:-5000}
WORK_DIR=${WORK_DIR:-"$PROJECT_ROOT/data/speedup_test"}
USER_ROLE_INPUT=${USER_ROLE_INPUT:-2}  # 2 = ROLE_USER
USER_ID=${USER_ID:-1}                 # Valid user id

mkdir -p "$WORK_DIR"
rm -f "$WORK_DIR"/*.c "$WORK_DIR"/*.o

# Generate compile-heavy C files (many functions + large switch)
for i in $(seq 1 "$NUM_FILES"); do
    file="$WORK_DIR/test_$i.c"
    cat > "$file" <<EOF
#include <stdint.h>
#include <stdio.h>

#define FUNC_COUNT ${FUNC_COUNT}
#define SWITCH_CASES ${SWITCH_CASES}

static inline uint64_t mix(uint64_t x) {
    x ^= x >> 23;
    x *= 0x2127599bf4325c37ULL;
    x ^= x >> 47;
    return x;
}

EOF

    for j in $(seq 1 "$FUNC_COUNT"); do
        echo "static uint64_t f_${j}(uint64_t x) { return mix(x + ${j}ULL); }" >> "$file"
    done

    cat >> "$file" <<'EOF'
static uint64_t dispatch(int k, uint64_t x) {
    switch (k) {
EOF

    for j in $(seq 1 "$SWITCH_CASES"); do
        idx=$(( (j - 1) % FUNC_COUNT + 1 ))
        echo "        case ${j}: return f_${idx}(x + ${j}ULL);" >> "$file"
    done

    cat >> "$file" <<EOF
        default: return x;
    }
}

int main(void) {
    uint64_t x = 0;
    for (int i = 0; i < 1000; i++) {
        x = dispatch(i % SWITCH_CASES, x + (uint64_t)i);
    }
    printf("%llu\n", (unsigned long long)x);
    return 0;
}
EOF
done

total_local=0
total_dist=0

for run in $(seq 1 "$RUNS"); do
        echo "Run $run/$RUNS"

        # Local serial compile
        rm -f "$WORK_DIR"/*.o
        LOCAL_TIME_FILE="/tmp/local_compile_time.txt"
        /usr/bin/time -f "%e" -o "$LOCAL_TIME_FILE" bash -c '
set -e
for f in "$0"/*.c; do
    gcc -c "$f" -o "${f%.c}.o"
done
' "$WORK_DIR"
        LOCAL_TIME=$(cat "$LOCAL_TIME_FILE")

        # Distributed compile via client
        rm -f "$WORK_DIR"/*.o
        DIST_TIME_FILE="/tmp/dist_compile_time.txt"
        /usr/bin/time -f "%e" -o "$DIST_TIME_FILE" bash -c '
set -e
printf "%s\n%s\n" "$0" "$1" | ./client "$2"/*.c
' "$USER_ROLE_INPUT" "$USER_ID" "$WORK_DIR"
        DIST_TIME=$(cat "$DIST_TIME_FILE")

        echo "  Local: $LOCAL_TIME s"
        echo "  Dist : $DIST_TIME s"

        total_local=$(awk -v a="$total_local" -v b="$LOCAL_TIME" 'BEGIN { printf "%.6f", a + b }')
        total_dist=$(awk -v a="$total_dist" -v b="$DIST_TIME" 'BEGIN { printf "%.6f", a + b }')
done

avg_local=$(awk -v t="$total_local" -v n="$RUNS" 'BEGIN { if (n > 0) printf "%.6f", t / n; else print "0" }')
avg_dist=$(awk -v t="$total_dist" -v n="$RUNS" 'BEGIN { if (n > 0) printf "%.6f", t / n; else print "0" }')
speedup=$(awk -v l="$avg_local" -v d="$avg_dist" 'BEGIN { if (d > 0) printf "%.3f", l/d; else print "inf" }')

echo "Average local compile time (s): $avg_local"
echo "Average distributed compile time (s): $avg_dist"
echo "Average speedup (local/dist): $speedup"
