#include "highscore.h"
#include "constants.h" // For defines
#include "input.h"     // For is_action_pressed
#include "hud.h"       // For draw_text, HUD_COL_*
#include <stdio.h>
#include <string.h>
#include <rp6502.h>
#include "music.h"

// Global Data
static HighScoreEntry high_scores[MAX_HIGH_SCORES];
HighScoreEntry todays_best;

// Draws a line of a specific character
void draw_repeat_char(uint8_t x, uint8_t y, uint8_t count, uint8_t ch, uint8_t color) {
    for(int i=0; i<count; i++) {
        // We can use a simplified draw_text logic here or construct a 1-char string
        char buf[2] = {ch, 0};
        draw_text(x + i, y, buf, color);
    }
}

void draw_bbs_frame(void) {
    // 1. TOP BORDER (Row 0)
    // ╔══════════════════╦══════════════════╗
    char top_row[41];
    sprintf(top_row, "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCB\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    draw_text(0, 0, top_row, HUD_COL_CYAN);

    // 2. VERTICAL LINES (Rows 1-13)
    for (int y = 1; y < 14; y++) {
        draw_text(0,  y, "\xBA", HUD_COL_CYAN); // Left
        draw_text(19, y, "\xBA", HUD_COL_CYAN); // Middle Divider
        draw_text(39, y, "\xBA", HUD_COL_CYAN); // Right
    }

    // 3. TITLE SEPARATOR (Row 2)
    // ╠══════════════════╬══════════════════╣
    char mid_row[41];
    sprintf(mid_row, "\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCE\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9");
    // Note: \xCE is the cross (╬). If you don't have it, use \xCB (╦) or similar.
    draw_text(0, 2, mid_row, HUD_COL_CYAN);

    // 4. BOTTOM BORDER (Row 14)
    // ╚══════════════════╩══════════════════╝
    char bot_row[41];
    sprintf(bot_row, "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCA\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");
    draw_text(0, 14, bot_row, HUD_COL_CYAN);
}

// Helper to clear specific lines for the score screen
static void clear_rows(uint8_t start_y, uint8_t height) {
    // We can't use a rect clear easily, so we draw spaces
    // Assuming MESSAGE_WIDTH is 40
    char empty_line[41];
    memset(empty_line, ' ', 40);
    empty_line[40] = 0;
    
    for(int y = 0; y < height; y++) {
        draw_text(0, start_y + y, empty_line, HUD_COL_BG);
    }
}

// ============================================================================
// DATA MANAGEMENT
// ============================================================================

void init_high_scores(void) {
    // Defaults for File
    for (uint8_t i = 0; i < MAX_HIGH_SCORES; i++) {
        high_scores[i].name[0] = 'R';
        high_scores[i].name[1] = 'P';
        high_scores[i].name[2] = '6';
        high_scores[i].name[3] = '\0';
        high_scores[i].saved = (5 - i) * 5; // 25, 20, 15...
        high_scores[i].lost = 0;
    }

    // Default for Session
    todays_best.name[0] = '-';
    todays_best.name[1] = '-';
    todays_best.name[2] = '-';
    todays_best.name[3] = '\0';
    todays_best.saved = 0;
    todays_best.lost = 0;
}

bool load_high_scores(void) {
    FILE* fp = fopen(HIGH_SCORE_FILE, "rb");
    if (!fp) {
        init_high_scores();
        return false;
    }
    size_t read = fread(high_scores, sizeof(HighScoreEntry), MAX_HIGH_SCORES, fp);
    fclose(fp);
    return (read == MAX_HIGH_SCORES);
}

void save_high_scores(void) {
    FILE* fp = fopen(HIGH_SCORE_FILE, "wb");
    if (fp) {
        fwrite(high_scores, sizeof(HighScoreEntry), MAX_HIGH_SCORES, fp);
        fclose(fp);
    }
}

// ============================================================================
// SCORING LOGIC
// ============================================================================

// Returns true if Score A is better than Score B
static bool is_better(uint8_t saved_a, uint8_t lost_a, uint8_t saved_b, uint8_t lost_b) {
    if (saved_a > saved_b) return true;
    if (saved_a == saved_b && lost_a < lost_b) return true;
    return false;
}

bool is_new_high_score(uint8_t saved, uint8_t lost) {
    // Check against the lowest entry
    return is_better(saved, lost, 
                     high_scores[MAX_HIGH_SCORES-1].saved, 
                     high_scores[MAX_HIGH_SCORES-1].lost);
}

bool is_new_todays_best(uint8_t saved, uint8_t lost) {
    return is_better(saved, lost, todays_best.saved, todays_best.lost);
}

void update_todays_best(uint8_t saved, uint8_t lost, const char* name) {
    if (is_new_todays_best(saved, lost)) {
        todays_best.saved = saved;
        todays_best.lost = lost;
        strncpy(todays_best.name, name, 3);
    }
}

void insert_high_score(uint8_t saved, uint8_t lost, const char* name) {
    // Find position
    int pos = -1;
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        if (is_better(saved, lost, high_scores[i].saved, high_scores[i].lost)) {
            pos = i;
            break;
        }
    }

    if (pos != -1) {
        // Shift down
        for (int i = MAX_HIGH_SCORES - 1; i > pos; i--) {
            high_scores[i] = high_scores[i-1];
        }
        // Insert
        strncpy(high_scores[pos].name, name, 3);
        high_scores[pos].saved = saved;
        high_scores[pos].lost = lost;
        save_high_scores();
    }
}

// ============================================================================
// RENDERING & INPUT
// ============================================================================

void draw_high_score_screen(void) {
    
    // --- 1. DRAW FRAME ---
    draw_bbs_frame();

    // --- 2. HEADER (Row 1) ---
    // Centered Title
    draw_text(2, 1, "MEGA CHOPLIFTER", HUD_COL_YELLOW);
    
    // Credits/Ver on right?
    draw_text(24, 1, "WAR UPDATES", HUD_COL_GREY);

    // --- 3. LEFT PANEL: TODAY'S BEST (Cols 1-18) ---
    
    draw_text(4, 3, "TODAY'S BEST", HUD_COL_WHITE);
    
    // Ace Name
    draw_text(2, 5, "\x10 ACE PILOT", HUD_COL_CYAN); // Arrow Icon
    draw_text(14, 5, todays_best.name, HUD_COL_YELLOW);

    // Stats
    char buf[20];
    
    draw_text(2, 7, "RESCUED:", HUD_COL_GREY);
    sprintf(buf, "%02d \x7F", todays_best.saved); // Number + House
    draw_text(14, 7, buf, HUD_COL_GREEN);

    draw_text(2, 8, "CASUALTIES:", HUD_COL_GREY);
    sprintf(buf, "%02d \x0F", todays_best.lost);  // Number + Splat
    draw_text(14, 8, buf, HUD_COL_RED);

    // Prompt (Bottom Left)
    // Blink "PRESS START" based on VSYNC
    if ((RIA.vsync / 32) % 2 == 0) {
        draw_text(4, 11, "PRESS START", HUD_COL_WHITE);
    }


    draw_text(21, 3, "NEW PILOTS NEEDED", HUD_COL_RED);
    draw_text(25, 4, "BE A HERO", HUD_COL_GREEN);

    // --- 4. RIGHT PANEL: HALL OF FAME (Cols 20-38) ---
    
    draw_text(24, 6, "HALL OF FAME", HUD_COL_WHITE);
    
    // Table Header
    // #  NAM  H  X
    draw_text(21, 8, "#", HUD_COL_GREY);
    draw_text(24, 8, "NAM", HUD_COL_GREY);
    draw_text(30, 8, "\x7F", HUD_COL_GREEN); // House
    draw_text(35, 8, "\x0F", HUD_COL_RED);   // Splat

    // List (Rows 5-9) - Top 5 Only to fit
    // (Or display all 5 compact if MAX_HIGH_SCORES is 5)
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        uint8_t y = 9 + i; // Double spacing? Or single? Let's do 2-line spacing if room
        if (y > 13) break; // Safety

        // Rank (1, 2, 3...)
        char rank[2] = {'1' + i, 0};
        draw_text(21, y, rank, HUD_COL_CYAN);

        // Name
        draw_text(24, y, high_scores[i].name, HUD_COL_WHITE);

        // Saved
        sprintf(buf, "%02d", high_scores[i].saved);
        draw_text(29, y, buf, HUD_COL_GREEN);

        // Lost
        sprintf(buf, "%02d", high_scores[i].lost);
        draw_text(34, y, buf, HUD_COL_RED);
    }
}

void enter_initials(uint8_t saved, uint8_t lost) {
    char name[4] = "AAA";
    uint8_t cursor = 0;
    uint8_t vsync_last = RIA.vsync;
    
    bool up_prev = false;
    bool down_prev = false;
    bool fire_prev = false;
    
    // Input Loop
    while (cursor < 3) {
        if (RIA.vsync == vsync_last) continue;
        vsync_last = RIA.vsync;
        
        // 1. Draw UI
        // Clear center area
        // (Assuming clear_text_screen was called before this)
        draw_text(13, 4, "GREAT FLYING!", HUD_COL_GREEN);
        draw_text( 7, 5, "YOU WILL BE REMEMBERED", HUD_COL_WHITE);
        
        char buf[32];
        sprintf(buf, "SAVED: %d  LOST: %d", saved, lost);
        draw_text(10, 9, buf, HUD_COL_WHITE);
        
        draw_text(12, 12, "ENTER NAME:", HUD_COL_YELLOW);
        
        // Draw Name with Cursor Blink
        for (int i = 0; i < 3; i++) {
            char c_buf[2] = {name[i], 0};
            uint8_t col = HUD_COL_WHITE;
            
            // Blink active char
            if (i == cursor && (vsync_last % 30 < 15)) col = HUD_COL_RED;
            
            draw_text(24 + i, 12, c_buf, col);
        }

        // 2. Handle Input
        // Read Gamepad 0
        handle_input();
        bool up = is_action_pressed(0, ACTION_THRUST);
        bool down = is_action_pressed(0, ACTION_REVERSE_THRUST);
        bool fire = is_action_pressed(0, ACTION_FIRE) || is_action_pressed(0, ACTION_PAUSE); // Start or Fire

        // UP
        if (up && !up_prev) {
            name[cursor]++;
            if (name[cursor] > 'Z') name[cursor] = 'A';
        }
        // DOWN
        if (down && !down_prev) {
            name[cursor]--;
            if (name[cursor] < 'A') name[cursor] = 'Z';
        }
        // ENTER
        if (fire && !fire_prev) {
            cursor++;
        }

        up_prev = up;
        down_prev = down;
        fire_prev = fire;

        update_music();

    }

    // Save Data
    update_todays_best(saved, lost, name);
    
    if (is_new_high_score(saved, lost)) {
        insert_high_score(saved, lost, name);
    }
}