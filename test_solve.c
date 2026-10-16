#include <stdio.h>
#include <stdlib.h>
#include "source/core.h"
typedef struct{int world,limit;char name[32];Grid g;}Level;
static Level L[100];
// BFS over (grid, orientation). Move = rotate +1 or -1 (gravity dir changes).
#define HS (1<<22)
typedef struct{Grid g;int dir;int depth;}Node;
static Node*q; static unsigned long long*hs;
static unsigned long long hsh(const Grid*g,int d){unsigned long long h=1469598103934665603ULL;for(int y=0;y<GH;y++)for(int x=0;x<GW;x++){h^=g->c[y][x];h*=1099511628211ULL;}h^=d;h*=1099511628211ULL;return h|1;}
static int seen(unsigned long long h){unsigned i=h&(HS-1);while(hs[i]){if(hs[i]==h)return 1;i=(i+1)&(HS-1);}hs[i]=h;return 0;}
int solve(Level*lv,int maxd){
    memset(hs,0,sizeof(unsigned long long)*HS);
    Grid g=lv->g; int ch; resolve(&g,0,&ch);
    int qh=0,qt=0; q[qt++]=(Node){g,0,0}; seen(hsh(&g,0));
    if(!fruit_left(&g)) return 0;
    while(qh<qt){ Node n=q[qh++]; if(n.depth>=maxd) continue;
        for(int s=-1;s<=1;s+=2){ Grid h=n.g; int nd=(n.dir+s+4)&3; resolve(&h,nd,&ch);
            if(!fruit_left(&h)) return n.depth+1;
            if(seen(hsh(&h,nd))) continue; if(qt<3000000) q[qt++]=(Node){h,nd,n.depth+1}; } }
    return -1;
}
int main(int argc,char**argv){
    FILE*f=fopen("romfs/levels.txt","r"); int n; fscanf(f,"%d",&n);
    for(int i=0;i<n;i++){ fscanf(f,"%d %d %31s",&L[i].world,&L[i].limit,L[i].name);
        for(int y=0;y<GH;y++) fscanf(f,"%10s",L[i].g.c[y]); }
    q=malloc(sizeof(Node)*3000000); hs=calloc(HS,8);
    int ok=0;
    for(int i=(argc>1?atoi(argv[1]):0);i<n;i++){ int r=solve(&L[i],40); printf("L%d w%d limit %d -> %d\n",i+1,L[i].world,L[i].limit,r); ok+=(r>=0); fflush(stdout);}    
    printf("solvable %d/%d\n",ok,n);
}
