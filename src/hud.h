#ifndef HUD_H
#define HUD_H

// HUD Text Plane
#define MESSAGE_WIDTH 40
#define MESSAGE_HEIGHT 15
#define MESSAGE_LENGTH (MESSAGE_WIDTH * MESSAGE_HEIGHT + 1) // +1 for null terminator
    
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
extern void update_lives_display(void);
extern void clear_text_screen(void);
extern void draw_text(uint8_t x, uint8_t y, const char* str, uint8_t color);

extern char message[];

#endif // HUD_H