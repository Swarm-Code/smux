#!/bin/bash
# Test the simplified enhanced format

echo "🧪 TESTING SIMPLIFIED ENHANCED FORMAT"
echo "====================================="
echo ""

echo "📋 SIMPLIFICATION CHANGES:"
echo "=========================="
echo "✅ Removed complex tree connectors (├─ └─)"
echo "✅ Removed excessive formatting and colors"
echo "✅ Simplified to match original tmux style"
echo "✅ Kept enhanced PROJECT/SESSION/WINDOW/PANE labels with icons"
echo ""

echo "📊 Current structure:"
smux list-projects
echo ""
smux list-sessions
echo ""

echo "🧪 TESTING SIMPLIFIED FORMAT STRINGS:"
echo "===================================="

echo "Testing simplified PROJECT format:"
smux display-message -t simple-test -p "#[bold,fg=#95E6CB]PROJECT 📂:#[default] #[fg=#BFBDB6]simple-test#[default] #[dim](2 sessions)#[default]"

echo ""
echo "Testing simplified SESSION format:"
smux display-message -t simple-test -p "#[fg=#E6A95E]SESSION 🖥️:#[default] 1 windows (attached)"

echo ""
echo "Testing simplified WINDOW format:"
smux display-message -t simple-test -p "#[fg=#D19A66]WINDOW:#[default] bash"

echo ""
echo "Testing simplified PANE format:"
smux display-message -t simple-test -p "#[fg=#D19A66]PANE:#[default] bash*"

echo ""
echo "🎯 EXPECTED SIMPLIFIED DISPLAY:"
echo "==============================="
echo "PROJECT 📂: simple-test (2 sessions)"
echo "SESSION 🖥️: 1 windows (attached)"
echo "WINDOW: bash"
echo "PANE: bash*"
echo ""

echo "🎯 KEY DIFFERENCES FROM ORIGINAL:"
echo "================================="
echo "• Added 'PROJECT 📂:' labels instead of plain project names"
echo "• Added 'SESSION 🖥️:' labels instead of plain session info"
echo "• Added 'WINDOW:' labels instead of plain window names"
echo "• Added 'PANE:' labels instead of plain pane commands"
echo "• Kept original simple structure without complex formatting"
echo ""

echo "🚀 HOW TO TEST:"
echo "==============="
echo "1. Attach to session: smux attach-session -t simple-session"
echo "2. Press Ctrl+B then 's' to open session selector"
echo "3. Look for the simplified enhanced labels above"
echo ""

echo "✨ This simplified version should now show the enhanced labels!"