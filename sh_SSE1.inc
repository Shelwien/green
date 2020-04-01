
struct SSEi_updstr {
  int   P;
  int   sw;
  word  *C1;
  word  *T;
};

template< int SSEQuant=7 >
struct SSEi {

  word P[SSEQuant];

  void Init( int Wi=0 ) {
    int i,p1; 
    // p*(1-mw) + 0.5*mw
    int SCw = (SCALE-Wi)/(SSEQuant-1);
    int INC = Wi/2 + 8192; // *0.5
    for( i=0,p1=INC; i<SSEQuant; i++,p1+=SCw ) P[i] = p1/*, printf( "%04X ", p1-8192 )*/; //SCw*i + INC;
//printf( "\n" );
  }

  int SSE_Pred( int iP, SSEi_updstr& X ) {

    int sseFreq = ((SSEQuant-1)*iP)>>SCALElog;

    X.sw = ((SSEQuant-1)*iP)&mSCALE;
    X.C1 = &P[sseFreq+0];
    int f  = (((SCALE-X.sw)*X.C1[0]+X.sw*X.C1[1])>>SCALElog) - 8192;

    if( f<=0     ) f=1;
    if( f>=SCALE ) f=mSCALE;

    X.P = f;

    return f;
  }

  void SSE_Update( byte c, int wr0, SSEi_updstr& X ) {

    X.P = X.P*(SCALE-wr0)>>SCALElog; // X.P *= (1-wr1);
    if( c==0 ) X.P+=wr0;

    int dC = (X.C1[0]-X.C1[1]);
    int sw_dC= ((X.sw*dC+mSCALE)>>SCALElog);
    X.C1[0] = X.P + sw_dC       +8192;
    X.C1[1] = X.P - (dC-sw_dC)  +8192;
  }

};

