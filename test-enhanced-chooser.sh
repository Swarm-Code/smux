#!/bin/bash
# Test Enhanced Choose-Tree Visual Formatting

echo "🎯 Testing Enhanced Session/Project/Window Selector"
echo "=================================================="
echo ""
echo "Enhanced Visual Hierarchy Features:"
echo "  📂 Projects: Bold, colored with folder icons"
echo "  🖥️  Sessions: Indented with tree connectors (├─)"
echo "  🪟 Windows: Further indented with terminal (└─)"
echo "  ● Active indicators: Highlighted with accent color"
echo ""

echo "Current sessions structure:"
timeout 5s smux list-sessions
echo ""

echo "Current projects structure:"
timeout 5s smux list-projects
echo ""

echo "🔍 Enhanced Choose-Tree Formatting Preview:"
echo ""
echo "Expected Visual Layout:"
echo ""
echo "📂 webapp - 2 sessions (Wed Oct 15 14:06:52 2025)"
echo "  ├─ 🖥️  backend - 3 windows ● ACTIVE"
echo "    └─ 🪟 bash"
echo "    └─ 🪟 api ●"
echo "    └─ 🪟 database"
echo "  ├─ 🖥️  frontend - 1 windows"
echo "    └─ 🪟 bash"
echo ""
echo "🖥️  test-fresh - 1 windows ● ACTIVE"
echo "  └─ 🪟 bash ●"
echo ""
echo "🖥️  test-formatting - 1 windows"
echo "  └─ 🪟 bash"
echo ""

echo "🚀 To test the enhanced selector:"
echo "  1. smux attach-session -t backend"
echo "  2. Press Ctrl+B then 's' to see enhanced session selector"
echo "  3. Press Ctrl+B then 'w' to see enhanced window selector"
echo "  4. Notice the visual hierarchy with icons and colors!"
echo ""

echo "💡 Visual Enhancements Applied:"
echo "  • Projects: Accent color (#95E6CB) with bold text and 📂 icon"
echo "  • Sessions: Tree structure (├─) with 🖥️ icon and status indicators"
echo "  • Windows: Indented (└─) with 🪟 icon and active highlighting"
echo "  • Active elements: Bright accent color with ● indicator"
echo "  • Consistent dark theme matching status bar"