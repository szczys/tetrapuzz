#include "box_game.h"
#include "KS0108.h"
#include "font5x8.h"
#include <util/delay.h>

/*
 * TODO: Fix 8-bit color in 3595_LCD.c (seems to be backwards??)
 * 			http://en.wikipedia.org/wiki/8-bit_color
 *
 * TODO: Clean this place up!
 */


/*
Program flow:
  -Initialize
  -Spawn piece
    -load from reference
    -calculate y_loc
    -draw piece
  -Wait for game input
*/

/**BOX_location[] Array********************
* Used to track where boxes are in the    *
* playing area.  One byte per column with *
* byte0 as the left column.  One bit per  *
* row with the LSB at the top.  If there  *
* are more than 8 rows, the array will be *
* Increased by BOX_board_right+1 bytes to *
* accomodate 8 more rows as needed.       *
*******************************************/

/*****************************************
* BOX_piece[] is a 4x4 representation of *
* the currently selected playing piece.  *
* When a piece spawns its shape is       *
* written to this array in the least     *
* significant nibble of each byte:       *
* Byte0= left, Byte3= right		         *
* The LSB of each nibble is the top of   *
* the display area.                      *
******************************************/

unsigned char BOX_piece[4];

static const char PROGMEM BOX_reference[7][4][4] = {
		//T
		{
				{
						0b00000010,
						0b00000011,
						0b00000010,
						0b00000000
				},

				{
						0b00000000,
						0b00000111,
						0b00000010,
						0b00000000
				},

				{
						0b00000001,
						0b00000011,
						0b00000001,
						0b00000000
				},

				{
						0b00000010,
						0b00000111,
						0b00000000,
						0b00000000
				}
		},

		// S
		{
				{
						0b00000010,
						0b00000011,
						0b00000001,
						0b00000000
				},

				{
						0b00000011,
						0b00000110,
						0b00000000,
						0b00000000
				},

				{
						0b00000010,
						0b00000011,
						0b00000001,
						0b00000000
				},

				{
						0b00000011,
						0b00000110,
						0b00000000,
						0b00000000
				}
		},

		// Z
		{
				{
						0b00000001,
						0b00000011,
						0b00000010,
						0b00000000
				},

				{
						0b00000110,
						0b00000011,
						0b00000000,
						0b00000000
				},

				{
						0b00000001,
						0b00000011,
						0b00000010,
						0b00000000
				},

				{
						0b00000110,
						0b00000011,
						0b00000000,
						0b00000000
				}
		},

		// L
		{
				{
						0b00000011,
						0b00000001,
						0b00000001,
						0b00000000
				},

				{
						0b00000000,
						0b00000001,
						0b00000111,
						0b00000000
				},

				{
						0b00000010,
						0b00000010,
						0b00000011,
						0b00000000
				},

				{
						0b00000000,
						0b00000111,
						0b00000100,
						0b00000000
				}
		},

		// J
		{
				{
						0b00000001,
						0b00000001,
						0b00000011,
						0b00000000
				},

				{
						0b00000000,
						0b00000100,
						0b00000111,
						0b00000000
				},

				{
						0b00000011,
						0b00000010,
						0b00000010,
						0b00000000
				},

				{
						0b00000000,
						0b00000111,
						0b00000001,
						0b00000000
				}
		},

		// Box
		{
				{
						0b00000011,
						0b00000011,
						0b00000000,
						0b00000000,
				},

				{
						0b00000011,
						0b00000011,
						0b00000000,
						0b00000000,
				},

				{
						0b00000011,
						0b00000011,
						0b00000000,
						0b00000000,
				},

				{
						0b00000011,
						0b00000011,
						0b00000000,
						0b00000000,
				}
		},

		// Line
		{
				{
						0b00000010,
						0b00000010,
						0b00000010,
						0b00000010
				},

				{
						0b00000000,
						0b00001111,
						0b00000000,
						0b00000000
				},

				{
						0b00000010,
						0b00000010,
						0b00000010,
						0b00000010
				},

				{
						0b00000000,
						0b00001111,
						0b00000000,
						0b00000000
				}
		}
};

//Variables
unsigned char BOX_location[array_size];
unsigned char x_loc, y_loc;     //Bottom left index of each piece
unsigned char cur_piece = 0;	//Index for BOX_reference
unsigned char rotate = 0;		//Index for piece rotation
unsigned char score;		//Track the number of rows completed

//Messages
static const char PROGMEM message1[] = { "Tetrapuzz!" };
static const char PROGMEM message2[] = { "click to"} ;
static const char PROGMEM message3[] = { "start" };
static const char PROGMEM message4[] = { "Game Over" };

//Functions

/*******************************
 * Display specific functions: *
 *   Change these to suit      *
 *   whatever display will     *
 *   be used.                  *
 *******************************/

void GLCD_string_sideways(unsigned char X, unsigned char Y, const char * str,unsigned char array_length)
{
	//TODO: Move this to the GLCD library

	//Loop through each char in array
	unsigned char temp_array[7]; //Array to store character data
	unsigned char tot_cols = (array_length-1) * 6;
	unsigned char temp_index = 0; //Will track each column (5 per char + space) in array
	unsigned char current_char; //Keeps track of what char to draw right now
	unsigned char char_column_info; //Stores data read in from font5x8.h
	while (temp_index <= tot_cols)
	{
	  //Write each column of all 7 chars in temp_array[] including spaces
	  for (unsigned char i=0; i<8; i++)
	  {
		  unsigned char read_column = temp_index % 6;
		  if (read_column) //We should be writing char data to a column
		  {
			  --read_column;
			  if (temp_index > tot_cols) current_char = ' '; //Fill with spaces when we run out of chars
			  else current_char = pgm_read_byte(str+(temp_index/6)); //Get which char we are currently writing
			  current_char -= 32; //Adjust to match the font5x8.h array

			  char_column_info = pgm_read_byte((char *)((int)font5x8 + (5 * current_char) + read_column)); //Read column in from font5x8.h
			  //GLCD_GoTo(10,0);
			  //GLCD_WriteData(char_column_info);
			  //while(1) { ;; }
			  for (unsigned char j=0; j<7; j++)
			  {
				  if (char_column_info & (1<<(6-j))) temp_array[j] |= 1<<i;
				  else temp_array[j] &= ~(1<<i);
			  }
		  }
		  else //We are currently writing a space after a char to a column
		  {
			  for (unsigned char j=0; j<7; j++)
			  {
				  temp_array[j] &= ~(1<<i);
			  }
		  }
		  ++temp_index; //We have written a column, increment the index


	  }
	  //Now display the data we have just stored
	  GLCD_GoTo(Y,X);
	  if (++X > 7) //Take care of line wrapping
	  {
		  X = 0;
		  Y -= 8;
	  }
	  for (unsigned char i=0; i<7; i++) GLCD_WriteData(temp_array[i]);
	}
}

void BOX_draw(unsigned char X, unsigned char Y, unsigned char color)
{
	  unsigned char temp_data;
	  if (X%2)
	  {
		  if (BOX_loc_return_bit(X-1,Y)) temp_data = 0xFF;
		  else temp_data = 0xF0;
	  }
	  else
	  {
		  if (BOX_loc_return_bit(X+1,Y)) temp_data = 0xFF;
		  else temp_data = 0x0F;
	  }
	  Y = (19*4)-(Y*4);
	  GLCD_GoTo(Y+8, (X/2)+1);
	  for (unsigned char i=0; i<4; i++)
	  {
		  //GLCD_SetPixel((y*6)+j,(x*6)+i,color);
		  GLCD_WriteData(temp_data);
	  }
}

void BOX_erase(unsigned char X, unsigned char Y)
{
	  unsigned char temp_data;
	  if (X%2)
	  {
		  if (BOX_loc_return_bit(X-1,Y)) temp_data = 0x0F;
		  else temp_data = 0x00;
	  }
	  else
	  {
		  if (BOX_loc_return_bit(X+1,Y)) temp_data = 0xF0;
		  else temp_data = 0x00;
	  }
	  Y = (19*4)-(Y*4);
	  GLCD_GoTo(Y+8, (X/2)+1);
	  for (unsigned char i=0; i<4; i++)
	  {
		  //GLCD_SetPixel((y*6)+j,(x*6)+i,color);
		  GLCD_WriteData(temp_data);
	  }
}

void BOX_pregame(void)
{

  GLCD_Initalize();

  GLCD_ClearScreen();

  GLCD_GoTo(40,4);
  GLCD_string_sideways(0,80,(char *)message1, (sizeof(message1) / sizeof(message1[0])));
  GLCD_string_sideways(1,60, (char *)message2, (sizeof(message2) / sizeof(message2[0])));
  GLCD_string_sideways(1,52, (char *)message3, (sizeof(message3) / sizeof(message3[0])));
}

void BOX_start_game(void)
{
  score = 0; //Reset score
  //Populate BOX_location[] with 0
  for (unsigned char i=0; i<array_size; i++) { BOX_location[i] = 0x00; }

  //Draw frame around playing area

  //Page 0
  unsigned char i;
  GLCD_GoTo(0,0);
  for (i=0; i<3; i++)
  {
	  GLCD_WriteData(0xAA);
	  GLCD_WriteData(0x55);
  }
  GLCD_WriteData(0xAA);
  for (i=0; i<41; i++)
  {
	  GLCD_WriteData(0xD5);
	  GLCD_WriteData(0xAA);
  }
  for (i=0; i<19; i++)
  {
	  GLCD_WriteData(0x55);
	  GLCD_WriteData(0xAA);
  }
  GLCD_WriteData(0x55);

  //Page 1-5
  for (unsigned char j=1; j<6; j++)
  {
	  GLCD_GoTo(0,j);
	  for (i=0; i<3; i++)
	  {
		  GLCD_WriteData(0xAA);
		  GLCD_WriteData(0x55);
	  }
	  GLCD_WriteData(0xAA);
	  GLCD_WriteData(0xFF);
	  GLCD_GoTo(88,j);
	  GLCD_WriteData(0xFF);
	  for (i=0; i<19; i++)
	  {
		  GLCD_WriteData(0x55);
		  GLCD_WriteData(0xAA);
	  }
	  GLCD_WriteData(0x55);
  }

  //Page 6
  GLCD_GoTo(0,6);
  for (i=0; i<3; i++)
  {
	  GLCD_WriteData(0xAA);
	  GLCD_WriteData(0x55);
  }
  GLCD_WriteData(0xAA);
  for (i=0; i<41; i++)
  {
	  GLCD_WriteData(0x55);
	  GLCD_WriteData(0xAB);
  }
  for (i=0; i<19; i++)
  {
	  GLCD_WriteData(0x55);
	  GLCD_WriteData(0xAA);
  }
  GLCD_WriteData(0x55);

  //Page 7
  GLCD_GoTo(0,7);
  for (i=0; i<64; i++)
  {
	  GLCD_WriteData(0xAA);
	  GLCD_WriteData(0x55);
  }

  BOX_rewrite_display(black, white);
  BOX_spawn();
}

void BOX_end_game(void)
{
  TCCR1B &= ~(1<<CS12 | 1<<CS11 | 1<<CS10);	//stop timer
  BOX_rewrite_display(white,black);

  GLCD_string_sideways(0,80, (char *)message4, (sizeof(message4) / sizeof(message4[0])));

  while(1) { }
}

void BOX_update_score(void)
{
	//Update the score on the display
}

/**********************************
 * End Display specific functions *
 **********************************/

/**********************************************
 * Functions that handle bits in BOX_location[]
 * BOX_loc_return_bit
 * BOX_loc_set_bit
 * BOX_loc_clear_bit
 ************************************************/

unsigned char BOX_loc_return_bit(unsigned char X, unsigned char Y)
{
  //Calculate array index and shift amount
  unsigned char array_index_offset = ((Y)/8)*(BOX_board_right+1);
  unsigned char shift_index = (Y)%8;		//How much to shift for our bit mask

  if (BOX_location[X+array_index_offset] & 1<<shift_index) return 1;
  else return 0;
}

void BOX_loc_set_bit(unsigned char X, unsigned char Y)
{
  //Calculate array index and shift amount
  unsigned char array_index_offset = ((Y)/8)*(BOX_board_right+1);
  unsigned char shift_index = (Y)%8;		//How much to shift for our bit mask

  BOX_location[X+array_index_offset] |= 1<<shift_index;
}

void BOX_loc_clear_bit(unsigned char X, unsigned char Y)
{
  //Calculate array index and shift amount
  unsigned char array_index_offset = ((Y)/8)*(BOX_board_right+1);
  unsigned char shift_index = (Y)%8;		//How much to shift for our bit mask

  BOX_location[X+array_index_offset] &= ~(1<<shift_index);
}

/********************************
 * Functions that handle bits in BOX_piece[]
 * BOX_piece_return_bit()
 * BOX_piece_set_bit()
 * BOX_piece_clear_bit()
 */

void BOX_store_loc(void)
{
  //Step through 4 columns
  for (unsigned char temp_col=0; temp_col<4; temp_col++)
  {
    //Only if x_loc is not out of bounds
    if ((unsigned char)(x_loc+temp_col) <= BOX_board_right)
    {
      //Step through 4 rows
      for (unsigned char temp_row=0; temp_row<4; temp_row++)
      {
		//Only if y_loc is not out of bounds
		if (y_loc-temp_row <= BOX_board_bottom)
		{
		  if (BOX_piece[temp_col] & 1<<(temp_row))	//Checks nibbles in Box_piece array
		  {
			BOX_loc_set_bit((unsigned char)(x_loc+temp_col),y_loc-temp_row);
		  }
		}
      }
    }
  }
}

void BOX_clear_loc(void)
{
  //Step through 4 columns
  for (unsigned char temp_col=0; temp_col<4; temp_col++)
  {
    //Only if x_loc is not out of bounds
    if ((unsigned char)(x_loc+temp_col) <= BOX_board_right)
    {
      //Step through 4 rows
      for (unsigned char temp_row=0; temp_row<4; temp_row++)
      {
		//Only if y_loc is not out of bounds
		if (y_loc-temp_row <= BOX_board_bottom)
		{
		  if (BOX_piece[temp_col] & 1<<(temp_row))	//Checks nibbles in Box_piece array
		  {
			BOX_loc_clear_bit((unsigned char)(x_loc+temp_col),y_loc-temp_row);
		  }
		}
      }
    }
  }
}

void BOX_load_reference(unsigned char piece, unsigned char rotation)
{
	  BOX_piece[0] = pgm_read_byte(&BOX_reference[piece][rotation][0]);
	  BOX_piece[1] = pgm_read_byte(&BOX_reference[piece][rotation][1]);
	  BOX_piece[2] = pgm_read_byte(&BOX_reference[piece][rotation][2]);
	  BOX_piece[3] = pgm_read_byte(&BOX_reference[piece][rotation][3]);
}

void BOX_rotate(unsigned char direction)
{
  //TODO: Allow for adjustments if rotation is prevented due to proximity

  BOX_clear_loc(); //Clear current location so we don't have false compares

  //Load in the candidate rotation
  unsigned char new_rotate = rotate;
  if (++new_rotate > 3) new_rotate = 0;
  BOX_load_reference(cur_piece,new_rotate);

  //Check for overlaps
  if (BOX_check(0, 0))
  {
	  //Overlap will be caused, restore piece settings and return
	  BOX_load_reference(cur_piece, rotate);
	  BOX_store_loc();
	  return;
  }
  //No overlap found, allow new rotation
  else
  {
	  //Load the current rotation back in and clear the piece from display
	  BOX_load_reference(cur_piece, rotate);
	  BOX_clear_piece();

	  //Load new rotation, display, and save its location
	  rotate = new_rotate;
	  BOX_load_reference(cur_piece, rotate);

	  BOX_store_loc();
	  BOX_write_piece();
  }
}



void BOX_write_piece(void)  //Writes piece to display
{
  for (unsigned char i=0; i<4; i++)  //Step through each of 4 columns
  {
    for (unsigned char j=0; j<4; j++) //Step through each of 4 rows
    {
    //prevent invalid indices from being written
      if ((y_loc-j) >= 0)
      {
		if (BOX_piece[i] & 1<<j)
		{
		  //TODO: change this for different colored playing pieces
		  BOX_draw(x_loc+i, y_loc-j, default_fg_color);
		}
      }
    }
  }
}

void BOX_clear_piece(void)  //Clears piece from display
{
  for (unsigned char i=0; i<4; i++)  //Step through each of 4 columns
  {
    for (unsigned char j=0; j<4; j++) //Step through each of 4 rows
    {
    //prevent invalid indices from being written
      if ((y_loc-j) >= 0)
      {
  		if (BOX_piece[i] & 1<<j)
  		{
  		  //TODO: change this for different colored playing pieces
  		  BOX_erase(x_loc+i, y_loc-j);
  		}
      }
    }
  }
}

void BOX_rewrite_display(unsigned char fgcolor, unsigned char bgcolor)	//Rewrites entire playing area
{
  for (unsigned char cols=0; cols<=BOX_board_right; cols++)
  {
	  for (unsigned char rows=0; rows<=BOX_board_bottom; rows++)
	  {
		  if(BOX_loc_return_bit(cols,rows)) BOX_draw(cols,rows,fgcolor);
		  else BOX_erase(cols,rows);
	  }
  }
}

void BOX_spawn(void)
{
  x_loc = 4;
  y_loc = 1;
  cur_piece = random_piece;
  rotate = 0;

  BOX_load_reference(cur_piece, rotate);  //load from reference



  //Check to see if we've filled the screen
  if (BOX_check(0,0))
  {
	  BOX_end_game();
  }


  BOX_store_loc(); //Store new location
  BOX_write_piece(); //draw piece

}

unsigned char BOX_check(signed char X_offset, signed char Y_offset)
{
	unsigned char temp_area[4] = { 0x00, 0x00, 0x00, 0x00 };
	unsigned char i;
	//Build compare mask in temp_area[]

		//Clear the current piece from BOX_location[] so we don't have a false overlap
			//Do this only if X_offset and Y_offset aren't both 0 (used for BOX_spawn)
		if (X_offset || Y_offset) BOX_clear_loc();
		//mask will be 4 sets of nibbles (2 bytes)
		for (i=0; i<4; i++)
		{
			//if out of bounds on the x axis
			if ((unsigned char)(x_loc+X_offset+i) > BOX_board_right) temp_area[i] = 0x0F;
			else
			{
				for (unsigned char j=0; j<4; j++)
				{
					//if we're out of bounds on the y axis
					if (((unsigned char)(y_loc+Y_offset-j) > BOX_board_bottom) ||
					   (BOX_loc_return_bit((unsigned char)(x_loc+X_offset+i),(unsigned char)(y_loc+Y_offset-j))))
					{
						temp_area[i] |= 1<<j;
					}
				}
			}
		}
		if (X_offset || Y_offset) BOX_store_loc(); //Restore the location we cleared earlier

	if ((temp_area[0] & BOX_piece[0]) | (temp_area[1] & BOX_piece[1]) | (temp_area[2] & BOX_piece[2]) | (temp_area[3] & BOX_piece[3]))
	{
		//Conflict has been found
		return 1;
	}
	else return 0;
}

void BOX_line_check(void)
{
  //TODO: Tweak this to enable scoring

  //Check every line on the playing area for complete rows and record them in an array
  unsigned char complete_lines[4];	//There will never be more than 4 complete rows
  unsigned char temp_index = 0;		//Index for complete_lines[]

  for (unsigned char board_rows=0; board_rows<=BOX_board_bottom; board_rows++)
  {
    unsigned char board_cols=0;
    while ((board_cols<=BOX_board_right) && (BOX_loc_return_bit(board_cols,board_rows)))
    {
    	//Complete row found, record in complete_lines[]
		if (board_cols == BOX_board_right) complete_lines[temp_index++] = board_rows;
		++board_cols;
    }
  }
  if (temp_index == 0) return;  //No complete lines found, return


  //If there are complete rows
    //TODO: Disable interrupts to pause game flow
    //TODO: Add an arbitrary delay, perhaps make complete lines flash?

  --temp_index;	//This was incremented one too many times earlier, get it back to the proper index.

  score += temp_index; //Add the completed rows to our score

  //Rewrite BOX_location[] data without completed lines
  unsigned char read_from_row = BOX_board_bottom;
  unsigned char write_to_row = BOX_board_bottom;

  //When we have read from all rows, this will be set to 0 and
  //remaining bits cleared from BOX_location[]
  unsigned char rows_left_to_read = 1;

  //Use variable i to iterate through every row of the board
  unsigned char i=0;
  while (i <= BOX_board_bottom)
  {
	  //If the current row is a complete line
	  if (read_from_row == complete_lines[temp_index])
	  {
		  //Decrement indexes
		  if (read_from_row == 0)
		  {
			  rows_left_to_read = 0;

			  //Change complete_lines[0] so we don't do this again
			  complete_lines[0] = BOX_board_bottom;
		  }
		  else
		  {
			  --read_from_row;
			  if (temp_index) --temp_index;
		  }
	  }

	  else
	  {
		  //Write data to all columns of current row
		  for (unsigned char col=0; col<=BOX_board_right; col++)
		  {
			  //If there are rows left to read from, do so.
			  if (rows_left_to_read)
			  {
				  if (BOX_loc_return_bit(col,read_from_row)) BOX_loc_set_bit(col, write_to_row);
				  else BOX_loc_clear_bit(col, write_to_row);
			  }
			  //There are no rows left to read from, fill with 0
			  else
			  {
				  BOX_loc_clear_bit(col, write_to_row);
			  }
		  }

		  //A row has now been read from, decrement the counter
		  --read_from_row;

		  //A row has now been written to, increment the counter, decrement the tracker
		  ++i;
		  --write_to_row;
	  }
  }

  BOX_rewrite_display(black, white);
  BOX_update_score();
}


void BOX_up(void)
{
  BOX_clear_loc();
  BOX_clear_piece();

  if (++cur_piece > 6) cur_piece = 0;
  x_loc = 4;
  y_loc = 0;

  BOX_spawn();
}

void BOX_dn(void)
{
  if (BOX_check(0, 1))
  {
    //Set piece here and spawn a new one
    BOX_rewrite_display(black, default_bg_color);
    BOX_line_check();
    BOX_spawn();
    return;
  }

  BOX_clear_loc();
  BOX_clear_piece();
  ++y_loc;

  BOX_store_loc();
  BOX_write_piece();
}

void BOX_lt(void)
{
  if (BOX_check(-1, 0)) return; //Do nothing if moving causes an overlap
  BOX_clear_loc();
  BOX_clear_piece();
  x_loc--;

  BOX_store_loc();
  BOX_write_piece();
}

void BOX_rt(void)
{
  if (BOX_check(1, 0)) return; //Do nothing if moving causes an overlap
  BOX_clear_loc();
  BOX_clear_piece();
  ++x_loc;

  BOX_store_loc();
  BOX_write_piece();
}