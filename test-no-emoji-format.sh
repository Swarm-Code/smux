#!/bin/bash
# Test simplified format without emojis

echo "🧪 TESTING SIMPLIFIED FORMAT WITHOUT EMOJIS"
echo "==========================================="
echo ""

echo "📋 CHANGES MADE:"
echo "================"
echo "✅ Removed 📂 emoji from PROJECT labels"
echo "✅ Removed 🖥️ emoji from SESSION labels"
echo "✅ Kept simple text-based labels for maximum compatibility"
echo ""

echo "🚀 Starting fresh smux instance..."
smux start-server

echo ""
echo "📊 Creating test structure:"
smux new-project -n testproject
smux new-session -d -s testsession -P testproject

echo ""
echo "📊 Current structure:"
smux list-projects
echo ""
smux list-sessions
echo ""

echo "🧪 TESTING SIMPLIFIED FORMAT STRINGS:"
echo "===================================="

echo "Testing PROJECT format (no emoji):"
smux display-message -t testsession -p "#[bold,fg=#95E6CB]PROJECT:#[default] #[fg=#BFBDB6]testproject#[default] #[dim](2 sessions)#[default]"

echo ""
echo "Testing SESSION format (no emoji):"
smux display-message -t testsession -p "#[fg=#E6A95E]SESSION:#[default] 1 windows (attached)"

echo ""
echo "🎯 EXPECTED SIMPLIFIED DISPLAY:"
echo "==============================="
echo "PROJECT: testproject (2 sessions)"
echo "SESSION: testsession - 1 windows (attached)"
echo "WINDOW: bash"
echo "PANE: bash*"
echo ""

echo "🚀 HOW TO TEST:"
echo "==============="
echo "1. Attach to session: smux attach-session -t testsession"
echo "2. Press Ctrl+B then 's' to open session selector"
echo "3. Look for 'PROJECT:' labels at the top level"
echo ""

echo "✨ Without emojis, PROJECT labels should now appear!"