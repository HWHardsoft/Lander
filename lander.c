/*
 *  Uzebox quick and dirty tutorial
 *  Copyright (C) 2008  Alec Bourque
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <uzebox.h>


#include "data/patches.inc"
#include "data/background.pic.inc" 
#include "data/rocketsprites.pic.inc" 
#include "data/backgroundTiles.map.inc" 
#include "data/rocketman-track.inc"


//Initialize a struct and define the block id
// 3 datasets: 8 byte name; 2 byte highscore

struct EepromBlockStruct ebs;



// function prototypes
void init(void);
void LanderPrintInt(int x,int y, unsigned int val,bool zeropad,u8 digits);
void LanderPrintByte(int x,int y, unsigned char val,bool zeropad);
void LanderPrint(int x, int y, const char *s);
void LanderPrintI(int x, int y, const char *s);
char get_map_tile(unsigned char x,unsigned char y,const char *map);
char get_sprite_tile(unsigned char x,unsigned char y);
char get_tile_pixel(unsigned char x,unsigned char y,char tile, const char *tileset);
void calculate_position(u8 angle, u8 B_engine);
char checkcollision(u8 xx, u8 yy, u8 angle, const char *map);
void animate_rocket(u8 xx, u8 yy, u8 angle, u8 B_engine);
void set_PM_mode(u8 mode);
void msg_window(u8 x1, u8 y1, u8 x2, u8 y2);
void view_bottom_line(u8 level);
int randomize(void);
void ready_steady_go(const char *map);
u8 set_def_highscore(void);
u8 view_highscore_entry(u8 x, u8 y, u8 entry, u8 load_data);
void clear_highsore(u8 entry);
u8 check_highscore(void);
void copy_highsore(u8 entry_from, u8 entry_to);
void edit_highscore_entry(u8 entry, u8 cursor_pos, u8 b_mode);
void show_highscore_char(u8 entry, u8 position, u8 cursor_on);
void fade_out_volume(void);


/* global definitons */
// program modes
#define PM_Intro	0	// program mode intro
#define PM_Level1	1	// program mode level 1
#define PM_Level2	2	// program mode level 2
#define PM_Level3	3	// program mode level 3
#define PM_Level4	4	// program mode level 4
#define PM_Level5	5	// program mode level 5
#define PM_Level6	6	// program mode level 6
#define PM_Level7	7	// program mode level 7
#define PM_Crash	10	// program mode crash
#define PM_HoF_view 20	// program mode view hall of fame
#define PM_HoF_edit 21	// program mode edit hall of fame

// angles
#define angle_0		0	// 0 degree									
#define angle_45	1	// 45 degree						0°	
#define angle_90	2	// 90 degree				   45°		315°		
#define angle_135	3	// 135 degree	
#define angle_180	4	// 180 degree				90°		R	   270°
#define angle_225	5	// 225 degree					
#define angle_270	6	// 270 degree				   135°		225°
#define angle_315	7	// 315 degree					   180°


#define gravity			2  // gravity 2 m/s²
#define e_acceleration	4	// engine acceleration 4 m/s²
#define delta_t		1		// 1s time difference for calculation



/* globale variable definitions */
int iFuel=300;		// fuel capacity
int iSCORE;			// Highscore
u8 program_mode;	// program mode (intro, level1, level2, ....
int ibuttons;		// status of SNES controller 1
u8 ani_count=0;		// counter for animated sprite frame
int V0H=0;			// v0 horizontal
int V0V=0;			// v0 vertical
int Xold=12000;		// old X position
int Yold=0;			// old Y position
u8 edit_entry;		// entry in hall o fame 	 
u8 cursor;			// cursor to edit entry 
u8 Music_on = true;	// set the background music on/off	 


void init(void)
// init program
{
  // init music	
  InitMusicPlayer(patches);
  //Set sprites to use.
  SetSpritesTileTable(rocket);	
  // load into screen
  set_PM_mode(PM_Intro);
  //Use block 20
  ebs.id = 20;
  if (!isEepromFormatted())
     return;

  if (EEPROM_ERROR_BLOCK_NOT_FOUND == EepromReadBlock(20,&ebs))
  {
	set_def_highscore();
  }	
}


int main(){
u8 rocket_angle=0;
u8 ibuttons_old;
int y_pos=0;
int x_pos=0;
u8 a=99,b = 0;

  // init program
  init();        
  // proceed game	
  while(1)
  {
    WaitVsync(2);	  
    // edit an entry in the hall of fame
    // get controller state
    ibuttons_old = ibuttons;
	ibuttons = ReadJoypad(0);
    switch(program_mode)
	{
	  case PM_Intro:
		// display blinking text:
		if (b == 14) {
		  LanderPrintI(9,12,PSTR("PRESS START"));		  
		  b = 0;
	
		} else if (b==7) {
		  LanderPrintI(9,12,PSTR("           "));

		}
		b++;
		// check start button
		if (BTN_START & ibuttons) {
		  b = 0;
		  rocket_angle=0;	
		  set_PM_mode(PM_Level1); 	
		}

		if (BTN_A & ibuttons) {
		    iSCORE = 0; 
			b = 0;
		    set_PM_mode(PM_HoF_view);
			
		}
		if (a == b) { 
		  LanderPrintI(10,15,PSTR("         ")); 
		  a = 99;
        } 
		if ((ibuttons & BTN_B) && !(ibuttons_old & BTN_B)) {
		  if (Music_on) {
		    Music_on = false;
			LanderPrintI(10,15,PSTR("MUSIC OFF"));
		  } else {  
		    Music_on = true;
			LanderPrintI(10,15,PSTR("MUSIC ON"));
		  }			    
		  a = b;	
		}		
	    break;	      


	  case PM_Crash:	
	  case PM_Level1:
	  case PM_Level2:
	  case PM_Level3:
	  case PM_Level4:
	  case PM_Level5:
	  case PM_Level6:
	  case PM_Level7:
	  	
		if ((ibuttons & BTN_RIGHT) && !(ibuttons_old & BTN_RIGHT)) {
		 if (rocket_angle == 0) rocket_angle = 8; 
		 rocket_angle--;
		}
		if ((ibuttons & BTN_LEFT) && !(ibuttons_old & BTN_LEFT)) {
		 rocket_angle++;
 		 if (rocket_angle >= 8) rocket_angle = 0; 
		}

    	// calculate and display fuel capacity 
 		if ((iFuel) && (ibuttons & BTN_A) && (program_mode != PM_Crash)) {
	  	  iFuel--;	      
		  TriggerFx(5, 70, false);
	    }
		LanderPrintInt(8,26,iFuel,1, 3);
     	


		// calculate speed, acceleration and position of the rocket
		if (!(b)) calculate_position(rocket_angle,(ibuttons & BTN_A) && (iFuel));
		if (Xold < 0) Xold = 0;
		if (Xold > 22400) Xold = 22400;
		if (Yold < 0) Yold = 0;
		x_pos = Xold / 100;
		y_pos = Yold / 100;

		// check for collision of rocket with the ground and save landing
		if (program_mode == PM_Level1) b = checkcollision(x_pos,y_pos,rocket_angle,Level1_map); 
		else if(program_mode == PM_Level2) b = checkcollision(x_pos,y_pos,rocket_angle,Level2_map); 
		else if(program_mode == PM_Level3) b = checkcollision(x_pos,y_pos,rocket_angle,Level3_map); 
		else if(program_mode == PM_Level4) b = checkcollision(x_pos,y_pos,rocket_angle,Level4_map); 
		else if(program_mode == PM_Level5) b = checkcollision(x_pos,y_pos,rocket_angle,Level5_map); 
		else if(program_mode == PM_Level6) b = checkcollision(x_pos,y_pos,rocket_angle,Level6_map); 
		else if(program_mode == PM_Level7) b = checkcollision(x_pos,y_pos,rocket_angle,Level7_map); 
		
		// crash
		if (b == 3) {
		   if (program_mode != PM_Crash) {
		     program_mode = PM_Crash;
		     ani_count = 0;														
           }
        // save landing 
		} else if ((b==1) || (b==2)) {	
		  // switch jet engine off
		  MapSprite2(1,rocket_Map_0_OFF,0);
		  // generate message
		  msg_window(8,10,22,15);
		  LanderPrint(11,12,PSTR("THE EAGLE"));
		  LanderPrint(10,13,PSTR("HAS LANDED!"));
		  // calculate Highscore			   	
          for(;iFuel > -1;iFuel--) { 	
		    iSCORE += b;	
			LanderPrintInt(8,26,iFuel,1, 3);    
			LanderPrintInt(28,26,iSCORE,1, 4);
			if ((iSCORE % 10) == 0) TriggerFx(4, 0xff, true);
		  	WaitVsync(1);
		  } 
		  WaitVsync(60);
       	  
		  if (program_mode == PM_Level1) set_PM_mode(PM_Level2);
		  else if(program_mode == PM_Level2) set_PM_mode(PM_Level3);
		  else if(program_mode == PM_Level3) set_PM_mode(PM_Level4);
		  else if(program_mode == PM_Level4) set_PM_mode(PM_Level5);
		  else if(program_mode == PM_Level5) set_PM_mode(PM_Level6);
		  else if(program_mode == PM_Level6) set_PM_mode(PM_Level7);
		  else if(program_mode == PM_Level7) {
		    fade_out_volume(); 
			b = 0;
		    set_PM_mode(PM_HoF_view);
          }
		  		  
		  break;	
   		}

		animate_rocket(x_pos,y_pos,rocket_angle,(ibuttons & BTN_A) && (iFuel));
		break;

	  case PM_HoF_edit:				
		// cursor blinking
		b++;
		if (b >= 10) b = 0;
		// proceed cursor position with left & right button
		if ((ibuttons & BTN_RIGHT) && !(ibuttons_old & BTN_RIGHT)) {
		  if (cursor < 7) {       	   
		  	show_highscore_char(edit_entry - 1, cursor, 0); 		 
		    cursor++; 			
          }
		}
		if ((ibuttons & BTN_LEFT) && !(ibuttons_old & BTN_LEFT)) {		 
 		  if (cursor) {
		    show_highscore_char(edit_entry - 1, cursor, 0); 
		    cursor--; 
          }
		}
		// chose character up & down button
		if ((ibuttons & BTN_UP) && !(ibuttons_old & BTN_UP)) {
		  edit_highscore_entry(edit_entry,cursor,BTN_UP); 
		}
		else if ((ibuttons & BTN_DOWN) && !(ibuttons_old & BTN_DOWN)) {		 
 		  edit_highscore_entry(edit_entry,cursor,BTN_DOWN); 
		}     
		// show cursor
		show_highscore_char(edit_entry - 1, cursor, b > 4);

		// store new entry
		if (ibuttons & BTN_A)   
		{
		  // store new highscore 
		  EepromWriteBlock(&ebs);
		  set_PM_mode(PM_Intro);
		}
		break;

	  case PM_HoF_view:				
		// time control		
		WaitVsync(2);
		if (b <= 200) b++;
		if (b == 100) LanderPrint(8,24,PSTR("PRESS BUTTON A"));
		if (((ibuttons & BTN_A) && !(ibuttons_old & BTN_A) && (b > 50)) || (b > 200))
		{
		  if (Music_on) fade_out_volume();
		  b = 0;	
		  set_PM_mode(PM_Intro);
		}
		break;


    }
		
  }
	
} 


// get the tile from the actual background map
char get_map_tile(unsigned char x,unsigned char y,const char *map) {
  x /= 8;
  y /= 8;
  return(pgm_read_byte(&(map[(30 * y) + 2 + x])));
}


// get the tile from the actual sprite
char get_sprite_tile(unsigned char x,unsigned char y) {
  if (x < 8) {
    if (y < 8) return(sprites[1].tileIndex);
    else return(sprites[3].tileIndex);

  } else {
    if (y < 8) return(sprites[2].tileIndex);
    else return(sprites[4].tileIndex);  
  
  }  
}


// get pixel value from a tile
char get_tile_pixel(unsigned char x,unsigned char y,char tile, const char *tileset) {
  
  return(pgm_read_byte(&(tileset[(64 * tile) + (8 * y) + x])));
}



// check the collision of the rocket with the ground
char checkcollision(u8 xx, u8 yy, u8 angle, const char *map) {
// returns 0 for no collision
// returns 1 for save landing on landing zone A
// returns 2 for save landing on landing zone B
// returns 3 for ground collision
u8 x,y;
u8 left_edge_tile, right_edge_tile;
unsigned char pix_sprite, pix_bg, tile_bg;
  // check landing

  // check collision
  for(y = 2; y < 14; y++)
  for(x = 0; x < 16; x+=2)
  {
    tile_bg = get_map_tile(xx + x, yy + y,map);
	if (((tile_bg >= 4) && (tile_bg <= 9)) || ((tile_bg >= 88) && (tile_bg <= 98)))
	{
	  pix_bg = get_tile_pixel((xx + x) % 8,(yy + y) % 8,tile_bg,backgroundTileset);      
	  if (angle < angle_225) pix_sprite = get_tile_pixel(x % 8,y % 8,get_sprite_tile(x,y),rocket);
	  else pix_sprite = get_tile_pixel((15 - x) % 8,y % 8,get_sprite_tile(x ,y),rocket);
      
	  // crash?
	  if ((pix_sprite != 0xFE) && (pix_bg)) {
	  	// save landing ?
	    if ((angle == angle_0) && (V0H < 40) && (V0V< 40) && ((pix_bg == 0xD9) || (pix_bg == 0x06))) {
		  // check position of landing zone
		  left_edge_tile = get_map_tile(xx + 2, yy + 12,map);
		  right_edge_tile = get_map_tile(xx + 12, yy + 12,map);

		  if ((left_edge_tile == 0x08) && (right_edge_tile == 0x08)) return(1);
		  if ((left_edge_tile == 0x09) && (right_edge_tile == 0x09)) return(2);	

	    }
	    //LanderPrintInt(21,26,V0H,1,5);			  	        	  		  
	    //LanderPrintInt(21,27,V0V,1,5);		
		/*	
	    LanderPrintByte(25,26,x,1);			  	        	  		  
	    LanderPrintByte(25,27,xx,1);		

		LanderPrintByte(29,26,y,1);	  	
        LanderPrintByte(29,27,yy,1);
		*/
	    return(3); 
      }
	}
  }
  return(0);
}



//Print an unsigned byte in decimal
void LanderPrintByte(int x,int y, unsigned char val,bool zeropad){
	unsigned char c,i;

	for(i=0;i<3;i++){
		c=val%10;
		if(val>0 || i==0){
			// SetFont(x--,y,c+CHAR_ZERO+RAM_TILES_COUNT);
			SetTile(x--, y, c + FONT_OFFSET + 0x0F);
		}else{
			if(zeropad){
				// SetFont(x--,y,CHAR_ZERO+RAM_TILES_COUNT);
				SetTile(x--, y, c + FONT_OFFSET + 0x0F);
			}else{
				// SetFont(x--,y,0+RAM_TILES_COUNT);
				SetTile(x--, y, 0 + FONT_OFFSET + 0x0F);
			}
		}
		val=val/10;
	}
		
}

//Print an unsigned int in decimal
void LanderPrintInt(int x,int y, unsigned int val,bool zeropad, u8 digits){
	unsigned char c,i;

	for(i=0;i<digits;i++){
		c=val%10;
		if(val>0 || i==0){
			//SetFont(x--,y,c+CHAR_ZERO+RAM_TILES_COUNT);
			SetTile(x--, y, c + FONT_OFFSET + 0x0F);
		}else{
			if(zeropad){
				// SetFont(x--,y,CHAR_ZERO+RAM_TILES_COUNT);
				SetTile(x--, y, c + FONT_OFFSET + 0x0F);
			}else{
				// SetFont(x--,y,0+RAM_TILES_COUNT);
				SetTile(x--, y, 0 + FONT_OFFSET + 0x0F);
			}
		}
		val=val/10;
	}
		
}



void LanderPrint(int x, int y, const char *s) {
// print text in game screens
	u8 c;
	int i = 0;

	while (1) {
		c = pgm_read_byte(&(s[i++]));

		if (c) {
			
			while (1) {
				if (c == ' ') {
					SetTile(x++, y, 0);	// space
					break;
				} else  {
					SetTile(x++, y, c + FONT_OFFSET - 0x21);
					break;
				}
				
			}			
		} else {
			break;
		}
	}
}


void LanderPrintI(int x, int y, const char *s) {
// print text in intro screen
	u8 c;
	int i = 0;

	while (1) {
		c = pgm_read_byte(&(s[i++]));

		if (c) {
			
			//while (1) {
			    switch (c) {

					case ' ':
    					SetTile(x++, y, 0);					
						break;

					case '.':
    					SetTile(x++, y, FONT_OFFSET_INTRO + 2);					
						break;

					case '$':
    					SetTile(x++, y, FONT_OFFSET_INTRO + 1);					
						break;

					case '0':
					case '1':
					case '2':
					case '3':
    					SetTile(x++, y, c + FONT_OFFSET_INTRO - 0x2D);					
						break;

				  	default:
						SetTile(x++, y, c + FONT_OFFSET_INTRO - 0x3A); // A..Z

				}
				
			//}			
		} else {
			break;
		}
	}
}



void msg_window(u8 x1, u8 y1, u8 x2, u8 y2) {
// draw a window with frame and black backgound on the screen

    // window backgound
	Fill(x1 + 1, y1 + 1, x2 - x1 - 1, y2 - y1 - 1,0);
	// upper frame
	Fill(x1 + 1, y1, x2 - x1 - 1, 1,162);
	// lower frame
	Fill(x1 + 1, y2, x2 - x1 - 1, 1,162);
	// left frame
	Fill(x1 , y1 + 1, 1, y2 - y1 - 1,165);
	// right frame
	Fill(x2, y1 + 1, 1 , y2 - y1 - 1,165);
	// upper, left corner
	SetTile(x1,y1,161);
	// upper, right corner
	SetTile(x2,y1,163);
	// lower, left corner
	SetTile(x1,y2,164);
	// lower, right corner
	SetTile(x2,y2,166);
}	




void calculate_position(u8 angle, u8 B_engine) {
// calculate x and y coordinates
char a_v = gravity;	//acceleration vertical
char a_h = 0;		//acceleration horizontal


	// set acceleration	
	switch (angle)
	{
		case angle_0:	//0 degree
			if (B_engine) a_v = gravity - e_acceleration;					
			break;

		case angle_45:	//45 degree
			if (B_engine) {
			  a_v = gravity - (e_acceleration / 2);	
			  a_h = - (e_acceleration / 2);
			}				
			break;

		case angle_315:	//315 degree
			if (B_engine) {
			  a_v = gravity - (e_acceleration / 2);	
			  a_h = e_acceleration / 2;	
			}  					
			break;

		case angle_90:	//90 degree			
			if (B_engine) a_h = - (e_acceleration);				 			
			break;

		case angle_270:	//270 degree
     		if (B_engine) a_h = e_acceleration;				 			
			break;

		case angle_135:	//135 degree
			if (B_engine) {
			  a_v = gravity + (e_acceleration / 2);	
			  a_h = - (e_acceleration / 2);				
			}
			break;

		case angle_225:	//225 degree
			if (B_engine) {
			  a_v = gravity + (e_acceleration / 2);	
			  a_h = e_acceleration / 2;					
            } 
			break;

		case angle_180:	//180 degree
			if (B_engine) a_v = gravity + e_acceleration;					
			break;

	}	
	// calculate speed
	V0V = (delta_t * a_v) + V0V;
	//yy = V0V * delta_t;
	V0H = (delta_t * a_h) + V0H;
	//xx = V0H * delta_t;
	// calculate posiion
	Xold += (V0H * delta_t);
	Yold += (V0V * delta_t);
			
}



void animate_rocket(u8 xx, u8 yy, u8 angle, u8 B_engine) {
// draw and animate the rocket
u8 spriteflag = 0;

  if (program_mode == PM_Crash) {
  
	// rocket explosion
    switch (ani_count)
	{
		case 0: // 1st picture for explosion
		  TriggerFx(3, 0xff, true);
		  MapSprite2(1,rocket_Map_exp_0,0);	
		  break;

		case 1: // 2nd picture for explosion
		  MapSprite2(1,rocket_Map_exp_1,0);	
		  break;
		
		case 2: // 3rd picture for explosion
		  MapSprite2(1,rocket_Map_exp_2,0);	
		  break;
		
		case 3: // 4th picture for explosion
		  MapSprite2(1,rocket_Map_exp_3,0);	
		  break;

		case 4: // 5th picture for explosion
		  MapSprite2(1,rocket_Map_exp_4,0);	
		  break;
		
		case 5: // 6st picture for explosion
		  MapSprite2(1,rocket_Map_exp_5,0);	
		  break;

		case 6: // 7st picture for explosion
		  MapSprite2(1,rocket_Map_exp_6,0);	
		  break;

		case 7: // 8st picture for explosion
		  MapSprite2(1,rocket_Map_exp_7,0);	
		  break;

		case 8: // 9st picture for explosion
		  MapSprite2(1,rocket_Map_exp_8,0);	
		  break;

		case 9: // delete last picture of explosion
		  MapSprite2(1,rocket_Map_hide,0);	
		  // message 	
		  msg_window(8,10,22,15);
		  LanderPrint(10,12,PSTR("YOU MADE A"));
		  LanderPrint(10,13,PSTR("NEW CRATER!"));
		  fade_out_volume();
		  break;
		

        case 60:
		  set_PM_mode(PM_HoF_view);
		  break;

    }
	WaitVsync(3);
  } else { 
    // normal rocket flight
 	switch (angle)
	{
		case angle_0:	//0 degree
		  
		  if (B_engine) {
		    //button A is pressed
		    if (ani_count & 0x01) MapSprite2(1,rocket_Map_0_ON2,spriteflag);
		  	else MapSprite2(1,rocket_Map_0_ON1,spriteflag);
					
		  } else {
		  	// button A is unpressed
		    MapSprite2(1,rocket_Map_0_OFF,spriteflag);
		   	
		  }	
		  break;	

		case angle_315:	//315 degree
			spriteflag = SPRITE_FLIP_X;

		case angle_45:	//45 degree
		  
		  if (B_engine) {
		    //button A is pressed
		    if (ani_count & 0x01) MapSprite2(1,rocket_Map_45_ON2,spriteflag);
		  	else MapSprite2(1,rocket_Map_45_ON1,spriteflag);
					
		  } else {
		  	// button A is unpressed
		    MapSprite2(1,rocket_Map_45_OFF,spriteflag);
		   	
		  }	
		  break;
		  	
		case angle_270:	//270 degree
			spriteflag = SPRITE_FLIP_X;

		case angle_90:	//90 degree
		  
		  if (B_engine) {
		    //button A is pressed
		    if (ani_count & 0x01) MapSprite2(1,rocket_Map_90_ON2,spriteflag);
		  	else MapSprite2(1,rocket_Map_90_ON1,spriteflag);
					
		  } else {
		  	// button A is unpressed
		    MapSprite2(1,rocket_Map_90_OFF,spriteflag);
		   	
		  }	
		  break;	

		case angle_225:	//225 degree
			spriteflag = SPRITE_FLIP_X;

		case angle_135:	//135 degree
		  
		  if (B_engine) {
		    //button A is pressed
		    if (ani_count & 0x01) MapSprite2(1,rocket_Map_135_ON2,spriteflag);
		  	else MapSprite2(1,rocket_Map_135_ON1,spriteflag);
					
		  } else {
		  	// button A is unpressed
		    MapSprite2(1,rocket_Map_135_OFF,spriteflag);
		   	
		  }	
		  break;	
	
		case angle_180:	//180 degree
		  
		  if (B_engine) {
		    //button A is pressed
		    if (ani_count & 0x01) MapSprite2(1,rocket_Map_180_ON2,spriteflag);
		  	else MapSprite2(1,rocket_Map_180_ON1,spriteflag);
					
		  } else {
		  	// button A is unpressed
		    MapSprite2(1,rocket_Map_180_OFF,spriteflag);
		   	
		  }	
		  break;

	}
	
  }
  MoveSprite(1,xx,yy,2,2);
	
  ani_count++;
  if (ani_count >= 200) ani_count = 0;

}


void set_PM_mode(u8 mode) {
// set parameters, tiles, background etc for choosed program mode
	
	
	switch (mode)
	{
	  
	  case	PM_Intro:
	  	
	    SetSpriteVisibility(false);
	    StopSong();
		ClearVram();
		WaitVsync(2);
		//Set the font and tiles to use.
  		SetTileTable(introTileset);
		// draw lander logo
  		DrawMap2(7,1,lander_logo_Map);
     	LanderPrintI(9,6,PSTR("VERSION 1.1"));


     	LanderPrintI(5,19,PSTR("$ 2011 HARTMUT WENDT"));
		LanderPrintI(2,21,PSTR("SOUNDTRACK BX CARSTEN KUNY"));
		
		LanderPrintI(2,24,PSTR("LICENCED UNDER GNU GPL V3"));
		LanderPrintI(5,26,PSTR("WWW.HWHARDSOFT.DE.VU"));
		WaitVsync(10);
		SetMasterVolume(130);	
		break;

      case	PM_HoF_view:
	    SetMasterVolume(130);
		if (Music_on) StartSong(champions_song);
	    SetSpriteVisibility(false);
	    edit_entry = check_highscore();
	    if (edit_entry == 2) copy_highsore(1,2);
	    if (edit_entry == 1) {
		  copy_highsore(1,2);
		  copy_highsore(0,1);
        }
		clear_highsore(edit_entry - 1);
		// reset cursor to left position
		cursor = 0;
	  	//Set the font and tiles to use.
  		SetTileTable(backgroundTileset);
	    ClearVram();
		msg_window(7,8,23,19);	
		LanderPrint(9,10,PSTR("HALL OF FAME"));
        view_highscore_entry(9,13,1,!(edit_entry));
        view_highscore_entry(9,15,2,!(edit_entry));
        view_highscore_entry(9,17,3,!(edit_entry));  
		if (edit_entry == 0) {
		  //WaitVsync(800); 
		  //set_PM_mode(PM_Intro);     
		  //mode = PM_Intro;
		} else {
		  mode = PM_HoF_edit;
		  LanderPrint(8,3,PSTR("CONGRATULATION"));	
		  LanderPrint(8,5,PSTR("NEW HIGHSCORE!"));	
		  LanderPrint(7,22,PSTR("ENTER YOUR NAME"));	
		  LanderPrint(6,24,PSTR("AND PRESS BUTTON A"));	
		}
		break;	  

	  case	PM_Level1:
	    Xold = 10000;
		Yold = 0;
		iFuel =500;
		V0V = 0;
		V0H = 0;
		iSCORE = 0;			
    	//Set the font and tiles to use.
  		SetTileTable(backgroundTileset);
	    ClearVram();
	  	// draw level1 map
  		DrawMap2(0,0,Level1_map);
		// draw bottom text line
		view_bottom_line(1);
		//start sound track
		if (Music_on) StartSong(rocketman_song);		
		ready_steady_go(Level1_map);
		SetSpriteVisibility(true);	
		break;

 	  case	PM_Level2:
	    Xold = 11000;
		Yold = 0;
		iFuel= 400;
		V0V = 50;
		V0H = 0;			  
	    ClearVram();
	  	// draw level1 map
  		DrawMap2(0,0,Level2_map);
		// draw bottom text line
		view_bottom_line(2);
		ready_steady_go(Level2_map);		
		SetSpriteVisibility(true);
		break;

 	  case	PM_Level3:
	    Xold = 10000;
		Yold = 0;
		iFuel = 400;
		V0V = 40;
		V0H = -40;			  
	    ClearVram();
	  	// draw level3 map
  		DrawMap2(0,0,Level3_map);
		// draw bottom text line
		view_bottom_line(3);
		ready_steady_go(Level3_map);
		SetSpriteVisibility(true);
		break;

 	  case	PM_Level4:
	    Xold = 10000;
		Yold = 0;
		iFuel = 350;
		V0V = 20;
		V0H = 20;			  
	    ClearVram();
	  	// draw level4 map
  		DrawMap2(0,0,Level4_map);
		// draw bottom text line
		view_bottom_line(4);
		ready_steady_go(Level4_map);
		SetSpriteVisibility(true);
		break;

 	  case	PM_Level5:
	    Xold = 10000;
		Yold = 0;
		iFuel = 400;
		V0V = 20;
		V0H = 00;	
		//Set the font and tiles to use.
  		SetTileTable(backgroundTileset);		  
	    ClearVram();
	  	// draw level5 map
  		DrawMap2(0,0,Level5_map);
		// draw bottom text line
		view_bottom_line(5);
		ready_steady_go(Level5_map);
		SetSpriteVisibility(true);
		break;

 	  case	PM_Level6:
	    Xold = 200;
		Yold = 0;
		iFuel = 550;
		V0V = 20;
		V0H = 00;	
		//Set the font and tiles to use.
  		SetTileTable(backgroundTileset);		  
	    ClearVram();
	  	// draw level6 map
  		DrawMap2(0,0,Level6_map);
		// draw bottom text line
		view_bottom_line(6);
		ready_steady_go(Level6_map);
		SetSpriteVisibility(true);
		break;

	  case	PM_Level7:
	    Xold = 27000;
		Yold = 0;
		iFuel = 600;
		V0V = 20;
		V0H = 00;	
		//Set the font and tiles to use.
  		SetTileTable(backgroundTileset);		  
	    ClearVram();
	  	// draw level6 map
  		DrawMap2(0,0,Level7_map);
		// draw bottom text line
		view_bottom_line(7);
		ready_steady_go(Level7_map);
		SetSpriteVisibility(true);
		break;


	}
	program_mode = mode;

}


void view_bottom_line(u8 level) {
// views the bottom line 
 	LanderPrint(1,26,PSTR("FUEL:"));
	LanderPrintByte(16,26,level,true);
	LanderPrint(10,26,PSTR("LEVEL-"));
	LanderPrint(19,26,PSTR("SCORE:"));
	LanderPrintInt(28,26,iSCORE,1, 4);
	WaitVsync(10);
}


void ready_steady_go(const char *map) {
// ready steady go count down
u8 x,y;
    SetSpriteVisibility(false);
	msg_window(10,9,20,17);
	LanderPrint(12,11,PSTR("READY,"));
	WaitVsync(60);
	LanderPrint(12,13,PSTR("STEADY,"));	
	WaitVsync(60);
	LanderPrint(12,15,PSTR("GO!"));	
	WaitVsync(60);
	// restore background
	for(y = 9; y<=17; y++)
	for(x = 10; x<=20; x++) {
	  SetTile(x,y,pgm_read_byte(&(map[(30 * y) + 2 + x])));

	}

}



u8 check_highscore(void) {
// check the actual highsore
u8 a;
int i1;
   // read the eeprom block
  if (!isEepromFormatted() || EepromReadBlock(20, &ebs))
        return(0);   
  for(a=0; a<3; a++) {
    i1 = (ebs.data[(a * 10)+8] * 256) + ebs.data[(a * 10)+9];
    if (iSCORE > i1) return(a + 1);
  }

  // highscore is lower as saved highscores 
  return(0);
}



void copy_highsore(u8 entry_from, u8 entry_to) {
// copy a highscore entry to another slot
u8 a;
   // read the eeprom block
  for(a=0; a<10; a++) {
    ebs.data[(entry_to * 10) + a] = ebs.data[(entry_from * 10) + a];
  } 
}


void clear_highsore(u8 entry) {
// clear the name in actual entry and set the score to highscore
u8 a;
  // clear name 
  for(a=0; a<8; a++) {
    ebs.data[(entry * 10) + a] = 0x20;
  }   
  // set score
  ebs.data[(entry * 10) + 8] = iSCORE / 256;
  ebs.data[(entry * 10) + 9] = iSCORE % 256;
}



u8 set_def_highscore(void) {
// write the default highscore list in the EEPROM
  // entry 1
  ebs.data[0] = 'H';
  ebs.data[1] = 'A';
  ebs.data[2] = 'R';
  ebs.data[3] = 'T';
  ebs.data[4] = 'M';
  ebs.data[5] = 'U';
  ebs.data[6] = 'T';
  ebs.data[7] = ' ';
  ebs.data[8] = 0x02;
  ebs.data[9] = 0xBC;
  // entry 2
  ebs.data[10] = 'M';
  ebs.data[11] = 'A';
  ebs.data[12] = 'R';
  ebs.data[13] = 'I';
  ebs.data[14] = 'E';
  ebs.data[15] = ' ';
  ebs.data[16] = ' ';
  ebs.data[17] = ' ';
  ebs.data[18] = 0x02;
  ebs.data[19] = 0x58;
  // entry 3
  ebs.data[20] = 'L';
  ebs.data[21] = 'U';
  ebs.data[22] = 'K';
  ebs.data[23] = 'A';
  ebs.data[24] = 'S';
  ebs.data[25] = ' ';
  ebs.data[26] = ' ';
  ebs.data[27] = ' ';
  ebs.data[28] = 0x01;
  ebs.data[29] = 0xF4;
  return(EepromWriteBlock(&ebs));
}


u8 view_highscore_entry(u8 x, u8 y, u8 entry, u8 load_data) {
// shows an entry of the higscore
u8 a,c;

  // read the eeprom block
  if (load_data)
  {
    if (!isEepromFormatted() || EepromReadBlock(20, &ebs))
        return(1);   
  }
  entry--;
  for(a = 0; a < 8;a++) {
	c = ebs.data[a + (entry * 10)];    
	if (c == ' ') SetTile(x + a, y, 0);	// space
	else SetTile(x + a, y, c + FONT_OFFSET - 0x21);

  }
  LanderPrintInt(x + 12, y, (ebs.data[(entry * 10)+8] * 256) + ebs.data[(entry * 10)+9], true, 4);
  return(0);
}


void edit_highscore_entry(u8 entry, u8 cursor_pos, u8 b_mode) {
// edit and view and char in the name of choosed entry    
entry--;
u8 c = ebs.data[(entry * 10) + cursor_pos];
  // proceed up & down button
  if (b_mode == BTN_UP) {
     c++;
     if (c > 'Z') c = ' '; 
     else if (c == '!') c = 'A';
  }
  if (b_mode == BTN_DOWN) {		 
     c--;      
     if (c == 0x1F) c = 'Z';
	 else if (c < 'A') c = ' ';
  }
  ebs.data[(entry * 10) + cursor_pos] = c;

}


void show_highscore_char(u8 entry, u8 position, u8 cursor_on) {
// shows a char of edited name
u8 c = ebs.data[(entry * 10) + position];
    if (cursor_on) SetTile(9 + position, (entry * 2) + 13, FONT_OFFSET + 2);   // show '_'
    else if (c == ' ') SetTile(9 + position, (entry * 2) + 13, 0);	// space
    else SetTile(9 + position, (entry * 2) + 13, c + FONT_OFFSET - 0x21); 	
}


void fade_out_volume(void) {
// fade out the master volume
u8 a;
 for(a = 130; a ; a--) {
   SetMasterVolume(a);	
   WaitVsync(1);
 } 
 StopSong();
 WaitVsync(3);

}
