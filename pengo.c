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
#define NB_SNOBEE 3

#define NB_SPRITES 16

void movePengo(int **map,int png_x, int png_y, int png_dx, int png_dy, int *png_state, int elec_cnt);

void NewCubePushed(int **map,int *psh_flag,int *psh_x,int *psh_y, int *psh_dx,int *psh_dy,int *psh_typ,int p_x,int p_y,int dp_x,int dp_y);

void CheckSnobeePushed(int **map,int *snb_x,int *snb_y, int *snb_state, int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy);

void CheckCubeStopped(int **map,int *snb_x,int *snb_y,int *snb_state,int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy);

void ShockSnobee(int *snb_x,int *snb_y,int snb_state,int png_x,int png_y,int png_dx,int png_dy);

void CreateMap(int **map);

void CreateSprite(char **spr_pix,int sprite_num,unsigned char *sprite_mem); 

void PutSprite(unsigned char **pixmap,unsigned char *sprite_mem, int sprite_num, int x, int y);


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

  int elec_cnt;

  int i,j;
  int key;
  int running;

  
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


  theLalEnv = new LalEnv(argc,argv);
  theLal = new Lal("Pengo",100,100);
  theLalEnv->AttachLal(theLal);

  printf("OK 2\n");
  theLal->SetLut(COLOR);
  theLal->NewMap("Pengo",Lblue,0,0,pmx,pmy,2,pixmap,&pt);
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
  elec_cnt = 0;

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
    movePengo(map,png_x,png_y,png_dx,png_dy,&png_state,elec_cnt);
    printf("state = %d ; dx = %d ; dy = %d\n", png_state,png_dx,png_dy);
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
    for (i = 0 ; i < 16 ; i++)
    {
      if (png_state == MOVING)
      {
        PutSprite(pixmap,sprite_mem,png_spr+(i%2),png_x*16+png_dx*i-W_BRDR,png_y*16+png_dy*i-W_BRDR);
      }
      theLal->Update();
      usleep(20000);
    }
    png_x+=png_dx;
    png_y+=png_dy;
  }
  
  delete theLal;
  delete theLalEnv;
  exit(0);
 
}


void movePengo(int **map,int png_x, int png_y, int png_dx, int png_dy, int *png_state, int elec_cnt)
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

void CheckSnobeePushed(int **map,int *snb_x,int *snb_y, int *snb_state, int *psh_flag,int *psh_x,int *psh_y,int *psh_dx,int *psh_dy)
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
              ((psh_y[i]+psh_dy[j]) == snb_y[j]))
          {
            // look if snobee crushed against static cube or border
            if (map[psh_x[i]+2*psh_dx[i]][psh_y[i]+2*psh_dy[i]] != 0)
            {
              snb_state[j] = CRUSHED;
            }
            else
            {
              snb_state[j] = PUSHED;
            }
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
      x = 2*(rand()%((X_MAP-3)/2))+3;
      y = 2*(rand()%((Y_MAP-3)/2))+3;
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
  LUT[5] = 100 ; // yellow 
  LUT[6] = 254;
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
  
                    
    
