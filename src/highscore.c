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
    // Screen is 40x15 chars
    
    // --- LEFT SIDE: TODAY'S BEST (Col 2) ---
    draw_text(2, 3, "TODAYS BEST", HUD_COL_YELLOW);
    
    char buf[32];
    sprintf(buf, "ACE: %s", todays_best.name);
    draw_text(2, 5, buf, HUD_COL_WHITE);
    
    sprintf(buf, "SAVED: %d", todays_best.saved);
    draw_text(2, 7, buf, HUD_COL_GREEN);
    
    sprintf(buf, "LOST:  %d", todays_best.lost);
    draw_text(2, 8, buf, HUD_COL_RED);

    // --- RIGHT SIDE: BEST PILOTS (Col 20) ---
    // Header
    draw_text(27, 3, "BEST PILOTS", HUD_COL_YELLOW);
    draw_text(29, 4, "NAM", HUD_COL_WHITE);
    draw_text(34, 4, "\x7F", HUD_COL_GREEN); // House Icon
    draw_text(37, 4, "\x0F", HUD_COL_RED);   // Splat Icon

    // List
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        // Format: "1. AAA   10   02"
        sprintf(buf, "%d.%s %02d %02d", 
                i+1, 
                high_scores[i].name, 
                high_scores[i].saved, 
                high_scores[i].lost);
        
        // Alternate colors for readability
        uint8_t col = (i == 0) ? HUD_COL_YELLOW : HUD_COL_WHITE;
        draw_text(27, 6 + i, buf, col);
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
        draw_text(13, 5, "GREAT FLYING!", HUD_COL_GREEN);
        
        char buf[32];
        sprintf(buf, "SAVED: %d  LOST: %d", saved, lost);
        draw_text(10, 7, buf, HUD_COL_WHITE);
        
        draw_text(12, 10, "ENTER NAME:", HUD_COL_YELLOW);
        
        // Draw Name with Cursor Blink
        for (int i = 0; i < 3; i++) {
            char c_buf[2] = {name[i], 0};
            uint8_t col = HUD_COL_WHITE;
            
            // Blink active char
            if (i == cursor && (vsync_last % 30 < 15)) col = HUD_COL_RED;
            
            draw_text(24 + i, 10, c_buf, col);
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