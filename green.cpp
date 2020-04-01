
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long long qword;

template <class T> void bzero( T (&p) ) { memset( p, 0, sizeof(T) ); }

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

FILE* Target;

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;
static const int mSCALE   = SCALE-1;

static const int S_wrA = (1+0) * (1);
static const int S_wrB = (0+0) * (1);
static const int S_mw = (1040+0) * (1);

#include "sh_SSE1.inc"

#include "sh_v2f.inc"

struct RangeCoder : Rangecoder {
  using Rangecoder::rc_BProcess;
  template< class SSE >
  uint rc_BProcess( SSE& S, qword freq, qword total, uint bit ) { 
    freq *= SCALE;
    freq /= total;
    if( freq<1 ) freq=1;
    if( freq>SCALE-1 ) freq=SCALE-1;
    uint p = SCALE-freq;
    SSEi_updstr X;
    p = S.SSE_Pred( p, X );
    bit = this->rc_BProcess( p, bit );
    S.SSE_Update( bit, S_wrA*128+S_wrB, X );
    return bit;
  }
#if 0
  uint rc_BProcess( qword freq, qword total, uint bit ) { 
    freq *= SCALE;
    freq /= total;
    if( freq<1 ) freq=1;
    if( freq>SCALE-1 ) freq=SCALE-1;
    return rc_BProcess( SCALE-freq, bit );
  }
#endif
};

#include "timer.inc"

enum {
  CNUM = 256,
  MAXORDER = 16,
  DEPTH = 1<<13,
  COEF  = 1<<13
};

void printstats( uint i, uint f_len ) {
#if 1
  static char s[256];
  sprintf( s, "inp=%i/%i out=%i", i, f_len, ftell(Target) ); fflush(stdout);
  printf( "%-78s\r", s );
#endif
}


static qword freq[CNUM];
static uint cidx[CNUM];


//#include "sh_mapping.inc"
//#include "MOD/sh_model_h.inc"
//#include "config_opt.inc"

#include "config.inc"

static SSEi<13> SSE[CNUM];

#if 0 
int __cdecl compare( const void* src, const void* dst ) {
  uint A = *(const uint*)src;
  uint B = *(const uint*)dst;
  return (freq[A]<freq[B]) ? (freq[A]==freq[B]) ? 0 : 1 : -1;
}

#else

#include "sh_qsort.inc"

struct TEST {
  // Returns neg if 1<2, 0 if 1=2, pos if 1>2.
  static int  c( uint* __restrict Z, int x, int y ) {
//    if( x==y ) return 0;
    uint A = Z[x];
    uint B = Z[y];
    return (freq[A]<freq[B]) ? (freq[A]==freq[B]) ? 0 : 1 : -1;
  }
  static void s( uint* __restrict Z, int x, int y ) {
    SWAP( Z[x], Z[y] );
  }
};

#endif



int main( int argc, char **argv ) {

  uint DECODE = argc<2 ? 0 : (argv[1][0]=='d');

  FILE* inp = fopen( (argc<3 ? (DECODE?"book1.ari":"book1")     : argv[2]), "rb" ); if( inp==0 ) return 1;
  FILE* out = fopen( (argc<4 ? (DECODE?"book1.unp":"book1.ari") : argv[3]), "wb" ); if( out==0 ) return 1;

  uint inplen = DECODE ? fread( &inplen, 1,4, inp ),inplen : flen(inp);
  Target = DECODE ? inp : out;

  byte* pad_inpbuf = new byte[MAXORDER+inplen]; if( pad_inpbuf==0 ) return 1; // unpacked data buffer
  uint* l2     = new uint[inplen]; if( l2==0 ) return 1;     // o3+ link buffer
  byte* inpbuf = &pad_inpbuf[MAXORDER];

  printf( DECODE?"Decoding.\n":"Encoding.\n" );

  if( !DECODE ) fwrite( &inplen, 1,4, out );
  fread( inpbuf,  1,3, inp ); // first 3 symbols
  fwrite( inpbuf, 1,3, out );

  RangeCoder rc;
  DECODE ? rc.StartDecode(Target) : rc.StartEncode(Target);

  static uint low[CNUM+1];
  static uint o0[CNUM]; bzero( o0 );
  static uint o1[CNUM][CNUM]; bzero( o1 ); // 256k
  static uint o2[CNUM][CNUM][CNUM]; bzero( o2 ); // 64M
  static uint p2[CNUM][CNUM][CNUM]; bzero( p2 ); // +64M, last occurence pointers
  static uint fn[MAXORDER+1-3][CNUM]; // dynamic tables by orders 3..MAXORDER
  static uint* orders[MAXORDER+1];

  StartTimer();

  uint c,i,j,k,n,total,flag;

  for( j=0; j<MAXORDER; j++ ) pad_inpbuf[j]=inpbuf[0];

  for( j=0; j<CNUM; j++ ) SSE[j].Init(S_mw);

//  27584,     4, 208,   0,
//for( k=0; k<=16; k++ ) printf( "%7i,%7i,%4i,%4i,\n", coef[k][0],coef[k][1],coef[k][2],coef[k][3] ); fflush(stdout);

  orders[0] = &o0[0];
  for( k=3; k<=MAXORDER; k++ ) orders[k]=&fn[k-3][0];

  for( i=3; i<inplen; i++ ) {

    if( CheckTimer(500) ) printstats(i,inplen);

    // context bytes
    byte c2=inpbuf[i-3], c1=inpbuf[i-2], c0=inpbuf[i-1]; 

    orders[1] = &o1[c0][0];
    orders[2] = &o2[c1][c0][0];

    for( j=0; j<CNUM; j++ ) freq[j]=1; // order-(-1)

    bzero( fn ); // clear the o3+ counts
    // scan through all occurences of current o3 context if there were any
    for( j=p2[c2][c1][c0],n=DEPTH; j; j=l2[j] ) {
      // check orders up to maxorder at each o3 match
      for( k=3; k<=MAXORDER; k++ ) {
        if( inpbuf[int(j-k)]!=inpbuf[int(i-k)] ) break; // maxorder reached
        fn[k-3][ inpbuf[j] ]++; // increment the freq
      }
      if( !--n ) break;
    }

    // context mixing loop
    for( j=0; j<=MAXORDER; j++ ) {
      uint (&ctx_freq)[CNUM] = *(uint(*)[CNUM])orders[j];
      for( k=0,n=0,total=0; k<CNUM; k++ ) total+=ctx_freq[k],n+=(ctx_freq[k]>0);
      if( total==0 ) break;
      uint w = coef[j][n==1]/(1+coef[j][2]+total/(coef[j][3]+1)); // mixer weight
      if( w ) for( k=0; k<CNUM; k++ ) freq[k] += qword(ctx_freq[k])*w;
    }

    // shift freqs into range supported by rangecoder
    qword t = 0;
    for( j=0; j<CNUM; j++ ) t += freq[j];
    for( j=0; j<CNUM; j++ ) cidx[j]=j;

#if 0
//    qsort( cidx, CNUM, sizeof(uint), &compare );
#else
    sh_qsort<TEST>( cidx, 0, CNUM-1 );
#endif
//for( j=0; j<CNUM; j++ ) printf( "%02X:%I64i ", cidx[j],freq[cidx[j]] ); printf( "\n" );

    if( !DECODE ) c = getc(inp);

    for( j=0; j<CNUM; j++ ) {
      if( !DECODE ) flag=(cidx[j]==c);
//      flag = rc.rc_BProcess( freq[cidx[j]], t, flag );
//      flag = rc.rc_BProcess( SSE[j], freq[cidx[j]], t, flag );
      flag = rc.rc_BProcess( SSE[cidx[j]], freq[cidx[j]], t, flag );
      if( flag ) { c=cidx[j]; break; }
      t -= freq[cidx[j]];
    }

    if( DECODE ) putc(c,out);

    inpbuf[i]=c; 

    // directly update o0-o2
    o0[c]++; o1[c0][c]++; o2[c1][c0][c]++;

    // link o3 contexts into lists for dynamic freq tables
    l2[i] = p2[c2][c1][c0]; p2[c2][c1][c0] = i; 
  }

  if( !DECODE ) rc.FinishEncode();

  printstats(inplen,inplen);
  printf( "\n" );

  return 0;
}
