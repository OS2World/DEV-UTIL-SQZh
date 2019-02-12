/* Squeezes header files */
/* Written Geo 18-Dec-90 */
/* OS/2 version 1.0 Nick Bethmann, 22 Apr 92 */
/* OS/2 version 1.01 Nick Bethmann, 6 Jun 92, updated command line */
/* FAPI version 1.02 Nick Bethmann, 6 Jan 93, char immediately after '/' was dupped! */

#define INCL_DOSFILEMGR
#include <os2.h>

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>
#include <string.h>

#include "token.h"

char basedir[CCHMAXPATH];
char targetdir[CCHMAXPATH];
char curdir[CCHMAXPATH];


BOOL NEAR process( char *infile,
                   char *outfile)
{
int         fi,fo;
unsigned    bytes;
PFBUFFER    ib,ob;
FILESTATUS  info1;
FILESTATUS  info2;

  fi = sopen( infile, O_BINARY | O_RDONLY, SH_DENYWR );
  if ( fi == -1) {
    printf( "ERROR: failed to OPEN %s\n", infile);
    return( FALSE );
  }

  fo = sopen( outfile, O_BINARY | O_RDWR, SH_DENYWR );
  if (fo == -1) {
    fo = sopen( outfile, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, SH_DENYWR,
                S_IREAD | S_IWRITE);
    if (fo == -1) {
      DosClose(fi);
      printf( "ERROR: failed to CREATE %s\n", outfile);
      return( FALSE );
    }
  }

        /* don't squeeze if file already squeezed */
  DosQFileInfo((HFILE)fi, 1, (PBYTE)&info1, sizeof(FILESTATUS));
  DosQFileInfo((HFILE)fo, 1, (PBYTE)&info2, sizeof(FILESTATUS));

        /* NOTE: only hpfs stores creation, so... don't check */
  if ( memcmp( &info1.fdateLastWrite,
               &info2.fdateLastWrite, sizeof(FDATE) + sizeof(FTIME)) != 0 ||
       info1.cbFile == info2.cbFile ) {
    DosWrite( 1, ".", 1, &bytes );
    DosNewSize( (HFILE)fo, (ULONG)0 );
    ib = createLex( fi );
    ob = createLex( fo );

    processFiles( ib, ob );

       /* stamp date time of file */
    DosBufReset( (HFILE)fo );
    DosQFileInfo((HFILE)fo, 1, (PBYTE)&info2, sizeof(FILESTATUS));
    info1.cbFile      = info2.cbFile;
    info1.cbFileAlloc = info2.cbFileAlloc;
    DosSetFileInfo((HFILE)fo, 1, (PBYTE)&info1, sizeof(FILESTATUS));

    closeLex(ob);
    closeLex(ib);
    return( TRUE );
  }else {
    DosClose(fi);
    DosClose(fo);
    DosWrite( 1, "-", 1, &bytes );
    return( FALSE );
  }

}  /* process */


void addSlash( char *path )
{
unsigned len = strlen(path);

  if ( len > 0 && path[len-1] != '\\' ) {
    path[len] = '\\';
    path[len+1] = 0;
  }

}  /* addSlash */


void removeWild( char *to,
                 char *from )
{
unsigned len = strlen(from);

  while ( from[len] != '*' && from[len] != '?' ) len--;
  while ( len > 0 && from[len] != '\\' && from[len] != ':' ) len--;

  if ( len == 0 )
    to[0] = 0;
  else {
    memcpy( to, from, len+1 );
    to[len+1] = 0;
  }

}  /* removeWild */


void dirs(char *dirName)
{
 unsigned     searchCount = 1;
 HDIR         hDir = HDIR_CREATE;
 FILEFINDBUF  fb;
 char         wildpath[CCHMAXPATH];
 char         basepath[CCHMAXPATH];
 unsigned     count = 0;

  if ( strchr(basedir, '*') != NULL || strchr(basedir, '?') != NULL ) {
    strcpy(wildpath, basedir);
    removeWild( basepath, basedir );
    strcat( basepath, curdir ); 
  }else {
    addSlash( basedir );
    strcpy(basepath, basedir);
    strcpy(wildpath, basedir);
    strcat( wildpath, dirName );
    strcat( wildpath, "\\*.h");
  }

 if ( DosFindFirst( wildpath, &hDir, FILE_NORMAL,
                    &fb, sizeof(fb), &searchCount, 0L) == 0 )
 do {
    if (fb.achName[0] == '.')
      continue;
    if (fb.attrFile & FILE_DIRECTORY)
    {
     char *ptr = curdir;

      while (*ptr)
        ptr++;

      strcat(ptr, "\\");
      strcat(ptr, fb.achName);
      {
      static char dirname[CCHMAXPATH];

        sprintf(dirname, "%s%s", targetdir, curdir);
        mkdir(dirname);
      }
      
      dirs(fb.achName);
      *ptr = '\0';
    }
    else
    {
      static char infile[CCHMAXPATH], outfile[CCHMAXPATH];

      strcpy( infile, basepath );
      strcat( infile, fb.achName );     

      strcpy( outfile, targetdir );
      strcat( outfile, curdir );
      strcat( outfile, fb.achName );

      if ( process(infile, outfile) )
        count++;
    }
  }while ( DosFindNext(hDir, &fb, sizeof(fb), &searchCount) == 0 );

 DosFindClose(hDir);
 printf( "%u\n", count );

} /* dirs */


int _cdecl main( int argc,
                 char *argv[] )
{
int   i;
char *ptr;

  if (argc < 3 ) {
    printf("SqzH, v1.02\nInsufficient args.\nusage : sqzh <target dir> <source dir>... \n");
    return(2);
  }

  strcpy(targetdir, argv[1]);
  mkdir(targetdir);
  addSlash( targetdir );
     /* convert all forward slashes to back slashes */
  for (ptr = targetdir; *ptr; ptr++)
    if (*ptr == '/')
      *ptr = '\\';

  for ( i=2; i<argc; i++ ) {
    strcpy(basedir, argv[i]);
    for (ptr = basedir; *ptr; ptr++)
      if (*ptr == '/')
        *ptr = '\\';

    curdir[0] = '\0';
    dirs(curdir);
  }

}  /* main */





