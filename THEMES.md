# Enhanced tmux-dotbar Status Bar Themes

## Quick Reference

Your smux now has **4 status bar theme options** ranging from original to ultra-minimal:

### Theme Options

1. **🌟 Ultra Minimal** - Maximum spacing, pure clean design
   ```bash
   smux source-file ultra-minimal.conf
   # OR
   ./apply-minimal.sh
   ```

2. **✨ Premium Minimal** - Unicode elements, elegant separators
   ```bash
   smux source-file premium-minimal.conf
   ```

3. **🎯 Minimal Dotbar** - Balanced approach, clean but compact
   ```bash
   smux source-file minimal-dotbar.conf
   ```

4. **📦 Original Dotbar** - Standard tmux-dotbar theme
   ```bash
   smux source-file dotbar-setup.conf
   ```

### Interactive Theme Selector

For easy switching between all themes:
```bash
./status-themes.sh
```

### Current Theme Features

**Premium Minimal** (currently active):
- Session display: `▌ session-name ▌` with Unicode separators
- Current window: `▶ window-name ◀` with directional arrows
- Time display: `⏰ HH:MM` with clock emoji
- Enhanced spacing and visual hierarchy
- **Enhanced session/project selector** with matching dark theme

### Theme Comparison

| Theme | Spacing | Unicode | Style |
|-------|---------|---------|-------|
| Ultra Minimal | 5 spaces | No | Maximum clean |
| Premium Minimal | 3 spaces | Yes (▌ ▶ ◀) | Elegant |
| Minimal Dotbar | 3 spaces | No | Balanced |
| Original | Compact | No | Standard |

### Tips

- **Ultra Minimal**: Best for maximum screen space
- **Premium Minimal**: Best balance of style and minimalism
- **Minimal Dotbar**: Good middle ground
- **Original**: Standard tmux-dotbar experience

All themes use the same dark color scheme (`#0B0E14` background) for consistency.

## Enhanced Session/Project Selector

All themes now include **enhanced selector styling** that matches the minimal dark aesthetic:

### Selector Features
- **Dark background**: `#0B0E14` (matches status bar)
- **Light text**: `#BFBDB6` (matches status bar text)
- **Accent cursor**: `#95E6CB` (matches prefix highlight)
- **Consistent pane indicators**: Proper color coordination

### How to Access Selectors
- **Session selector**: `Ctrl+B` then `s`
- **Window selector**: `Ctrl+B` then `w`
- **Navigation**: Arrow keys to move, Enter to select, `q` to cancel

### Before vs After
**Before**: Yellow background with black text (default tmux)
**After**: Dark minimal theme matching your status bar

### Testing
Run the selector test script:
```bash
./test-selector.sh
```

This enhancement ensures your entire smux interface has a cohesive, minimal dark appearance.