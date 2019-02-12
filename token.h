#define  LEX_BUFFER (16 * 1024)

typedef struct _FBUFFER {
  unsigned  curChar;
  unsigned  lastChar;
  unsigned  bytesRead;
  int       fhandle;
  char      *bufferPos;
  char      *buffer;     /* will alloc larger */
} FBUFFER, *PFBUFFER;

PFBUFFER createLex( int fhandle );

void closeLex( PFBUFFER context );

BOOL processFiles( PFBUFFER in,
                   PFBUFFER out );
