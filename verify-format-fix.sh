#!/bin/bash
# Verify that format string fixes resolved the "nobold" display issue

echo "🔍 VERIFYING FORMAT STRING FIXES"
echo "================================="
echo ""

echo "📊 Current Test Structure:"
echo "Projects:"
smux list-projects
echo ""
echo "Sessions:"
smux list-sessions
echo ""

echo "🧪 TESTING FORMAT STRINGS DIRECTLY:"
echo "===================================="

# Create a test session for format testing
smux new-session -d -s format-test 2>/dev/null || true

echo "Testing PROJECT format string:"
smux display-message -t format-test -p "#[fg=#95E6CB,bold]PROJECT 📂 test-project#[fg=#BFBDB6]#[nobold] - 2 sessions #[fg=#565B66](Wed Oct 15)#[default]"

echo ""
echo "Testing SESSION format string:"
smux display-message -t format-test -p "#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  test-session#[nobold] #[fg=#565B66]- 1 windows #[fg=#95E6CB,bold]● ACTIVE#[default]"

echo ""
echo "🎯 VERIFICATION RESULTS:"
echo "========================"
echo "✅ If you see proper formatting above without 'nobold]' errors, the fix worked!"
echo "❌ If you still see 'nobold]' or malformed text, there are still format issues."

# Clean up
smux kill-session -t format-test 2>/dev/null || true

echo ""
echo "🔍 Format string verification complete!"