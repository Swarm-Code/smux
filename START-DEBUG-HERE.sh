#!/bin/bash

# 🎯 MASTER SMUX SCROLL DEBUG SETUP
# Everything you need to reproduce and analyze the Claude Code scrolling issue

set -e

cd /home/alejandro/Swarm/smux

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║   SMUX SCROLL DEBUG SETUP - Analyzing Claude Code Jitter      ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Validate environment
echo "✓ Step 1: Checking environment..."
if ! command -v strace &> /dev/null; then
    echo "⚠️  strace not found - system call tracing will be limited"
else
    echo "   strace available ✓"
fi

# Step 2: Initialize debug infrastructure
echo ""
echo "✓ Step 2: Initializing debug infrastructure..."
rm -f /tmp/smux_scroll_debug.log /tmp/smux_scroll_trace.log
touch /tmp/smux_scroll_debug.log
echo "   Debug log: /tmp/smux_scroll_debug.log"

# Step 3: Kill existing sessions
echo ""
echo "✓ Step 3: Cleaning up old sessions..."
for session in debug-scroll test-trace scroll-test; do
    ./smux kill-session -t "$session" 2>/dev/null || true
done
sleep 1

# Step 4: Create main debug session
echo ""
echo "✓ Step 4: Creating smux session..."
echo "   Creating 'debug-scroll' session (220x60)..."
./smux new-session -d -s "debug-scroll" -x 220 -y 60 \
    -c /home/alejandro/Swarm/smux \
    -e "DEBUG_MODE=1"

# Step 5: Split panes for monitoring
echo ""
echo "✓ Step 5: Setting up monitoring panes..."

# Split into left (output) and right (monitoring)
./smux split-window -t "debug-scroll" -h -l 110

# Right pane: Display header and wait for logs
./smux send-keys -t "debug-scroll:0.1" "clear; echo '═══════════════════════════════════════'; echo '📊 SCROLL DEBUG MONITOR'; echo '═══════════════════════════════════════'; echo ''; echo 'Waiting for scroll events...'; echo ''; tail -f /tmp/smux_scroll_debug.log" Enter

sleep 0.5

# Display instructions
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅ SETUP COMPLETE - READY TO DEBUG                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

echo "📋 QUICK START GUIDE:"
echo ""
echo "1️⃣  TERMINAL A - Attach to smux (LEFT pane for your commands):"
echo "   cd /home/alejandro/Smux/smux && ./smux attach -t debug-scroll"
echo ""
echo "2️⃣  TERMINAL B - Monitor TTY output in real-time:"
echo "   watch -n 0.1 'cat /tmp/smux_scroll_debug.log | tail -30'"
echo ""
echo "3️⃣  Generate rapid output in the LEFT pane (Terminal A):"
echo ""
echo "   Option A - Quick test (10 lines):"
echo "   seq 1 500 | while read i; do echo \"Line \$i: \"; sleep 0.01; done"
echo ""
echo "   Option B - Rapid burst (immediate):"
echo "   yes 'This is line with some content that should scroll and cause flicker' | head -100"
echo ""
echo "   Option C - Use Claude Code or paste large output"
echo ""
echo "4️⃣  Watch for flickering in Terminal A (left pane)"
echo ""
echo "5️⃣  Check the RIGHT pane for scroll events appearing in real-time"
echo ""
echo "6️⃣  When done, press Ctrl+C to stop. Check analysis:"
echo ""
echo "═════════════════════════════════════════════════════════════════"
echo ""
echo "🔍 DEBUG FILES:"
echo "   - /tmp/smux_scroll_debug.log      (Main event log)"
echo "   - /tmp/smux_scroll_trace.log      (System call trace)"
echo ""
echo "📊 ANALYSIS COMMANDS:"
echo "   tail -100 /tmp/smux_scroll_debug.log                    # View recent events"
echo "   grep '\\[SCROLL\\]' /tmp/smux_scroll_debug.log | wc -l   # Count scroll events"
echo "   grep '\\[SYNC' /tmp/smux_scroll_debug.log | head -20    # Check sync markers"
echo ""
echo "🎯 KEY METRICS TO WATCH:"
echo "   1. Scroll event frequency (should be < 100/sec)"
echo "   2. Sync markers (should wrap all scrolls)"
echo "   3. TTY buffer size (should stay < 65536)"
echo "   4. Redraw triggers (should be minimal, not per scroll)"
echo ""
echo "═════════════════════════════════════════════════════════════════"
echo ""

# Display current sessions
echo "📋 Current sessions:"
./smux list-sessions

echo ""
echo "🚀 Ready to reproduce the issue!"
echo ""
echo "Press ENTER to show session details:"
read

# Show what's running
./smux list-windows -t debug-scroll

