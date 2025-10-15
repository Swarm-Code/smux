#!/bin/bash
# Test the actual choose-tree output to identify "nobold" issues

echo "🔍 TESTING REAL CHOOSE-TREE OUTPUT"
echo "=================================="
echo ""

echo "📊 Setting up test environment:"
echo "Creating multiple projects and sessions..."

# Create test structure
smux new-project -n project1 2>/dev/null || true
smux new-project -n project2 2>/dev/null || true
smux new-session -d -s session1 -P project1 2>/dev/null || true
smux new-session -d -s session2 -P project1 2>/dev/null || true
smux new-session -d -s session3 -P project2 2>/dev/null || true

echo "✅ Test structure created!"
echo ""

echo "📊 Current structure:"
smux list-projects
echo ""
smux list-sessions
echo ""

echo "🎯 TESTING CHOOSE-TREE FORMAT OUTPUT:"
echo "====================================="

# Test choose-tree format by using the actual mode-tree format
echo "Testing choose-tree session format..."

# Capture the actual format being used
smux attach-session -t session1 -c 'display-message -p "#{window_tree_default_format}"' 2>/dev/null || echo "Could not capture format"

echo ""
echo "Testing with manual format simulation..."

# Test the window-tree format directly
echo "PROJECT format test in context:"
smux display-message -t session1 -p "$(cat << 'EOF'
#{?project_format,
\n#[fg=#95E6CB,bold]PROJECT 📂 #{project_name}#[fg=#BFBDB6]#[nobold] - #{project_sessions} sessions #[fg=#565B66](#{t:project_created})#[default]\n
,}
EOF
)" 2>/dev/null || echo "Format test failed"

echo ""
echo "🎯 DIRECT WINDOW-TREE FORMAT TEST:"
echo "=================================="

# Test if we can see the actual format being processed
smux show-options -g window-tree-format 2>/dev/null || echo "No global window-tree-format set"

echo ""
echo "✅ Choose-tree format testing complete!"
echo "Look for any 'nobold]' or malformed formatting above."