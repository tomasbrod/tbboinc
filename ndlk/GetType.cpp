/*
 * File:    GetType.cpp
 * Author:  S Harry White
 * Created: 2011-09-13
 * Updated: 2020-06-13
 *   Add prime number magic squares
 */

#include "stdafx.h"
#include <conio.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Windows.h>

int R, C,           // The order of the rectangle.
    N,              // R==C?R:-1
    M,              // R==C, N-1
    NN,             // R*C
    Nk,             // k range of Uzigzagk, Vzigzagk
    Sum2, Sum4;

#define Assert(_Expression) if (!(_Expression)) \
  printf("Assertion failed: \"%s\", line %d\n", #_Expression, __LINE__);

typedef unsigned int Uint;

struct t_ways { char ways, lrud; };
struct t_ways bent, *Uzig, *Vzig, *VzigA;
const int left=1, right=2, up=4, down=8; const bool F=false, T=true;

Uint magicConstant,
     rowConstant,
     colConstant,
     halfMagicConstant; // 1 base,
                        // (halfMagicConstant valid only for N doubly even).
Uint magicSum,
     rowSum,
     colSum,
     halfMagicSum,      // Use for 0 or 1 base,
                        // (halfMagicSum valid only for N doubly even).
     chkSum;            // Used for otherMagic squares

int **xRectangle=NULL, **yRectangle=NULL, **tRectangle=NULL, *opp,
     allocatedR, allocatedC, rectangleNum; const int startSize=32;
bool zeroBase, *numUsed=NULL, **QR, *compact;

enum magicType {
  notMagic, normalSemiMagic, otherSemiMagic, normalMagic, otherMagic,
  biMagic, triMagic, prime, prime1,
  adjacentCorner, adjacentSide, Associative, nearAssociative,
  Concentric, Bordered, Bordering, Symlateral, nearSymlateral,
  Pandiagonal, nearPandiagonal, Ultramagic, Complete, Compact, mostPerfect,
  Pandiagonal1Way, bentDiagonal, Franklin,
  Uzigzag, Vzigzag, VzigzagA, selfComplement, LS, DLS,
  AssociativeDLS, nearAssociativeDLS, PLS, WPLS, UltramagicDLS,
  Sym, DSym, CSym, nearCSym,
  nfr, nfc, nfr_nfc, nbd, selfTranspose, OLS, SOLS
};

const magicType firstType=notMagic, lastType=SOLS;
magicType squareType, prevType;

struct t_type { bool isType; char *name; };
const int numTypes=lastType-firstType+1;
struct t_type squareTypes[numTypes]=
{
  {F, "not magic"},
  {F, "normal semi-magic"},
  {F, "other semi-magic"},
  {F, "normal magic"},
  {F, "other magic"},
  {F, "bimagic"},
  {F, "trimagic"},
  {F, "prime"},
  {F, "prime & 1"},
  {F, "adjacent corner"},
  {F, "adjacent side"},
  {F, "associative"},
  {F, "near-associative"},
  {F, "concentric"},
  {F, "bordered"},
  {F, "bordering"},
  {F, "symlateral"},
  {F, "near-symlateral"},
  {F, "pandiagonal"},
  {F, "near-pandiagonal"},
  {F, "ultramagic"},
  {F, "complete"},
  {F, "compact"},
  {F, "most-perfect"},
  {F, "pandiagonal 1-way"},
  {F, "bent diagonal"},
  {F, "Franklin"},
  {F, "U zigzag"},
  {F, "V zigzag"},
  {F, "V zigzagA"},
  {F, "self-complement"},
  {F, "Latin"},
  {F, "diagonal Latin"},
  {F, "associative"},
  {F, "near-associative"},
  {F, "pandiagonal"},
  {F, "weakly pandiagonal"},
  {F, "ultramagic"},
  {F, "axial symmetric"},
  {F, "double axial symmetric"},
  {F, "center symmetric"},
  {F, "near center symmetric"},
  {F, "nfr"},
  {F, "nfc"},
  {F, "nfr nfc"},
  {F, "natural \\diagonal"},
  {F, "self-transpose"},
  {F, "orthogonal pair"},
  {F, "self-orthogonal"}
};

int typeCount[numTypes], *typeCountCompact, typeCountBent[5];
struct t_int5 { int x[5]; }; struct t_int5 *typeCountUzig, *typeCountVzig, *typeCountVzigA;
//------------------------------------------------------------------------------------------------

char *storeAllocFail="Storage allocation failed"; bool bell=T;
bool reportError(char *msg) {
//   -----------
  printf("%sError: %s.\n", bell?"\a\n":"",  msg);
  if (bell) bell=F; return F;
} // reportError

void freeRectangle(int*** rectangle, const int size) {
//   -------------
  if (*rectangle!=NULL) {
    for(int i=0; i<size; i++) free((*rectangle)[i]);
    free(*rectangle); *rectangle=NULL;
  }
} // freeRectangle

void freeBoolSquare(bool*** square, const int size) {
//   --------------
  if (*square!=NULL) {
    for(int i=0; i<size; i++) free((*square)[i]);
    free(*square); *square=NULL;
  }
} // freeBoolSquare

void freeStore() {
//   ---------
   freeRectangle(&xRectangle, allocatedR);
   freeRectangle(&yRectangle, allocatedR);
   freeRectangle(&tRectangle, allocatedR);
   freeBoolSquare(&QR, allocatedR+1); allocatedR=0; allocatedC=0;
   free(opp); opp=NULL; free(numUsed); numUsed=NULL;
   free(Uzig); Uzig=NULL; free(Vzig); Vzig=NULL; free(VzigA); VzigA=NULL;
   free(compact); compact=NULL; free(typeCountCompact); typeCountCompact=NULL;
   free(typeCountUzig); typeCountUzig=NULL; free(typeCountVzig); typeCountVzig=NULL;
   free(typeCountVzigA); typeCountVzigA=NULL; 
} // freeStore

bool allocateRectangle(int*** rectangle, const int nR, const int nC) {
//   -----------------
  bool ok;

  *rectangle=(int**) malloc(nR*sizeof(int*)); ok=(*rectangle!=NULL);
  if (ok) {
    int numAllocated=0;
    for(int i=0; i<nR; i++) {
      int* p=(int*) malloc(nC*sizeof(int)); (*rectangle)[i]=p;
      if (p==NULL) { numAllocated=i; ok=F; break; }
    }
    if (!ok) freeRectangle(rectangle, numAllocated);
  }
  return ok;
} // allocateRectangle

bool allocateBoolSquare(bool*** square, const int size) {
//   ------------------
  bool ok;

  *square=(bool**) malloc(size*sizeof(bool*)); ok=(*square!=NULL);
  if (ok) {
    int numAllocated=0;
    for(int i=0; i<size; i++) {
      bool* p=(bool*) malloc(size*sizeof(bool)); (*square)[i]=p;
      if (p==NULL) { numAllocated=i; ok=F; break; }
    }
    if (!ok) freeBoolSquare(square, numAllocated);
  }
  return ok;
} // allocateBoolSquare

bool allocateInts(int **x, const int size) {
//   ------------
  return ((*x=(int*) malloc(size*sizeof(int)))!=NULL);
} // allocateInts

bool allocateWays(struct t_ways **x, const int size) {
//   -----------
  return ((*x=(struct t_ways*) malloc(size*sizeof(struct t_ways)))!=NULL);
} // allocateWays

bool allocateBools(bool **x, const int size) {
//   ------------
  return ((*x=(bool*) malloc(size*sizeof(bool)))!=NULL);
} // allocateBools

bool allocate5Ints(struct t_int5 **x, const int size) {
//   -------------
  return ((*x=(struct t_int5*) malloc(size*sizeof(struct t_int5)))!=NULL);
  return T;
} // allocate5Ints

bool allocateStore() {
//   -------------
  bool ok=T; int nR=R, nC=C;
  if (nR<startSize) nR=startSize; if (nC<startSize) nC=startSize;
  if ((nR>allocatedR)|(nC>allocatedC)) {
    const int nRnC=nR*nC, nways=nR/2+1; freeStore();
    if (ok=allocateRectangle(&xRectangle, nR, nC))
      if (ok=allocateRectangle(&yRectangle, nR, nC))
        if (ok=allocateRectangle(&tRectangle, nR, nC))
          if (ok=allocateBoolSquare(&QR, nR+1))
            if (ok=allocateInts(&opp, nR+1))
              if (ok=allocateBools(&numUsed, nRnC+1))
                if (ok=allocateWays(&Uzig, nways))
                  if (ok=allocateWays(&Vzig, nways))
                    if (ok=allocateWays(&VzigA, nR+1))
                      if (ok=allocateBools(&compact, nways))
                        if (ok=allocateInts(&typeCountCompact, nways))
                          if (ok=allocate5Ints(&typeCountUzig, nways))
                            if (ok=allocate5Ints(&typeCountVzig, nways))
                              if (ok=allocate5Ints(&typeCountVzigA, nR+1))  
                                { allocatedR=nR; allocatedC=nC; }
    if (!ok) { reportError(storeAllocFail); freeStore(); }
  }
  return ok;
} // allocateStore
//----------------------------------------------------------------------------------------------

const magicType basicType[2][2]=
  { {otherSemiMagic, normalSemiMagic}, {otherMagic, normalMagic} };

typedef magicType (*t_mtippcici)(int **, const int, const int);
t_mtippcici isMagic, isNormal;
magicType isMagicSq(int **x, const int nr, const int nc) { // nr==nc
//        ---------
  int sumX, sumY, sumXY=0, sumYX=0; const int nm1=nr-1;

  for (int i=0; i<nr; i++) {
    sumX=0; sumY=0;
    for (int j=0; j<nr; j++) {
      sumX+=x[i][j]; sumY+=x[j][i];
    }
	   if (i==0) chkSum=sumX;
    if ((sumX!=chkSum)|(sumY!=chkSum)) return notMagic;
    sumXY+=x[i][nm1-i]; sumYX+=x[i][i];
  }
  return basicType[(sumXY==chkSum)&(sumYX==chkSum)][F];
} // isMagicSq

magicType isMagicRect(int **x, const int nr, const int nc) {
//        -----------
  for (int r=0; r<nr; r++) {
    int sum=0; for (int c=0; c<nc; c++) sum+=x[r][c];
    if (r==0) chkSum=sum; else if (sum!=chkSum) return notMagic;
  }
  for (int c=0; c<nc; c++) {
    int sum=0; for (int r=0; r<nr; r++) sum+=x[r][c];
    if (c==0) chkSum=sum; else if (sum!=chkSum) return notMagic;
  }
  return otherMagic;
} // isMagicRect
//---------------------------------------------------------------------------

magicType isNormalSq(int **x, const int nr, const int nc) { // R==C
//        ----------
  const int min=zeroBase?0:1, max=NN+min-1, nm1=nr-1;
  Uint sumX, sumY, sumXY=0, sumYX=0;

  for (int i=min; i<=max; i++) numUsed[i]=F;
  for (int i=0; i<nr; i++) {
    sumX=0; sumY=0;
    for (int j=0; j<nr; j++) {
      const int tmp=x[i][j]; numUsed[tmp]=T;
      sumX+=tmp; sumY+=x[j][i];
    }
    if (i==0) chkSum=sumX;
    if ((sumX!=chkSum)|(sumY!=chkSum)) return notMagic;
    sumXY+=x[i][nm1-i]; sumYX+=x[i][i];
  }

  bool allNumbersUsed=T;
  for (int i=min; i<=max; i++)
    if (!numUsed[i]) { allNumbersUsed=F; break; }
  return basicType[(sumXY==chkSum)&(sumYX==chkSum)]
                  [allNumbersUsed&(chkSum==magicSum)];
} // isNormalSq

magicType isNormalRect(int **x, const int nr, const int nc) {
//        ------------
  const int min=zeroBase?0:1, max=NN+min-1, nm1=nr-1;
  Uint chkSumR, chkSumC;

  for (int i=min; i<=max; i++) numUsed[i]=F;
  for (int r=0; r<nr; r++) {
    int sum=0;
    for (int c=0; c<nc; c++) {
      const int tmp=x[r][c]; numUsed[tmp]=T; sum+=tmp;
    }
    if (r==0) chkSumR=sum; else if (sum!=chkSumR) return notMagic;
  }
  for (int c=0; c<nc; c++) {
    int sum=0; for (int r=0; r<nr; r++) sum+=x[r][c];
    if (c==0) chkSumC=sum; else if (sum!=chkSumC) return notMagic;
  }
  bool allNumbersUsed=T;
  for (int i=min; i<=max; i++)
    if (!numUsed[i]) { allNumbersUsed=F; break; }
  return basicType[T][allNumbersUsed&(chkSumR==rowSum)&(chkSumC==colSum)];
} // isNormalRect

magicType isLatin(int **x, const int n) { // R==C
//        --------
  const int min=zeroBase?0:1, max=n+min-1, m=n-1; bool ls=T, dls=T;

  for (int r=0; r<n; ++r) {
    for (int i=min; i<=max; i++) numUsed[i]=F;
    for (int c=0; c<n; ++c) numUsed[x[r][c]]=T;
    for (int i=min; i<=max; ++i) if (!numUsed[i]) { ls=F; break; }
    if (!ls) break;
  }

  if (ls) for (int c=0; c<n; ++c) {
    for (int i=min; i<=max; i++) numUsed[i]=F;
    for (int r=0; r<n; ++r) numUsed[x[r][c]]=T;
    for (int i=min; i<=max; ++i) if (!numUsed[i]) { ls=F; break; }
    if (!ls) break;
  }

  if (ls) {
    for (int i=min; i<=max; i++) numUsed[i]=F;
    for (int r=0; r<n; ++r) numUsed[x[r][r]]=T;
    for (int i=min; i<=max; ++i) if (!numUsed[i]) { dls=F; break; }
    if (dls) {
      for (int i=min; i<=max; i++) numUsed[i]=F;
      for (int r=0; r<n; ++r) numUsed[x[r][m-r]]=T;
      for (int i=min; i<=max; ++i) if (!numUsed[i]) { dls=F; break; }
    }
  } else dls=F;
  return dls?DLS:ls?LS:isNormalSq(x, n, n);
} // isLatin
//---------------------------------------------------------------------------

bool readError; int smallestRead, biggestRead;
bool readRectangle(FILE *rfp) {
//   -------------
  int smallest=LONG_MAX, biggest=LONG_MIN;

  for (int r=0; r<R; r++) {
    for (int c=0; c<C; c++) {
	  int tmp, rv;

      if ( (rv=fscanf_s(rfp, "%d", &tmp))==1) {
        if (tmp<smallest) smallest=tmp;
        if (tmp>biggest) biggest=tmp;
        xRectangle[r][c]=tmp;
      } else {
        if ( (rv!=EOF)|(r!=0)|(c!=0) ) {
          readError=T;
          printf("\a\nError reading square from file.\n");
        }
        return F;
      }
    }
  }
  zeroBase=(smallest==0);
  if (zeroBase) {
    magicSum=magicConstant-N;
    rowSum=rowConstant-C; colSum=colConstant-R;
    halfMagicSum=halfMagicConstant-N/2;
    Sum2=NN-1;
  } else {
   magicSum=magicConstant; rowSum=rowConstant; colSum=colConstant;
   halfMagicSum=halfMagicConstant;
   Sum2=NN+1;
  }
  Sum4=Sum2+Sum2;

  if ( (smallest<0)|(biggest>NN) )
    squareType=isMagic(xRectangle, R, C);
  else if ((R==C)&(R!=1)&(smallest<=1)&(biggest==(smallest+M)))
    squareType=isLatin(xRectangle, R);
  else
    squareType=isNormal(xRectangle, R, C);
  smallestRead=smallest; biggestRead=biggest; ++rectangleNum;
  return T;
} // readRectangle
//-------------------------------------------------------------------------------

bool isAdjacentCorner(int **x, const int n) {
//   ----------------
  if ((n&3)!=0) return F; // only doubly even

  const int m=n-1, l=n-2, S2=Sum2; int v;
  v=x[0][0];  // corners
  if (v+x[1][1]!=S2) return F;
  v=x[0][m];
  if (v+x[1][m-1]!=S2) return F;
  v=x[m][0];
  if (v+x[m-1][1]!=S2) return F;
  v=x[m][m];
  if (v+x[m-1][m-1]!=S2) return F;

  for (int i=1; i<m; ++i) { // rest of top, bottom, left, right
    v=x[0][i];
    if ((v+x[1][i-1]!=S2)&(v+x[1][i+1]!=S2)) return F;
    v=x[m][i];
    if ((v+x[m-1][i-1]!=S2)&(v+x[m-1][i+1]!=S2)) return F;
    v=x[i][0];
    if ((v+x[i-1][1]!=S2)&(v+x[i+1][1]!=S2)) return F;
    v=x[i][m];
    if ((v+x[i-1][m-1]!=S2)&(v+x[i+1][m-1]!=S2)) return F;
  }

  for (int r=1; r<m; ++r) for (int c=1; c<m; ++c) { 
    v=x[r][c];
    if ((v+x[r-1][c-1]!=S2)&(v+x[r-1][c+1]!=S2)&
        (v+x[r+1][c-1]!=S2)&(v+x[r+1][c+1]!=S2))
      return F;
  }
  return squareTypes[adjacentCorner].isType=T;
} // isAdjacentCorner

bool isAdjacentSideEven(int **x, const int n) {
//   ------------------
  const int m=n-1, l=n-2, S2=Sum2; int v;

  v=x[0][0];  // corners
  if ((v+x[0][1]!=S2)&(v+x[1][0]!=S2)) return F;
  v=x[0][m];
  if ((v+x[0][l]!=S2)&(v+x[1][m]!=S2)) return F;
  v=x[m][0];
  if ((v+x[m][1]!=S2)&(v+x[l][0]!=S2)) return F;
  v=x[m][m];
  if ((v+x[m][l]!=S2)&(v+x[l][m]!=S2)) return F;

  for (int i=1; i<m; ++i) { // rest of top, bottom, left, right
    v=x[0][i];
    if ((v+x[0][i-1]!=S2)&(v+x[0][i+1]!=S2)&(v+x[1][i]!=S2)) return F;
    v=x[m][i];
    if ((v+x[m][i-1]!=S2)&(v+x[m][i+1]!=S2)&(v+x[l][i]!=S2)) return F;
    v=x[i][0];
    if ((v+x[i-1][0]!=S2)&(v+x[i+1][0]!=S2)&(v+x[i][1]!=S2)) return F;
    v=x[i][m];
    if ((v+x[i-1][m]!=S2)&(v+x[i+1][m]!=S2)&(v+x[i][l]!=S2)) return F;
  }

  for (int r=1; r<m; ++r) for (int c=1; c<m; ++c) { 
    v=x[r][c];
    if ((v+x[r-1][c]!=S2)&(v+x[r][c+1]!=S2)&(v+x[r+1][c]!=S2)&(v+x[r][c-1]!=S2))
      return F;
  }
  return squareTypes[adjacentSide].isType=T;
} // isAdjacentSideEven
//---------------------------------------------------------------------------

bool isAdjacentSideOdd(int **x, const int n) {
//   -----------------
  const int m=n-1, l=n-2, S2=Sum2, S1=S2/2; int v;

  v=x[0][0];  // corners
  if ((v+x[0][1]!=S2)&(v+x[1][0]!=S2)) if (v!=S1) return F;
  v=x[0][m];
  if ((v+x[0][l]!=S2)&(v+x[1][m]!=S2)) if (v!=S1) return F;
  v=x[m][0];
  if ((v+x[m][1]!=S2)&(v+x[l][0]!=S2)) if (v!=S1) return F;
  v=x[m][m];
  if ((v+x[m][l]!=S2)&(v+x[l][m]!=S2)) if (v!=S1) return F;

  for (int i=1; i<m; ++i) { // rest of top, bottom, left, right
    v=x[0][i];
    if ((v+x[0][i-1]!=S2)&(v+x[0][i+1]!=S2)&(v+x[1][i]!=S2)) if (v!=S1) return F;
    v=x[m][i];
    if ((v+x[m][i-1]!=S2)&(v+x[m][i+1]!=S2)&(v+x[l][i]!=S2)) if (v!=S1) return F;
    v=x[i][0];
    if ((v+x[i-1][0]!=S2)&(v+x[i+1][0]!=S2)&(v+x[i][1]!=S2)) if (v!=S1) return F;
    v=x[i][m];
    if ((v+x[i-1][m]!=S2)&(v+x[i+1][m]!=S2)&(v+x[i][l]!=S2)) if (v!=S1) return F;
  }

  for (int r=1; r<m; ++r) for (int c=1; c<m; ++c) { 
    v=x[r][c];
    if ((v+x[r-1][c]!=S2)&(v+x[r][c+1]!=S2)&(v+x[r+1][c]!=S2)&(v+x[r][c-1]!=S2))
      if (v!=S1) return F;
  }
  return squareTypes[adjacentSide].isType=T;
} // isAdjacentSideOdd
//----------------------------------------------------------------------------------------

bool isAdjacent(int **x, const int n)
//   ----------
{
  return isAdjacentCorner(x, n)||(n&1)?isAdjacentSideOdd(x, n):isAdjacentSideEven(x, n);
} // isAdjacent
//----------------------------------------------------------------------------------------

bool isNearAssociative(int **x, const int nr, const int nc, const int S2) {
//   -----------------
  const int nrd2=nr/2, nrm1=nr-1, ncm1=nc-1, maxCount=2; int count=0;

  for (int r=0; r<nrd2; ++r) for (int c=0; c<nc; ++c) {
    if (x[r][c]+x[nrm1-r][ncm1-c]!=S2) if (++count>maxCount) return F;
  }
  return squareTypes[nearAssociative].isType=T;
} // isNearAssociative

bool isAssociative(int **x, const int nr, const int nc, const int S2) {
//   -------------
  if (((squareType==normalMagic))&((nr&3)==2)&((nc&3)==2)) return isNearAssociative(x, nr, nc, S2);
  bool odd=((nr&1)==1);
  const int nrd2=nr/2, nrm1=nr-1, ncd2=nc/2, ncm1=nc-1;

  for (int r=0; r<nrd2; ++r) for (int c=0; c<nc; ++c) {
    if (x[r][c]+x[nrm1-r][ncm1-c]!=S2) return F;
  }
  if (odd) for (int c=0; c<ncd2; ++c)
    if (x[nrd2][c]+x[nrd2][ncm1-c]!=S2) return F;
  return squareTypes[Associative].isType=T;
} // isAssociative
//---------------------------------------------------------------------------

bool isUltramagic() {
//   ------------
  return squareTypes[Ultramagic].isType =
    squareTypes[Associative].isType&squareTypes[Pandiagonal].isType;
} // isUltramagic

bool isContiguous(int **x, const int m, int *first) {
//   ------------
  const int low=*first, high=low+m+m-3;
  int lo=LONG_MAX, hi=LONG_MIN, o=(N-m)/2, l=o+m-1, tmp=0;

  // Corners
  tmp=min(x[o][o], x[l][l]); if (tmp<lo) lo=tmp; if (tmp>hi) hi=tmp;
  tmp=min(x[o][l], x[l][o]); if (tmp<lo) lo=tmp; if (tmp>hi) hi=tmp;

  // Top, bottom
  for (int c=o+1; c<l; ++c) {
    tmp=min(x[o][c], x[l][c]); if (tmp<lo) lo=tmp; if (tmp>hi) hi=tmp;
  }

  // Left, right
  for (int r=o+1; r<l; ++r) {
    tmp=min(x[r][o], x[r][l]); if (tmp<lo) lo=tmp; if (tmp>hi) hi=tmp;
  }
  if ((lo!=low)|(hi!=high) ) return F;
  *first=hi+1; return T;
} // isContiguous
//-------------------------------------------------------------------------------

bool isBordered(int **x, const int n) {
//   ----------
  int first=zeroBase?0:1; const int end=(n&1)?3:6;

  for (int m=n; m >= end; m -= 2) if (!isContiguous(x, m, &first)) return F;
  return T;
} // isBordered
//-------------------------------------------------------------------------------

bool isBordering(int **x, const int n) {
//   -----------
  if (!(n&1)) { // only even
    int first=zeroBase?8:9;

    for (int m=6; m<=n; m+=2) if (!isContiguous(x, m, &first)) return F;
    return T;
  }
  return F;
} // isBordering
//-------------------------------------------------------------------------------

bool isBorder(int **x, const int m, const int S2) {
//   --------
  const int o=(N-m)/2, l=o+m-1;

  // Corners
  if ((x[o][o]+x[l][l])!=S2) return F;
  if ((x[o][l]+x[l][o])!=S2) return F;

  // Top, bottom
  for (int c=o+1; c<l; ++c) if ((x[o][c]+x[l][c])!=S2) return F;

  // Left, right
  for (int r=o+1; r<l; ++r) if ((x[r][o]+x[r][l])!=S2) return F;

  return T;
} // isBorder

bool isConcentric(int **x, const int n) {
//   ------------
  if (n==3) return T; if (n==4) return F;
  const int start=(n&1)?5:6, S2=(squareType==normalMagic)?Sum2:x[0][0]+x[n-1][n-1];

  for (int m=start; m<=n; m+=2) if (!isBorder(x, m, S2)) return F;
  if (squareType!=normalMagic) return squareTypes[Bordered].isType=T;
  if (isBordered(x, n)) squareTypes[Bordered].isType=T;
  else if (isBordering(x, n)) squareTypes[Bordering].isType=T;
  return squareTypes[Concentric].isType=T;
} // isConcentric
//------------------------------------------------------------------------------------

bool isFranklin(int **x, const int n) {
//   ----------
  if ((n&3)==0) { // else halfMagicSum not valid
    if (squareTypes[bentDiagonal].isType&(bent.ways==4)&compact[2]) {
      const int nd2=n/2; 
      for (int i=0; i<n; ++i) {
        int rowH=0, colH=0;
        for (int j=0; j<nd2; ++j) { rowH+=x[i][j]; colH+=x[j][i]; }
        if ((rowH!=halfMagicSum)|(colH!=halfMagicSum)) {
          //if (squareTypes[normalSemiMagic].isType) { // report only for normal magic
          //  squareTypes[bentDiagonal].isType=F;
          //  squareTypes[Compact].isType=F;
          //}
          return F;
        }
      }
      return squareTypes[Franklin].isType =T;
    }
  }
  return F;
} // isFranklin
//-------------------------------------------------------------------------------------

bool isBentDiagonal(int **x, const int n) {
//   --------------
  const int nm1=n-1, nd2=n/2, nd2p1=nd2+1; int waysBent=0;
  const bool odd=(n&1); bool bentL=T, bentR=T, bentU=T, bentD=T;

  for (int i=n-1; i >= 0; --i) { // bent left ?
    int sum=0;
    for (int r=0, c=i; r<nd2; ++r, --c) {
      const int cModn=c<0?c+n:c;
      sum+=x[r][cModn]+x[nm1-r][cModn];
    }
    if (odd) { int j=i+nd2p1; j=j<n?j:j-n; sum+=x[nd2][j]; }
    if (sum!=magicSum) { bentL=F; break; }
  }
  if (bentL) { bent.lrud|=left; ++waysBent; }

  for (int i=0; i<n; ++i) { // bent right ?
    int sum=0;
    for (int r=0, c=i; r<nd2; ++r, ++c) {
      const int cModn=c<n?c:c-n;
      sum+=x[r][cModn]+x[nm1-r][cModn];
    }
    if (odd) { int j=i+nd2; j=j<n?j:j-n; sum+=x[nd2][j]; }
    if (sum!=magicSum) { bentR=F; break; }
  }
  if (bentR) { bent.lrud|=right; ++waysBent; }

  for (int i=n-1; i >= 0; --i) { // bent up ?
    int sum=0;
    for (int c=0, r=i; c<nd2; ++c, --r) {
      const int rModn=r<0?r+n:r;
      sum+=x[rModn][c]+x[rModn][nm1-c];
    }
    if (odd) { int j=i+nd2p1; j=j<n?j:j-n; sum+=x[j][nd2]; }
    if (sum!=magicSum) { bentU=F; break; }
  }
  if (bentU) { bent.lrud|=up; ++waysBent; }

  for (int i=0; i<n; ++i) { // bent down ?
    int sum=0;
    for (int c=0, r=i; c<nd2; ++c, ++r) {
      const int rModn=r<n?r:r-n;
      sum+=x[rModn][c]+x[rModn][nm1-c];
    }
    if (odd) { int j=i+nd2; j=j<n?j:j-n; sum+=x[j][nd2]; }
    if (sum!=magicSum) { bentD=F; break; }
  }
  if (bentD) { bent.lrud|=down; ++waysBent; }

  bent.ways=waysBent;
  return squareTypes[bentDiagonal].isType=(waysBent>0);
} // isBentDiagonal
//--------------------------------------------------------------------------------------------------------

//void printUzigzag(int **x, int n, int k) {
////   ------------
//  printf("Uzigzag %d\n", k);
//  if (Uzig[k].lrud&left) {
//    printf("left\n");
//    for (int c=0; c<n; ++c) {
//      int sum=0, i=c, j=k, incr=-1; bool t=T;
//      for (int r=0; r<n; ++r) {
//        printf(" %d", x[r][i]);
//        if (--j==0) { incr=-incr; t=F; j=k; }
//        if (t) i+=incr; t=T; if (i<0) i+=n; else if (i>=n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//
//  if (Uzig[k].lrud&right) {
//    printf("right\n");
//    for (int c=0; c<n; ++c) {
//      int sum=0, i=c, j=k, incr=1; bool t=T;
//      for (int r=0; r<n; ++r) {
//        printf(" %d", x[r][i]);
//        if (--j==0) { incr=-incr; t=F; j=k; }
//        if (t) i+=incr; t=T; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//
//  if (Uzig[k].lrud&up) {
//    printf("up\n");
//    for (int r=0; r<n; ++r) {
//      int sum=0, i=r, j=k, incr=-1; bool t=T;
//      for (int c=0; c<n; ++c) {
//        printf(" %d", x[i][c]);
//        if (--j==0) { incr=-incr; t=F; j=k; }
//        if (t) i+=incr; t=T; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//
//  if (Uzig[k].lrud&down) {
//    printf("down\n");
//    for (int r=0; r<n; ++r) {
//      int sum=0, i=r, j=k, incr=1; bool t=T;
//      for (int c=0; c<n; ++c) {
//        printf(" %d", x[i][c]);
//        if (--j==0) { incr=-incr; t=F; j=k; }
//        if (t) i+=incr; t=T; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//} // printUzigzag

bool isUzigzag(int **x, const int n, const int k) {
//   ---------
  int ways=0, lrud=0; bool zigL=T, zigR=T, zigU=T, zigD=T;

  for (int c=0; c<n; ++c) { // left ?
    int sum=0, i=c, j=k, incr=-1; bool t=T;
    for (int r=0; r<n; ++r) {
      sum+=x[r][i];
      if (--j==0) { incr=-incr; t=F; j=k; }
      if (t) i+=incr; t=T; if (i<0) i+=n; else if (i>=n) i-=n;
    }
    if (sum!=magicSum) { zigL=F; break; }
  }
  if (zigL) { lrud|=left; ++ways; }

  for (int c=0; c<n; ++c) { // right ?
    int sum=0, i=c, j=k, incr=1; bool t=T;
    for (int r=0; r<n; ++r) {
      sum+=x[r][i];
      if (--j==0) { incr=-incr; t=F; j=k; }
      if (t) i+=incr; t=T; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigR=F; break; }
  }
  if (zigR) { lrud|=right; ++ways; }

  for (int r=0; r<n; ++r) { // up ?
    int sum=0, i=r, j=k, incr=-1; bool t=T;
    for (int c=0; c<n; ++c) {
      sum+=x[i][c];
      if (--j==0) { incr=-incr; t=F; j=k; }
      if (t) i+=incr; t=T; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigU=F; break; }
  }
  if (zigU) { lrud|=up; ++ways; }

  for (int r=0; r<n; ++r) { // down ?
    int sum=0, i=r, j=k, incr=1; bool t=T;
    for (int c=0; c<n; ++c) {
      sum+=x[i][c];
      if (--j==0) { incr=-incr; t=F; j=k; }
      if (t) i+=incr; t=T; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigD=F; break; }
  }
  if (zigD) { lrud|=down; ++ways; }

  //if (ways>0) printUzigzag(x,n,k);
  if (ways>0) { Uzig[k].ways=ways; Uzig[k].lrud=lrud; return squareTypes[Uzigzag].isType=T; }
  return F;
} // isUzigzag

//void printVzigzag(int **x, int n, int k) {
////   ------------
//  printf("Vzigzag %d\n", k);
//  if (Vzig[k].lrud&left) {
//    printf("left\n");
//    for (int c=0; c<n; ++c) {
//      int sum=0, i=c, j=k, incr=-1;
//      for (int r=0; r<n; ++r) {
//        printf(" %d", x[r][i]);
//        if (--j==0) { incr=-incr; j=k-1; }
//        i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//
//  if (Vzig[k].lrud&right) {
//    printf("right\n");
//    for (int c=0; c<n; ++c) {
//      int sum=0, i=c, j=k, incr=1;
//      for (int r=0; r<n; ++r) {
//        printf(" %d", x[r][i]);
//        if (--j==0) { incr=-incr; j=k-1; }
//        i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//
//  if (Vzig[k].lrud&up) {
//    printf("up\n");
//    for (int r=0; r<n; ++r) {
//      int sum=0, i=r, j=k, incr=-1;
//      for (int c=0; c<n; ++c) {
//        printf(" %d", x[i][c]);
//        if (--j==0) { incr=-incr; j=k-1; }
//        i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//
//  if (Vzig[k].lrud&down) {
//    printf("down\n");
//    for (int r=0; r<n; ++r) {
//      int sum=0, i=r, j=k, incr=1;
//      for (int c=0; c<n; ++c) {
//        printf(" %d", x[i][c]);
//        if (--j==0) { incr=-incr; j=k-1; }
//        i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
//      }
//      putchar('\n');
//    }
//  }
//} // printVzigzag

bool isVzigzag(int **x, const int n, const int k) {
//   ---------
  int ways=0, lrud=0; bool zigL=T, zigR=T, zigU=T, zigD=T;

  for (int c=0; c<n; ++c) { // left ?
    int sum=0, i=c, j=k, incr=-1;
    for (int r=0; r<n; ++r) {
      sum+=x[r][i];
      if (--j==0) { incr=-incr; j=k-1; }
      i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigL=F; break; }
  }
  if (zigL) { lrud|=left; ++ways; }

  for (int c=0; c<n; ++c) { // right ?
    int sum=0, i=c, j=k, incr=1;
    for (int r=0; r<n; ++r) {
      sum+=x[r][i];
      if (--j==0) { incr=-incr; j=k-1; }
      i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigR=F; break; }
  }
  if (zigR) { lrud|=right; ++ways; }

  for (int r=0; r<n; ++r) { // up ?
    int sum=0, i=r, j=k, incr=-1;
    for (int c=0; c<n; ++c) {
      sum+=x[i][c];
      if (--j==0) { incr=-incr; j=k-1; }
      i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigU=F; break; }
  }
  if (zigU) { lrud|=up; ++ways; }

  for (int r=0; r<n; ++r) { // down ?
    int sum=0, i=r, j=k, incr=1;
    for (int c=0; c<n; ++c) {
      sum+=x[i][c];
      if (--j==0) { incr=-incr; j=k-1; }
      i+=incr; if (i<0) i+=n; else if (i >= n) i-=n;
    }
    if (sum!=magicSum) { zigD=F; break; }
  }
  if (zigD) { lrud|=down; ++ways; }

  //if (ways>0) printVzigZag(x,n,k);
  if (ways>0) { Vzig[k].ways=ways; Vzig[k].lrud=lrud; return squareTypes[Vzigzag].isType=T; }
  return F;
} // isVzigzag
//-----------------------------------------------------------------------------------------------

bool matchSubVzigzagA(const int k, const int ways, const int lrud) {
//   ----------------
  if ((Vzig[2].ways==ways)&(Vzig[2].lrud==lrud)) return T;
  const int kd2p1=k/2+1;
  for (int f=3; f<=kd2p1; ++f) {
    const int c=f-1; int s=f;
    while (s<=k) { if ((s==k)&(VzigA[f].ways==ways)&(VzigA[f].lrud==lrud)) return T; s+=c; }
  }
  return F;
} // matchSubVzigzagA

bool isVzigzagA(int **x, const int n, const int k) {
//   ----------
  const int km1=k-1, nmkm1=n-km1; int ways=0, lrud=0;
  bool zigL=T, zigR=T, zigU=T, zigD=T;

  for (int c=0; c<n; ++c) { // VzigA left ?
    int sum1=0, sum2=0, j=c >= km1?c-km1:c-km1+n;
    for (int r=0; r<n; r+=2) sum1+=x[r][c];
    for (int r=1; r<n; r+=2) sum2+=x[r][j];
    if ((sum1+sum2)!=magicSum) { zigL=F; break; }
  }
  if (zigL) { lrud|=left; ++ways; }

  for (int c=0; c<n; ++c) { // VzigA right ?
    int sum1=0, sum2=0, j=c<nmkm1?c+km1:c+km1-n;
    for (int r=0; r<n; r+=2) sum1+=x[r][c];
    for (int r=1; r<n; r+=2) sum2+=x[r][j];
    if ((sum1+sum2)!=magicSum) { zigR=F; break; }
  }
  if (zigR) { lrud|=right; ++ways; }

  for (int r=0; r<n; ++r) { // VzigA up ?
    int sum1=0, sum2=0, i=r >= km1?r-km1:r-km1+n;
    for (int c=0; c<n; c+=2) sum1+=x[r][c];
    for (int c=1; c<n; c+=2) sum2+=x[i][c];
    if ((sum1+sum2)!=magicSum) { zigU=F; break; }
  }
  if (zigU) { lrud|=up; ++ways; }

  for (int r=0; r<n; ++r) { // VzigA down ?
    int sum1=0, sum2=0, i=r<nmkm1?r+km1:r+km1-n;
    for (int c=0; c<n; c+=2) sum1+=x[r][c];
    for (int c=1; c<n; c+=2) sum2+=x[i][c];
    if ((sum1+sum2)!=magicSum) { zigD=F; break; }
  }
  if (zigD) { lrud|=down; ++ways; }

  if (ways>0) {
    if (matchSubVzigzagA(k, ways, lrud)) return F;
    VzigA[k].ways=ways; VzigA[k].lrud=lrud; return squareTypes[VzigzagA].isType=T;
  }
  return F;
} // isVzigzagA
//---------------------------------------------------------------------------

bool isCompactK(int **x, const int n, const int k) {
//   ----------
  const int m=n-1; int Sk=0;
  for (int r=0; r<k; ++r) for (int c=0; c<k; ++c) Sk+=x[r][c];
  
  for (int r=0; r<n; ++r) {
    for (int c=0; c<n; ++c) { int S=0;
      for (int i=r; i<(r+k); ++i) { int im=(i>=n)?i-n:i;
        for (int j=c; j<(c+k); ++j) { int jm=(j>=n)?j-n:j;
        S+=x[im][jm]; }
      }
      //if (k==2) printf("r %d c %d S %d\n", r, c, S);
      if (S!=Sk) return F;
    }
  }
  return compact[k]=T;
} // isCompactK

bool subCompact(const int k) {
//   ----------
  const int kd2=k/2;
  for (int f=2; f<=kd2; ++f) if (((k%f)==0)&compact[f]) return T;
  return F;
} // subCompact

bool isCompact(int **x, const int n) {
//   ---------
  bool any=F;
  for (int k=2; k<Nk; ++k) if ((n%k)==0) {
    if (subCompact(k)) continue;
    if (isCompactK(x,n,k)) { compact[k]=T; any=T; }
  }
  return squareTypes[Compact].isType=any;
} // isCompact
//--------------------------------------------------------------------------------

bool isComplete(int **x, const int n) {
//   ----------
  if ((n&3)!=0) return F; // only doubly even
  const int nd2=n/2, nd2m1=nd2-1, S2=Sum2;

  for (int r=0; r<nd2m1; ++r) { // no need to check last pair in each diag
    const int i=r+nd2;
    for (int c=0; c<n; ++c) {
      int j=c+nd2; j=j<n?j:j-n;
      if ((x[r][c]+x[i][j])!=S2) return F;
    }
  }
  return squareTypes[Complete].isType=T;
} // isComplete
//---------------------------------------------------------------------------

bool isNearPandiagonal(int **x, const int n) {
//   -----------------
  int count=0; const int nm1=n-1, maxCount=4;

  for (int i=0; i<n; ++i) {
    int sumYX=0, sumXY=0;
    for (int r=0, c=i; r<n; ++r, ++c) {
      const int cModn=c<n?c:c-n;
      sumYX+=x[r][cModn]; sumXY+=x[r][nm1-cModn];
    }
    if (sumYX!=magicSum) ++count; if (sumXY!=magicSum) ++count;
    if (count>maxCount) return F;
  }
  return squareTypes[nearPandiagonal].isType=T;
} // isNearPandiagonal

bool isMostPerfect() {
//   -------------
  return squareTypes[mostPerfect].isType=
    compact[2]&squareTypes[Complete].isType;
}

bool isPandiagonal(int **x, const int n, const int Sum) {
//   -------------
  if (((n&3)==2)&(squareType==normalMagic)) return isNearPandiagonal(x, n);
  const int nm1=n-1; bool panBack=T, panForward=T; 

  for (int i=0; i<n; ++i) {
    int sumYX=0, sumXY=0;
    for (int r=0, c=i; r<n; ++r, ++c) {
      const int cModn=c<n?c:c-n; sumYX+=x[r][cModn]; sumXY+=x[r][nm1-cModn];
    }
    if (sumYX!=Sum) panBack=F; if (sumXY!=Sum) panForward=F;
    if (!(panBack|panForward)) return F;
  }
  if (panBack&panForward) {
    if (squareType==normalMagic) isComplete(x, n);
    return squareTypes[Pandiagonal].isType=T;
  }
  return squareTypes[Pandiagonal1Way].isType=T;
} // isPandiagonal
//---------------------------------------------------------------------------

bool isSymlateralOdd(int **x, const int n) {
//   ---------------
  const int nd2=n/2, nm1=n-1, S2=Sum2;

  // Diagonals
  for (int r=0; r<nd2; ++r) {
    if (x[r][r]+x[nm1-r][nm1-r]!=S2) return F;
    if (x[r][nm1-r]+x[nm1-r][r]!=S2) return F;
  }

  // Center row(s), column(s)
  for (int i=0; i<nd2; ++i) {
    if (x[i][nd2]+x[nm1-i][nd2]!=S2) return F;
    if (x[nd2][i]+x[nd2][nm1-i]!=S2) return F;
  }

  // Rest of rows, columns
  for (int i=0; i<nd2; ++i) {
    for (int j=i+1; j<nd2; ++j) {
      if (x[i][j]+x[i][nm1-j]!=S2) return F;
      if (x[j][i]+x[nm1-j][i]!=S2) return F;
    }
  }
  return squareTypes[Symlateral].isType=T;
} // isSymlateralOdd
//---------------------------------------------------------------------------

bool isSymlateralEven(int **x, const int n) {
//   ----------------
  const int nd2=n/2, nm1=n-1, nd2m1=nd2-1, S2=Sum2, maxCount=8; int count=0;

  // Diagonals
  for (int r=0; r<nd2; ++r) {
    if (x[r][r]+x[nm1-r][nm1-r]!=S2) if (++count>maxCount) return F;
    if (x[r][nm1-r]+x[nm1-r][r]!=S2) if (++count>maxCount) return F;
  }

  // Center row(s), column(s)
  for (int j=nd2m1; j<=nd2; ++j) {
    for (int i=0; i<nd2m1; ++i) {
      if (x[i][j]+x[nm1-i][j]!=S2) if (++count>maxCount) return F;
      if (x[j][i]+x[j][nm1-i]!=S2) if (++count>maxCount) return F;
    }
  }

  // Rest of rows, columns
  for (int i=0; i<nd2m1; ++i) {
    for (int j=i+1; j<nd2m1; ++j) {
      if (x[i][j]+x[i][nm1-j]!=S2) return F;
      if (x[j][i]+x[nm1-j][i]!=S2) return F;
    }
  }
  return squareTypes[nearSymlateral].isType=T;
} // isSymlateralEven
//---------------------------------------------------------------------------

bool isSymlateral(int **x, const int n)
//   ------------
{
  if (n<=4) return F;
  return (n&1)?isSymlateralOdd(x, n):isSymlateralEven(x, n);
} // isSymlateral
//---------------------------------------------------------------------------

bool isSelfComplement(int **x, const int n)
//   ----------------
{
  if (n&1) return F; // only even
  const int m=n-1, mid=n/2, S2=Sum2;

  if ((x[0][0]+x[0][m])==S2) {
    for (int r=0; r<n; ++r)
      for (int c=0; c<mid; ++c)
        if ((x[r][c]+x[r][m-c])!=S2) return F;
  } else if ((x[0][0]+x[m][0])==S2) {
    for (int r=0; r<mid; ++r)
      for (int c=0; c<n; ++c)
        if ((x[r][c]+x[m-r][c])!=S2) return F;
  } else return F;

  return squareTypes[selfComplement].isType=T;
} // isSelfComplement
//------------------------------- Multimagic ---------------------------------

bool biPandiagonal;
bool isBiPanDiagonal(int **x, const int n, const int bSum) {
//   ---------------
  const int nm1=n-1;

  // Main diagonals already checked in isBiMagic.
  for (int i=1; i<n; ++i) {
    int sumYX=0, sumXY=0, z;
    for (int r=0, c=i; r<n; ++r, ++c) {
      const int cModn=c<n?c:c-n;
      z=x[r][cModn]; sumYX+=z*z; z=x[r][nm1-cModn]; sumXY+=z*z;
    }
    if ( (sumYX!=bSum)|(sumXY!=bSum) ) return F;
  }
  return T;
} // isBiPanDiagonal

bool isBiMagic(int **x, const int n)
//   ---------
{
  if (n<8) return F;
  const int nn=NN; int sumXY=0, sumYX=0, bSum; // bSum=n*(nn+1)*(nn+nn+1)/6

  for (int r=0; r<n; ++r) {
    int sumX=0, sumY=0, z;
    for (int c=0; c<n; ++c) { z=x[r][c]; sumX+=z*z; z=x[c][r]; sumY+=z*z; }

    if (r==0) bSum=sumX;
    if ((sumX!=bSum)|(sumY!=bSum)) { return F; }
    z=x[r][n-r-1]; sumXY+=z*z; z=x[r][r]; sumYX+=z*z;
  }
  if ( (sumXY!=bSum)|(sumYX!=bSum) ) return F;

  biPandiagonal=squareTypes[Pandiagonal].isType && isBiPanDiagonal(x, n, bSum);
  return squareTypes[biMagic].isType=T;
} // isBiMagic 
//---------------------------------------------------------------------------

bool isTriMagic(int **x, const int n)
//   ----------
{
  if (n<12) return F;
  const int nn=NN; int sumXY=0, sumYX=0, tSum; // tSum=n*nn*(nn+1)*(nn+1)/4

  for (int r=0; r<n; ++r) {
    int sumX=0, sumY=0, z;
    for (int c=0; c<n; ++c) { z=x[r][c]; sumX+=z*z*z; z=x[c][r]; sumY+=z*z*z; }

    if (r==0) tSum=sumX;
    if ((sumX!=tSum)|(sumY!=tSum)) { return F; }
    z=x[r][n-r-1]; sumXY+=z*z*z; z=x[r][r]; sumYX+=z*z*z;
  }
  if ( (sumXY!=tSum)|(sumYX!=tSum) ) return F;
  return squareTypes[triMagic].isType=T;
} // isTriMagic
//------------------------------------- Prime Magic --------------------------------------

bool isPrime(const int n) {
//   -------
  int i=2; if (n<=1) return F;
  while ((i*i)<=n) if ((n%i++)==0) return F; return T;
} // isPrime

bool checkUnique(int **x, const int n) {
//   -----------
  bool ok=T; char *msg="Square %d duplicate number %d.\n";
  for (int r=0; r<n; ++r) {
    for (int c=0; c<n; ++c) {
      const int t=x[r][c]; bool ok1=T;
      for (int c1=c+1; c1<n; ++c1) if (x[r][c1]==t) {
        printf(msg, rectangleNum, t); ok1=F; break; }
      if (ok1) for (int r1=r+1; r1<n; ++r1) {
        for (int c1=0; c1<n; ++c1) if (x[r1][c1]==t) {
          printf(msg, rectangleNum, t); ok1=F; break; }
        if (!ok1) break;
      }
      if (!ok1) ok=F;
    }
  }
  return ok;
} // checkUnique

bool isPrimeMagic(int **x, const int n) {
//   ------------
  bool has1=F; const int nd2=n/2;
  for (int r=0; r<n; ++r) for (int c=0; c<n; ++c)
    if (x[r][c]==1) has1=T; else if (!isPrime(x[r][c])) {
      if ((n&1)&(r==nd2)&(c==nd2)) // Supposed to be prime? Could check rest of square.
        printf("Middle number %d is not prime.\n", x[r][c]); return F; 
    }
  if (checkUnique(x, n)) {
    isAssociative(x, R, C, x[0][0]+x[n-1][n-1]); isPandiagonal(x, n, chkSum);
    isConcentric(x, n); isBiMagic(x, n);
    return has1 ? squareTypes[prime1].isType=T : squareTypes[prime].isType=T;
  }
  return F;
} // isPrimeMagic
//---------------------------------------- Latin ------------------------------------------

bool isNearAssociativeDLS(int **x, const int n) {
//   --------------------
  const int nd2=n/2, m=n-1, S2=zeroBase?n-1:n+1, maxCount=2; int count=0;

  for (int r=0; r<nd2; ++r) for (int c=0; c<n; ++c) {
    if (x[r][c]+x[m-r][m-c]!=S2) if (++count>maxCount) return F;
  }
  return squareTypes[nearAssociativeDLS].isType=T;
} // isNearAssociativeDLS

bool isAssociativeDLS(int **x, const int n) {
//   ----------------
  if ((n&3)==2) return isNearAssociativeDLS(x, n); const bool odd=(n&1);
  const int nd2=n/2, m=n-1, S2=zeroBase?n-1:n+1;
  for (int r=0; r<nd2; ++r) if ((x[r][r]+x[m-r][m-r])!=S2) return F;
  for (int r=0; r<m; ++r) for (int c=r+1; c<n; ++c)
    if (x[r][c]+x[m-r][m-c]!=S2) return F;
  return squareTypes[AssociativeDLS].isType=T;
} // isAssociativeDLS

bool isWPLS(int **x, const int n) { // Weakly pandiagonal. All diagonals have same sum.
//   ------
  const int nm1=n-1; int Sn=0; for (int r=0; r<n; ++r) Sn+=x[r][r];

  for (int i=1; i<n; ++i) {
    int S=0; for (int r=0, c=i; r<n; ++r, ++c) S+=x[r][c<n?c:c-n];
    if (S!=Sn) return F;
  }
  for (int i=0; i<n; ++i) {
    int S=0; for (int r=0, c=i; r<n; ++r, ++c) S+=x[r][nm1-(c<n?c:c-n)];
    if (S!=Sn) return F;
  }
  return squareTypes[WPLS].isType=T;
} // isWPLS

// Pandiagonal, Knut Vik design. Not even and not a multiple of 3. N%6 equals 1 or 5
bool isPLS(int **x, const int n) {
//   -----
  const int mod6=n%6, nm1=n-1;
  if ((mod6==1)|(mod6==5)) {
    const int min=zeroBase?0:1, max=n+min-1, m=n-1; bool pls=T;
    for (int i=0; i<n; ++i) {
      for (int j=min; j<=max; j++) numUsed[j]=F;
      for (int r=0, c=i; r<n; ++r, ++c) numUsed[x[r][c<n?c:c-n]]=T;
      for (int j=min; j<=max; ++j) if (!numUsed[j]) { pls=F; break; }
    }
    if (pls) for (int i=0; i<n; ++i) {
      for (int j=min; j<=max; j++) numUsed[j]=F;
      for (int r=0, c=i; r<n; ++r, ++c) numUsed[x[r][nm1-(c<n?c:c-n)]]=T;
      for (int j=min; j<=max; ++j) if (!numUsed[j]) { pls=F; break; }
    }
    if (!pls) isWPLS(x, n);
    return squareTypes[PLS].isType=pls;
  }
  return isWPLS(x, n);
} // isPLS

bool isUltramagicDLS(int **x, const int n) {
//   ---------------
  return squareTypes[UltramagicDLS].isType=squareTypes[AssociativeDLS].isType&
    (squareTypes[PLS].isType|squareTypes[WPLS].isType);
} // isUltramagicDLS

bool isDSym(int **x, const int n) { 
//   ------
  const int nd2=n/2, m=n-1; for (int v=0; v<=n; ++v) opp[v]=-1;
  for (int c=0; c<n; ++c) {
    if (opp[x[0][c]]>=0) {
      if (opp[x[0][c]]!=x[m][c]) return squareTypes[Sym].isType=T;
    } else { opp[x[0][c]]=x[m][c]; opp[x[m][c]]=x[0][c]; }
  }
  for (int r=1; r<nd2; ++r) for (int c=0; c<n; ++c) {
    if (x[r][c]!=opp[x[m-r][c]]) return squareTypes[Sym].isType=T;
  }
  return squareTypes[DSym].isType=T;
} // isDSym

bool rowsSym;
bool isSym(int **x, const int n) { 
//   -----
  const int nd2=n/2, m=n-1; int symRows=T; for (int v=0; v<=n; ++v) opp[v]=-1;
  for (int r=0; r<n; ++r)
    if (opp[x[r][0]]>=0) { if (opp[x[r][0]]!=x[r][m]) { symRows=F; break; }
    } else { opp[x[r][0]]=x[r][m]; opp[x[r][m]]=x[r][0]; }

  if (symRows) for (int r=0; r<n; ++r) {
    for (int c=1; c<nd2; ++c) if (x[r][c]!=opp[x[r][m-c]]) { symRows=F; break; }
    if (!symRows) break;
  }
  if (symRows) { rowsSym=T; return isDSym(x, n); }

  for (int v=0; v<=n; ++v) opp[v]=-1;
  for (int c=0; c<n; ++c)
    if (opp[x[0][c]]>=0) {
      if (opp[x[0][c]]!=x[m][c]) return F;
    } else { opp[x[0][c]]=x[m][c]; opp[x[m][c]]=x[0][c]; }

  for (int r=1; r<nd2; ++r) for (int c=0; c<n; ++c)
    if (x[r][c]!=opp[x[m-r][c]]) return F;

  rowsSym=F; return squareTypes[Sym].isType=T;
} // isSym

bool isNearCSym(int **x, const int n) {
//   ----------
  if ((n&3)!=2) return F;
  if ((squareType==DLS)&squareTypes[nearAssociativeDLS].isType) return F;

  const int nd2=n/2, nm1=n-1; int **count=tRectangle, viols=0;
  for (int r=0; r<n; ++r) for (int c=0; c<n; ++c) count[r][c]=0;
  for (int r=0; r<nd2; ++r) for (int c=0; c<n; ++c) {
    int v=x[r][c], o=x[nm1-r][nm1-c]; if (!zeroBase) { --v; --o; } ++count[v][o];
  }

  for (int v=0; v<n; ++v) {
    bool gt_one=F;
    for (int o=0; o<n; ++o) if (count[v][o]>0) {
      if (count[v][o]==1) ++viols; else { if (gt_one) return F; gt_one=T; }
    }
  }
  return (viols>2)?F:squareTypes[nearCSym].isType=T;
} // isNearCSym

bool isCSym(int **x, const int n) {
//   ------
  if (squareType==DLS) {
    if ((n&3)==2) return isNearCSym(x,n);
    if (squareTypes[AssociativeDLS].isType) return F;
  }
  
  const int nd2=n/2, m=n-1;
  for (int v=0; v<=n; ++v) opp[v]=-1;
  for (int c=0; c<n; ++c) {
    if (opp[x[0][c]]>=0) {
      if (opp[x[0][c]]!=x[m][m-c]) return F;
    } else { opp[x[0][c]]=x[m][m-c]; opp[x[m][m-c]]=x[0][c]; }
  }
  for (int r=1; r<nd2; ++r) if (x[r][r]!=opp[x[m-r][m-r]]) return F;
  for (int r=1; r<m; ++r) for (int c=r+1; c<n; ++c)
    if (x[r][c]!=opp[x[m-r][m-c]]) return F;
  return squareTypes[CSym].isType=T;
} // isCSym

bool isOLS(int **x, int **y, bool **b, const int n) {
//   -----
 if ((rectangleNum!=1)&((prevType==LS)|(prevType==DLS))) {
    const int min=zeroBase?0:1, max=zeroBase?n-1:n;
    for (int r=0; r<=n; ++r) for (int c=0; c<=n; ++c) b[r][c]=F;
      // x's zeroBase may be different from y's
    for (int r=0; r<n; ++r) for (int c=0; c<n; ++c) {
      const int u=x[r][c], v=y[r][c]; if (b[u][v]) return F; b[u][v]=T;
    }
    return squareTypes[OLS].isType=T;
  }
  return F;
} // isOLS

bool isSOLS(int **x, bool **b, const int n) {
//   ------
  const int min=zeroBase?0:1, max=zeroBase?n-1:n;
  for (int r=min; r<=max; ++r) for (int c=min; c<=max; ++c) b[r][c]=F;
  for (int r=0; r<n; ++r) for (int c=r; c<n; ++c) {
    const int u=x[r][c], v=x[c][r]; if (b[u][v]) return F;
    b[u][v]=T; b[v][u]=T;
  }
  return squareTypes[SOLS].isType=T;
} // isSOLS

bool isSelfTranspose(int **x, const int n) {
//   ---------------
  for (int r=0; r<n; ++r) for (int c=r+1; c<n; ++c) if (x[r][c]!=x[c][r]) return F;
  return squareTypes[selfTranspose].isType=T;
} // isSelfTranspose

bool isNFR(int **x, const int n) {
//   -----
  const int min=zeroBase?0:1;
  for (int i=0; i<n; ++i) if (x[0][i]!=(i+min)) return F;
  return squareTypes[nfr].isType=T;
} // isNFR

bool isNFC(int **x, const int n) {
//   -----
  const int min=zeroBase?0:1;
  for (int i=0; i<n; ++i) if (x[i][0]!=(i+min)) return F;
  return squareTypes[nfc].isType=T;
} // isNFC

bool isNFR_NFC(int **x, const int n) {
//   ---------
  return squareTypes[nfr_nfc].isType=(squareTypes[nfr].isType&squareTypes[nfc].isType);
} // isNFR_NFC

bool isNBD(int **x, const int n) {
//   -----
  const int min=zeroBase?0:1;
  for (int i=0; i<n; ++i) if (x[i][i]!=(i+min)) return F;
  return squareTypes[nbd].isType=T;
} // isNBD
//------------------------------------------------------------------------------------------------------

void incrTypeCount(const int i) {
//   -------------
  switch (i) {
    case bentDiagonal:
      ++typeCountBent[bent.ways];
      if (bent.ways==4) ++typeCount[i]; else typeCountBent[0]=1; // there is 1, 2, or 3
      break;
    case Compact:
      for (int k=2; k<Nk; ++k) {
        if (compact[k]) { ++typeCountCompact[k]; if (k==2) ++typeCount[i]; else typeCountCompact[0]=1; }
      }
      break;
    case Uzigzag:
      for (int k=2; k<Nk; ++k) {
        const int ways=Uzig[k].ways;
        if (ways>0) {
          ++typeCountUzig[k].x[ways]; typeCountUzig[k].x[0]=1; // there is 1, 2, 3, or 4
          if ((k==2)&(ways==4)) ++typeCount[i]; else typeCountUzig[0].x[0]=1;
        }
      }
      break;
    case Vzigzag:
      for (int k=2; k<Nk; ++k) {
        const int ways=Vzig[k].ways;
        if (ways>0) {
          ++typeCountVzig[k].x[ways]; typeCountVzig[k].x[0]=1; // there is 1, 2, 3, or 4
          if ((k==2)&(ways==4)) ++typeCount[i]; else typeCountVzig[0].x[0]=1;
        }
      }
      break;
    case VzigzagA:
      for (int k=3; k<=N; ++k) {
        const int ways=VzigA[k].ways;
        if (ways>0) {
          ++typeCountVzigA[k].x[ways]; typeCountVzigA[k].x[0]=1; // there is 1, 2, 3, or 4
          typeCountVzigA[0].x[0]=1; // there is VzigzagA
        }
      }
      break;
    default: ++typeCount[i]; break;
  }
} // incrTypeCount

bool putWaysBent(struct t_ways x, FILE *wfp) {
//   -----------
  char waysBentStr[30], *s=waysBentStr; bool started=F;
  *s++=' '; *s++=(x.ways==1)?'1':(x.ways==2)?'2':'3'; *s++='-';  *s++='w';  *s++='a';  *s++='y';
  *s++=' '; *s++='(';
  if (x.lrud&left) { *s++='l'; *s++='e'; *s++='f'; *s++='t'; started=T; }
  if (x.lrud&right) { if (started) { *s++=','; *s++=' '; } else started=T;
    *s++='r'; *s++='i'; *s++='g'; *s++='h'; *s++='t'; 
  }
  if (x.lrud&up) { if (started) { *s++=','; *s++=' '; } else started=T;
    *s++='u'; *s++='p';
  }
  if (x.lrud&down) { if (started) { *s++=','; *s++=' '; } else started=T;
    *s++='d'; *s++='o'; *s++='w'; *s++='n';
  }
  *s++=')'; *s='\0'; return (fprintf(wfp, "%s", waysBentStr)>0);
} // putWaysBent

bool printType(FILE *wfp) {
//   ---------
  //if (wfp==NULL) { // no detail
  //  for (int i=firstType; i<=lastType; ++i) if (squareTypes[i].isType) incrTypeCount(i);
  //  return T;
  //}
  bool start=T, first=T;
  if (rectangleNum%10==1)
    if (fprintf(wfp, "\n%d ...\n", rectangleNum)<=0) return F;
  for (int i=firstType; i<=lastType; ++i) {
    if (squareTypes[i].isType) {
      incrTypeCount(i);
      if (!start) {
        if (fputc(',', wfp)==EOF) return F;
        if (fputc(' ', wfp)==EOF) return F;
      }
      if (fprintf(wfp, "%s", squareTypes[i].name)<=0) return F;
      switch (i) {
        case Compact:
          first=!compact[2];
          for (int j=3; j<Nk; ++j) if (compact[j]) {
            if (first) { if (fprintf(wfp, "%d", j)<=0) return F; }
            else if (fprintf(wfp, ", %s%d", squareTypes[i].name, j)<=0) return F;
            first=F;
          }
          break;
        case bentDiagonal:
          if (bent.ways!=4) { if (!putWaysBent(bent, wfp)) return F; }
          break;
        case Uzigzag:
          first=(Uzig[2].ways==0);
          if (!first&(Uzig[2].ways!=4)) if (!putWaysBent(Uzig[2], wfp)) return F;
          for (int j=3; j<Nk; ++j) {
            const int ways=Uzig[j].ways;
            if (ways>0) {
              if (first) { if (fprintf(wfp, "%d", j)<=0) return F; }
              else if (fprintf(wfp, ", %s%d", squareTypes[i].name, j)<=0) return F;
              if (ways!=4) if (!putWaysBent(Uzig[j], wfp)) return F;
              first=F;
            }
          }
          break;
        case Vzigzag:
          first=(Vzig[2].ways==0);
          if (!first&(Vzig[2].ways!=4)) if (!putWaysBent(Vzig[2], wfp)) return F;
          for (int j=3; j<Nk; ++j) {
            const int ways=Vzig[j].ways;
            if (ways>0) {
              if (first) { if (fprintf(wfp, "%d", j)<=0) return F; }
              else if (fprintf(wfp, ", %s%d", squareTypes[i].name, j)<=0) return F;
              if (ways!=4) if (!putWaysBent(Vzig[j], wfp)) return F;
              first=F;
            }
          }
          break;
        case VzigzagA:
          //first=(VzigA[3].ways==0);
          //if (!first&(VzigA[3].ways!=4)) if (!putWaysBent(VzigA[3], wfp)) return F;
          first=T;
          for (int j=3; j<=N; ++j) {
            const int ways=VzigA[j].ways;
            if (ways>0) {
              if (first) { if (fprintf(wfp, "%d", j)<=0) return F; }
              else if (fprintf(wfp, ", %s%d", squareTypes[i].name, j)<=0) return F;
              if (ways!=4) if (!putWaysBent(VzigA[j], wfp)) return F;
              first=F;
            }
          }
          break;
        case biMagic:     
          if (biPandiagonal)
            if (fprintf(wfp, " (biPandiagonal)")<=0) return F;
          break;
        case Sym:
          if (fprintf(wfp, " (%ss)", rowsSym?"row":"column")<=0) return F;
          break;
        default:
          break;
      }
      start=F;
    }
  }
  return fputc('\n', wfp)!=EOF;
} // printType

bool getLatinTypes(int **x, int **y, const int R, FILE *wfp) {
//   -------------
  isOLS(x, y, QR, R); isSOLS(x, QR, R); isSym(x,R); isNFR(x,R); isNFC(x,R); isNBD(x,R);
  if (squareType==LS) { isNFR_NFC(x,R); isSelfTranspose(x,R); }
  if (squareType==DLS) isAssociativeDLS(x,R); isCSym(x,R); // isCSym after isAssociativeDLS
  if ((squareType==LS)&((N&3)==2)&(!squareTypes[CSym].isType)) isNearCSym(x,R);
  isPLS(x,R); isUltramagicDLS(x,R);
  int **t=xRectangle; xRectangle=yRectangle; yRectangle=t;
  return printType(wfp);
} // getLatinTypes

void initGetTypes() {
//   ------------
  for (int i=firstType; i<=lastType; ++i) squareTypes[i].isType=F;
  squareTypes[squareType].isType=T;
    if (R==C) {
    bent.ways=0; bent.lrud=0;
    for (int i=0; i<Nk; ++i) {
      Uzig[i].ways=0; Uzig[i].lrud=0; Vzig[i].ways=0; Vzig[i].lrud=0; compact[i]=F;
    }
    for (int i=0; i<=N; ++i) { VzigA[i].ways=0; VzigA[i].lrud=0; }
  }
} // initGetTypes

void getUzigzags(int **x, const int n) {
//   -----------
  if ((n&3)==0) { // only doubly even
    const int nd4=n/4;  // n/2 is bent diagonal
    for (int k=2; k<=nd4; ++k) { const int len=k+k; if ((n%len)==0) isUzigzag(x,n,k); }
  }
} // getUzigzags

void getVzigzags(int **x, const int n) {
//   -----------
  if ((n&3)==2) return; // no singly even
  if (n>3) {
    if (n&1) {
      const int lim=n/2+2;
      for (int j=2; j<lim; j+=2)
        if ((((n-1)%j)==0)|(((n+1)%j)==0)) isVzigzag(x,n,(j+2)/2); // ?
    } else {
      const int lim=n/2+1;
      for (int j=2; j<lim; j+=2) if ((n%j)==0) isVzigzag(x,n,(j+2)/2);
    }
  }
} // getVzigzags

bool subVzigzagA(const int k) {
//   -----------
  const int kd2p1=k/2+1;
  for (int f=3; f<=kd2p1; ++f) {
    const int c=f-1; int s=f;
    while (s<=k) { if ((s==k)&(VzigA[f].ways==4)) return T; s+=c; }
  }
  return F;
} // subVzigzagA

void getVzigzagAs(int **x, const int n) {
//   ------------
  if (n>3) {
    if (Vzig[2].ways!=4) {
      isVzigzagA(x,n,3); isVzigzagA(x,n,4);
      for (int k=5; k<=n; ++k) { if (subVzigzagA(k)) continue; isVzigzagA(x,n,k); }
    }
  }
} // getVzigzagAs

bool getTypes(int **x, const int R, const int C, FILE *wfp) {
//   --------
  initGetTypes();
  if ((squareType==LS)|(squareType==DLS)) return getLatinTypes(x, yRectangle, R, wfp);
  else {
    if (R==C) {
      if ((R==1)&(squareType==normalMagic)) squareTypes[LS].isType=T;
      if (R>=3) {
        if (R==3) {
          if (squareType==normalMagic) {
            squareTypes[Associative].isType=T;
            squareTypes[Concentric].isType=T;
            squareTypes[Bordered].isType=T;
          } else if (squareType==otherMagic) isPrimeMagic(x, R);
        } else { // R>3
          if ((squareType==normalSemiMagic)|(squareType==normalMagic)) {
            isCompact(x, R); isBentDiagonal(x, R)&&isFranklin(x, R);
            isAdjacent(x, R)||isSelfComplement(x, R);
            getUzigzags(x, R); getVzigzags(x, R); getVzigzagAs(x, R);
            if (squareType==normalMagic) {
              isAssociative(x, R, C, Sum2); isPandiagonal(x, R, magicSum); isUltramagic(); isMostPerfect();
              isConcentric(x, R)||isSymlateral(x, R);
              isBiMagic(x, R)&&isTriMagic(x, R);
            }
          } else if (squareType==otherMagic) isPrimeMagic(x, R);
        }
      }
    } else { // non-square rectangle
      if (squareType==normalMagic) isAssociative(x, R, C, Sum2);
    }
    return printType(wfp);
  }
} // getTypes
//-------------------------------------------------------------------------------------------

void initGlobals() {
//  ------------
  allocatedR=0; allocatedC=0;
  if (R==C) {
    N=R; M=N-1; isMagic=isMagicSq; isNormal=isNormalSq; Nk=N/2+1;
  } else {
    N=-1; isMagic=isMagicRect; isNormal=isNormalRect;
  }
  NN=R*C; 

  readError=F; rectangleNum=0;
  if (R==C) {
    if ((N&1)==1) { // odd
      magicConstant=(NN+1)/2*N;
    } else {
      if ((N&2)==0) { // doubly even
        halfMagicConstant=(NN+1)*(N/4);
          // magicConstant/2 might give F results on overflow
        magicConstant=halfMagicConstant+halfMagicConstant; // overflow ok
      } else { // singly even
        magicConstant=(NN+1)*(N/2);
      }
      rowConstant=magicConstant; colConstant=magicConstant;
    }
  } else {
    const int S2=NN+1; rowConstant=C*S2/2; colConstant=R*S2/2;
  }
} // initGlobals
//---------------------------------------------------------------------------

void get_rest_of_line(int c) {
//   ----------------
  if (c!='\n') do { c=getchar(); } while (c!='\n');
}

bool getY() {
//   ----
  int c;  do { c=getchar(); }
          while ((c==' ')|(c=='\t')|(c=='\n'));
  get_rest_of_line(c);  return (c=='Y')|(c=='y');
}

bool getInts(int *p, int *q, int c) {
//   -------
 bool ok=F; *p=0; *q=0;

  if (c<0) do { c=getchar(); } while ((c==' ')|(c=='\t'));
  if ( ('1'<=c)&(c<='9') ) {
    int i=c-'0';
    while ( ('0'<=(c=getchar()))&(c<='9') )
      i=i*10+c-'0';
    *p=i;

    if ((c==' ')|(c=='\t')) {
      do { c=getchar(); } while ((c==' ')|(c=='\t'));
      if ( ('1'<=c)&(c<='9') ) {
        int i=c-'0';
        while ( ('0'<=(c=getchar()))&(c<='9') )
          i=i*10+c-'0';
        *q=i;
      }
    }
    if (*q==0) *q=*p; ok=T;
  }  
  get_rest_of_line(c);
  if (!ok) return reportError("Invalid input"); return T;
} // getInts

bool getYorInts(int *p, int *q) {
//   ----------
  bool ok=F; int c; *p=-1;

  do { c=getchar(); } while ((c==' ')|(c=='\t')|(c=='\n') );
  if ( (c=='Y')|(c=='y') ) ok=T;
  else if ( (c!='N')&(c!='n') ) return getInts(p, q, c);  
  get_rest_of_line(c);
  return ok;
} // getYorInts

bool getFileName(char *buf, const int size) {
//   -----------
  int c, i=0; char *s=buf;

  do { c=getchar(); }
  while ((c==' ')|(c=='\t')|(c=='\n') ); *s=c;
  while (i++<size) if ( (*++s=getchar())=='\n') break;
  if (*s!='\n') {
    printf("\nFile name too long.\n"); get_rest_of_line(*s); return F;
  }
  *s='\0';
  return T;
} // getFileName
//--------------------------------------------------------------------------

const int bufSize=1024, outSize=bufSize+50;
FILE *openInput(char *buf, const int size) {
//    ---------
  char *rFname=NULL;

  do {
    printf("\nEnter the name of the squares file: ");
    if (getFileName(buf, size-4)) {  // reserve 4 to add .txt
      rFname=buf; break;
    } else {
       printf("\a\nCan't read the file name. "
              "Try again? y (yes) or n (no) ");
	   if (!getY()) break;
    }
  } while (T);

  FILE *rfp=NULL;
  if (rFname!=NULL) {
    char *s=buf; bool txt=F;
    while (*s++!='\0');
    while (--s!=buf)
      if (*s=='.') {
        txt=(*++s=='t')&(*++s=='x')&(*++s=='t')&(*++s=='\0');
        break;
      }
    if (!txt) strcat_s(buf, size, ".txt");
    if (fopen_s(&rfp, rFname, "r")!=0) {
      const int msgSize=bufSize+50; char msg[msgSize];
      sprintf_s(msg, msgSize, "\a\nCan't open for read %s", buf);
      perror(msg);
    }
  }
  return rfp;
} // openInput
//--------------------------------------------------------------------------

FILE *openOutput(char* inFname) {
//    ----------
  char buf[outSize];
  const int baseSize=bufSize+25; char baseName[baseSize];
  char *s=inFname;
  while (*s++!='\0');
  while (--s!=inFname) if (*s=='.') { *s='\0'; break; }
  sprintf_s(baseName, baseSize, "%sTypeDetail", inFname);
  sprintf_s(buf, outSize, "%s.txt", baseName);
  int sub=0;

  do {
    if (_access_s(buf, 00)==ENOENT)
      break;
    else
      sprintf_s(buf, outSize, "%s_%d.txt", baseName, ++sub);
  } while (T);

  FILE *wfp=NULL;
  if (fopen_s(&wfp, buf, "w")==0)
    printf(".. writing type information to file %s\n", buf);
  else {
    char msg[outSize];
    sprintf_s(msg, outSize, "\a\nCan't open for write %s", buf);
    perror(msg);
  }
  return wfp;
} // openOutput
//-------------------------------------------------------------------------------------------------

void printCounts() {
//   -----------
  printf("\nCounts\n------\n");
  for (int i=firstType; i<=lastType; ++i) {
     if (typeCount[i]!=0) printf("%10d %s\n", typeCount[i], squareTypes[i].name);
     switch (i) {
       case Compact:
         if (typeCountCompact[0]!=0) {
           for (int j=3; j<Nk; ++j) if (typeCountCompact[j]!=0) {
             printf("%10d %s%d\n", typeCountCompact[j], squareTypes[i].name, j);
           }
         }
         break;
      case bentDiagonal:
        if (typeCountBent[0]!=0) {
          for (int j=3; j>0; --j) if (typeCountBent[j]!=0) {
            printf("%10d %s %d-way\n", typeCountBent[j], squareTypes[i].name, j);
          }
        }
        break;
      case Uzigzag:
        if (typeCountUzig[0].x[0]!=0) { // there are 1,2, or 3 way 2 or other U zigzag k
          if (typeCountUzig[2].x[0]!=0) {
            for (int j=3; j>0; --j) if (typeCountUzig[2].x[j]!=0)
              printf("%10d %s %d-way\n", typeCountUzig[2].x[j], squareTypes[i].name, j);
          }
          for (int j=3; j<Nk; ++j) if (typeCountUzig[j].x[0]!=0) {
            if (typeCountUzig[j].x[4]!=0)
              printf("%10d %s%d\n", typeCountUzig[j].x[4], squareTypes[i].name, j);
            for (int k=3; k>0; --k) if (typeCountUzig[j].x[k]!=0) {
              printf("%10d %s%d %d-way\n", typeCountUzig[j].x[k], squareTypes[i].name, j, k);
            }
          }
        }
        break;
      case Vzigzag:
        if (typeCountVzig[0].x[0]!=0) { // there are 1,2, or 3 way 2 or other V zigzag k
          if (typeCountVzig[2].x[0]!=0) {
            for (int j=3; j>0; --j) if (typeCountVzig[2].x[j]!=0)
              printf("%10d %s %d-way\n", typeCountVzig[2].x[j], squareTypes[i].name, j);
          }
          for (int j=3; j<Nk; ++j) if (typeCountVzig[j].x[0]!=0) {
            if (typeCountVzig[j].x[4]!=0)
              printf("%10d %s%d\n", typeCountVzig[j].x[4], squareTypes[i].name, j);
            for (int k=3; k>0; --k) if (typeCountVzig[j].x[k]!=0) {
              printf("%10d %s%d %d-way\n", typeCountVzig[j].x[k], squareTypes[i].name, j, k);
            }
          }
        }
        break;
      case VzigzagA:
        if (typeCountVzigA[0].x[0]!=0) { // there are V zigzagA k
          //if (typeCountVzigA[3].x[0]!=0) {
          //  for (int j=3; j>0; --j) if (typeCountVzigA[3].x[j]!=0)
          //    printf("%10d %s %d-way\n", typeCountVzigA[3].x[j], squareTypes[i].name, j);
          //}
          for (int j=3; j<=N; ++j) if (typeCountVzigA[j].x[0]!=0) {
            if (typeCountVzigA[j].x[4]!=0)
              printf("%10d %s%d\n", typeCountVzigA[j].x[4], squareTypes[i].name, j);
            for (int k=3; k>0; --k) if (typeCountVzigA[j].x[k]!=0) {
              printf("%10d %s%d %d-way\n", typeCountVzigA[j].x[k], squareTypes[i].name, j, k);
            }
          }
        }
        break;
      default: break;
    }
  }
} // printCounts
//--------------------------------------------------------------------------------------------------

void outputLocalTime() {
//   --------------
  time_t startTime=time(NULL);
  struct tm local;  localtime_s(&local, &startTime);  char dateTime[100];
  size_t slen=strftime(dateTime, 100, "%A %Y-%m-%d %X %Z\n\n\0", &local);

  printf(dateTime);
} // outputLocalTime
//-----------------------------------------------------------------------------------

void printElapsedTime(time_t startTime) {
//   ---------------- 
  const int et=(int)difftime(time(NULL), startTime);
  if (et>0)
    printf("\nelapsed time %d:%02d:%02d\n", et/3600, et%3600/60, et%60);
} // printElapsedTime
//------------------------------------------------------------------------------------

void initTypeCounts() {
//   --------------
  for (int i=firstType; i<=lastType; ++i) typeCount[i]=0;
  for (int i=0; i<5; ++i) typeCountBent[i]=0;
  for (int i=0; i<Nk; ++i) {
    for (int j=0; j<5; ++j) { typeCountUzig[i].x[j]=0; typeCountVzig[i].x[j]=0; }
    typeCountCompact[i]=0;
  }
  for (int i=0; i<=N; ++i) for (int j=0; j<5; ++j) typeCountVzigA[i].x[j]=0;
} // initTypeCounts

int main() {
//  ----
  bool inputOrder=T;
  const int inSize=bufSize+4; // +4 to add .txt if needed
  char buf[inSize]; int squaresFiles=0;

  outputLocalTime();
  do {
    if (inputOrder) { printf("\nOrder? "); if (!getInts(&R, &C, -1)) break; }
    if ( allocateStore() ) {
      initGlobals(); FILE *rfp=openInput(buf, inSize);
      time_t startTime=time(NULL);

      if (rfp!=NULL) {
        //FILE *wfp=NULL; { // no detail
        FILE *wfp=openOutput(buf);
        if (wfp!=NULL) {
          bool writeError=F; rectangleNum=0; initTypeCounts();
          while (readRectangle(rfp)) {
            writeError=!getTypes(xRectangle, R, C, wfp);
            prevType=squareType;
            if (writeError) { perror("\n\aError writing file"); break; }
          }
          if (writeError) break; printCounts(); ++squaresFiles; fclose(wfp);
        }
        fclose(rfp);
      } // rfp!=NULL
      printElapsedTime(startTime);
    } // allocateSquares
    printf("\nAnother order? input y (yes), n (no) or the order: ");
    if (getYorInts(&R, &C)) inputOrder=(R<0); else break;
  } while (T);

  freeStore();
  printf("\nPress a key to close the console.");
  while (!_kbhit()) Sleep(250); // Win32
  return 0;
} // main