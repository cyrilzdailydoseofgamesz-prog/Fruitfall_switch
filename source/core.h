// Fruitfall remake core: rules only, no platform code.
#pragma once
#include <string.h>
#define GW 10
#define GH 10
typedef struct { char c[GH][GW+1]; } Grid;

static inline int is_fruit(char ch){ return ch=='a'||ch=='b'||ch=='o'||ch=='s'; }

// gravity dir: 0=down 1=left 2=up 3=right
static const int DX[4]={0,-1,0,1}, DY[4]={1,0,-1,0};

// Move every fruit one step in dir if free. Returns 1 if anything moved.
static inline int fall_step(Grid*g,int dir){
    int moved=0;
    int xs=(DX[dir]>0)?GW-1:0, xe=(DX[dir]>0)?-1:GW, xd=(DX[dir]>0)?-1:1;
    int ys=(DY[dir]>0)?GH-1:0, ye=(DY[dir]>0)?-1:GH, yd=(DY[dir]>0)?-1:1;
    for(int y=ys;y!=ye;y+=yd) for(int x=xs;x!=xe;x+=xd){
        if(!is_fruit(g->c[y][x])) continue;
        int nx=x+DX[dir], ny=y+DY[dir];
        if(nx<0||ny<0||nx>=GW||ny>=GH) continue;
        if(g->c[ny][nx]=='.'){ g->c[ny][nx]=g->c[y][x]; g->c[y][x]='.'; moved=1; }
    }
    return moved;
}
static inline void settle(Grid*g,int dir){ while(fall_step(g,dir)); }

// Mark groups of >=3 touching same fruit. Returns number cleared (mask in out[][]).
static inline int find_matches(const Grid*g,char out[GH][GW]){
    int seen[GH][GW]; memset(seen,0,sizeof seen); memset(out,0,GH*GW);
    int total=0;
    for(int y=0;y<GH;y++) for(int x=0;x<GW;x++){
        char f=g->c[y][x]; if(!is_fruit(f)||seen[y][x]) continue;
        int st[GH*GW][2],n=0,sp=0; st[sp][0]=x; st[sp][1]=y; sp++; seen[y][x]=1;
        int mem[GH*GW][2];
        while(sp){ sp--; int cx=st[sp][0],cy=st[sp][1]; mem[n][0]=cx; mem[n][1]=cy; n++;
            for(int d=0;d<4;d++){ int nx=cx+DX[d],ny=cy+DY[d];
                if(nx<0||ny<0||nx>=GW||ny>=GH||seen[ny][nx]||g->c[ny][nx]!=f) continue;
                seen[ny][nx]=1; st[sp][0]=nx; st[sp][1]=ny; sp++; } }
        if(n>=3){ for(int i=0;i<n;i++) out[mem[i][1]][mem[i][0]]=1; total+=n; }
    }
    return total;
}
static inline int clear_matches(Grid*g){
    char m[GH][GW]; int n=find_matches(g,m);
    for(int y=0;y<GH;y++) for(int x=0;x<GW;x++) if(m[y][x]) g->c[y][x]='.';
    return n;
}
// Full resolve: settle, clear, repeat. Returns chain length via *chain, fruit cleared total.
static inline int resolve(Grid*g,int dir,int*chain){
    int total=0; *chain=0; settle(g,dir);
    for(;;){ int n=clear_matches(g); if(!n) break; total+=n; (*chain)++; settle(g,dir); }
    return total;
}
static inline int fruit_left(const Grid*g){
    int n=0; for(int y=0;y<GH;y++) for(int x=0;x<GW;x++) n+=is_fruit(g->c[y][x]); return n;
}
