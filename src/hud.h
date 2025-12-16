#ifndef HUD_H
#define HUD_H

    
    
// --- HUD CONSTANTS (ANSI 8-bit Palette) ---
#define HUD_COL_BG      0   // Black / Transparent
#define HUD_COL_RED     9   // Bright Red
#define HUD_COL_GREEN   10  // Bright Green
#define HUD_COL_YELLOW  11  // Bright Yellow
#define HUD_COL_WHITE   15  // Bright White

// Icons (CP437)
#define ICON_FACE_FILLED 0x02 // ☻
#define ICON_HOUSE       0x7F // ⌂
// Choose ONE of these for the "Lost" icon:
#define ICON_DEAD_X      0x58 // X (Strikeout)
#define ICON_DEAD_SPLAT  0x0F // ☼ (Explosion/Splat)
#define ICON_DEAD_GHOST  0x01 // ☺ (Hollow Face)
#define ICON_DEAD_GRAVE  0xC5 // ┼ (Grave Marker)  


extern void update_hud(void);

extern char message[];

#endif // HUD_H