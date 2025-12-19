#ifndef HUD_H
#define HUD_H

// HUD Text Plane
#define MESSAGE_WIDTH 40
#define MESSAGE_HEIGHT 15
#define MESSAGE_LENGTH (MESSAGE_WIDTH * MESSAGE_HEIGHT + 1) // +1 for null terminator
    
// --- HUD CONSTANTS (ANSI 8-bit Palette) ---
// --- HUD CONSTANTS (Corrected ANSI Palette) ---
#define HUD_COL_BG      0   // Black
#define HUD_COL_RED     9   // Bright Red
#define HUD_COL_GREEN   10  // Bright Green
#define HUD_COL_YELLOW  11  // Bright Yellow (Fixed)
#define HUD_COL_BLUE    12  // Bright Blue
#define HUD_COL_MAGENTA 13  // Bright Magenta
#define HUD_COL_CYAN    14  // Bright Cyan   (Fixed)
#define HUD_COL_WHITE   15  // Bright White

// 7 is "Light Grey" (Dim White). 
// 8 is "Dark Grey" (Bright Black).
// If 8 looked yellow/brown to you, 7 should give you the clean grey you want.
#define HUD_COL_GREY    7

// --- CP437 GLYPHS ---
#define GLYPH_FACE      0x02 // ☻
#define GLYPH_HEART     0x03 // ♥
#define GLYPH_ARROW_R   0x10 // ►
#define GLYPH_HOUSE     0x7F // ⌂
#define GLYPH_SKULL     0x0F // ☼ (Splat/Sun)

// Box Drawing (Double Line)
#define BOX_H           0xCD // ═
#define BOX_V           0xBA // ║
#define BOX_TL          0xC9 // ╔
#define BOX_TR          0xBB // ╗
#define BOX_BL          0xC8 // ╚
#define BOX_BR          0xBC // ╝
#define BOX_T_DOWN      0xCB // ╦
#define BOX_T_UP        0xCA // ╩
#define BOX_T_RIGHT     0xCC // ╠
#define BOX_T_LEFT      0xB9 // ╣

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

extern void draw_sortie_message(int lives_left);
extern void clear_sortie_message(void);

extern char message[];

#endif // HUD_H