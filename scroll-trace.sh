#!/bin/bash

# Real-time Scroll Event Tracer
# Uses strace + pattern matching to capture scroll-related system calls

OUTPUT_FILE="/tmp/smux_scroll_trace.log"
SMUX_PID=""

echo "🔍 SMUX Scroll Event Tracer"
echo "============================"
echo ""

# Find smux server PID
SMUX_PID=$(pgrep -f "smux.*server" | head -1)

if [ -z "$SMUX_PID" ]; then
    echo "⚠️  No smux server running. Starting one..."
    cd /home/alejandro/Swarm/smux
    ./smux new-session -d -s test-trace -x 200 -y 50
    sleep 1
    SMUX_PID=$(pgrep -f "smux.*server" | head -1)
fi

if [ -z "$SMUX_PID" ]; then
    echo "❌ Could not find smux server PID"
    exit 1
fi

echo "📌 Smux server PID: $SMUX_PID"
echo "🎯 Capturing scroll-related events..."
echo ""
echo "Output file: $OUTPUT_FILE"
echo ""

> "$OUTPUT_FILE"

# Run strace on smux server and capture relevant syscalls
# We're looking for:
# - write() calls (output to terminal)
# - ioctl() calls (terminal control)
# - poll()/select() (event timing)

strace -p "$SMUX_PID" \
    -e trace=write,ioctl,pwrite64,writev \
    -o "$OUTPUT_FILE" \
    -f -ff -s 256 \
    2>&1 &

STRACE_PID=$!

echo "🟢 Tracing active (PID: $STRACE_PID)"
echo ""
echo "Instructions:"
echo "1. In another terminal, attach to smux:"
echo "   cd /home/alejandro/Swarm/smux && ./smux attach -t test-trace"
echo ""
echo "2. Trigger rapid output (copy-paste lots of text or run)"
echo "   seq 1 100 | while read i; do echo \"Line $i: $(date) Lorem ipsum dolor sit amet\"; done"
echo ""
echo "3. When flickering occurs, press Ctrl+C here to stop tracing"
echo ""
echo "Watching output:"
tail -f "$OUTPUT_FILE" | head -1000

kill $STRACE_PID 2>/dev/null || true

echo ""
echo "✅ Trace complete. Analysis:"
echo ""

# Simple analysis
if [ -s "$OUTPUT_FILE" ]; then
    write_count=$(grep -c "^write" "$OUTPUT_FILE" || echo 0)
    ioctl_count=$(grep -c "^ioctl" "$OUTPUT_FILE" || echo 0)

    echo "Write syscalls: $write_count"
    echo "Ioctl syscalls: $ioctl_count"
    echo ""
    echo "Full output saved to: $OUTPUT_FILE"
    echo ""
    echo "Sample write calls:"
    grep "^write" "$OUTPUT_FILE" | head -10
fi

