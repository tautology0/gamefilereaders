#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

typedef unsigned long uint32;

typedef struct
{
   uint32 magic;
   uint32 version;
   uint32 sod;
   uint32 eod;
} header_t;

typedef struct
{
   char   *filename;
   uint32 start;
   uint32 length;
   uint32 unknown1;
   uint32 unknown2;
} fileh_t;

int
mkpath(const char *s){
        char *q, *r = NULL, *path = NULL, *up = NULL;
        int rv;

        rv = -1;
        if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
                return (0);

        if ((path = strdup(s)) == NULL)
                exit(1);
     
        if ((q = strdup(s)) == NULL)
                exit(1);

        if ((r = dirname(q)) == NULL)
                goto out;
        
        if ((up = strdup(r)) == NULL)
                exit(1);

        if ((mkpath(up) == -1) && (errno != EEXIST))
                goto out;

        if ((mkdir(path) == -1) && (errno != EEXIST))
                rv = -1;
        else
                rv = 0;

out:
        if (up != NULL)
                free(up);
        free(q);
        free(path);
        return (rv);
}

int main(int argc, char **argv)
{
   FILE *infile;
   header_t header;
   fileh_t files[8192];
   uint32 i=0,j=0, numfiles;
   
   infile=fopen(argv[1], "rb");
   if (infile == NULL)
   {
      fprintf(stderr,"Can't open file %s\n",argv[1]);
      exit(1);
   }
   
   fread(&header, sizeof(header_t), 1, infile);
   
   printf("Magic:   %x\n", header.magic);
   printf("Version: %x\n", header.version);
   printf("sod:     %x\n", header.sod);
   printf("eod:     %x\n", header.eod);
   
   // Now read the file headers
   fseek(infile, header.sod, SEEK_SET);
   
   // Read and skip the first 4 bytes
   fread(&numfiles, sizeof(uint32), 1, infile);
   printf("Files:   %d\n", numfiles);
   while (i<numfiles && !feof(infile))
   {
      files[i].filename=malloc(512);
      memset(files[i].filename, '\0', 512);
      j=0;
      do
      {
         files[i].filename[j]=fgetc(infile);
      } while (files[i].filename[j++] != '\0');
      fread(&(files[i].start), sizeof(uint32), 1, infile);
      fread(&(files[i].length), sizeof(uint32), 1, infile);
      fread(&(files[i].unknown1), sizeof(uint32), 1, infile);
      fread(&(files[i].unknown2), sizeof(uint32), 1, infile);

      printf(" File %d: %s\n",i, files[i].filename);
      /*printf(" Filename:  %s\n", files[i].filename);
      printf(" Start:     %x\n", files[i].start);
      printf(" Length:    %x\n", files[i].length);
      printf(" Unknown 1: %x\n", files[i].unknown1);
      printf(" Unknown 2: %x\n", files[i].unknown2);*/
      
      // Extract the files, first do the directory
      char filecopy[512], *fileptr;
      void *buffer;
      FILE *outfile;
      uint32 tmpfile=ftell(infile);
      
      strcpy(filecopy, files[i].filename);
      fileptr=strrchr(filecopy, '/');
      if (fileptr!=NULL)
      {
         *fileptr='\0';
         mkpath(filecopy);
      }
      outfile=fopen(files[i].filename, "wb");
      buffer=malloc(files[i].length);
      fseek(infile, files[i].start, SEEK_SET);
      fread(buffer, files[i].length, 1, infile);
      fwrite(buffer, files[i].length, 1, outfile);
      fclose(outfile);
      free(buffer);
      fseek(infile, tmpfile, SEEK_SET);
      i++;
   }
   fclose(infile);
}