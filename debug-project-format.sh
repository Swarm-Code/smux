#!/bin/bash
# Debug why PROJECT labels aren't showing

echo "🔍 DEBUGGING PROJECT FORMAT ISSUE"
echo "=================================="
echo ""

echo "📊 Current structure:"
smux list-projects
echo ""
smux list-sessions
echo ""

echo "🧪 TESTING FORMAT COMPONENTS:"
echo "============================="

echo "Testing project_format callback:"
smux display-message -t simple-test -p "project_format value: #{project_format}"

echo ""
echo "Testing project variables:"
smux display-message -t simple-test -p "project_name: #{project_name}"
smux display-message -t simple-test -p "project_sessions: #{project_sessions}"

echo ""
echo "Testing complete PROJECT format:"
smux display-message -t simple-test -p "#[bold,fg=#95E6CB]PROJECT 📂:#[default] #[fg=#BFBDB6]#{project_name}#[default] #[dim](#{project_sessions} sessions)#[default]"

echo ""
echo "Testing conditional format structure:"
smux display-message -t simple-test -p "#{?project_format,PROJECT FORMAT WORKS,NOT PROJECT FORMAT}"

echo ""
echo "🎯 ANALYSIS:"
echo "============"
echo "• If project_format shows '1' -> callback is working"
echo "• If project_name/project_sessions show values -> variables are available"
echo "• If conditional test shows 'PROJECT FORMAT WORKS' -> conditional logic works"
echo "• If all above work but choose-tree doesn't show PROJECT labels -> issue is in format string structure"