/*
 * EXECUTIL.C
 * Copyright 1994 Bryan Walker DBA WalkerWerks.  All rights reserved.
 *
 * Support functions for EXECPRES.EXE
 *
 * This sample is provided to help illustrate examples for the class
 * "Executing Programs in OS/2" for ColoradOS/2 1994.  The code is for
 * example purposes only and not as a teaching tool for general programming principals.
 * The author has no liability for any use of the code either private or commercial,
 * or for any damages arrising from such use.
 *
 */
#define TRUE  1
#define FALSE 0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * A replacement for strtok.  It was used
 * because early versions of Cset++ didn't
 * handle two tokens side by side correctly.
 * (i.e. ,, )
 */
char *StrTok(char *string1, char *string2)
{
static char *curptr ;
char *rtnptr ;
char *token ;

if(string1 != NULL)
   curptr = string1 ;

if(curptr == NULL || *curptr == 0)
   return NULL ;

token = rtnptr = curptr ;
while(*token != 0)
   {
   if(strchr(string2, *token) != NULL)
      {
      *token = 0 ;
      curptr = token + 1 ;
      break;
      }
   else
      curptr = NULL ;
   token++ ;
   } ;
return rtnptr ;
}


/*
 * Parses a PSZ in the CSV format where
 * each field is separated by a comma,
 * fields with imbedded commas are
 * surrounded by double quotes, and
 * literal double quotes are identified
 * as by two double quotes side by side
 * (i.e. "").  Works exactly like strtok
 * except the string2 identifier is not
 * necessary because the only acceptable
 * token is the comma.  So to continue
 * parsing the same string send NULL
 * as string1.  Returns NULL if at
 * end of PSZ.
 */
char *CsvTok(char *string1)
{
static char *curcsvptr ;
static int  LastWasQuote ;
char *rtnptr ;
char *token, *tmp ;
char *string2 = "," ;
long len ;

if(string1 != NULL)
   {
   curcsvptr = string1 ;
   LastWasQuote = FALSE ;
   }

if(curcsvptr == NULL || *curcsvptr == 0)
   return NULL ;
if( *curcsvptr == '"' && curcsvptr[1] != '"')
   {
   LastWasQuote = TRUE ;
   curcsvptr ++ ;
   }
token = rtnptr = curcsvptr ;
if(LastWasQuote)
   {
   while(*token != 0)
      {
      if(*token == '"' && token[1] != '"')
         {
         *token = 0 ;
         curcsvptr = token + 2 ;
         break;
         }
      else if (*token == '"' && token[1] == '"')
         token++ ;
      else
         curcsvptr = NULL ;
      token++ ;
      } ;
   LastWasQuote = FALSE ;
   }
else
   {
   while(*token != 0)
      {
      if(strchr(string2, *token) != NULL)
         {
         *token = 0 ;
         curcsvptr = token + 1 ;
         break;
         }
      else
         curcsvptr = NULL ;
      token++ ;
      } ;
   }

/*
 * turn any double quotes into single quotes
 */
token = rtnptr;
while( (tmp = strstr(token, "\"\"")) != NULL)
   {
   token = tmp+1 ;
   len = strlen(token) ;
   memmove(tmp, token, len) ;
   tmp[len] = 0 ;
   token = tmp ;
   } ;

return rtnptr ;
}
