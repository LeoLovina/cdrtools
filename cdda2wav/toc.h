/* @(#)toc.h	1.3 00/01/25 Copyright 1998,1999 Heiko Eissfeldt */
extern unsigned cdtracks;
extern int have_CD_extra;
extern int have_CD_text;

int GetTrack __PR(( unsigned long sector ));
long FirstTrack  __PR(( void ));
long LastTrack  __PR(( void ));
long FirstAudioTrack  __PR(( void ));
long LastAudioTrack  __PR(( void ));
long GetEndSector  __PR(( unsigned long p_track ));
long GetStartSector  __PR(( unsigned long p_track ));
long GetLastSectorOnCd __PR(( unsigned long p_track ));
int CheckTrackrange __PR(( unsigned long from, unsigned long upto ));
unsigned find_an_off_sector __PR((unsigned lSector, unsigned SectorBurstVal));
void DisplayToc  __PR(( void ));
unsigned FixupTOC __PR((unsigned no_tracks));
void calc_cddb_id __PR((void));
void calc_cdindex_id __PR((void));
void Read_MCN_ISRC __PR(( void ));
unsigned ScanIndices __PR(( unsigned trackval, unsigned indexval, int bulk ));
int   handle_cdtext __PR(( void ));
