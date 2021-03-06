#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>

#define BITSET(x, n)   (((x) & (1<<(n)))?1:0)

unsigned char currentbyte=0;
char bitptr=-1;
unsigned char *workarea;
int buffptr;

typedef struct
{
   unsigned char map[32][24];
   unsigned char teleport[24];
   unsigned char visible;
   unsigned char password;
   unsigned char score;
   unsigned char palette[4];
} mapdata;

BITMAP *sprites[48];
RGB colours[255];

unsigned char readbits(buffer, ptr)
unsigned char *buffer;
{
   unsigned char out=0,i,bit;
   
   for (i=0; i<5; i++)
   {
      if (bitptr == -1)
      {
         // Current byte is empty
         currentbyte=buffer[buffptr++];
         bitptr=7;
      }
      
      // grab the bit at bitptr and put it in out
      bit=BITSET(currentbyte,bitptr);
      out|=bit;
      if (i<4) out=out << 1;
      bitptr--;
   }
   return out;
}

void decrypt(buffer, size)
unsigned char *buffer;
int size;
{
   unsigned char eor, byte;
   int ptr=0;
   eor=0;
   
   while (ptr < size)
   {
      byte=buffer[ptr];
      byte=byte ^ eor;
      eor=eor-3;

      buffer[ptr++]=byte;
   }
}

mapdata readmap(buffer)
unsigned char *buffer;
{
   mapdata map;
   int x,y,i;
   // Now read the stream
   for (y=0; y < 24; y++)
   {
      for (x=0; x < 32; x++)
      {
         map.map[x][y]=readbits(buffer, buffptr);
      }
   }
   for (i=0;i<24;i++)
   {
      map.teleport[i]=workarea[buffptr++];
   }
   map.visible=workarea[buffptr++];
   map.password=workarea[buffptr++];
   map.score=workarea[buffptr++]+workarea[buffptr++]*256;
   for (i=0;i<4;i++)
   {
      map.palette[i]=workarea[buffptr++];
   }
   return map;
}

BITMAP *readsprite(buffer, xsize, ysize, mode)
unsigned char *buffer;
int xsize, ysize, mode;
{
   int x,y,i,j,size;
   int ppb, bpixel=1;
   unsigned char byte, colour;
   BITMAP *sprite;

   // set up stuff for mode

   if (mode==1 || mode==5) ppb=4;
   if (mode==4 || mode==0) ppb=8;
   if (mode==2) ppb=2;
   if (mode==2 || mode==5) bpixel=2;
   
   // workspace
   sprite=create_bitmap(xsize*bpixel, ysize);
   size=(xsize/ppb)*ysize;
   
   // Now make the sprites
   x=0; y=0;
   for (i=0; i<size; i++)
   {
      byte=workarea[buffptr++];
      
      for (j=0;j<4;j++)
      {
         colour=BITSET(byte,7-j);
         colour <<= 1;
         colour |= BITSET(byte,3-j);
         putpixel(sprite,x,y,colour);
         putpixel(sprite,x+1,y,colour);
         x+=2;
      }
      x-=8;
      y+=1;
      if ((y%8) == 0)
      {
         y-=8;
         x+=8;
      }
      if (x>31)
      {
         x=0;
         y+=8;
      }
   }
   return sprite;
}


int main(argc, argv)
int argc;
char **argv;
{
   FILE *infile;
   unsigned char byte;
   unsigned char ptr;
   int i,j,k;
   mapdata maps[4];
   unsigned char x,y;
   char outfile[255];   
   
   allegro_init();

   PALETTE pal;
   RGB black = { 0, 0, 0 };
   RGB red = { 63, 0, 0 };
   RGB green = { 0, 63, 0 };
   RGB yellow = { 63, 63, 0 };   
   RGB magenta = { 63, 0, 63 };
   RGB blue = { 0, 0, 63 };
   RGB cyan = { 0, 63, 63 };
   RGB white = { 63, 63, 63 };
  
   colours[0]=black; colours[1]=red;
   colours[2]=green; colours[3]=yellow;
   colours[4]=blue;  colours[5]=magenta;
   colours[6]=cyan;  colours[7]=white;
   
   // workarea to store the file
   workarea=malloc(9520);
   if (workarea==NULL)
   {
      printf("Could not allocate enough memory\n");
      exit(1);
   }
      
   infile=fopen(argv[1],"rb");
   strncpy(outfile,argv[2],255);
   
   if (infile == NULL)
   {
      printf("Could not load file %s\n",argv[1]);
   }
   fread(workarea,9520,1,infile);
   fclose(infile);
   decrypt(workarea, 9520);
   
   // Now parse the map data
   printf("Parsing map data\n");
   buffptr=0x1b50;
   for (i=0; i<4; i++)
   {
      maps[i]=readmap(workarea);
   }

   // Create the sprites bitmaps
   printf("Creating bitmaps\n");   
   buffptr=0;
   for (i=0;i<48;i++)
   {
      sprites[i]=readsprite(workarea,16,32,5);
   }

   // Now make the big map
   set_color(10,&white);
   for (i=0; i<4; i++)
   {
      BITMAP *bigmap=create_bitmap(1088,832);
      char filename[256];
      
      // Set the palette
      for (j=0;j<4;j++)
      {
         set_color(j,&colours[maps[i].palette[j]]);
      }
      // Place a border
      for (x=0; x<34; x++)
      {
         draw_sprite(bigmap, sprites[2],x*32,0);
         draw_sprite(bigmap, sprites[2],x*32,25*32);
      }
      for (y=1; y<25; y++)
      {
         draw_sprite(bigmap, sprites[2],0, y*32);
         draw_sprite(bigmap, sprites[2],33*32, y*32);
      }
      
      for (x=0; x<32; x++)
      {
         for (y=0; y<24; y++)
         {
            draw_sprite(bigmap, sprites[maps[i].map[x][y]], (x+1)*32,(y+1)*32);
         }
      }
      
      // Highlight the transporters
      for (j=0; j < 6; j++)
      {
         int source, destination;
         
         source=maps[i].teleport[j*4]+maps[i].teleport[(j*4)+1]*256;
         destination=maps[i].teleport[(j*4)+2]+maps[i].teleport[(j*4)+3]*256;
         
         if (source < 6176 && destination < 6176)
         {
            x=source % 32 + 1;
            y=source / 32 + 1;
            textprintf_ex(bigmap,font,x*32+4,y*32+4,10,0,"%d",j);
            x=destination % 32 + 1;
            y=destination / 32 + 1;
            textprintf_ex(bigmap,font,x*32+4,y*32+4,10,0,"%d",j);
         }
      }
         
      // And save it
      sprintf(filename,"%s%d.bmp",outfile,i);
      get_palette(pal);
      save_bitmap(filename, bigmap, pal);
      free(bigmap);
   }
}

END_OF_MAIN();

