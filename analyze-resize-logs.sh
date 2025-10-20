#!/bin/bash
# Resize Investigation Log Analysis Script
# Analyzes logs from smux resize instrumentation

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Smux Resize Investigation - Log Analysis                    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if logs exist
if [ ! -f "/tmp/smux_resize.log" ]; then
    echo "❌ ERROR: /tmp/smux_resize.log not found!"
    echo "   Run smux with Claude Code first to generate logs."
    exit 1
fi

echo "📊 Log Files Status:"
echo "─────────────────────────────────────────────────────────────────"
ls -lh /tmp/smux_*.log 2>/dev/null || echo "  No smux logs found"
echo ""

# Resize Events Analysis
echo "🔍 RESIZE EVENT ANALYSIS:"
echo "─────────────────────────────────────────────────────────────────"
RESIZE_COUNT=$(grep -c "PANE_RESIZE" /tmp/smux_resize.log 2>/dev/null || echo "0")
echo "  Total resize events: $RESIZE_COUNT"

if [ "$RESIZE_COUNT" -gt 0 ]; then
    echo ""
    echo "  First resize:"
    grep "PANE_RESIZE" /tmp/smux_resize.log | head -1 | sed 's/^/    /'

    echo ""
    echo "  Last resize:"
    grep "PANE_RESIZE" /tmp/smux_resize.log | tail -1 | sed 's/^/    /'

    echo ""
    echo "  Resize deltas (Y-axis changes):"
    grep "PANE_RESIZE" /tmp/smux_resize.log | awk '{print $7}' | sort | uniq -c | sed 's/^/    /'

    echo ""
    echo "  First 10 resize events:"
    grep "PANE_RESIZE" /tmp/smux_resize.log | head -10 | sed 's/^/    /'
fi

echo ""

# Scroll Events Analysis (if available)
if [ -f "/tmp/smux_scroll_trigger.log" ]; then
    echo "📜 SCROLL EVENT ANALYSIS:"
    echo "─────────────────────────────────────────────────────────────────"
    SCROLL_COUNT=$(grep -c "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log 2>/dev/null || echo "0")
    echo "  Total scroll events: $SCROLL_COUNT"

    if [ "$SCROLL_COUNT" -gt 0 ]; then
        # Calculate scroll rate
        START_TS=$(grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | head -1 | sed 's/.*\[\([0-9]*\).*/\1/')
        END_TS=$(grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | tail -1 | sed 's/.*\[\([0-9]*\).*/\1/')
        DURATION=$((END_TS - START_TS))

        if [ "$DURATION" -gt 0 ]; then
            SCROLL_RATE=$((SCROLL_COUNT / DURATION))
            echo "  Duration: ${DURATION}s"
            echo "  Scroll rate: ~${SCROLL_RATE} scrolls/second"
        fi

        echo ""
        echo "  Screen size changes (sy values):"
        grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | \
            awk '{for(i=1;i<=NF;i++) if($i~/^sy=/) print $i}' | \
            sort | uniq -c | sed 's/^/    /'
    fi

    echo ""
fi

# Correlation Analysis
if [ "$RESIZE_COUNT" -gt 0 ] && [ "$SCROLL_COUNT" -gt 0 ]; then
    echo "🔗 CORRELATION ANALYSIS (Resize ↔ Scroll):"
    echo "─────────────────────────────────────────────────────────────────"

    # Get first resize timestamp
    FIRST_RESIZE_TS=$(grep "PANE_RESIZE" /tmp/smux_resize.log | head -1 | sed 's/.*\[\([0-9.]*\)\].*/\1/')
    echo "  First resize at: $FIRST_RESIZE_TS"

    # Find scrolls around that time
    echo ""
    echo "  Scrolls around first resize:"
    grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | \
        awk -v ts="$FIRST_RESIZE_TS" '
            BEGIN {
                ts_num = ts + 0
                window = 1.0  # 1 second window
            }
            {
                # Extract timestamp from log line
                match($0, /\[([0-9.]+)\]/, arr)
                line_ts = arr[1] + 0

                if (line_ts >= ts_num - window && line_ts <= ts_num + window) {
                    print "    " $0
                }
            }
        ' | head -20

    echo ""
    echo "  Resize-to-scroll ratio: $((SCROLL_COUNT / (RESIZE_COUNT > 0 ? RESIZE_COUNT : 1))) scrolls per resize event"
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Analysis Complete                                            ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "📋 Next Steps:"
echo "  1. Review resize patterns above"
echo "  2. Check if resize count is high (>10 suggests root cause)"
echo "  3. Look for correlation between resize and scroll bursts"
echo "  4. Examine /tmp/smux_pty_raw.log for ANSI resize sequences"
echo ""
echo "🔍 Manual Investigation Commands:"
echo "  # View all resize events:"
echo "    cat /tmp/smux_resize.log"
echo ""
echo "  # View resize pattern with timestamps:"
echo "    grep PANE_RESIZE /tmp/smux_resize.log | awk '{print \$1, \$5, \$6, \$7}'"
echo ""
echo "  # Check for ANSI resize sequences in PTY raw data:"
echo "    grep -a '\\x1b\\[8' /tmp/smux_pty_raw.log || echo 'No ANSI resize sequences found'"
echo ""
