#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint32;

typedef struct
{
   uint32 unknown1;
   uint32 unknown2;
   uint32 offset;
   uint32 size;
} file_header_t;

int main(int argc, char **argv)
{
   FILE *infile, *outfile;
   uint32 working, nfiles;
   char outname[256];
   file_header_t files[65535];
   int i;
   unsigned char *buffer;
   
   infile=fopen(argv[1], "rb");
   fseek(infile, 8, SEEK_SET);
   
   fread(&nfiles, 4, 1, infile);
   printf("Number of files: %d\n", nfiles);
   fseek(infile, 0x18, SEEK_SET);
   for (i=0;i<nfiles;i++)
   {
      fread(&files[i], sizeof(file_header_t), 1, infile);
   }
   
   // Now save the files
   for (i=0; i<nfiles;i++)
   {

      fseek(infile, files[i].offset, SEEK_SET);
      buffer=malloc(files[i].size);
      fread(buffer, files[i].size, 1, infile);
      sprintf(outname,"%d.dat", i);
      printf("File %d: %s %x\n",i,outname,files[i].offset);
      outfile=fopen(outname, "wb");
      fwrite(buffer, files[i].size, 1, outfile);
      fclose(outfile);
      free(buffer);
   }
}
      
   
   