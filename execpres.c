/*
 * EXECPRES.C
 * Copyright 1994 Bryan Walker DBA WalkerWerks.  All rights reserved.
 *
 * User interface for
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
#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include "execpres.h"
#define  NOTIFYQUEUE "\\QUEUES\\EXECPRES.QUE"

extern ULONG StartAppDos(CHAR *app, CHAR *parm, CHAR *dir, CHAR *env, SHORT type, BOOL activate, CHAR *queue) ;
extern ULONG StartAppWin(CHAR *app, CHAR *parm, CHAR *dir, CHAR *env, SHORT type, BOOL activate, HWND hwndNotify) ;
MRESULT EXPENTRY ExecProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) ;
VOID WatchQueue(PVOID info) ;

struct _thddata
   {
   HQUEUE hque ;
   HWND   hwnd ;
   } ;

/*
 * main initializes PM creates a message queue and
 * processes the only dialog for this application.
 */
main(int argc, char *argv[], char *envp[])
{
HAB hab ;
HMQ hmq ;
HWND hwndDlg ;
QMSG qmsg ;
ULONG pid, tid ;
HSWITCH hsw ;
SWCNTRL swctl = { NULLHANDLE , NULLHANDLE , NULLHANDLE , 0 , 0 ,
                  SWL_VISIBLE , SWL_JUMPABLE , "ExecPres" , 0 } ;

hab = WinInitialize(0) ;
hmq = WinCreateMsgQueue(hab, 20) ;


hwndDlg = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, ExecProc, NULLHANDLE, IDD_TOP, NULL ) ;
WinQueryWindowProcess (hwndDlg, &pid, &tid);
swctl.hwnd = hwndDlg;
swctl.idProcess = pid;
hsw = WinAddSwitchEntry (&swctl);

WinProcessDlg(hwndDlg) ;

WinRemoveSwitchEntry ( hsw ) ;
WinDestroyMsgQueue(hmq) ;
WinTerminate(hab) ;
return 0 ;
}

/*
 * The dialog procedure for the panel where users enter the program
 * information to run.  This is the only dialog in the program.
 */
MRESULT EXPENTRY ExecProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
SHORT x ;
HAPP  happ ;
LONG  error ;

/*
 * Instance data that needs to be maintained across
 * PM messages.  Since PM destroys the stack between
 * calls these either must be allocated or declared
 * as static.  By allocating them and setting the
 * User window word to the value of the pointer the
 * data for multiple instances of a window is stored with
 * the window structure through the pointer.
 */
struct StructVars
   {
   CHAR  App[CCHMAXPATH] ;
   CHAR  Parm[CCHMAXPATH] ;
   CHAR  Path[CCHMAXPATH] ;
   CHAR  Env[501] ;
   SHORT ApType ;
   SHORT ForeBack ;
   SHORT LaunchType ;
   struct _thddata thddata ;
   } *sv ;

sv = (struct StructVars *) WinQueryWindowULong(hwnd, QWL_USER) ;
switch(msg)
   {
   /*
    * WinStartApp() causes PM to post this message to the notify window
    * for child apps that terminate.  MP1 contains the HAPP and MP2 contains
    * the result code.  In addition the thread that monitors the queue for DosStartSession
    * termination messages posts this message with the session id in MP1.
    */
   case WM_APPTERMINATENOTIFY:
      happ = LONGFROMMP(mp1) ;
      error = LONGFROMMP(mp2) ;
      sprintf(sv->App, "Application %lu terminated with code %ld", happ, error) ;
      WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, sv->App, "ExecPres", 0L, MB_NOICON|MB_ENTER) ;
      return 0;

   case WM_INITDLG:
      /*
       * Allocate the instance data.
       */
      if( (sv = malloc( sizeof( struct StructVars ) ) ) == NULL )
         {
         WinDismissDlg(hwnd, FALSE ) ;
         return 0 ;
         }
      memset( sv, 0, sizeof(struct StructVars) ) ;
      /*
       * Create the message queue for DosStartSession and start a thread
       * to monitor it.
       */
      DosCreateQueue( &sv->thddata.hque, QUE_FIFO|QUE_CONVERT_ADDRESS, NOTIFYQUEUE ) ;
      sv->thddata.hwnd = hwnd ;
      _beginthread( WatchQueue, NULL, 8000, &sv->thddata ) ;

      /*
       * Setup some defaults for the dialog.
       */
      WinSetWindowULong(hwnd, QWL_USER, (ULONG) sv ) ;
      WinSendDlgItemMsg(hwnd, IDE_PROGRAM,   EM_SETTEXTLIMIT, MPFROMSHORT(256), 0L) ;
      WinSendDlgItemMsg(hwnd, IDE_PARMS,     EM_SETTEXTLIMIT, MPFROMSHORT(256), 0L) ;
      WinSendDlgItemMsg(hwnd, IDE_DIRECTORY, EM_SETTEXTLIMIT, MPFROMSHORT(256), 0L) ;
      WinSendDlgItemMsg(hwnd, IDE_ENV      , EM_SETTEXTLIMIT, MPFROMSHORT(500), 0L) ;
      WinSendDlgItemMsg(hwnd, IDR_NO, BM_SETCHECK, MPFROMSHORT(TRUE), 0L) ;
      return 0 ;

   case WM_COMMAND:
      switch(COMMANDMSG(&msg)->cmd)
         {
         case DID_OK:
            /*
             * Extract the entered data from the dialog and
             * call the appropriate launching function.
             */
            WinQueryDlgItemText(hwnd, IDE_PROGRAM,   sizeof(sv->App),  sv->App) ;
            WinQueryDlgItemText(hwnd, IDE_PARMS,     sizeof(sv->Parm), sv->Parm) ;
            WinQueryDlgItemText(hwnd, IDE_DIRECTORY, sizeof(sv->Path), sv->Path) ;
            WinQueryDlgItemText(hwnd, IDE_ENV, sizeof(sv->Env), sv->Env) ;
            sv->ApType = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, IDR_PM,
                                       BM_QUERYCHECKINDEX, 0L, 0L)) ;
            if(sv->ApType == -1)
               {
               WinAlarm(HWND_DESKTOP, WA_ERROR) ;
               return 0 ;
               }
            sv->ForeBack = SHORT1FROMMR(WinSendDlgItemMsg( hwnd, IDR_NO,
                             BM_QUERYCHECKINDEX, 0L, 0L)) ;
            sv->LaunchType = SHORT1FROMMR(WinSendDlgItemMsg( hwnd, IDR_DOS,
                             BM_QUERYCHECKINDEX, 0L, 0L)) ;
            if( sv->LaunchType )
               StartAppWin(sv->App, sv->Parm, sv->Path, sv->Env, sv->ApType, sv->ForeBack, hwnd ) ;
            else
               StartAppDos(sv->App, sv->Parm, sv->Path, sv->Env, sv->ApType, sv->ForeBack, NOTIFYQUEUE ) ;
            return 0 ;

         case DID_CANCEL:
            /*
             * Reset the dialog to its default blank state for
             * new information.
             */
            WinSetDlgItemText(hwnd, IDE_PROGRAM, "") ;
            WinSetDlgItemText(hwnd, IDE_PARMS, "") ;
            WinSetDlgItemText(hwnd, IDE_DIRECTORY, "") ;
            WinSetDlgItemText(hwnd, IDE_ENV, "") ;
            for(x = 0 ; x < 6 ; x++)
               WinSendDlgItemMsg(hwnd, IDR_PM+x, BM_SETCHECK, MPFROMSHORT(FALSE), 0L) ;
            WinSendDlgItemMsg(hwnd, IDR_NO,      BM_SETCHECK, MPFROMSHORT(TRUE), 0L) ;
            WinSendDlgItemMsg(hwnd, IDR_DOS,    BM_SETCHECK, MPFROMSHORT(TRUE), 0L) ;
            WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, IDE_PROGRAM) ) ;
            return 0 ;

         case IDB_EXIT:
            /*
             * Notify the queue monitoring thread to terminate.
             * then exit the dialog effectively exiting the program.
             */
            DosWriteQueue( sv->thddata.hque, 400, 0L, NULL, 0 ) ;
            WinDismissDlg(hwnd, TRUE) ;
            return 0 ;
         }
      break ;

   case WM_DESTROY:
      /*
       * Clean up our resources.
       */
      DosCloseQueue( sv->thddata.hque ) ;
      free(sv) ;
      return 0 ;
   }
return WinDefDlgProc(hwnd, msg, mp1, mp2) ;
}


/*
 * This thread watches the DosQueue that is created for OS/2
 * to notify the program through.  It then posts a WM_APPTERMINATENOTIFY
 * message to the dialog just like WinStartApp().  The difference is that
 * instead of a HAPP the session id is placed in mp1.
 *
 * A queue application data value of 400 is used to signal the thread
 * to exit.  When this value is written to the queue the thread will
 * not loop to start watching the queue again.
 */
VOID WatchQueue(PVOID info)
{
HQUEUE que = ((struct _thddata * )info)->hque;
HWND hwndNotify = ((struct _thddata * )info)->hwnd;
REQUESTDATA rq ;
ULONG len ;
struct  ProgDat
   {
   USHORT sid ;
   SHORT  rc ;
   } *pDat ;
BYTE  priority ;

while (DosReadQueue( que, &rq, &len, (PVOID) &pDat, 0, DCWW_WAIT, &priority, 0 ) == 0)
   {
   if( rq.ulData == 0 && pDat != NULL )
      WinPostMsg( hwndNotify, WM_APPTERMINATENOTIFY,
                  MPFROMLONG( (ULONG)pDat->sid ), MPFROMLONG( (LONG) pDat->rc ) ) ;
   if( pDat != NULL )
      DosFreeMem(pDat) ;
   if( rq.ulData == 400 )
      return ;
   }
return ;
}
