#!/bin/bash
# tmux-dotbar activation script for smux
# Run this to apply the tmux-dotbar theme to your current smux session

echo "Applying tmux-dotbar theme to smux..."

# Declare the plugin
smux set-option -g @plugin 'vaaleyard/tmux-dotbar'

# Apply the complete tmux-dotbar theme
smux source-file dotbar-setup.conf

echo "✅ tmux-dotbar theme applied successfully!"
echo "🎨 Your status bar now has the dark theme with session highlighting"
echo ""
echo "Theme features:"
echo "  • Dark background (#0B0E14)"
echo "  • Session name highlighting"
echo "  • Prefix indicator (changes color when prefix key pressed)"
echo "  • Window status with separators"
echo "  • Minimalist design"