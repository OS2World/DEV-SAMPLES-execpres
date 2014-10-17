/*
 * RUNPROG.C
 * Copyright 1994 Bryan Walker DBA WalkerWerks.  All rights reserved.
 *
 * Examples of using DosStartSession and WinStartApp API calls.
 *
 * This sample is provided to help illustrate examples for the class
 * "Executing Programs in OS/2" for ColoradOS/2 1994.  The code is for
 * example purposes only and not as a teaching tool for general programming principals.
 * The author has no liability for any use of the code either private or commercial,
 * or for any damages arrising from such use.
 *
 */
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_PM
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>


char *CsvTok(char *string1) ;

/*
 * This function demonstrates using DosStartSession to execute
 * programs.
 */
ULONG StartAppDos(CHAR *app, CHAR *parm, CHAR *dir, CHAR *env, SHORT type, BOOL activate, CHAR *queue)
{
CHAR  homedir[CCHMAXPATH], buff1[CCHMAXPATH], finalenv[600], *ptrenv = finalenv, *tokptr ;
CHAR  ObjBuf[100], Drive[2], *buff2 ;
ULONG len, action, drvnum, drvmap ;
APIRET aprc ;
STARTDATA StartData ;
ULONG SessionID ;
PID   pid ;
BOOL  SetDrive = FALSE ;
HSWITCH hswitch ;

memset(&StartData, 0, sizeof(StartData)) ;
StartData.Length = sizeof(StartData);     /* Length of STARTDATA structure */
StartData.Related = SSF_RELATED_CHILD;    /* child so OS/2 will notify of termination.*/
StartData.FgBg = SSF_FGBG_BACK;           /* start in background */
StartData.TraceOpt = SSF_TRACEOPT_NONE;   /* no debug tracing enabled */
StartData.PgmTitle = NULL;                /* no default title. */
StartData.TermQ = queue ;                 /* passed notification queue. */
StartData.InheritOpt = SSF_INHERTOPT_PARENT; /* inherit our environment for working directory */
StartData.IconFile = 0;                   /* no specified icon file */
StartData.PgmHandle = 0;                  /* OS/2 1.3 holdover not used */
StartData.PgmControl = SSF_CONTROL_VISIBLE ; /* visible on desktop */
StartData.ObjectBuffer = ObjBuf;          /* error buffer */
StartData.ObjectBuffLen = 100;            /* size of the error buffer */
StartData.PgmName = app ;                 /* program name */
StartData.PgmInputs = parm ;              /* command line for program */

/*
 * Pass environment variables in CSV format.  If not null
 * parse out the individual variables and build the
 * environment string.  It is formatted with each environment=value
 * statement separated by a NULL character.  The final variable is
 * followed by two NULL characters.
 *
 * i.e. PATH=C:\\;'\0'LIB=C:\\;'\0''\0'
 *
 */
if(env != NULL && *env != 0)
   {
   memset( finalenv, 0, sizeof(finalenv)) ;
   StartData.Environment = finalenv ;
   if( ( tokptr = CsvTok( env )) != NULL) /* CsvTok is a CSV format tokenizer */
      {
      do
         {
         strcpy( ptrenv, tokptr ) ;
         ptrenv += strlen(tokptr) + 1;
         tokptr = CsvTok( NULL ) ;
         }while ( tokptr != NULL ) ;
      }

   }
/*
 * Convert the passed type into the appropriate PM type
 */
switch(type)
   {
   case 0:
      StartData.SessionType = PROG_PM ;
      break;
   case 1:
      StartData.SessionType = PROG_WINDOWABLEVIO ;
      break;
   case 2:
      StartData.SessionType = PROG_FULLSCREEN ;
      break;
   case 3:
      StartData.SessionType = PROG_WINDOWEDVDM ;
      break;
   case 4:
      StartData.SessionType = PROG_VDM ;
      break;
   case 5:
      /*
       * For windows applications using DosStartSession
       * you must call WINOS2.COM or WIN.COM and make the
       * desired program the first commandline parameter followed
       * by the programs commandline.
       *
       * First allocate a buffer for the new commandline.
       * then copy the program name and real commandline
       * into the buffer to create the new commandline.
       * Finally set the program name to win.com.  WINOS2.COM
       * is also on the computer as WIN.COM so use that instead
       * so the program will also work in OS/2 for Windows products.
       */
      len = strlen( app ) + strlen ( parm ) + 2 ;
      buff2 = malloc( len ) ;
      if(buff2 == NULL )
         return NULLHANDLE ;
      sprintf( buff2, "%s %s", app, parm ) ;
      strcpy( buff1, "win.com") ;
      StartData.PgmName = buff1 ;
      StartData.PgmInputs = buff2 ;
      StartData.SessionType = PROG_31_STDSEAMLESSCOMMON ;
      break;
   case 6:
      len = strlen( app ) + strlen ( parm ) + 2 ;
      buff2 = malloc( len ) ;
      if(buff2 == NULL )
         return NULLHANDLE ;
      sprintf( buff2, "%s %s", app, parm ) ;
      strcpy( buff1, "win.com") ;
      StartData.PgmName = buff1 ;
      StartData.PgmInputs = buff2 ;
      StartData.SessionType = PROG_DEFAULT ;
      break;
   }

/*
 * Get the current drive and directory
 * so that the program can switch to the
 * requested drive and directory before
 * starting the application then return
 * the the original one.
 */
action = CCHMAXPATH ;
DosQueryCurrentDisk(&drvnum, &drvmap) ;
DosQueryCurrentDir(drvnum, homedir, &action) ;

if(dir[1] == ':')  /*Move to the drive & directory requested by the user*/
   {
   SetDrive = TRUE ;
   Drive[0] = dir[0] ;
   Drive[1] = 0 ;
   DosSetDefaultDisk(toupper(Drive[0]) - 64) ;
   if(strlen(dir) > 2)
       DosSetCurrentDir(&dir[2]) ;
   }
else if (strlen(dir) > 0)
   DosSetCurrentDir(dir) ;

aprc = DosStartSession(&StartData, &SessionID, &pid) ;

/*
 * Get the switch handle using the process id.  Then
 * set it active to move it to the foreground.
 */
if( activate && (aprc == 0 || aprc == ERROR_SMG_START_IN_BACKGROUND) )
   {
   while( (hswitch = WinQuerySwitchHandle(NULLHANDLE, pid) ) == NULLHANDLE )
      DosSleep(300L) ;
   WinSwitchToProgram( hswitch ) ;
   }

/*
 * free the allocated buffer
 */
if( type == 5 || type == 6 )
   free(buff2) ;

/*
 * reset the active drive and directory
 */
DosSetDefaultDisk(drvnum) ;
DosSetCurrentDir(homedir) ;

/*
 * Post an error if the program didn't start.
 * ERROR_SMG_START_IN_BACKGROUND is only informational
 * that the parent was not in the foreground so the new
 * app could not be started in the foreground.  The
 * app is still started.
 */
if( aprc != 0 && aprc != ERROR_SMG_START_IN_BACKGROUND )
   {
   sprintf(buff1, "Program failed error 0x%04x", aprc) ;
   WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, buff1, "", 0L, MB_ICONHAND|MB_ENTER) ;
   return 0 ;
   }


return SessionID ;
}



ULONG StartAppWin(CHAR *app, CHAR *parm, CHAR *dir, CHAR *env, SHORT type, BOOL activate, HWND hwndNotify)
{
CHAR  buff1[CCHMAXPATH], finalenv[600], *ptrenv = finalenv, *tokptr ;
ULONG len, action ;
PROGDETAILS pDetails ;
HAPP  happ ;
HSWITCH hswitch ;
ERRORID err ;
USHORT  errid ;

/*
 * Point the structure elements to the appropriate
 * buffers.
 */
memset(&pDetails, 0, sizeof(PROGDETAILS)) ;
pDetails.Length = sizeof(pDetails); /* Length of pDetails structure */
pDetails.pszExecutable = app ;      /* program name */
pDetails.pszStartupDir = dir ;      /* default directory for new app. */
pDetails.pszParameters = parm ;     /* command line */
pDetails.progt.fbVisible = SHE_VISIBLE ;


/*
 * Pass environment variables in CSV format.  If not null
 * parse out the individual variables and build the
 * environment string.  It is formatted with each environment=value
 * statement separated by a NULL character.  The final variable is
 * followed by two NULL characters.
 *
 * i.e. PATH=C:\\;'\0'LIB=C:\\;'\0''\0'
 *
 */
if(env != NULL && *env != 0)
   {
   memset( finalenv, 0, sizeof(finalenv)) ;
   pDetails.pszEnvironment = finalenv ;
   if( ( tokptr = CsvTok( env )) != NULL) /* CsvTok is a CSV format tokenizer */
      {
      do
         {
         strcpy( ptrenv, tokptr ) ;
         ptrenv += strlen(tokptr) + 1;
         tokptr = CsvTok( NULL ) ;
         }while ( tokptr != NULL ) ;
      }

   }

/*
 * Convert type to appropriate PM type.
 */
switch(type)
   {
   case 0:
      pDetails.progt.progc = PROG_PM ;
      break;
   case 1:
      pDetails.progt.progc = PROG_WINDOWABLEVIO ;
      break;
   case 2:
      pDetails.progt.progc = PROG_FULLSCREEN ;
      break;
   case 3:
      pDetails.progt.progc = PROG_WINDOWEDVDM ;
      break;
   case 4:
      pDetails.progt.progc = PROG_VDM ;
      break;
   /*
    * NOTE: Windows sessions only work correctly
    * if you use SAF_INSTALLEDCMDLINE.  If you
    * do not specify this flag you must start
    * WINOS2.COM or WIN.COM and pass the program
    * name for fullscreen or seperate seamless
    * sessions.  For common seamless sessions without
    * this flag you must start WINOS2.COM or WIN.COM
    * for the first windows session to be activated
    * then start just the desired program for any
    * additional sessions while a windows session is
    * active.
    */
   case 5:
      pDetails.progt.progc = PROG_31_ENHSEAMLESSCOMMON ;
      break;
   case 6:
      pDetails.progt.progc = PROG_31_ENH ;
      break;
   }

/*
 * foreground or background
 */
if( activate )
   pDetails.swpInitial.hwndInsertBehind = HWND_TOP ;
else
   pDetails.swpInitial.hwndInsertBehind = HWND_BOTTOM ;

/*
 * Start the application.
 */
happ = WinStartApp( hwndNotify, &pDetails, NULL, NULL, SAF_INSTALLEDCMDLINE|SAF_STARTCHILDAPP) ;

/*
 * Get error and post message.
 */
if( happ == NULLHANDLE )
   {
   err = WinGetLastError( WinQueryAnchorBlock( hwndNotify ) ) ;
   errid = ERRORIDERROR(err) ;
   sprintf(buff1, "Program failed error %x", errid) ;
   WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, buff1, "", 0L, MB_ICONHAND|MB_ENTER) ;
   }

return happ ;
}
