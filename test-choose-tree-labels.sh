#!/bin/bash
# Test the corrected choose-tree format templates

echo "🧪 TESTING CORRECTED CHOOSE-TREE FORMAT TEMPLATES"
echo "================================================="
echo ""

echo "📊 Current Structure:"
smux list-projects
echo ""
smux list-sessions
echo ""

echo "🎯 TESTING CORRECTED FORMAT TEMPLATES:"
echo "======================================"

# Test the corrected PROJECT template
echo "Testing corrected PROJECT template:"
smux display-message -t final-test -p "#[fg=#95E6CB,bold]PROJECT 📂 final-test#[nobold]#[fg=#BFBDB6] - 2 sessions #[fg=#565B66](Wed Oct 15)#[default]"

echo ""
echo "Testing corrected SESSION template:"
smux display-message -t final-test -p "#[fg=#E6A95E,bold]SESSION #[nobold]#[fg=#BFBDB6]🖥️  final-session #[fg=#565B66]- 1 windows #[fg=#95E6CB,bold] ● ACTIVE#[default]"

echo ""
echo "Testing corrected PANE template:"
smux display-message -t final-test -p "#[fg=#475266]    └─ #[fg=#D19A66]PANE #[fg=#565B66]🪟 #{window_name} #[fg=#95E6CB] ●#[default]"

echo ""
echo "🎯 VERIFICATION RESULTS:"
echo "========================"
echo "✅ All format templates should display correctly without syntax errors"
echo "✅ PROJECT, SESSION, and PANE labels should be visible"
echo "✅ No 'nobold]' or other malformed text should appear"

echo ""
echo "🎯 NEXT STEP: Check actual choose-tree interface!"
echo "Press Ctrl+B then 's' to open session selector and verify enhanced labels appear"

echo ""
echo "🔍 If labels still don't appear in actual interface, there may be additional"
echo "   configuration or template override issues to investigate."