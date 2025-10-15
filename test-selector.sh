#!/bin/bash
# Test Enhanced Session/Project Selector with Minimal Dark Theme

echo "🎯 Testing Enhanced Session/Project Selector"
echo "============================================"
echo ""
echo "Enhanced features applied:"
echo "  • Dark background: #0B0E14 (matching status bar)"
echo "  • Light text: #BFBDB6 (matching status bar)"
echo "  • Accent cursor: #95E6CB (matching prefix highlight)"
echo "  • Consistent pane indicators"
echo ""
echo "Testing selector appearance..."
echo ""

# Apply the current theme to ensure selector styling is active
smux source-file premium-minimal.conf

echo "✅ Premium minimal theme applied with enhanced selector"
echo ""
echo "🔍 To test the selector:"
echo "  • Press Ctrl+B then 's' for session selector"
echo "  • Press Ctrl+B then 'w' for window selector"
echo "  • Use arrow keys to navigate"
echo "  • Press Enter to select"
echo "  • Press 'q' to cancel"
echo ""
echo "The selector now uses the same minimal dark theme as your status bar!"
echo ""
echo "💡 Current selector settings:"
smux show-options -g mode-style
smux show-options -g prompt-cursor-colour
smux show-options -g display-panes-active-colour
smux show-options -g display-panes-colour