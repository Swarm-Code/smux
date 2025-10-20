#!/bin/bash

# SMUX Scroll Event Real-Time Monitor
# This script launches smux and monitors scroll events in real-time

set -e

DEBUG_LOG="/tmp/smux_scroll_debug.log"
STATS_FILE="/tmp/smux_scroll_stats.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Initialize files
> "$DEBUG_LOG"
> "$STATS_FILE"

echo -e "${CYAN}=== SMUX SCROLL EVENT MONITOR ===${NC}"
echo -e "${CYAN}Debug log: $DEBUG_LOG${NC}"
echo -e "${CYAN}Stats file: $STATS_FILE${NC}"
echo ""

# Function to analyze debug log in real-time
analyze_log() {
    local last_line=0
    local scroll_count=0
    local sync_nesting=0
    local last_redraw_time=0

    while true; do
        if [ -f "$DEBUG_LOG" ]; then
            # Get new lines since last read
            local current_lines=$(wc -l < "$DEBUG_LOG" || echo 0)

            if [ "$current_lines" -gt "$last_line" ]; then
                # Get new entries
                tail -n +$((last_line + 1)) "$DEBUG_LOG" | while read -r line; do
                    if [[ "$line" =~ \[SCROLL\] ]]; then
                        echo -e "${RED}[SCROLL]${NC} $line" | sed 's/\[SCROLL\] //'
                        ((scroll_count++))
                    elif [[ "$line" =~ \[SYNC_START\] ]]; then
                        echo -e "${GREEN}[SYNC_START]${NC} $line" | sed 's/\[SYNC_START\] //'
                        ((sync_nesting++))
                    elif [[ "$line" =~ \[SYNC_END\] ]]; then
                        echo -e "${GREEN}[SYNC_END]${NC} $line" | sed 's/\[SYNC_END\] //'
                        ((sync_nesting--))
                    elif [[ "$line" =~ \[REDRAW\] ]]; then
                        echo -e "${YELLOW}[REDRAW]${NC} $line" | sed 's/\[REDRAW\] //'
                    elif [[ "$line" =~ \[BUFFER\] ]]; then
                        echo -e "${BLUE}[BUFFER]${NC} $line" | sed 's/\[BUFFER\] //'
                    else
                        echo "$line"
                    fi
                done

                last_line=$current_lines
            fi
        fi

        sleep 0.1
    done
}

# Start log analyzer in background
analyze_log &
ANALYZER_PID=$!

# Cleanup function
cleanup() {
    echo -e "\n${CYAN}=== MONITOR STOPPED ===${NC}"
    kill $ANALYZER_PID 2>/dev/null || true

    # Print summary statistics
    if [ -f "$DEBUG_LOG" ]; then
        echo -e "${CYAN}=== DEBUG LOG SUMMARY ===${NC}"
        local scroll_events=$(grep -c "\[SCROLL\]" "$DEBUG_LOG" || echo 0)
        local sync_starts=$(grep -c "\[SYNC_START\]" "$DEBUG_LOG" || echo 0)
        local sync_ends=$(grep -c "\[SYNC_END\]" "$DEBUG_LOG" || echo 0)
        local redraws=$(grep -c "\[REDRAW\]" "$DEBUG_LOG" || echo 0)

        echo "Total Scroll Events: $scroll_events"
        echo "Sync Start Events: $sync_starts"
        echo "Sync End Events: $sync_ends"
        echo "Redraw Events: $redraws"
        echo ""
        echo "Full debug log saved to: $DEBUG_LOG"
    fi
}

trap cleanup EXIT

# Start smux server if not running
echo -e "${CYAN}Starting smux server...${NC}"
cd /home/alejandro/Swarm/smux

# Create a new smux session
./smux new-session -d -s "debug-session" -x 200 -y 50 -c /tmp

# Create a second window
./smux new-window -t "debug-session" -n "monitor"

# Show session details
echo -e "${CYAN}Smux session started. PID tracking enabled.${NC}"
echo -e "${CYAN}Session name: debug-session${NC}"
echo ""
echo -e "${YELLOW}=== INSTRUCTIONS ===${NC}"
echo -e "1. Attach to smux: ${GREEN}./smux attach -t debug-session:0${NC}"
echo -e "2. Run Claude Code in the first window"
echo -e "3. Trigger rapid output to reproduce the jitter"
echo -e "4. Watch scroll events appear here in real-time"
echo -e "5. Press Ctrl+C in THIS window to stop monitoring and see summary${NC}"
echo ""
echo -e "${CYAN}=== LIVE EVENT STREAM ===${NC}"

# Keep monitoring until interrupted
wait

