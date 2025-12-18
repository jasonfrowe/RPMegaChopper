#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_HIGH_SCORES 5
#define HIGH_SCORE_NAME_LEN 3
#define HIGH_SCORE_FILE "HIGHSCMF.DAT"

typedef struct {
    char name[HIGH_SCORE_NAME_LEN + 1];
    uint8_t saved;
    uint8_t lost;
} HighScoreEntry;

// Globals
extern HighScoreEntry todays_best; // Track session best separately

// Core Functions
void init_high_scores(void);
bool load_high_scores(void);
void save_high_scores(void);

// Logic
bool is_new_high_score(uint8_t saved, uint8_t lost);
bool is_new_todays_best(uint8_t saved, uint8_t lost);
void update_todays_best(uint8_t saved, uint8_t lost, const char* name);
void insert_high_score(uint8_t saved, uint8_t lost, const char* name);

// Rendering & Input
void draw_high_score_screen(void);
void enter_initials(uint8_t saved, uint8_t lost);

#endif