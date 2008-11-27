#include <avr/pgmspace.h>

#define BOX_board_top		0
#define BOX_board_bottom	7
#define BOX_board_left		0
#define BOX_board_right		11

#define default_fg_color	red
#define default_bg_color	white

volatile unsigned char random_piece;	//Used to select a piece "randomly" (but not really)

//Prototypes
void BOX_store_loc(void);
void BOX_clear_loc(void);
void BOX_load_reference(unsigned char piece, unsigned char rotation);
void BOX_rotate(unsigned char direction);
void BOX_draw(unsigned char X, unsigned char Y, unsigned char color);
void BOX_erase(unsigned char X, unsigned char Y, unsigned char color);
void BOX_write_piece(void);
void BOX_clear_piece(void);
void BOX_rewrite_display(unsigned char fgcolor, unsigned char bgcolor);
void BOX_spawn(void);
unsigned char BOX_check(signed char X_offset, signed char Y_offset);
void BOX_line_check(void);
void BOX_up(void);
void BOX_dn(void);
void BOX_lt(void);
void BOX_rt(void);
void BOX_start_game(void);
void BOX_end_game(void);