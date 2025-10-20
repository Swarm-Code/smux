#!/bin/bash

# Rapid Output Generator - Simulates Claude Code streaming output

# Generate rapid output that will cause scrolling/redrawing
# Adjust the sleep duration to control output rate

OUTPUT_RATE_MS=${1:-10}  # milliseconds between lines
NUM_LINES=${2:-500}      # number of lines to output

echo "🚀 Generating rapid output ($NUM_LINES lines, ${OUTPUT_RATE_MS}ms interval)"
echo "====================================================================="
echo ""

for i in $(seq 1 $NUM_LINES); do
    # Simulate Claude Code output with varying line lengths
    case $((i % 5)) in
        0)
            echo "Stream [$(date +%H:%M:%S)]: Processing query... Tokens: $((i * 47)) | Buffer: $((i * 128)) bytes"
            ;;
        1)
            printf "█"
            ;;
        2)
            echo "Output line $i: Lorem ipsum dolor sit amet consectetur adipiscing elit sed do eiusmod tempor"
            ;;
        3)
            echo "[DEBUG] Event ID: $((i * 1337)) | Status: ACTIVE | Memory: $((i * 2))MB"
            ;;
        *)
            echo "$i: $(for j in $(seq 1 $((20 + RANDOM % 50))); do echo -n "x"; done)"
            ;;
    esac

    # Sleep for specified interval
    sleep 0.0${OUTPUT_RATE_MS}
done

echo ""
echo "✅ Rapid output complete!"

