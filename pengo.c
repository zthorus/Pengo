// Pengo

#include "lal.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// Map (playground) constants

#define X_MAP 15
#define Y_MAP 17
#define ICE 1
#define DIAMOND 2
#define BORDER 3
#define W_BRDR 8
#define NB_GATES 80

// Pengo states
#define IDLE 0
#define MOVING 1
#define PUSHING 2
#define CRASHING 3
#define ELEC 4

// Snobee states
#define ACTIVE 0
#define REBIRTH 3
#define DEAD 6
#define PUSHED 7
#define CRUSHED 8
#define SHOCKED 9

#define MAX_PUSH 10
#define MAX_CRASH 10
#define NB_SNOBEE 3

#define HORIZONTAL 0
#define VERTICAL   1

#define NB_SPRITES 24 

void MovePengo(int **map,int png_x, int png_y, int png_dx, int png_dy, int *png_state, int elec_cnt);

void NewCubePushed(int **map,int *psh_flag,int *psh_x,int *psh_y, int *psh_dx,int *psh_dy,int *psh_typ,int p_x,int p_y,int dp_x,int dp_y);

void NewCubeCrashed(int *crsh_flag,int *crsh_x,int *crsh_y,int x,int y);

void CheckSnobeePushed(int **map,int *snb_x,int *snb_y,int *snb_dx,int *snb_dy,int *snb_state, int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy);

void CheckCubeStopped(int **map,int *snb_x,int *snb_y,int *snb_state,int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy);

void ShockSnobee(int *snb_x,int *snb_y,int snb_state,int png_x,int png_y,int png_dx,int png_dy);

void MoveSnobees(int **map,int *snb_state, int *snb_x, int *snb_y, int *snb_dx, int *snb_dy, int *snb_ax, int p_x, int p_y, int *crsh_flag, int *crsh_x, int *crsh_y);

void CreateMap(int **map);

void CreateSprite(char **spr_pix,int sprite_num,unsigned char *sprite_mem); 

void PutSprite(unsigned char **pixmap,unsigned char *sprite_mem, int sprite_num, int x, int y);

void EraseSprite(unsigned char **pixmap,int x,int y);

int main(int argc, char **argv)
{
  LalEnv *theLalEnv;
  Lal *theLal;
  Point pt;

  unsigned char **pixmap;
  unsigned char *sprite_mem;
  //char spr_pix[16][17];
  char **spr_pix;

  int pmx = (X_MAP-2)*16 + 2*W_BRDR;
  int pmy = (Y_MAP-2)*16 + 2*W_BRDR + 1;

  int **map;

  int png_x,png_y;
  int png_dx,png_dy;
  int png_state;
  int png_spr;

  int psh_flag[MAX_PUSH];
  int psh_x[MAX_PUSH];
  int psh_y[MAX_PUSH];
  int psh_dx[MAX_PUSH];
  int psh_dy[MAX_PUSH];
  int psh_typ[MAX_PUSH];

  int crsh_flag[MAX_CRASH];
  int crsh_x[MAX_CRASH];
  int crsh_y[MAX_CRASH];

  int snb_state[NB_SNOBEE];
  int snb_x[NB_SNOBEE];
  int snb_y[NB_SNOBEE];
  int snb_dx[NB_SNOBEE];
  int snb_dy[NB_SNOBEE];
  int snb_ax[NB_SNOBEE];
  int snb_spr;

  int elec_cnt;

  int i,j,k,l;
  int key;
  int running;
  int xc,yc;
  int dxc,dyc;
  int ct;

  
  pixmap = (unsigned char **)malloc(pmx*sizeof(unsigned char *));
  if (pixmap == NULL)
  {
    printf("Cannot allocate memory for array: pixmap\n");
    exit(0);
  }
  for (i = 0 ; i < pmx ; i++)
  {
    pixmap[i] = (unsigned char *)malloc(pmy*sizeof(unsigned char));
    if (pixmap[i] == NULL)
    {
      printf("Cannot allocate memory for array: pixmap[%d]\n",i);
      exit(0);
    }
    else
    {
      // fill pixmap with cyan (background color)
      for (j = 0; j < (pmy-1); j++) pixmap[i][j] = 208;
    }
  }
  // last line of pixmap = 0 and 255 to use full color LUT 
  for (i = 0 ; i < pmx ; i+=2)
  {
     pixmap[i][pmy-1] = 0;
     pixmap[i+1][pmy-1] = 255;
  }
  
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
 
  strcpy(spr_pix[1], "0000004444000000");
  strcpy(spr_pix[2], "0000044444400000");
  strcpy(spr_pix[3], "0000442442440000");
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
  strcpy(spr_pix[3], "0000024444440000");
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
  strcpy(spr_pix[3], "0000444444200000");
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

  strcpy(spr_pix[0], "0000000000000000");
  strcpy(spr_pix[1], "0000000000000000");
  strcpy(spr_pix[2], "0000006666000000");
  strcpy(spr_pix[3], "0000066666600000");
  strcpy(spr_pix[4], "0000666666660000");
  strcpy(spr_pix[5], "0006662662666000");
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
  strcpy(spr_pix[5], "0006666666620000");
  strcpy(spr_pix[6], "0006666666690000");
  strcpy(spr_pix[7], "0066666666660000");
  strcpy(spr_pix[8], "0066666666655500");
  strcpy(spr_pix[9], "0066666666660000");
  strcpy(spr_pix[10],"0066666666660000");
  strcpy(spr_pix[11],"0066666666600000");
  strcpy(spr_pix[12],"0066666666600000");
  strcpy(spr_pix[13],"0006666666000000");

  CreateSprite(spr_pix,20,sprite_mem);

  strcpy(spr_pix[5], "0000266666666000");
  strcpy(spr_pix[6], "0000966666666000");
  strcpy(spr_pix[7], "0000666666666600");
  strcpy(spr_pix[8], "0055566666666600");
  strcpy(spr_pix[9], "0000666666666600");
  strcpy(spr_pix[10],"0000666666666600");
  strcpy(spr_pix[11],"0000066666666600");
  strcpy(spr_pix[12],"0000066666666600");
  strcpy(spr_pix[13],"0000006666666000");

  CreateSprite(spr_pix,21,sprite_mem);

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

  theLalEnv = new LalEnv(argc,argv);
  theLal = new Lal("Pengo",100,100);
  theLalEnv->AttachLal(theLal);

  printf("OK 2\n");
  theLal->SetLut(COLOR);
  theLal->NewMap("Pengo",Lblue,0,0,pmx,pmy,3,pixmap,&pt);
  theLal->Prepare();
  theLal->Show();

  printf("OK 3\n");
  CreateMap(map);
  printf("OK 4\n");

  for (i = 1; i < (X_MAP-1) ; i++)
  {
    for (j = 1; j < (Y_MAP-1) ; j++)
    {
      if (map[i][j] == ICE)
      {
        printf("%d %d\n",i,j); 
        PutSprite(pixmap,sprite_mem,0,i*16-W_BRDR,j*16-W_BRDR);
      }
      if (map[i][j] == DIAMOND)
      {
        PutSprite(pixmap,sprite_mem,1,i*16-W_BRDR,j*16-W_BRDR);
      }
    }
  }

  png_x = 8;
  png_y = 8;

  snb_x[0] = 4;
  snb_y[0] = 4;
  snb_ax[0] = VERTICAL;
  snb_dx[0] = 0;
  snb_dy[0] = 1;
  snb_state[0] = ACTIVE;
  snb_x[1] = 10;
  snb_y[1] = 4;
  snb_ax[1] = HORIZONTAL;
  snb_dx[1] = -1;
  snb_dy[1] = 0;
  snb_state[1] = ACTIVE;
  snb_x[2] = 10;
  snb_y[2] = 10;
  snb_ax[2] = VERTICAL;
  snb_dx[2] = 0;
  snb_dy[2] = -1;
  snb_state[2] = ACTIVE;

  elec_cnt = 0;
  for (i = 0 ; i < MAX_PUSH ; i++) psh_flag[i] = 0;
  for (i = 0 ; i < MAX_CRASH ; i++) crsh_flag[i] = 0;

  PutSprite(pixmap,sprite_mem,2,png_x*16-W_BRDR,png_y*16-W_BRDR);

  printf("OK 5\n");
       
  theLal->Update();
  printf("OK 6\n");

  running=1;
  while(running)
  {
    png_dx = 0;
    png_dy = 0;
    key = theLal->GetTheKeyNoBlock();
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
        default:
          running = 0;
          break;
      }
    }
    MovePengo(map,png_x,png_y,png_dx,png_dy,&png_state,elec_cnt);
    printf("state = %d ; dx = %d ; dy = %d\n", png_state,png_dx,png_dy);
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
      PutSprite(pixmap,sprite_mem,png_spr+2,png_x*16-W_BRDR,png_y*16-W_BRDR);
      png_dx = 0;
      png_dy = 0;
    }
    MoveSnobees(map,snb_state,snb_x,snb_y,snb_dx,snb_dy,snb_ax,png_x,png_y,crsh_flag,crsh_x,crsh_y);
    CheckSnobeePushed(map,snb_x,snb_y,snb_dx,snb_dy,snb_state,psh_flag,psh_x,psh_y,psh_dx,psh_dy);
    printf("snobee motion OK\n");
    // animations

    for (i = 1 ; i <= 16 ; i++)
    {
      // pengo

      if (png_state == MOVING)
      {
        PutSprite(pixmap,sprite_mem,png_spr+(i%2),png_x*16+png_dx*i-W_BRDR,png_y*16+png_dy*i-W_BRDR);
      }

      // pushed cubes (moving twice faster than other sprites)

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
          printf("xc = %d ; yc = %d ; dxc = %d ; dyc = %d ; ct = %d\n",xc,yc,dxc,dyc,ct); 
          if ((i == 8) || (i == 16))
          {
            // cube has moved from cell
            map[xc][yc] = 0;
            map[xc+dxc][yc+dyc] = ct;
            // check if cube can still move
            if (map[xc+2*dxc][yc+2*dyc] != 0)
            {
              psh_flag[j] = 0;
              // check if snobee was crushed by cube
              for (l = 0 ; l < NB_SNOBEE ; l++)
              {
                if ((snb_state[l] == CRUSHED) && (snb_x[l] == xc+dxc) && (snb_y[l] == yc+dyc))
                  snb_state[l] = DEAD;
              }
            }
            psh_x[j] += dxc;
            psh_y[j] += dyc;
            xc = psh_x[j];
            yc = psh_y[j];
            // update coordinates of pushed snobees
            for (l = 0 ; l < NB_SNOBEE ; l++)
            {
              if ((snb_state[l] == PUSHED) && (snb_x[l] == xc) && (snb_y[l] == yc))
              {
                snb_x[l] += dxc;
                snb_y[l] += dyc;
              }
            } 
            // check if Snobee half-way in front of cube => pushed or crashed
            
            if (i == 8)
              CheckSnobeePushed(map,snb_x,snb_y,snb_dx,snb_dy,snb_state,psh_flag,psh_x,psh_y,psh_dx,psh_dy);
          }
          EraseSprite(pixmap,xc*16+2*dxc*(k-1)-W_BRDR,yc*16+2*dyc*(k-1)-W_BRDR);
          PutSprite(pixmap,sprite_mem,ct-1,xc*16+2*dxc*k-W_BRDR,yc*16+2*dyc*k-W_BRDR);
        }
      }

      // crashed cubes
      
      for (j = 0 ; j < MAX_CRASH ; j++)
      {
        if (crsh_flag[j] == 1)
        {
          xc = crsh_x[j];
          yc = crsh_y[j];
          printf("crashing cube %d at (%d ; %d)\n", j,xc,yc);
          k= i/4;
          if (k != 4) PutSprite(pixmap,sprite_mem,14+k,xc*16-W_BRDR,yc*16-W_BRDR);
          else // if i == 16 (end of animation loop)
          {
            EraseSprite(pixmap,xc*16-W_BRDR,yc*16-W_BRDR);
            map[xc][yc] = 0;
            crsh_flag[j] = 0;
          }
        }
      }

      // snobees

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
            PutSprite(pixmap,sprite_mem,snb_spr,snb_x[j]*16+snb_dx[j]*i+k-W_BRDR,snb_y[j]*16+snb_dy[j]*i+k-W_BRDR);
            break;

          case PUSHED:
            k = i%8; 
            PutSprite(pixmap,sprite_mem,18,snb_x[j]*16+2*snb_dx[j]*k-W_BRDR,snb_y[j]*16+2*snb_dy[j]*k-W_BRDR);
            break;

          case CRUSHED:
            PutSprite(pixmap,sprite_mem,22,snb_x[j]*16-W_BRDR,snb_y[j]*16-W_BRDR);
            break;
        }
      } 
      theLal->Update();
      usleep(16000);
    }
    // update player coordinates on map 
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
  }
  
  delete theLal;
  delete theLalEnv;
  exit(0);
 
}


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
        printf("pushing \n");
      }
      else
      {
        // look if ice-cube will be crashed
        switch (map[png_x+png_dx][png_y+png_dy])
        {
          case ICE:
            p_state = CRASHING;
            printf("pushing \n");
            break;
          case DIAMOND: 
            p_state = IDLE;
            break;
        }
      }
    }
    // look if Pengo is touching the border
    if (map[png_x+png_dx][png_y+png_dy] == BORDER)
    {
      // look if electric border can be activated 
      if (elec_cnt == 0) p_state = ELEC; else p_state = IDLE;
    }
  }
  *png_state = p_state;
}


void NewCubePushed(int **map,int *psh_flag,int *psh_x,int *psh_y, int *psh_dx,int *psh_dy,int *psh_typ,int p_x,int p_y,int dp_x,int dp_y)
{ 
  int i;
 
  // allocate entry in table
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

void NewCubeCrashed(int *crsh_flag,int *crsh_x,int *crsh_y,int x,int y)
{
  int i;
 
  // allocate entry in table
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
  
void CheckSnobeePushed(int **map,int *snb_x,int *snb_y,int *snb_dx,int *snb_dy,int *snb_state, int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy)
{
  int i,j;

  for (i = 0 ; i < MAX_PUSH; i++)
  {
    if (psh_flag[i] == 1)
    {
      // look if pushed cube is pushing a snobee
      for (j = 0; j < NB_SNOBEE ; j++)
      {
        if ((snb_state[j] == ACTIVE) || (snb_state[j] == PUSHED))
        {
          if (((psh_x[i]+psh_dx[i]) == snb_x[j]) &&
              ((psh_y[i]+psh_dy[i]) == snb_y[j]))

          {
            printf("-- cube: (%d ; %d) - snobee (%d ; %d) \n",psh_x[i],psh_y[i],snb_x[j],snb_y[j]); 
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
          // look if snobee is about to cross a pushed cube
          // (can also be half-way).  If so, consider it doomed 
          if (((psh_x[i]+psh_dx[i]) == (snb_x[j]+snb_dx[j])) &&
              ((psh_y[i]+psh_dy[i]) == (snb_y[j]+snb_dy[j])) &&
              (snb_state[j] == ACTIVE))
          {
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
            // force snobee to be in front of pushed cube
            snb_x[j] += snb_dx[j];
            snb_y[j] += snb_dy[j];
          } 
        }
      }
    }
  }
}


void CheckCubeStopped(int **map,int *snb_x,int *snb_y,int *snb_state,int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy)
{
  int i,j;

  for (i = 0 ; i < MAX_PUSH; i++)
  {
    // look if pushed cube has reached limit
    if (map[psh_x[i]+psh_dx[i]][psh_y[i]+psh_dy[i]] != 0)
    {
      psh_flag[i] = 0;
      // look if there is a crushed snobee between cube and limit
      for (j = 0; j < NB_SNOBEE ; j++)
      {
        if ((snb_state[j] == CRUSHED) && (snb_x[j] == psh_x[i]) &&
         (snb_y[j] == psh_y[i]))
        {
            snb_state[j] = DEAD;
        }
      }
    }
  }   
}


void ShockSnobee(int *snb_x,int *snb_y,int *snb_state,int png_x,int png_y,int png_dx,int png_dy)
{
  int i;

  for (i = 0; i < NB_SNOBEE ; i++)
  {
    if (((snb_x[i] == png_x) && (png_dx != 0)) ||
        ((snb_y[i] == png_y) && (png_dy != 0)))
    {
      if (snb_state[i] == ACTIVE) snb_state[i] = SHOCKED;
    }
  }
}


void MoveSnobees(int **map,int *snb_state, int *snb_x, int *snb_y, int *snb_dx, int *snb_dy, int *snb_ax, int p_x, int p_y, int *crsh_flag, int *crsh_x, int *crsh_y)
{
  int i,k;
  int change_axis;

  for (i = 0 ; i < NB_SNOBEE ; i++)
  {
    if (snb_state[i] == ACTIVE)
    {
      change_axis = 0;
      k = rand()%3;
      if (snb_ax[i] == VERTICAL)
      {
        if (snb_dy[i] == 0) // i.e., if snobee was crashing cube before...
        { 
          if ((snb_y[i] - p_y) > 0) snb_dy[i] = -1; 
          if ((snb_y[i] - p_y) < 0) snb_dy[i] = 1; 
        }
        if (snb_y[i] == p_y) change_axis = 1;
        else
        {
          if (map[snb_x[i]][snb_y[i]+snb_dy[i]] != 0)
          {
            if ((map[snb_x[i]][snb_y[i]+snb_dy[i]] == ICE) && (k == 0))
            {
              NewCubeCrashed(crsh_flag,crsh_x,crsh_y,snb_x[i]+snb_dx[i],snb_y[i]+snb_dy[i]);
              snb_dy[i] = 0;
            }
            else
            {
              if (k == 2) snb_dy[i] = -snb_dy[i];
              else change_axis = 1;
            }
          }
        }
        if (change_axis == 1)
        {
          snb_ax[i] = HORIZONTAL;
          snb_dy[i] = 0;
          switch(k)
          {
            case 0: 
              if ((snb_x[i] - p_x) > 0) snb_dx[i] = -1; 
              if ((snb_x[i] - p_x) < 0) snb_dx[i] = 1; 
              break;
            case 1:
              snb_dx[i] = -1;
              break;
           case 2:
             snb_dx[i] = 1;
             break;
          }
        }
      }
      else
      {
        if (snb_dx[i] == 0)
        {
          if ((snb_x[i] - p_x) > 0) snb_dx[i] = -1; 
          if ((snb_x[i] - p_x) < 0) snb_dx[i] = 1;
        } 
        if (snb_x[i] == p_x) change_axis = 1;
        else
        {
          if (map[snb_x[i]+snb_dx[i]][snb_y[i]] != 0)
          {
            if ((map[snb_x[i]+snb_dx[i]][snb_y[i]] == ICE) && (k == 0))
            {
              NewCubeCrashed(crsh_flag,crsh_x,crsh_y,snb_x[i]+snb_dx[i],snb_y[i]+snb_dy[i]);
              snb_dx[i] = 0;
            }
            else 
            {
              if (k == 2) snb_dx[i] = -snb_dx[i];
              else change_axis = 1;
            }
          }
        }
        if (change_axis == 1)
        {
          snb_ax[i] = VERTICAL;
          snb_dx[i] = 0;
          switch(k)
          {
            case 0: 
              if ((snb_y[i] - p_y) > 0) snb_dy[i] = -1; 
              if ((snb_y[i] - p_y) < 0) snb_dy[i] = 1; 
              break;
            case 1:
              snb_dy[i] = -1;
              break;
            case 2:
              snb_dy[i] = 1;
              break;
          }

        }
         
      }
      //
      //snb_dx[i] = snb_dy[i] = 0;
      //
    }
    //printf("snb_dx = %d ; snb_dy = %d ; snb_ax = %d ; axis changed = %d ; state = %d \n", snb_dx[i],snb_dy[i],snb_ax[i],change_axis,snb_state[i]); 
  }
} 


  
void CreateMap(int **map)
{
  int i,j;
  int x,y;
  int d;

  // variable for maze creation
  int **v;  // array of visited cells
  int xc,yc; // number of cells along x and y
  int nc;    // total number of cells
  int nv;    // number of visited cells
  int nn;    // number of possible next cells (unvisited yet)
  int cx[4]; // x of possible next cell
  int cy[4]; // y of possible next cell
  int sx[100]; // stack of x visited cells
  int sy[100]; // stack of y visited cells
  int sp;      // stack pointer
  int k;       // random number
  int t;       // time counter

  // create the borders

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

  // create first an empty field within the borders

  for (i = 1 ; i < (X_MAP-1) ; i++)
  {
    for (j = 1 ; j < (Y_MAP-1) ; j++)
    {
      map[i][j] = 0;
    }
  }

  // create a grid of ice cubes

  for (i = 1 ; i < (X_MAP-1) ; i+=2)
  {
    for (j = 1 ; j < (Y_MAP-1) ; j++)
    {
      map[i][j] = ICE;
    }
  }
  for (j = 1 ; j < (Y_MAP-1) ; j+=2)
  {
    for (i = 1 ; i < (X_MAP-1) ; i++)
    {
      map[i][j] = ICE;
    }
  }

  printf("OK map 0\n");

  // create a "maze" = random paths between the cells of the grid

  xc = (X_MAP-3)/2;
  yc = (Y_MAP-3)/2;

  // create array of visited cells (visited by the maze generator algorithm) 

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

  // first visited cell = at the center
  nv = 1;
  x = xc/2;
  y = yc/2;
  v[x][y] = 1;

  nc = xc*yc;
  sp = 0;
  t = 0;

  // visit all the cells (if it doesn´t take too long) 
  while (nv < nc)
  {
    printf("nv = %d ; t = %d\n",nv,t);
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
    // if all neighboring cells already visited => step-back
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
      printf("sp = %d ; k = %d ; nn = %d ; x = %d ; y = %d\n",sp,k,nn,x,y);
      map[x+cx[k]+2][y+cy[k]+2] = 0;
      x = cx[k];
      y = cy[k];
      v[x][y] = 1;
      nv++;
      t++;
      // if timeout, stop here
      if (t > xc*yc*4) break;
    }
  }

  printf("OK map 1\n");

  // put diamonds

  for (i = 0 ; i < 3 ; i++)
  {
    d = 0;
    while (d == 0)
    {
      x = 2*(rand()%((X_MAP-5)/2))+3;
      y = 2*(rand()%((Y_MAP-5)/2))+3;
      if (map[x][y] == ICE)
      {
         map[x][y] = DIAMOND;
         d = 1;
      }
    }
  }
  free(v);
} 

// Create a sprite
             
void CreateSprite(char **spr_pix,int sprite_num,unsigned char *sprite_mem)
{
  int i,j;
  int a,n;
  unsigned char LUT[10];
  LUT[0] = 208 ; //cyan
  LUT[1] = 237;  // blue
  LUT[2] = 212 ; // light cyan
  LUT[3] = 178 ; // green 
  LUT[4] = 50 ; // red 
  LUT[5] = 90 ; // yellow 
  LUT[6] = 80; // orange
  LUT[7] = 30;  // dark red
  LUT[8] = 254;
  LUT[9] = 254;

  a = 256*sprite_num;

  for (i = 0 ; i < 16 ; i++)
  {
    for (j = 0 ; j < 16 ; j++)
    {
      printf("a = %d\n",a);
      printf("%c\n", spr_pix[i][j]);
      n = (int)spr_pix[i][j]-48;
      printf("%d\n",n);
      printf("%d\n",LUT[n]);
      sprite_mem[a] = LUT[n];
      a++;
    }
  }
} 


// Put a sprite on the playscreen

void PutSprite(unsigned char **pixmap,unsigned char *sprite_mem, int sprite_num, int x, int y)
{
  int i,j;
  int a;
  
  a = 256*sprite_num;

  for (j = 0 ; j < 16 ; j++)
  {
    for (i = 0 ; i < 16 ; i++)
    {
      pixmap[x+i][y+j] = sprite_mem[a];
      a++;
    }
  }
}
  

// Erase a sprite from the playscreen
                    
void EraseSprite(unsigned char **pixmap,int x,int y)
{
  int i,j;
  
  for (j = 0 ; j < 16 ; j++)
  {
    for (i = 0 ; i < 16 ; i++)
    {
      pixmap[x+i][y+j] = 208;
    }
  }
}

    
