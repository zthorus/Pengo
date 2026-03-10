// Pengo

// By S. Morel, Zthorus-Labs, 2026

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// Map (playground) constants

#define X_MAP 15  // Map size (including borders)
#define Y_MAP 17
#define ICE 1       // Regular ice cube
#define DIAMOND 2   // Diamond cube
#define BORDER 3    // Border
#define W_BRDR 8    // Border width (in pixels)
#define W_HDR 11    // Header (displaying info like score) in pixels

// Pengo states

#define IDLE 0
#define MOVING 1
#define PUSHING 2
#define CRASHING 3
#define ELEC 4

// Snobee states

#define ACTIVE 0
#define REBIRTH 9     // Any state between 1 and 9 is "rebirth"
#define DEAD 10 
#define PUSHED 70
#define CRUSHED 80
#define SHOCKED -6    // Any state between -6 and -1 is "shocked"

#define MAX_PUSH 10   // Maximum number of cubes being pushed
#define MAX_CRASH 10  // Maximum number of cubes being crashed
#define NB_SNOBEE 3   // Number of snobees

#define HORIZONTAL 0  // Axes of Snobee motions
#define VERTICAL   1

#define NB_SPRITES 32  // Number of 16x16 sprites defined

// Codes of keys (similar to keysymdef.h)

#define LEFT_ARROW_K  0xFF51 // XK_Left
#define RIGHT_ARROW_K 0xFF53 // XK_Right
#define UP_ARROW_K    0xFF52 // XK_Left
#define DOWN_ARROW_K  0xFF54 // XK_Right
#define QUIT_K 0x0051        // Doesn't seem to work
#define PAUSE_K 0x0050
#define ESC_K 0xFF1B
 
void MovePengo(int **map,int png_x, int png_y, int png_dx, int png_dy, int *png_state, int elec_cnt);

void NewCubePushed(int **map,int *psh_flag,int *psh_x,int *psh_y, int *psh_dx,int *psh_dy,int *psh_typ,int p_x,int p_y,int dp_x,int dp_y);

void NewCubeCrashed(int *crsh_flag,int *crsh_x,int *crsh_y,int x,int y);

void CheckSnobeePushed(XImage *xim, unsigned long *lut,int **map,int *snb_x,int *snb_y,int *snb_dx,int *snb_dy,int *snb_state, int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy,int halfway);

void MoveSnobees(int **map,int *snb_state, int *snb_x, int *snb_y, int *snb_dx, int *snb_dy, int *snb_dm, int *snb_ax, int p_x, int p_y, int *crsh_flag, int *crsh_x, int *crsh_y,int level);

void CreateMap(int **map);

void DisplayBorder(XImage *xim,unsigned long *lut,int pmx,int pmy,int color);

void CreateColor(Display *display,Colormap colmap,unsigned long *lut,int i,int r,int g,int b);

void CreateSprite(char **spr_pix,int sprite_num,unsigned char *sprite_mem); 

void PutSprite(XImage *xim,unsigned long *lut,unsigned char *sprite_mem, int sprite_num, int x, int y);

void EraseSprite(XImage *xim,unsigned long *lut,int x,int y);

void Dot(XImage *xim,unsigned long *lut,int x,int y,int color);

long GetTheKeyNoBlock(Display *display);

void WaitKeyReleased(Display *display);

void CreateCharset(unsigned char *charset,char *charsethexa);

void PrintChar(XImage *xim, unsigned long *lut,unsigned char *charset,int c,int x,int ink, int bgd);

void PrintScore(XImage *xim, unsigned long *lut,unsigned char *charset,int s,int x,int ink, int bgd);


int main(int argc, char **argv)
{
  // X11-specific variables

  Display *display;
  Visual  *visu;
  XEvent report;
  int screen;
  unsigned int depth;
  GC gc;
  Window win;
  XSizeHints sizeHints;
  char *im;
  XImage *xim;       // Pixmap displayed in the window
  Colormap colmap;
  unsigned long lut[16];  // Look-up Table of colors

  int bgd_r[3];  // Background color components (depending on level) 
  int bgd_g[3];
  int bgd_b[3];
  int fgd_r[3];  // Foreground color (to display score) components
  int fgd_g[3];
  int fgd_b[3];

  unsigned char *sprite_mem;     // Memory containing sprite shapes
  char **spr_pix;                // Array for sprite shape definition
  char charsethexa[256];         // character set bitmap in hexadecimal
  unsigned char charset[128];    // Memory containing character set bitmap

  // "Virtual" pixmap size (actual pixmap is displayed with a x3 zoom factor) 
  int pmx = (X_MAP-2)*16 + 2*W_BRDR;
  int pmy = (Y_MAP-2)*16 + 2*W_BRDR + W_HDR;
  // Actual pixmap size
  int xw = 3*pmx;
  int yw = 3*pmy;
 
  int **map;  // Map of the game (each cell= cube, border element or nothing)

  int png_x,png_y;    // Coordinates of Pengo
  int png_dx,png_dy;  // Motion directions of Pengo
  int png_state;      // Pengo state (= action done by Pengo)
  int png_spr;        // Index of sprite representing Pengo

  int psh_flag[MAX_PUSH];  // Pushed-cube validity flags 
  int psh_x[MAX_PUSH];     // Coordinates of pushed cubes
  int psh_y[MAX_PUSH];
  int psh_dx[MAX_PUSH];    // Motion directions of pushed cubes
  int psh_dy[MAX_PUSH];
  int psh_typ[MAX_PUSH];   // Type of pushed cube (ice or diamond)

  int crsh_flag[MAX_CRASH];  // Crashed-cube validity flags
  int crsh_x[MAX_CRASH];     // Coordinates of crashed cubes
  int crsh_y[MAX_CRASH];

  int snb_state[NB_SNOBEE];   // Snobee states
  int snb_x[NB_SNOBEE];       // Coordinates of Snobees
  int snb_y[NB_SNOBEE];
  int snb_dx[NB_SNOBEE];      // Motion directions of Snobees
  int snb_dy[NB_SNOBEE];
  int snb_dm[NB_SNOBEE];      // Previous motion direction of Snobees 
  int snb_ax[NB_SNOBEE];      // Motion axis (H or V) of Snobees
  int snb_spr;                // Index of sprite representing a Snobee 

  int spiral_x[8];  // Spiral pattern used to find new Snobee location
  int spiral_y[8];  // (to avoid to have Snobee erasing a diamond)
  spiral_x[0] = 1; spiral_x[1] = 0; spiral_x[2] = -1; spiral_x[3] = -1;
  spiral_x[1] = 0; spiral_x[5] = 0; spiral_x[6] = 1 ; spiral_x[7] = 1;
  spiral_y[0] = 0; spiral_y[1] = 1; spiral_y[2] = 0; spiral_y[3] = 0;
  spiral_y[1] = -1; spiral_y[5] = -1; spiral_y[6] = 0 ; spiral_y[7] = 0;

  int i,j,k,l;
  int key;
  int running;   // If 1, program is running 
  int playing;   // If 1, game is running 
  int killed;    // If 1, Pengo has been killed by a Snobee
  int gameOver;  // If 1, all Pengo lives have been lost
  int completed; // If 1, level has been completed (3 diamonds aligned)
  int lives;     // Number of Pengo lives remaining
  int score;
  int level;     // Current game level
  int x,y;
  int xc,yc;    // Cube coordinates
  int dxc,dyc;  // Motion directions of cube
  int ct;       // Cube type
  int done;
  int quit;
  int elec_cnt; // Countdown before electric border can be activated again

 
   // Memory allocations
 
  spr_pix = (char **)malloc(16*sizeof(char *));
  if (spr_pix == NULL)
  {
    printf("Cannot allocate memory for array: spr_pix\n");
    exit(0);
  }
  for (i = 0 ; i < 16 ; i++)
  {
    spr_pix[i] = (char *)malloc(17*sizeof(char));
    if (spr_pix[i] == NULL)
    {
      printf("Cannot allocate memory for array: spr_pix[%d]\n",i);
      exit(0);
    }
  }   
  sprite_mem = (unsigned char *)malloc(256*NB_SPRITES*sizeof(unsigned char));
  if (sprite_mem == NULL)
  {
    printf("Cannot allocate memory for array: sprite_mem\n");
    exit(0);
  }
  map = (int **)malloc(X_MAP*sizeof(int *));
  if (map == NULL)
  {
    printf("Cannot allocate memory for array: map\n");
    exit(0);
  }
  for (i = 0 ; i < X_MAP ; i++)
  {
    map[i] = (int *)malloc(Y_MAP*sizeof(int));
    if (map[i] == NULL)
    {
      printf("Cannot allocate memory for array: map[%d]\n",i);
      exit(0);
    }
  }

  // Sprite shape definition
  // (each digit is the color index in the LUT)

  // Cubes

  strcpy(spr_pix[0], "0000000000000000");
  strcpy(spr_pix[1], "0011111111111100");
  strcpy(spr_pix[2], "0112211111122110");
  strcpy(spr_pix[3], "0121111111111210");
  strcpy(spr_pix[4], "0121111111111210");
  strcpy(spr_pix[5], "0111111111111110");
  strcpy(spr_pix[6], "0111111111111110");
  strcpy(spr_pix[7], "0111111111111110");
  strcpy(spr_pix[8], "0111111111111110");
  strcpy(spr_pix[9], "0111111111111110");
  strcpy(spr_pix[10],"0111111111111110");
  strcpy(spr_pix[11],"0121111111111210");
  strcpy(spr_pix[12],"0121111111111210");
  strcpy(spr_pix[13],"0112211111122110");
  strcpy(spr_pix[14],"0011111111111100");
  strcpy(spr_pix[15],"0000000000000000");
 
  CreateSprite(spr_pix,0,sprite_mem); 

  strcpy(spr_pix[1], "0111111111111110");
  strcpy(spr_pix[2], "0111111001111110");
  strcpy(spr_pix[3], "0111110000111110");
  strcpy(spr_pix[4], "0111100000011110");
  strcpy(spr_pix[5], "0111033333301110");
  strcpy(spr_pix[6], "0110033333300110");
  strcpy(spr_pix[7], "0100033333300010");
  strcpy(spr_pix[8], "0100033333300010");
  strcpy(spr_pix[9], "0110033333300110");
  strcpy(spr_pix[10],"0111033333301110");
  strcpy(spr_pix[11],"0111100000011110");
  strcpy(spr_pix[12],"0111110000111110");
  strcpy(spr_pix[13],"0111111001111110");
  strcpy(spr_pix[14],"0111111111111110");
 
  CreateSprite(spr_pix,1,sprite_mem);

  // Pengo
 
  strcpy(spr_pix[1], "0000004444000000");
  strcpy(spr_pix[2], "0000044444400000");
  strcpy(spr_pix[3], "0000448448440000");
  strcpy(spr_pix[4], "0000449449440000");
  strcpy(spr_pix[5], "0004444444444000");
  strcpy(spr_pix[6], "0044445555444400");
  strcpy(spr_pix[7], "0040444554440400");
  strcpy(spr_pix[8], "0040444444440400");
  strcpy(spr_pix[9], "0040444444440400");
  strcpy(spr_pix[10],"0000444444440000");
  strcpy(spr_pix[11],"0000044444400000");
  strcpy(spr_pix[12],"0000005005000000");
  strcpy(spr_pix[13],"0000005005550000");
  strcpy(spr_pix[14],"0000555000000000");
 
  CreateSprite(spr_pix,2,sprite_mem);

  strcpy(spr_pix[12],"0000005005000000");
  strcpy(spr_pix[13],"0000555005000000");
  strcpy(spr_pix[14],"0000000005550000");

  CreateSprite(spr_pix,3,sprite_mem);

  strcpy(spr_pix[10],"0040444444440400");
  strcpy(spr_pix[11],"0040044444400400");
  strcpy(spr_pix[12],"0000005005000000");
  strcpy(spr_pix[13],"0000005005000000");
  strcpy(spr_pix[14],"0000555005550000");

  CreateSprite(spr_pix,4,sprite_mem);

  strcpy(spr_pix[1], "0000004444000000");
  strcpy(spr_pix[2], "0000044444400000");
  strcpy(spr_pix[3], "0000444444440000");
  strcpy(spr_pix[4], "0000444444440000");
  strcpy(spr_pix[5], "0004444444444000");
  strcpy(spr_pix[6], "0044444444444400");
  strcpy(spr_pix[7], "0040444444440400");
  strcpy(spr_pix[8], "0040444444440400");
  strcpy(spr_pix[9], "0040444444440400");
  strcpy(spr_pix[10],"0000444444440000");
  strcpy(spr_pix[11],"0000044444400000");
  strcpy(spr_pix[12],"0000005005000000");
  strcpy(spr_pix[13],"0000005005550000");
  strcpy(spr_pix[14],"0000555000000000");

  CreateSprite(spr_pix,5,sprite_mem);

  strcpy(spr_pix[12],"0000005005000000");
  strcpy(spr_pix[13],"0000555005000000");
  strcpy(spr_pix[14],"0000000005550000");

  CreateSprite(spr_pix,6,sprite_mem);

  strcpy(spr_pix[1], "0040004444000400");
  strcpy(spr_pix[2], "0040044444400400");
  strcpy(spr_pix[3], "0040444444440400");
  strcpy(spr_pix[4], "0040444444440400");
  strcpy(spr_pix[5], "0044444444444400");
  strcpy(spr_pix[6], "0004444444444000");
  strcpy(spr_pix[7], "0000444444440000");
  strcpy(spr_pix[8], "0000444444440000");
  strcpy(spr_pix[9], "0000444444440000");

  CreateSprite(spr_pix,7,sprite_mem);

  strcpy(spr_pix[1], "0000004444000000");
  strcpy(spr_pix[2], "0000044444400000");
  strcpy(spr_pix[3], "0000084444440000");
  strcpy(spr_pix[4], "0000094444440000");
  strcpy(spr_pix[5], "0000044444444000");
  strcpy(spr_pix[6], "0000554744474000");
  strcpy(spr_pix[7], "0005554744474000");
  strcpy(spr_pix[8], "0000044744744000");
  strcpy(spr_pix[9], "0000044744744000");
  strcpy(spr_pix[10],"0000044477444000");
  strcpy(spr_pix[11],"0000004444440000");
  strcpy(spr_pix[12],"0000000505000000");
  strcpy(spr_pix[13],"0000000500500000");
  strcpy(spr_pix[14],"0000055505500000");
  
  CreateSprite(spr_pix,8,sprite_mem);

  strcpy(spr_pix[12],"0000000505000000");
  strcpy(spr_pix[13],"0000505005000000");
  strcpy(spr_pix[14],"0000050555000000");

  CreateSprite(spr_pix,9,sprite_mem);

  strcpy(spr_pix[7], "0444444444474000");
  strcpy(spr_pix[8], "0044444444744000");
  strcpy(spr_pix[9], "0000047777444000");
  strcpy(spr_pix[10],"0000044444444000");
  strcpy(spr_pix[11],"0000004444440000");
  strcpy(spr_pix[12],"0000000055000000");
  strcpy(spr_pix[13],"0000000055000000");
  strcpy(spr_pix[14],"0000005555000000");

  CreateSprite(spr_pix,10,sprite_mem);

  strcpy(spr_pix[1], "0000004444000000");
  strcpy(spr_pix[2], "0000044444400000");
  strcpy(spr_pix[3], "0000444444800000");
  strcpy(spr_pix[4], "0000444444900000");
  strcpy(spr_pix[5], "0004444444400000");
  strcpy(spr_pix[6], "0004744474550000");
  strcpy(spr_pix[7], "0004744474555000");
  strcpy(spr_pix[8], "0004474474400000");
  strcpy(spr_pix[9], "0004474474400000");
  strcpy(spr_pix[10],"0004447744400000");
  strcpy(spr_pix[11],"0000444444000000");
  strcpy(spr_pix[12],"0000050500000000");
  strcpy(spr_pix[13],"0000500500000000");
  strcpy(spr_pix[14],"0000050555000000");
  
  CreateSprite(spr_pix,11,sprite_mem);

  strcpy(spr_pix[12],"0000050500000000");
  strcpy(spr_pix[13],"0000050050500000");
  strcpy(spr_pix[14],"0000055505000000");

  CreateSprite(spr_pix,12,sprite_mem);

  strcpy(spr_pix[7], "0004744444444440");
  strcpy(spr_pix[8], "0004474444444400");
  strcpy(spr_pix[9], "0004447777400000");
  strcpy(spr_pix[10],"0004444444400000");
  strcpy(spr_pix[11],"0000444444000000");
  strcpy(spr_pix[12],"0000005500000000");
  strcpy(spr_pix[13],"0000005500000000");
  strcpy(spr_pix[14],"0000005555000000");

  CreateSprite(spr_pix,13,sprite_mem);

  // Crashing ice cube

  strcpy(spr_pix[0], "0000000000000000");
  strcpy(spr_pix[1], "0000000000000000");
  strcpy(spr_pix[2], "0000000000000000");
  strcpy(spr_pix[3], "0000000000000000");
  strcpy(spr_pix[4], "0100110101101210");
  strcpy(spr_pix[5], "0101111111110110");
  strcpy(spr_pix[6], "0110011111101110");
  strcpy(spr_pix[7], "0111101111101110");
  strcpy(spr_pix[8], "0111010111010010");
  strcpy(spr_pix[9], "0110111110011010");
  strcpy(spr_pix[10],"0110111101111110");
  strcpy(spr_pix[11],"0101011011011210");
  strcpy(spr_pix[12],"0121101011101210");
  strcpy(spr_pix[13],"0112011011020110");
  strcpy(spr_pix[14],"0011111111111100");
  strcpy(spr_pix[15],"0000000000000000");

  CreateSprite(spr_pix,14,sprite_mem);

  strcpy(spr_pix[4], "0000000000000000");
  strcpy(spr_pix[5], "0000000000000000");
  strcpy(spr_pix[6], "0000000000000000");
  strcpy(spr_pix[7], "0001001010001100");

  CreateSprite(spr_pix,15,sprite_mem);

  strcpy(spr_pix[7], "0000000000000000");
  strcpy(spr_pix[8], "0000000000000000");
  strcpy(spr_pix[9], "0000000000000000");
  strcpy(spr_pix[10],"0001001011001010");

  CreateSprite(spr_pix,16,sprite_mem);
  
  strcpy(spr_pix[10],"0000000000000000");
  strcpy(spr_pix[11],"0000000000000000");
  strcpy(spr_pix[12],"0000000000000000");
  strcpy(spr_pix[13],"0002001010000100");

  CreateSprite(spr_pix,17,sprite_mem);

  // Snobee

  strcpy(spr_pix[0], "0000000000000000");
  strcpy(spr_pix[1], "0000000000000000");
  strcpy(spr_pix[2], "0000006666000000");
  strcpy(spr_pix[3], "0000066666600000");
  strcpy(spr_pix[4], "0000666666660000");
  strcpy(spr_pix[5], "0006668668666000");
  strcpy(spr_pix[6], "0006669669666000");
  strcpy(spr_pix[7], "0066666666666600");
  strcpy(spr_pix[8], "0066666556666600");
  strcpy(spr_pix[9], "0066666666666600");
  strcpy(spr_pix[10],"0066666666666600");
  strcpy(spr_pix[11],"0066666666666600");
  strcpy(spr_pix[12],"0006666666666000");
  strcpy(spr_pix[13],"0000666666660000");
  strcpy(spr_pix[14],"0000000000000000");
  strcpy(spr_pix[15],"0000000000000000");

  CreateSprite(spr_pix,18,sprite_mem);

  strcpy(spr_pix[5], "0006666666666000");
  strcpy(spr_pix[6], "0006666666666000");
  strcpy(spr_pix[8], "0066666666666600");
  
  CreateSprite(spr_pix,19,sprite_mem);

  strcpy(spr_pix[4], "0000666666600000");
  strcpy(spr_pix[5], "0006666666680000");
  strcpy(spr_pix[6], "0006666666690000");
  strcpy(spr_pix[7], "0066666666660000");
  strcpy(spr_pix[8], "0066666666655500");
  strcpy(spr_pix[9], "0066666666660000");
  strcpy(spr_pix[10],"0066666666660000");
  strcpy(spr_pix[11],"0066666666600000");
  strcpy(spr_pix[12],"0066666666600000");
  strcpy(spr_pix[13],"0006666666000000");

  CreateSprite(spr_pix,20,sprite_mem);

  strcpy(spr_pix[5], "0000866666666000");
  strcpy(spr_pix[6], "0000966666666000");
  strcpy(spr_pix[7], "0000666666666600");
  strcpy(spr_pix[8], "0055566666666600");
  strcpy(spr_pix[9], "0000666666666600");
  strcpy(spr_pix[10],"0000666666666600");
  strcpy(spr_pix[11],"0000066666666600");
  strcpy(spr_pix[12],"0000066666666600");
  strcpy(spr_pix[13],"0000006666666000");

  CreateSprite(spr_pix,21,sprite_mem);

  // Crushed snobee (exploding)

  strcpy(spr_pix[0], "0000000000000000");
  strcpy(spr_pix[1], "0000000000000000");
  strcpy(spr_pix[2], "0000000000000000");
  strcpy(spr_pix[3], "0000000000000000");
  strcpy(spr_pix[4], "0000000400000000");
  strcpy(spr_pix[5], "0000400400400000");
  strcpy(spr_pix[6], "0000040404000000");
  strcpy(spr_pix[7], "0000000000000000");
  strcpy(spr_pix[8], "0004440000444000");
  strcpy(spr_pix[9], "0000000000000000");
  strcpy(spr_pix[10],"0000040404000000");
  strcpy(spr_pix[11],"0000400400400000");
  strcpy(spr_pix[12],"0000000400000000");
  strcpy(spr_pix[13],"0000000000000000");
  strcpy(spr_pix[14],"0000000000000000");
  strcpy(spr_pix[15],"0000000000000000");

  CreateSprite(spr_pix,22,sprite_mem);

  // Shocked Snobee

  strcpy(spr_pix[4], "0000000000000000");
  strcpy(spr_pix[5], "0000066666600000");
  strcpy(spr_pix[6], "0006666666666000");
  strcpy(spr_pix[7], "0066668668666600");
  strcpy(spr_pix[8], "0666669669666660");
  strcpy(spr_pix[9], "6666666666666666");
  strcpy(spr_pix[10],"6666666666666666");
  strcpy(spr_pix[11],"0666666666666660");
  strcpy(spr_pix[12],"0000000000000000");
  strcpy(spr_pix[13],"0000000000000000");
  strcpy(spr_pix[14],"0000000000000000");
  strcpy(spr_pix[15],"0000000000000000");

  CreateSprite(spr_pix,23,sprite_mem);

  // Snobee egg ("rebirth" state)

  strcpy(spr_pix[4], "0000000660000000");
  strcpy(spr_pix[5], "0000006666000000");
  strcpy(spr_pix[6], "0000066666600000");
  strcpy(spr_pix[7], "0000066666600000");
  strcpy(spr_pix[8], "0000066666600000");
  strcpy(spr_pix[9], "0000066666600000");
  strcpy(spr_pix[10],"0000006666000000");
  strcpy(spr_pix[11],"0000000660000000");

  CreateSprite(spr_pix,24,sprite_mem);

  // Snobee killing Pengo

  strcpy(spr_pix[0], "0000000000000000");
  strcpy(spr_pix[1], "0000000000000000");
  strcpy(spr_pix[2], "0000006666000000");
  strcpy(spr_pix[3], "0000066666600000");
  strcpy(spr_pix[4], "0000666666660000");
  strcpy(spr_pix[5], "0006668668666000");
  strcpy(spr_pix[6], "0006669669666000");
  strcpy(spr_pix[7], "0066666566666600");
  strcpy(spr_pix[8], "0066665566666600");
  strcpy(spr_pix[9], "0064745544446600");
  strcpy(spr_pix[10],"0044744444444000");
  strcpy(spr_pix[11],"0444444444444000");
  strcpy(spr_pix[12],"0444444444444005");
  strcpy(spr_pix[13],"0444444444444005");
  strcpy(spr_pix[14],"0444444444444555");
  strcpy(spr_pix[15],"0044444444444000");

  CreateSprite(spr_pix,25,sprite_mem);

  // Character bitmaps

  strcpy(charsethexa,"3C42425A42423C"); // 0 
  strcat(charsethexa,"0818280808083E"); // 1
  strcat(charsethexa,"7C02023C40407E"); // 2
  strcat(charsethexa,"7C02023C02027C"); // 3
  strcat(charsethexa,"1828487E08083E"); // 4
  strcat(charsethexa,"7E40407C02027C"); // 5
  strcat(charsethexa,"3E40407C42423C"); // 6
  strcat(charsethexa,"7E020408102040"); // 7
  strcat(charsethexa,"3C42423C42423C"); // 8
  strcat(charsethexa,"3C42423E02027C"); // 9
  strcat(charsethexa,"08387838080808"); // Flag (level marker)
  strcat(charsethexa,"1C2A367F7F1436"); // Mini-Pengo (life marker)
  strcat(charsethexa,"00000000000000"); // Space character

  CreateCharset(charset,charsethexa);

  // Create the display 

  display = XOpenDisplay(NULL);
  if (display == NULL)
  {
    fprintf(stderr,"Cannot connect to X-server\n");
    exit(-1);
  }
  screen = DefaultScreen(display);
  visu = XDefaultVisual(display,screen);
  gc = XCreateGC(display,RootWindow(display,screen),0,NULL);
  depth = DisplayPlanes(display,screen);

  // Create the window 
     
  win = XCreateSimpleWindow(display,RootWindow(display,screen),100,100,xw,yw,4,BlackPixel(display,screen),WhitePixel(display,screen));
  sizeHints.flags = PPosition|PSize;
  sizeHints.x = 100;
  sizeHints.y = 100;
  sizeHints.width = xw;
  sizeHints.height= yw;
  XSetStandardProperties(display,win,"Pengo","Pengo",0,argv,argc,&sizeHints);
  XSelectInput(display,win,ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|StructureNotifyMask);

  // Create the color map

  colmap=XCreateColormap(display,win,visu,AllocNone);
  XSetWindowColormap(display,win,colmap);
  XInstallColormap(display,colmap);
  XMapWindow(display,win);

  CreateColor(display,colmap,lut,0,0,54016,61440);
  CreateColor(display,colmap,lut,1,0,0,61440);
  CreateColor(display,colmap,lut,2,0,53248,61440);
  CreateColor(display,colmap,lut,3,0,61400,3584);
  CreateColor(display,colmap,lut,4,59392,0,0);
  CreateColor(display,colmap,lut,5,61400,45056,0);
  CreateColor(display,colmap,lut,6,61400,34816,0);
  CreateColor(display,colmap,lut,7,34816,0,0);
  CreateColor(display,colmap,lut,8,65535,65535,65535);
  CreateColor(display,colmap,lut,9,0,0,0);
  CreateColor(display,colmap,lut,10,0,0,0);
 
  bgd_r[0] = 20000; bgd_g[0] = 54016; bgd_b[0] = 61440;
  bgd_r[1] = 0; bgd_g[1] = 0; bgd_b[1] = 0;
  bgd_r[2] = 61440; bgd_g[2] = 50000; bgd_b[2] = 40000;
  fgd_r[0] = 0; fgd_g[0] = 0; fgd_b[0] = 0;
  fgd_r[1] = 0; fgd_g[1] = 54816; fgd_b[1] = 61400;
  fgd_r[2] = 0; fgd_g[2] = 0; fgd_b[2] = 0;

  // Create the pixmap of the window 

  im=(char *)malloc(4*xw*yw*sizeof(char));
  if (im==NULL)
  {
    fprintf(stderr,"Cannot allocate memory for im (window pixmap)\n");
    exit(-1);
  }
  xim=XCreateImage(display,visu,depth,ZPixmap,0,im,xw,yw,32,0);

  running = 1;
  quit = 0;

  while (running)
  {  
    // Initialize game

    CreateMap(map);

    gameOver = 0;
    lives = 3;
    score = 0;
    completed = 0;
    level = 1;

    while (!gameOver)
    { 
      // Select background and score colors depending on level
 
      CreateColor(display,colmap,lut,0,bgd_r[(level-1)%3],bgd_g[(level-1)%3],bgd_b[(level-1)%3]);
      CreateColor(display,colmap,lut,10,fgd_r[(level-1)%3],fgd_g[(level-1)%3],fgd_b[(level-1)%3]);

      // Clear pixmap with background color, draw the map 

      for (i = 0 ; i < pmx ; i++)
      { 
        for (j = 0; j < pmy; j++) Dot(xim,lut,i,j,0); 
      }
      printf("OK 2\n");
      for (i = 1; i < (X_MAP-1) ; i++)
      {
        for (j = 1; j < (Y_MAP-1) ; j++)
        {
          if (map[i][j] == ICE) PutSprite(xim,lut,sprite_mem,0,i*16,j*16);
          if (map[i][j] == DIAMOND) PutSprite(xim,lut,sprite_mem,1,i*16,j*16);
        }
      }
      printf("OK 3\n");
      DisplayBorder(xim,lut,pmx,pmy,3);
      printf("OK 4\n");

      png_x = 6;
      png_y = 7;
      killed = 0;

      // Check Pengo location is not occupied by a diamond
      // If so, look around for a new location (an ice-cube can be sacrified) 

      done = 0;
      i = 0;
      while (!done)
      {
        if (map[png_x][png_y] != DIAMOND)
        {
           done = 1;
           if (map[png_x][png_y] == ICE) map[png_x][png_y] = 0;
        }
        else
        {
          png_x += spiral_x[i];
          png_y += spiral_y[i];
          i++;
        }
      }

      // Initialize Snobee variables

      snb_x[0] = 3;
      snb_y[0] = 3;
      snb_ax[0] = VERTICAL;
      snb_dx[0] = 0;
      snb_dy[0] = 1;
      snb_dm[0] = 0;
      snb_state[0] = REBIRTH;
      snb_x[1] = 11;
      snb_y[1] = 3;
      snb_ax[1] = HORIZONTAL;
      snb_dx[1] = 1;
      snb_dy[1] = 0;
      snb_dm[1] = 0;
      snb_state[1] = REBIRTH;
      snb_x[2] = 3;
      snb_y[2] = 13;
      snb_ax[2] = VERTICAL;
      snb_dx[2] = 0;
      snb_dy[2] = -1;
      snb_dm[2] = 0;
      snb_state[2] = REBIRTH;

      // Check Snobee initial location (same rule as for Pengo)

      for (i = 0 ; i < NB_SNOBEE ; i++)
      {
        done = 0;
        j = 0;
        while (!done)
        {
          if (map[snb_x[i]][snb_y[i]] != DIAMOND)
          {
             done = 1;
             if (map[snb_x[i]][snb_y[i]] == ICE) map[snb_x[i]][snb_y[i]] = 0;
          }
          else
          {
            snb_x[i] += spiral_x[j];
            snb_y[i] += spiral_y[j];
            j++;
          }
        }
      }

      // Initialize various game variables
 
      elec_cnt = 0;
      for (i = 0 ; i < MAX_PUSH ; i++) psh_flag[i] = 0;
      for (i = 0 ; i < MAX_CRASH ; i++) crsh_flag[i] = 0;

      // Display Pengo, score, number of lives, level flags

      PutSprite(xim,lut,sprite_mem,2,png_x*16,png_y*16);
      PrintScore(xim,lut,charset,score,2,10,0);
      for (i = 0 ; i < lives ; i++)
      {
        PrintChar(xim,lut,charset,11,10+i,4,0);
      }
      for (i = 0 ; i < level ; i++)
      {
        if (i < 10) PrintChar(xim,lut,charset,10,24-i,4,0);
      } 

      XPutImage(display,win,gc,xim,0,0,0,0,xw,yw);

      // Main loop of the game

      playing=1;
      while(playing)
      {
        png_dx = 0;
        png_dy = 0;
        key = GetTheKeyNoBlock(display); // Poll the keyboard
        if (key != -1)
        {
          switch(key)
          {
            case UP_ARROW_K:
              png_dx = 0;
              png_dy = -1;
              png_spr = 5;
              break;
            case DOWN_ARROW_K:
              png_dx = 0;
              png_dy = 1;
              png_spr = 2;
              break;
            case LEFT_ARROW_K:
              png_dx = -1;
              png_dy = 0;
              png_spr = 8;
              break;
            case RIGHT_ARROW_K:
              png_dx = 1;
              png_dy = 0;
              png_spr = 11;
              break;
            case QUIT_K:
            case ESC_K:
              playing = 0;
              quit = 1;
              break;
          }
        }
        MovePengo(map,png_x,png_y,png_dx,png_dy,&png_state,elec_cnt);

        // Actions depending on Pengo state

        if (png_state == PUSHING)
        {
          NewCubePushed(map,psh_flag,psh_x,psh_y,psh_dx,psh_dy,psh_typ,png_x,png_y,png_dx,png_dy);
        }
        if (png_state == CRASHING)
        {
          NewCubeCrashed(crsh_flag,crsh_x,crsh_y,png_x+png_dx,png_y+png_dy);
        }
        if (png_state == IDLE)
        {
          png_dx = 0;
          png_dy = 0;
        }
        if ((png_state == PUSHING) || (png_state == ELEC) || (png_state == CRASHING))
        {
          PutSprite(xim,lut,sprite_mem,png_spr+2,png_x*16,png_y*16);
          png_dx = 0;
          png_dy = 0;
        }
        if (png_state == ELEC)
        {
          // Activate electric border and check if any Snobee has been shocked

          elec_cnt = 15;
          for (i = 0 ; i < NB_SNOBEE ; i++)
          {
            if ((snb_state[i] == ACTIVE) && ((snb_x[i] == 1)
                || (snb_x[i] == (X_MAP-2)) || (snb_y[i] == 1) ||
                   (snb_y[i] == (Y_MAP -2))))
              {
                snb_state[i] = SHOCKED;
              }
          }
        }
        MoveSnobees(map,snb_state,snb_x,snb_y,snb_dx,snb_dy,snb_dm,snb_ax,png_x,png_y,crsh_flag,crsh_x,crsh_y,level);
        CheckSnobeePushed(xim,lut,map,snb_x,snb_y,snb_dx,snb_dy,snb_state,psh_flag,psh_x,psh_y,psh_dx,psh_dy,0);

        // Animations

        for (i = 1 ; i <= 16 ; i++)
        {
           // 1) Pengo

          if (png_state == MOVING)
          {
            PutSprite(xim,lut,sprite_mem,png_spr+((i/2)%2),png_x*16+png_dx*i,png_y*16+png_dy*i);
          }

          // 2) Pushed cubes and snobees (moving twice faster than other
          //    sprites), if any

          for (j = 0 ; j < MAX_PUSH ; j++)
          {
            if (psh_flag[j] == 1)
            {
              xc = psh_x[j];
              yc = psh_y[j];
              dxc = psh_dx[j];
              dyc = psh_dy[j];
              ct = psh_typ[j];
              k = i%8;
              if ((i == 8) || (i == 16))
              {
                // Cube has moved from cell on the map

                map[xc][yc] = 0;
                map[xc+dxc][yc+dyc] = ct;

                // Check if cube can still move

                if (map[xc+2*dxc][yc+2*dyc] != 0)
                {
                  psh_flag[j] = 0;
                  score += 10;
                  PrintScore(xim,lut,charset,score,2,10,0);

                  // Check if snobee was crushed by cube

                  for (l = 0 ; l < NB_SNOBEE ; l++)
                  {
                    if ((snb_state[l] == CRUSHED) && (snb_x[l] == xc+dxc) && (snb_y[l] == yc+dyc))
                    {
                      snb_state[l] = DEAD;
                      score +=100;
                      PrintScore(xim,lut,charset,score,2,10,0);
                    }
                  }
                }
                psh_x[j] += dxc;
                psh_y[j] += dyc;
                xc = psh_x[j];
                yc = psh_y[j];

                // Update coordinates of pushed snobees

                for (l = 0 ; l < NB_SNOBEE ; l++)
                {
                  if ((snb_state[l] == PUSHED) && (snb_x[l] == xc) && (snb_y[l] == yc))
                  {
                    snb_x[l] += dxc;
                    snb_y[l] += dyc;
                  }
                } 
                // Check if Snobee half-way in front of cube => pushed or crushed
            
                if (i == 8)
                  CheckSnobeePushed(xim,lut,map,snb_x,snb_y,snb_dx,snb_dy,snb_state,psh_flag,psh_x,psh_y,psh_dx,psh_dy,1);

                // Check if diamonds are grouped and aligned

                if ((psh_flag[j] == 0) && (ct == DIAMOND))
                {
                  if (xc < (X_MAP - 2))
                  {
                    if ((map[xc+1][yc] == DIAMOND) && (map[xc+2][yc] == DIAMOND))
                      completed = 1;
                  }
                  if ((map[xc+1][yc] == DIAMOND) && (map[xc-1][yc] == DIAMOND))
                    completed = 1;
                  if (xc > 1)
                  {
                    if  ((map[xc-2][yc] == DIAMOND) && (map[xc-1][yc] == DIAMOND))
                      completed = 1;
                  }
                  if (yc < (Y_MAP - 2))
                  {
                    if ((map[xc][yc+1] == DIAMOND) && (map[xc][yc+2] == DIAMOND))
                    completed = 1;
                  }
                  if ((map[xc][yc+1] == DIAMOND) && (map[xc][yc-1] == DIAMOND))
                    completed = 1;
                  if (yc > 1)
                  {
                    if  ((map[xc][yc-2] == DIAMOND) && (map[xc][yc-1] == DIAMOND))
                    completed = 1;
                  }
                  if (completed == 1) playing = 0;
                }
                printf("Done\n");  
              }
              printf("Moving pushed cube sprite\n");
              EraseSprite(xim,lut,xc*16+2*dxc*(k-1),yc*16+2*dyc*(k-1));
              PutSprite(xim,lut,sprite_mem,ct-1,xc*16+2*dxc*k,yc*16+2*dyc*k);
              printf("Done\n");  
            }
          }

          // 3) Crashed cubes
      
          for (j = 0 ; j < MAX_CRASH ; j++)
          {
            if (crsh_flag[j] == 1)
            {
              xc = crsh_x[j];
              yc = crsh_y[j];
              k= i/4;
              if (k != 4) PutSprite(xim,lut,sprite_mem,14+k,xc*16,yc*16);
              else // If i == 16 (= end of animation loop)
              {
                EraseSprite(xim,lut,xc*16,yc*16);
                map[xc][yc] = 0;
                crsh_flag[j] = 0;
                score += 5;
                PrintScore(xim,lut,charset,score,2,10,0);
              }
            }
          }

          // 4) Snobees

          for (j = 0 ; j < NB_SNOBEE ; j++)
          {
            switch(snb_state[j])
            {
              case ACTIVE:
                k = (i/4)%2;
                snb_spr = 18; 
                if ((snb_dx[j] == 0) && (snb_dy[j] == 1)) snb_spr = 18; 
                if ((snb_dx[j] == 0) && (snb_dy[j] == -1)) snb_spr = 19; 
                if ((snb_dx[j] == 1) && (snb_dy[j] == 0)) snb_spr = 20; 
                if ((snb_dx[j] == -1) && (snb_dy[j] == 0)) snb_spr = 21;
                PutSprite(xim,lut,sprite_mem,snb_spr,snb_x[j]*16+snb_dx[j]*i+k,snb_y[j]*16+snb_dy[j]*i+k);
                break;

              case PUSHED:
                k = i%8; 
                PutSprite(xim,lut,sprite_mem,18,snb_x[j]*16+2*snb_dx[j]*k,snb_y[j]*16+2*snb_dy[j]*k);
                break;

              case CRUSHED:
                if ((i == 1) || (i == 8)) PutSprite(xim,lut,sprite_mem,22,snb_x[j]*16,snb_y[j]*16);
                if ((i == 2) || (i == 9)) EraseSprite(xim,lut,snb_x[j]*16,snb_y[j]*16);
                break;
            
              case SHOCKED:
                PutSprite(xim,lut,sprite_mem,23,snb_x[j]*16,snb_y[j]*16);
                break;
            }
            if ((snb_state[j] > ACTIVE) && (snb_state[j] <= REBIRTH))
            {
              k = (i/4)%2;
              PutSprite(xim,lut,sprite_mem,24,snb_x[j]*16+k,snb_y[j]*16+k);
            }
          }
      
          // 5) Electrified border triggered 

          if (png_state == ELEC)
          {
            if ((i%2) == 0)  DisplayBorder(xim,lut,pmx,pmy,3); 
            else DisplayBorder(xim,lut,pmx,pmy,4);
          }

          // Check if pengo and snobee are in contact when at half-way

          if (i == 8)
          {
            for (j = 0 ; j < NB_SNOBEE ; j++)
            {
              if (snb_state[j] == ACTIVE)
              {
                if ((((png_x + png_dx) == snb_x[j]) && ((png_y + png_dy) == snb_y[j])) || ((png_x == (snb_x[j] + snb_dx[j])) && (png_y == (snb_y[j] + snb_dy[j]))) || (((png_x + png_dx) == (snb_x[j] + snb_dx[j])) && ((png_y + png_dy) == (snb_y[j] + snb_dy[j]))))
                {
                  printf("png x = %d ; png dx = %d ; png y = %d ; png dy =%d ; snb x = %d ; snb dx = %d ; snb y = %d ; snb dy = %d\n",png_x,png_dx,png_y,png_dy,snb_x[j],snb_dx[j],snb_y[j],snb_dy[j]);
                  EraseSprite(xim,lut,16*snb_x[j]+8*snb_dx[j],16*snb_y[j]+8*snb_dy[j]);
                  EraseSprite(xim,lut,16*png_x+8*png_dx,16*png_y+8*png_dy);
                  killed = 1;
                  printf("Killed at half-way\n");
                  playing = 0;
                  i = 17;     // => Stop animation immediately
                }
              }
            }
          }
          XPutImage(display,win,gc,xim,0,0,0,0,xw,yw);
 
          usleep(25000);
        }
        // End of animation loop
     
        // Update Pengo and Snobee coordinates on map 

        png_x+=png_dx;
        png_y+=png_dy;
        for (j = 0 ; j < NB_SNOBEE ; j++)
        {
          if (snb_state[j] == ACTIVE)
          {
            snb_x[j] += snb_dx[j];
            snb_y[j] += snb_dy[j];
          }
        }
        printf("End update coords\n");

        // Check if Pengo and a Snobee touch each other

        for (i = 0 ; i < NB_SNOBEE ; i++)
        {
          if ((png_x == snb_x[i]) && (png_y == snb_y[i]))
          {
            if (snb_state[i] == ACTIVE)
            {
              killed = 1;
              playing = 0;
            }
            if ((snb_state[i] >= SHOCKED) && (snb_state[i] < ACTIVE))
            {
              printf("Killed shocked snobee !\n");
              snb_state[i] = DEAD;
              score += 100;
              PrintScore(xim,lut,charset,score,2,10,0);
              printf("Score: %d\n", score);
            }
          }
        }

        // Generate new coordinates for rebirth of dead snobees

        for (i = 0 ; i < NB_SNOBEE ; i++)
        {
          if (snb_state[i] == DEAD)
          { 
            done = 0;
            while (!done)
            {
              x = rand()%(X_MAP-2)+1;
              y = rand()%(Y_MAP-2)+1;
              if (map[x][y] == 0)     // Check the cell is empty
              {
                snb_x[i] = x;
                snb_y[i] = y;
                done = 1;
              }
            }
          }
        }

        // Update non-active Snobee states

        for (i = 0 ; i < NB_SNOBEE ; i++)
        {
          // shocked
          if (snb_state[i] < ACTIVE) snb_state[i]++;
          // dead
          if ((snb_state[i] <= DEAD) && (snb_state[i] > ACTIVE)) snb_state[i]--;
        }

        // "Cock" the electric border if it has been triggered

        if (elec_cnt > 0) elec_cnt--; 
      }
      // End of main loop of game (while playing)
  
      if (killed)
      {
        PutSprite(xim,lut,sprite_mem,25,png_x*16,png_y*16);
        XPutImage(display,win,gc,xim,0,0,0,0,xw,yw);
        killed = 0;
        lives--;
        if (lives == 0) gameOver = 1;
        usleep(1000000);
      }
      if (completed)
      {
        score+=1000;

        // Grant a bonus based on the number of remaining ice cubes

        for (j = 1 ; j < (Y_MAP - 1) ; j++)
        {
          for (i = 1 ; i < (X_MAP -1) ; i++)
          {
            if (map[i][j] == ICE) score+=50;
          }
          PrintScore(xim,lut,charset,score,2,10,0);
          XPutImage(display,win,gc,xim,0,0,0,0,xw,yw);
          if ((j%2) == 0)  DisplayBorder(xim,lut,pmx,pmy,3); 
          else DisplayBorder(xim,lut,pmx,pmy,4);
          usleep(250000);
        }
        CreateMap(map);
        completed = 0;
        level++;
      }
      if (quit) 
      {
        gameOver = 1;
        running = 0;
      }  
    }
    if (running)
    {
      PrintChar(xim,lut,charset,12,10,10,0);
      // interleaving (screen dimming effect)  
      for (j = 3*W_HDR ; j < yw ; j+=2)
      {
        for (i = 0 ; i < xw ; i++) XPutPixel(xim,i,j,lut[9]);
      } 
      XPutImage(display,win,gc,xim,0,0,0,0,xw,yw);
      usleep(4000000);
      WaitKeyReleased(display);
      done = 0;
      while (!done)
      {
        key = GetTheKeyNoBlock(display);
        if (key != -1)
        {
          done = 1;
          if ((key == QUIT_K) || (key == ESC_K)) running = 0;
        }
        usleep(20000);
      }
    }
  }
  // End of "while running" loop
  
  exit(0);
 
}

// Function to find out the new Pengo State
 
void MovePengo(int **map,int png_x, int png_y, int png_dx, int png_dy, int *png_state, int elec_cnt)
{
  int p_state;

  p_state = IDLE;

  if ((png_dx != 0) || (png_dy != 0))
  {
    p_state= MOVING;
    // look of ice-cube or diamond in the way of Pengo
    if ((map[png_x+png_dx][png_y+png_dy] == ICE) ||
        (map[png_x+png_dx][png_y+png_dy] == DIAMOND))
    {
      // look if cube or diamond can be pushed
      if (map[png_x+2*png_dx][png_y+2*png_dy] == 0)
      {
        p_state = PUSHING;
      }
      else
      {
        // look if ice-cube will be crashed
        switch (map[png_x+png_dx][png_y+png_dy])
        {
          case ICE:
            p_state = CRASHING;
            break;
          case DIAMOND: 
            p_state = IDLE;
            break;
        }
      }
    }
    // Look if Pengo is touching the border

    if (map[png_x+png_dx][png_y+png_dy] == BORDER)
    {
      // Look if electric border can be activated 
      if (elec_cnt == 0) p_state = ELEC; else p_state = IDLE;
    }
  }
  *png_state = p_state;
}


// Function to create a new pushed cube

void NewCubePushed(int **map,int *psh_flag,int *psh_x,int *psh_y, int *psh_dx,int *psh_dy,int *psh_typ,int p_x,int p_y,int dp_x,int dp_y)
{ 
  int i;
 
  // Allocate new entry in table (1st available)

  for (i = 0 ; i < MAX_PUSH; i++)
  {
    if (psh_flag[i] == 0)
    {
      psh_flag[i] = 1;
      psh_x[i] = p_x+dp_x;
      psh_y[i] = p_y+dp_y;
      psh_dx[i] = dp_x;
      psh_dy[i] = dp_y;
      psh_typ[i] = map[p_x+dp_x][p_y+dp_y];
      break;
    }
  }
}


// Function to create a new crashed cube

void NewCubeCrashed(int *crsh_flag,int *crsh_x,int *crsh_y,int x,int y)
{
  int i;
 
  // Allocate new entry in table (1st available)

  for (i = 0 ; i < MAX_CRASH; i++)
  {
    if (crsh_flag[i] == 0)
    {
      crsh_flag[i] = 1;
      crsh_x[i] = x;
      crsh_y[i] = y;
      break;
    }
  }
}
 
// Function to check if a Snobee is pushed or crushed by a pushed cube
 
void CheckSnobeePushed(XImage *xim, unsigned long *lut,int **map,int *snb_x,int *snb_y,int *snb_dx,int *snb_dy,int *snb_state, int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy,int halfway)
{
  int i,j;

  for (i = 0 ; i < MAX_PUSH; i++)
  {
    if (psh_flag[i] == 1)
    {
      // Look if pushed cube is pushing a snobee

      for (j = 0; j < NB_SNOBEE ; j++)
      {
        if ((snb_state[j] == ACTIVE) || (snb_state[j] == PUSHED))
        {
          if (((psh_x[i]+psh_dx[i]) == snb_x[j]) &&
              ((psh_y[i]+psh_dy[i]) == snb_y[j]))

          {
            // Snobee can be half-way exiting the cube path. Better to erase it

            if (snb_state[j] == ACTIVE) EraseSprite(xim,lut,16*snb_x[j]+8*halfway*snb_dx[j],16*snb_y[j]+8*halfway*snb_dy[j]);
            if (map[psh_x[i]+2*psh_dx[i]][psh_y[i]+2*psh_dy[i]] != 0)
            {
              snb_state[j] = CRUSHED;
              snb_dx[j] = 0;
              snb_dy[j] = 0;
            }
            else
            {
              snb_state[j] = PUSHED;
              snb_dx[j] = psh_dx[i];
              snb_dy[j] = psh_dy[i];
            }
          }
          else
          {
            // Look if snobee is about to cross a pushed cube
            // (can also be half-way).  If so, consider it as doomed 

            if (((psh_x[i]+psh_dx[i]) == (snb_x[j]+snb_dx[j])) &&
                ((psh_y[i]+psh_dy[i]) == (snb_y[j]+snb_dy[j])) &&
                (snb_state[j] == ACTIVE))
            {
              // "Teleport" snobee to be in front of pushed cube

              EraseSprite(xim,lut,16*snb_x[j]+8*halfway*snb_dx[j],16*snb_y[j]+8*halfway*snb_dy[j]);
              snb_x[j] += snb_dx[j];
              snb_y[j] += snb_dy[j];

              if (map[psh_x[i]+2*psh_dx[i]][psh_y[i]+2*psh_dy[i]] != 0)
              {
                snb_state[j] = CRUSHED;
                snb_dx[j] = 0;
                snb_dy[j] = 0;
              }
              else
              {
                snb_state[j] = PUSHED;
                snb_dx[j] = psh_dx[i];
                snb_dy[j] = psh_dy[i];
              }
            }
          } 
        }
      }
    }
  }
}

// Function to determine the Snobee motions
// The algorithm is random but with a probability (depending on the level)
// that the Snobee chases Pengo. If a Snobee meets a cube on its way, it 
// can make a U-turn, turn left or right or turn to track Pengo. The choice
// is random, but the probability to chase Pengo increases with the level.
// Also, if a Snobee has a free path on its side that allows to chase Pengo,
// it can take it (again, probability to do it increases with the level) 

void MoveSnobees(int **map,int *snb_state, int *snb_x, int *snb_y, int *snb_dx, int *snb_dy, int *snb_dm, int *snb_ax, int p_x, int p_y, int *crsh_flag, int *crsh_x, int *crsh_y, int level)
{
  int i,k,m,p,q;
  int findingNextMove;

  if (level > 2) p = 1; else p = 4-level;
  q = 4+level;

  for (i = 0 ; i < NB_SNOBEE ; i++)
  {
    if (snb_state[i] == ACTIVE)
    {
      findingNextMove = 1;
      while(findingNextMove)
      {
        k = rand()%4;
        if (snb_ax[i] == VERTICAL)
        {
          // Snobee was crashing cube, restore its motion direction
          if (snb_dy[i] == 0) snb_dy[i] = snb_dm[i];
          m = map[snb_x[i]+snb_dx[i]][snb_y[i]+snb_dy[i]];
          if (m != 0)
          {
            // Cube on the way of Snobee

            if ((m == ICE) && (k == 0))
            {
              // Snobee crashes the cube in its way

              NewCubeCrashed(crsh_flag,crsh_x,crsh_y,snb_x[i]+snb_dx[i],snb_y[i]+snb_dy[i]);
              snb_dm[i] = snb_dy[i]; // Save motion direction
              snb_dy[i] = 0;
              findingNextMove = 0;
            }
            else
            {
              k = rand()%q;
              switch(k)
              {
                case 0: snb_dy[i] = -snb_dy[i];  // U-turn
                        break;
                case 1: snb_ax[i] = HORIZONTAL;  // Right-angle turn 
                        snb_dy[i] = 0;
                        snb_dx[i] = -1;
                        break;
                case 2: snb_ax[i] = HORIZONTAL;  // Right-angle turn
                        snb_dy[i] = 0;           // (other direction)
                        snb_dx[i] = 1;
                        break;
                default: snb_ax[i] = HORIZONTAL; // Right-angle turn
                        snb_dy[i] = 0;           // to chase Pengo
                        if (snb_dx[i] < p_x) snb_dx[i] = 1;
                        else snb_dx[i] = -1;
                        break; 
              }
            }
          }
          else
          {
            // Right-angle turn to chase Pengo, if no cube on the way

            k = rand()%p;
            if ((k == 0) && (map[snb_x[i]+1][snb_y[i]] == 0) 
                && (snb_x[i] < p_x)) 
            {
              snb_ax[i] = HORIZONTAL;
              snb_dy[i] = 0;
              snb_dx[i] = 1;
            }
            if ((k == 0) && (map[snb_x[i]-1][snb_y[i]] == 0) 
                && (snb_x[i] > p_x)) 
            {
              snb_ax[i] = HORIZONTAL;
              snb_dy[i] = 0;
              snb_dx[i] = -1;
            }
            findingNextMove = 0;
          }
        }
        else
        {
          if (snb_dx[i] == 0) snb_dx[i] = snb_dm[i]; // snobee was crashing cube
          m = map[snb_x[i]+snb_dx[i]][snb_y[i]+snb_dy[i]];
          if (m != 0)
          {
            if ((m == ICE) && (k == 0))
            {
              NewCubeCrashed(crsh_flag,crsh_x,crsh_y,snb_x[i]+snb_dx[i],snb_y[i]+snb_dy[i]);
              snb_dm[i] = snb_dx[i]; // memorize direction
              snb_dx[i] = 0;
              findingNextMove = 0;
            }
            else
            {
              k = rand()%q;
              switch(k)
              {
                case 0: snb_dx[i] = -snb_dx[i];
                        break;
                case 1: snb_ax[i] = VERTICAL;
                        snb_dx[i] = 0;
                        snb_dy[i] = -1;
                        break;
                case 2: snb_ax[i] = VERTICAL;
                        snb_dx[i] = 0;
                        snb_dy[i] = 1;
                        break;
                default: snb_ax[i] = VERTICAL;
                        snb_dx[i] = 0;
                        if (snb_dy[i] < p_y) snb_dy[i] = 1;
                        else snb_dy[i] = -1;
                        break; 
              }
            }
          }
          else
          {
            k = rand()%p;
            if ((k == 0) && (map[snb_x[i]][snb_y[i]+1] == 0)
                && (snb_y[i] < p_y)) 
            {
              snb_ax[i] = VERTICAL;
              snb_dy[i] = 1;
              snb_dx[i] = 0;
            }
            if ((k == 0) && (map[snb_x[i]][snb_y[i]-1] == 0) 
                && (snb_x[i] > p_x)) 
            {
              snb_ax[i] = VERTICAL;
              snb_dy[i] = -1;
              snb_dx[i] = 0;
            }
            findingNextMove = 0;
          }
        }
      }
      // No-motion hack for debug
      //snb_dx[i] = snb_dy[i] = 0;
      //
    }
  }
} 


// Fuction to create the game map
  
void CreateMap(int **map)
{
  int i,j;
  int x,y;
  int d;

  // Variables for maze creation

  int **v;     // Array of visited cells
  int xc,yc;   // Number of cells along x and y
  int nc;      // Total number of cells
  int nv;      // Number of visited cells
  int nn;      // Number of possible next cells (unvisited yet)
  int cx[4];   // x of possible next cell
  int cy[4];   // y of possible next cell
  int sx[100]; // Stack of x visited cells
  int sy[100]; // Stack of y visited cells
  int sp;      // Stack pointer
  int k;       // Random number
  int t;       // Time counter

  // Create the borders

  for (i = 0 ; i < X_MAP ; i++)
  {
     map[i][0] = BORDER;
     map[i][Y_MAP-1] = BORDER;
  }
  for (i = 0 ; i < Y_MAP ; i++)
  {
     map[0][i] = BORDER;
     map[X_MAP-1][i] = BORDER;
  }

  // Create first an empty field within the borders

  for (i = 1 ; i < (X_MAP-1) ; i++)
  {
    for (j = 1 ; j < (Y_MAP-1) ; j++)
    {
      map[i][j] = 0;
    }
  }

  // Create a grid of ice cubes

  for (i = 2 ; i < (X_MAP-1) ; i+=2) 
  {
    for (j = 1 ; j < (Y_MAP-1) ; j++)
    {
      map[i][j] = ICE;
    }
  }
  for (j = 2 ; j < (Y_MAP-1) ; j+=2)
  {
    for (i = 1 ; i < (X_MAP-1) ; i++)
    {
      map[i][j] = ICE;
    }
  }

  // Create a "maze" = random paths between the cells of the grid

  // Number of cells along x and y

  xc = (X_MAP-1)/2;
  yc = (Y_MAP-1)/2;

  // Create array of visited cells (visited by the maze generator algorithm) 

  v = (int **)malloc(xc*sizeof(int *));
  if (v == NULL)
  {
    printf("Cannot allocate memory for array: v\n");
    exit(0);
  }
  for (i = 0 ; i < xc ; i++)
  {
    v[i] = (int *)malloc(yc*sizeof(int));
    if (v[i] == NULL)
    {
      printf("Cannot allocate memory for array: v\n");
      exit(0);
    }
    for (j = 0 ; j < yc ; j++) v[i][j] = 0;
  }

  // First visited cell is at the center

  nv = 1;
  x = xc/2 + 1;
  y = yc/2 + 1;
  v[x][y] = 1;

  nc = xc*yc;
  sp = 0;
  t = 0;

  // Visit all the cells of the grid (if it doesn´t take too long)
 
  while (nv < nc)
  {
    nn = 0;
    if ((y > 0) && (v[x][y-1] == 0))
    {
      cx[nn] = x;
      cy[nn] = y-1;
      nn++;
    }
    if ((y < (yc-1)) && (v[x][y+1] == 0))
    {
      cx[nn] = x;
      cy[nn] = y+1;
      nn++;
    }
    if ((x > 0) && (v[x-1][y] == 0))
    {
      cx[nn] = x-1;
      cy[nn] = y;
      nn++;
    }
    if ((x < (xc-1)) && (v[x+1][y] == 0))
    {
      cx[nn] = x+1;
      cy[nn] = y;
      nn++;
    }
    // If all neighboring cells already visited => step-back
    if (nn == 0)
    {
      if (sp > 0)
      {
        sp--;
        x = sx[sp];
        y = sy[sp];
      }
      else break;
    }
    else
    {
      k = (int)rand()%nn;
      sp++;
      sx[sp] = x;
      sy[sp] = y;
      map[x+cx[k]+1][y+cy[k]+1] = 0;
      x = cx[k];
      y = cy[k];
      v[x][y] = 1;
      nv++;
      t++;
      // If timeout, stop here
      if (t > xc*yc*4) break;
    }
  }

  // Put diamonds

  for (i = 0 ; i < 3 ; i++)
  {
    d = 0;
    while (d == 0)
    {
      x = 2*(rand()%((X_MAP-5)/2))+2; // +3  
      y = 2*(rand()%((Y_MAP-5)/2))+2;
      if (map[x][y] == ICE)
      {
         map[x][y] = DIAMOND;
         d = 1;
      }
    }
  }
  free(v);
} 


// Fuction to display the border 

void DisplayBorder(XImage *xim,unsigned long *lut,int pmx,int pmy,int color)
{
  int i,j;

  for (j = 0 ; j < W_BRDR ; j++)
  {
    for (i = 0 ; i < pmx ; i++)
    {
      Dot(xim,lut,i,j+W_HDR,color);
    }
  }
  for (j = W_BRDR ; j < (pmy - W_BRDR - W_HDR) ; j++)
  { 
    for (i = 0 ; i < (W_BRDR-1) ; i++)
    {
      Dot(xim,lut,i,j+W_HDR,color);
      Dot(xim,lut,pmx-W_BRDR+i+1,j+W_HDR,color);
    }
  }
  for (j = (pmy-W_BRDR) ; j < pmy ; j++)
  {
    for (i = 0 ; i < pmx ; i++)
    {
      Dot(xim,lut,i,j,color);
    }
  }
}


// Function to create a color in the LUT 

void CreateColor(Display *display,Colormap colmap,unsigned long *lut,int i,int r,int g,int b)
{
  XColor colorx;

  colorx.red = r;
  colorx.green = g;
  colorx.blue = b;
  XAllocColor(display,colmap,&colorx);
  lut[i] = colorx.pixel;
}


// Function to create a sprite shape
             
void CreateSprite(char **spr_pix,int sprite_num,unsigned char *sprite_mem)
{
  int i,j;
  int a,n;

  a = 256*sprite_num;

  for (i = 0 ; i < 16 ; i++)
  {
    for (j = 0 ; j < 16 ; j++)
    {
      n = (int)spr_pix[i][j]-48;
      sprite_mem[a] = n;
      //sprite_mem[a] = LUT[n];
      a++;
    }
  }
} 


// Function to put a sprite on the playscreen

void PutSprite(XImage *xim,unsigned long *lut,unsigned char *sprite_mem, int sprite_num, int x, int y)
{
  int i,j;
  int a;
  
  a = 256*sprite_num;

  for (j = 0 ; j < 16 ; j++)
  {
    for (i = 0 ; i < 16 ; i++)
    {
      Dot(xim,lut,x+i-W_BRDR,y+j-W_BRDR+W_HDR,sprite_mem[a]);
      a++;
    }
  }
}
  

// Function to erase a sprite from the playscreen
                    
void EraseSprite(XImage *xim,unsigned long *lut,int x,int y)
{
  int i,j;
  
  for (j = 0 ; j < 16 ; j++)
  {
    for (i = 0 ; i < 16 ; i++)
    {
      Dot(xim,lut,x+i-W_BRDR,y+j-W_BRDR+W_HDR,0);
    }
  }
}

   
// Function to draw a dot (that is actually a block of 3x3 pixels) 

void Dot(XImage *xim,unsigned long *lut,int x,int y,int color)
{ 
  int xx,yy;
  
  xx = 3*x;
  yy = 3*y;

  XPutPixel(xim,xx,yy,lut[color]);
  XPutPixel(xim,xx+1,yy,lut[color]);
  XPutPixel(xim,xx+2,yy,lut[color]);
  XPutPixel(xim,xx,yy+1,lut[color]);
  XPutPixel(xim,xx+1,yy+1,lut[color]);
  XPutPixel(xim,xx+2,yy+1,lut[color]);
  XPutPixel(xim,xx,yy+2,lut[color]);
  XPutPixel(xim,xx+1,yy+2,lut[color]);
  XPutPixel(xim,xx+2,yy+2,lut[color]);
} 


// Function to poll the keyboard

long GetTheKeyNoBlock(Display *display)
{
  XEvent report;
  char kBuf[2];
  long kk;
  KeySym theKey;
  int stillPressed;
  
  kk=-1;
  
  if (XCheckMaskEvent(display,KeyPressMask,&report) == True)
  { 
    XLookupString(&(report.xkey),kBuf,2,&theKey,NULL);
    kk=(long)theKey;
    stillPressed = 1;
    while(stillPressed)
    {  
      if (XCheckMaskEvent(display,KeyPressMask,&report) == True)
      {
        stillPressed = 1;
      }
      else stillPressed = 0;
    }
  }
  return(kk);
}


// Function to make sure no key is pressed

void WaitKeyReleased(Display *display)
{
  long done;
  XEvent report;

  done = 0; 
  while (!done)
  { 
    XNextEvent(display,&report);
    if (report.type == KeyRelease) done = 1;
  }
}


// Function to create the character set bitmap

void CreateCharset(unsigned char *charset,char *charsethexa)
{
  int i,k;
  unsigned char x,y;

  k = 0;
  for (i = 0 ; i < 104 ; i++)
  {
    x = charsethexa[k];
    if ((x >= '0') && (x <= '9')) x -= 48;
    if ((x >= 'A') && (x <= 'F')) x -= 55;
    x *= 16;
    y = charsethexa[k+1];
    if ((y >= '0') && (y <= '9')) y -= 48;
    if ((y >= 'A') && (y <= 'F')) y -= 55;
    x += y;
    charset[i] = x;
    k += 2;
  }
}


// Function to print a character on the header line

void PrintChar(XImage *xim, unsigned long *lut,unsigned char *charset,int c,int x,int ink, int bgd)
{
  int i,j,k;
  unsigned char v,m;
  k =7*c;

  for (i = 0 ; i < 7 ; i++) 
  {
    v = (unsigned char)charset[k+i];
    m = 128;
    for (j = 0 ; j < 8 ; j++)
    {
      if ((v & m) == 0) Dot(xim,lut,8*x+j,i+2,bgd);
      else Dot(xim,lut,8*x+j,i+2,ink);
      m /= 2;
    }
  }
}


// Function to print the score on the header line

void PrintScore(XImage *xim, unsigned long *lut,unsigned char *charset,int s,int x,int ink, int bgd)
{
  int d,i,m,n;

  m = 100000;
  if (s > 999999) n = 999999; else n = s;
  for (i = 0 ; i < 6 ; i++)
  {
    d = n/m;
    PrintChar(xim,lut,charset,d,2+i,ink,bgd);
    n = n%m;
    m /= 10;
  }
} 

