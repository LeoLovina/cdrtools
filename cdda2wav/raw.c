/* @(#)raw.c	1.2 99/12/19 Copyright 1998,1999 Heiko Eissfeldt */
#ifndef lint
static char     sccsid[] =
"@(#)raw.c	1.2 99/12/19 Copyright 1998,1999 Heiko Eissfeldt";

#endif
#include "config.h"
#include "sndfile.h"

static int InitSound __PR(( void ));

static int InitSound ( )
{
  return 0;
}

static int ExitSound __PR(( void ));

static int ExitSound ( )
{
  return 0;
}

static unsigned long GetHdrSize __PR(( void ));

static unsigned long GetHdrSize( )
{
  return 0L;
}


struct soundfile rawsound =
{
  (int (*) __PR((int audio, long channels,
                 unsigned long myrate, long nBitsPerSample,
                 unsigned long expected_bytes))) InitSound,

  (int (*) __PR((int audio, unsigned long nBytesDone))) ExitSound,

  GetHdrSize,

  1		/* needs big endian samples */
};
