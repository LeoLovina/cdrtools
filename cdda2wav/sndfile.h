/* @(#)sndfile.h	1.2 99/12/19 Copyright 1998,1999 Heiko Eissfeldt */
/* generic soundfile structure */

struct soundfile {
  int (* InitSound) __PR(( int audio, long channels, unsigned long rate, long nBitsPerSample, unsigned long expected_bytes));
  int (* ExitSound) __PR(( int audio, unsigned long nBytesDone ));
  unsigned long (* GetHdrSize) __PR(( void ));

  int need_big_endian;
};
