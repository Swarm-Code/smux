#!/bin/bash
# Test the ACTUAL hang scenario: creating a project in an active session

echo "===== TESTING ACTUAL HANG SCENARIO ====="
echo ""

# Clean slate
pkill -9 smux 2>/dev/null
sleep 1
rm -f /tmp/smux-debug-trace.log
rm -f /tmp/smux-1000/default*
rm -rf ~/.smux

echo "Step 1: Create initial session to keep server alive..."
timeout 15s /home/alejandro/Swarm/smux/smux new -s initial-session -d 2>&1
sleep 2

echo ""
echo "Step 2: Verify server is running..."
pgrep -a smux

echo ""
echo "Step 3: Check debug log for initial startup..."
tail -20 /tmp/smux-debug-trace.log

echo ""
echo "Step 4: Now attempt to create a NEW PROJECT (this might hang)..."
echo "  Running: smux new-project test-project-hang"
rm -f /tmp/smux-debug-trace.log  # Clear log to see just this command

# Monitor with timeout
(
    timeout 15s /home/alejandro/Swarm/smux/smux new-project test-project-hang 2>&1 | tee /tmp/new-project-result.log
) &
CMD_PID=$!
echo "  Command PID: $CMD_PID"

# Real-time monitoring
for i in {1..15}; do
    sleep 1

    if ! ps -p $CMD_PID > /dev/null 2>&1; then
        echo "  [$i s] Command completed"
        break
    fi

    echo "  [$i s] Still running... checking debug log..."

    # Show last few lines
    if [ -f /tmp/smux-debug-trace.log ]; then
        tail -3 /tmp/smux-debug-trace.log | sed 's/^/      /'
    fi

    # Check if it's stuck
    if [ $i -ge 5 ]; then
        echo "      ^^^ IF SAME LINES REPEATING = INFINITE LOOP/HANG"
    fi
done

wait $CMD_PID 2>/dev/null
EXIT_CODE=$?

echo ""
echo "Step 5: Results"
echo "  Exit code: $EXIT_CODE"
if [ -f /tmp/new-project-result.log ]; then
    echo "  Output:"
    cat /tmp/new-project-result.log | sed 's/^/    /'
fi

echo ""
echo "Step 6: Full debug log from new-project command:"
if [ -f /tmp/smux-debug-trace.log ]; then
    tail -200 /tmp/smux-debug-trace.log
else
    echo "  NO DEBUG LOG GENERATED"
fi

echo ""
echo "Step 7: Server processes still running?"
pgrep -a smux

echo ""
echo "===== END TEST ====="

# Cleanup
pkill -9 smux 2>/dev/null
