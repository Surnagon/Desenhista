#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "drawer/drawer.h"

// todo: mover join das threada spara esse arquivo

int main(void)
{
	printf("\r\n cabine desenhista");
   char inputstr[100];
   char * pch;
   char filename[22];

	drawer_start();
   
   printf("Command list: \r\n");
   printf("\t abort : abort any drawing in progreess \r\n");
   printf("\t stop  : stop the drawer routines \r\n");
   printf("\t new filename : start new drawing based on black pixels coordinates on file names as filename parameter \r\n");
   printf("\t next : emulate the conclusion of previous pixel \r\n");

   while (1)
   {
      
      fgets(inputstr, 100, stdin);

      pch = strtok(inputstr, " \n");
      if (pch == NULL)
      {
         continue;
      }

      if (!strcmp(pch,"abort"))
      {
         drawer_abort();
      }
      else if (!strcmp(pch, "stop"))
      {
         drawer_stop();
      }
      else if (!strcmp(pch, "new"))
      {
         pch = strtok (NULL," \n");
         if (pch != NULL)
         {
            strcpy(filename, pch);
            drawer_request(filename);
         }
      }
      else if (!strcmp(pch, "next"))
      {
         drawer_notify_pixel_done();
      }
      else
      {
         printf("\r\n unknown command \r\n");
      }
   }
}
