#define  INCL_DOSFILEMGR
#include <os2.h>

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#include "token.h"

#define TAB        9
#define LF        10
#define FF        12
#define CR        13
#define EOFILE    26
#define D_QUOTE   34
#define SPACE     ' '
#define OPCHAR    '='
#define IDCHAR    'A'

#define STATE_ENTRY static BOOL NEAR PASCAL
#define LAST_CHAR ((int)'~'+1)

  /* cheat and not pass these, this ain't threaded */
static FBUFFER IN;
static FBUFFER OUT;


/****************************************************************************
* Create a lex/file buffer.
*****************************************************************************/
PFBUFFER createLex( int fhandle )
{
PFBUFFER lex = malloc( sizeof(FBUFFER)+LEX_BUFFER );

  if ( lex ) {
    lex->fhandle   = fhandle;
    lex->bytesRead = 0;  
    lex->curChar   = CR;
    lex->lastChar  = CR;
    lex->buffer    = (void *)(lex + 1);
    lex->bufferPos = lex->buffer;
  }
  return( lex );

} /* createLex */


/****************************************************************************
* Close a lex/file buffer.
*****************************************************************************/
void closeLex( PFBUFFER lex )
{
   DosClose( lex->fhandle );
   free( lex );

} /* closeLex */


/****************************************************************************
*  Get the next character from the lexical analyzer IN.
*****************************************************************************/
static unsigned NEAR nextChar( void )
{
unsigned data;

  if ( IN.bytesRead == 0 ) {
    unsigned err = DosRead( IN.fhandle, IN.buffer,
                            LEX_BUFFER, &IN.bytesRead );
    if ( err != 0 ) {
      printf( "Dos file error %u\n", err );
      IN.bytesRead = 0;
    }
    if ( IN.bytesRead != LEX_BUFFER )
      IN.buffer[IN.bytesRead] = EOFILE;   /* set end of file */
    IN.bufferPos = IN.buffer;
  }
  data = (unsigned)(*(IN.bufferPos));
  IN.bufferPos++;
  IN.bytesRead--;

  return( data );

} /* nextChar */


/****************************************************************************
*  Put the next character into the file buffer.
*****************************************************************************/
static void NEAR putChar( unsigned data )
{

  if ( data == EOFILE || OUT.bytesRead == LEX_BUFFER ) {
    unsigned bytes;
    DosWrite( OUT.fhandle, OUT.buffer, OUT.bytesRead, &bytes );
    OUT.bytesRead = 0;
  }

  OUT.buffer[OUT.bytesRead] = (char)data;
  OUT.bytesRead++;

} /* putChar */


/****************************************************************************
*  Remove white space.
*****************************************************************************/
STATE_ENTRY whiteSpace( void )
{
unsigned chr;

           /* if an id char, put in a separator */
  if ( IN.lastChar == IDCHAR ) {
    putChar( SPACE );
    IN.lastChar = SPACE;
  }
  do {
    chr = nextChar( );
  }while ( chr == SPACE || chr == TAB );

  IN.curChar = chr;
  return( FALSE );

}  /* whiteSpace */


/****************************************************************************
* Deal with CR/LF remove multiple instances of them.
*****************************************************************************/
STATE_ENTRY cariageReturn( void )
{
unsigned chr;

  if ( IN.lastChar != CR ) {
    putChar( CR );
    putChar( LF );
    IN.lastChar = CR;
  }
  do {
    chr = nextChar( );
  }while ( chr == CR || chr == LF );
  IN.curChar = chr;
  return( FALSE );

}  /* cariageReturn */


/****************************************************************************
*  Any operator character.
*****************************************************************************/
STATE_ENTRY opChar( void )
{

  putChar(IN.curChar );
  IN.lastChar = OPCHAR;  /* an operator acts as a separator */
  IN.curChar = nextChar( );
  return( FALSE );

}  /* opChar */


/****************************************************************************
*  Any id/numeric character.
*****************************************************************************/
STATE_ENTRY idChar( void )
{

  putChar(IN.curChar );
  IN.lastChar = IDCHAR;  /* must separate with white space */
  IN.curChar = nextChar( );
  return( FALSE );

}  /* idChar */


/****************************************************************************
*  Double Quote char, a string follows.  Allow C style escape sequences in
* the string.
*****************************************************************************/
STATE_ENTRY doubleQuote( void )
{
unsigned chr;

  putChar(D_QUOTE );
  chr = nextChar( );
  for (;;) {
    while ( chr != D_QUOTE && chr != '\\' && chr != EOFILE ) {
      putChar(chr);
      chr = nextChar();
    }
    if ( chr == EOFILE )
      return( TRUE );
    else if ( chr == '\\' ) {   /* accept whatever is next */
      putChar(chr );
      chr = nextChar();
    }else
      break;
  }
  putChar(D_QUOTE );
  IN.lastChar = SPACE;  /* acts as a separator */
  IN.curChar = nextChar( );
  return( FALSE );

}  /* doubleQuote */


/****************************************************************************
*  Divide character
*****************************************************************************/
STATE_ENTRY divChar( void )
{
unsigned chr;
  
  chr = nextChar( );
  if ( chr == '/' ) {		    /* beginning of C++ comment */
    while ( chr != EOFILE && chr != CR )
      chr = nextChar();
    if ( chr == EOFILE )
      return( TRUE );
  }else if ( chr != '*' ) {    /* plane old divide char */
    IN.lastChar = SPACE;
    putChar('/');
  }else {                      /* beginning of comment */
    chr = nextChar();
    for (;;) {
      while ( chr != EOFILE && chr != '*')
        chr = nextChar();
      if ( chr == EOFILE )
        return( TRUE );
      else {
        chr = nextChar();
        if ( chr == '/' )
         break;
      }
    }
    chr = nextChar( );
  }

  IN.curChar = chr;
  return( FALSE );

}  /* divChar */


/****************************************************************************
*  A char that should not occur in state 1.
*****************************************************************************/
STATE_ENTRY bogusChar( void )
{

  printf( "unknown char '%c' in stream\n", IN.curChar );
  IN.lastChar = IDCHAR;  /* must separate with white space */
  IN.curChar = nextChar( );
  return( TRUE );

}  /* bogusChar */

/****************************************************************************
* End of file char.
*****************************************************************************/
STATE_ENTRY eofChar( void )
{

  IN.curChar = nextChar( );
  return( TRUE );

}  /* eofChar */


/****************************************************************************
* Read and dump the file.
*****************************************************************************/
BOOL processFiles( PFBUFFER in,
                   PFBUFFER out )
{

static BOOL (NEAR PASCAL *state[LAST_CHAR])(void) = {
  bogusChar,  /* 0 */
  bogusChar,  /* 1 */
  bogusChar,  /* 2 */
  bogusChar,  /* 3 */
  bogusChar,  /* 4 */
  bogusChar,  /* 5 */
  bogusChar,  /* 6 */
  bogusChar,  /* 7 */
  bogusChar,  /* 8 */
  whiteSpace, /* Tab */
  whiteSpace, /* Line Feed, if in pair, sucked up by CR */
  bogusChar,  /* 11 */
  bogusChar,  /* 12 */
  cariageReturn,  /* 13 */
  bogusChar,  /* 14 */
  bogusChar,  /* 15 */
  bogusChar,  /* 16 */
  bogusChar,  /* 17 */
  bogusChar,  /* 18 */
  bogusChar,  /* 19 */
  bogusChar,  /* 20 */
  bogusChar,  /* 21 */
  bogusChar,  /* 22 */
  bogusChar,  /* 23 */
  bogusChar,  /* 24 */
  bogusChar,  /* 25 */
  eofChar,    /* Eof */
  bogusChar,  /* 27 */
  bogusChar,  /* 28 */
  bogusChar,  /* 29 */
  bogusChar,  /* 30 */
  bogusChar,  /* 31 */
  whiteSpace,  /* Blank */
  opChar,      /* ! */
  doubleQuote, /* " */
  opChar,      /* # */
  bogusChar,   /* $ */
  opChar,      /* % */
  opChar,      /* & */
  opChar,      /* ' */
  opChar,      /* ( */
  opChar,      /* ) */
  opChar,      /* * */
  opChar,      /* + */
  opChar,      /* , */
  opChar,      /* - */
  idChar,      /* . */
  divChar,     /* / */
  idChar,      /* 0 */
  idChar,      /* 1 */
  idChar,      /* 2 */
  idChar,      /* 3 */
  idChar,      /* 4 */
  idChar,      /* 5 */
  idChar,      /* 6 */
  idChar,      /* 7 */
  idChar,      /* 8 */
  idChar,      /* 9 */
  opChar,     /* : */
  opChar,     /* ; */
  opChar,     /* < */
  opChar,     /* = */
  opChar,     /* > */
  opChar,     /* ? */
  bogusChar,  /* @ */
  idChar,  /* A */
  idChar,  /* B */
  idChar,  /* C */
  idChar,  /* D */
  idChar,  /* E */
  idChar,  /* F */
  idChar,  /* G */
  idChar,  /* H */
  idChar,  /* I */
  idChar,  /* J */
  idChar,  /* K */
  idChar,  /* L */
  idChar,  /* M */
  idChar,  /* N */
  idChar,  /* O */
  idChar,  /* P */
  idChar,  /* Q */
  idChar,  /* R */
  idChar,  /* S */
  idChar,  /* T */
  idChar,  /* U */
  idChar,  /* V */
  idChar,  /* W */
  idChar,  /* X */
  idChar,  /* Y */
  idChar,  /* Z */
  opChar,  /* [ */
  opChar,  /* \ */
  opChar,  /* ] */
  opChar,  /* ^ */
  idChar,  /* _ */
  bogusChar,  /* ` */
  idChar,  /* a */
  idChar,  /* b */
  idChar,  /* c */
  idChar,  /* d */
  idChar,  /* e */
  idChar,  /* f */
  idChar,  /* g */
  idChar,  /* h */
  idChar,  /* i */
  idChar,  /* j */
  idChar,  /* k */
  idChar,  /* l */
  idChar,  /* m */
  idChar,  /* n */
  idChar,  /* o */
  idChar,  /* p */
  idChar,  /* q */
  idChar,  /* r */
  idChar,  /* s */
  idChar,  /* t */
  idChar,  /* u */
  idChar,  /* v */
  idChar,  /* w */
  idChar,  /* x */
  idChar,  /* y */
  idChar,  /* z */
  opChar,  /* { */
  opChar,  /* | */
  opChar,  /* } */
  opChar   /* ~ */
};

  memcpy( &IN, in, sizeof(FBUFFER) );
  memcpy( &OUT, out, sizeof(FBUFFER) );

  while ( IN.curChar < LAST_CHAR && (state[IN.curChar])() == FALSE ) ;

  if ( IN.curChar >= LAST_CHAR )
    bogusChar();

    /* flush the buffer */
  putChar( EOFILE );

  return( IN.curChar == EOFILE );

}  /* processFile */

