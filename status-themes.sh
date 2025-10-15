#!/bin/bash
# tmux-dotbar Status Bar Theme Selector
# Choose your preferred bigger, minimal style

echo "🎨 tmux-dotbar Status Bar Themes"
echo "================================="
echo ""
echo "Choose your preferred style:"
echo ""
echo "1) Ultra Minimal    - Maximum spacing, clean design"
echo "2) Premium Minimal  - Unicode elements, elegant separators"
echo "3) Original dotbar  - Standard tmux-dotbar theme"
echo ""
read -p "Enter your choice (1-3): " choice

case $choice in
    1)
        echo ""
        echo "🌟 Applying Ultra Minimal theme..."
        smux source-file ultra-minimal.conf
        echo "✅ Ultra Minimal applied!"
        echo "   • Session name with 4-space padding"
        echo "   • Window tabs with 5-space padding"
        echo "   • Clean time display"
        ;;
    2)
        echo ""
        echo "✨ Applying Premium Minimal theme..."
        smux source-file premium-minimal.conf
        echo "✅ Premium Minimal applied!"
        echo "   • Unicode separators (▌ ▶ ◀)"
        echo "   • Visual window separators (│)"
        echo "   • Clock emoji in time display"
        echo "   • Enhanced current window highlighting"
        ;;
    3)
        echo ""
        echo "🎯 Applying Original dotbar theme..."
        smux source-file dotbar-setup.conf
        echo "✅ Original dotbar applied!"
        echo "   • Standard tmux-dotbar styling"
        echo "   • Compact design"
        ;;
    *)
        echo ""
        echo "❌ Invalid choice. Please run again and select 1, 2, or 3."
        exit 1
        ;;
esac

echo ""
echo "🚀 Your status bar theme has been updated!"
echo "💡 To switch themes anytime, run: ./status-themes.sh"