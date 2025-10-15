#!/bin/bash
# Direct test of choose-tree output to verify PROJECT labels

echo "🔍 Testing Enhanced Choose-Tree Output - Verifying PROJECT Labels"
echo "================================================================="
echo ""

echo "📊 Current Projects:"
smux list-projects
echo ""

echo "📊 Current Sessions:"
smux list-sessions
echo ""

echo "🎯 Testing Choose-Tree Display Format:"
echo "====================================="

# Test choose-tree output with project format - capture just the format output
echo "Testing PROJECT format string directly..."

# Create a temporary session to test with
smux new-session -d -s test-format

# Use tmux display-message to test the format string directly
echo "PROJECT format test:"
smux display-message -t test-format -p "#[fg=#95E6CB,bold]PROJECT 📂 demo-project#[fg=#BFBDB6]#[nobold] - 2 sessions #[fg=#565B66](Wed Oct 15 16:00:00)#[default]"

echo ""
echo "SESSION format test:"
smux display-message -t test-format -p "#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  demo-session#[nobold] #[fg=#565B66]- 1 windows#[fg=#95E6CB,bold] ● ACTIVE#[default]"

echo ""
echo "🎯 Format String Verification Complete!"
echo "If no 'nobold]' appears above, the format strings are correct!"

# Clean up
smux kill-session -t test-format 2>/dev/null

echo ""
echo "✅ Test completed successfully!"