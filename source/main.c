// Fruitfall remake for Nintendo Switch (SDL2). Rules in core.h, data in romfs.
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __SWITCH__
#include <switch.h>
#include <sys/stat.h>
#define ROM "romfs:/"
#define SAVE "sdmc:/switch/fruitfall.sav"
#else
#define ROM "romfs/"
#define SAVE "fruitfall.sav"
#endif
#include "core.h"
#include "font.h"

#define SW 1280
#define SH 720
typedef struct{int world,limit;char name[32];Grid g;}Level;
static Level LV[100]; static int NL;
static SDL_Window*win; static SDL_Renderer*ren;

// ---------- audio ----------
typedef struct{Uint8*buf;Uint32 len;}Snd;
typedef struct{Snd*s;Uint32 pos;int loop,on;float vol;}Voice;
static Snd SN[32]; static const char*SNAME[]={"back","bonus1","combo1","combo2","combo3","combo4","combo5","combo6","combo7","cursor","error","frithit1","frithit2","frithit3","nowin2","rotate1","rotate2","select","timer","title"};
enum{S_BACK,S_BONUS,S_COMBO1,S_COMBO2,S_COMBO3,S_COMBO4,S_COMBO5,S_COMBO6,S_COMBO7,S_CURSOR,S_ERROR,S_HIT1,S_HIT2,S_HIT3,S_NOWIN,S_ROT1,S_ROT2,S_SELECT,S_TIMER,S_TITLE,S_N};
static Voice VO[16]; static SDL_AudioDeviceID adev; static int musicOn=0;
static void mixcb(void*u,Uint8*st,int n){ (void)u; Sint16*o=(Sint16*)st; int ns=n/2; memset(st,0,n);
    for(int v=0;v<16;v++){ Voice*p=&VO[v]; if(!p->on)continue;
        for(int i=0;i<ns;i++){ if(p->pos+2>p->s->len){ if(p->loop)p->pos=0; else{p->on=0;break;} }
            int x=o[i]+(int)(*(Sint16*)(p->s->buf+p->pos)*p->vol); if(x>32767)x=32767; if(x<-32768)x=-32768; o[i]=x; p->pos+=2; } } }
static void play(int id,float vol,int loop){ if(!adev||!SN[id].buf)return; SDL_LockAudioDevice(adev);
    int f=-1; for(int v=0;v<16;v++) if(!VO[v].on){f=v;break;} if(f<0)f=0;
    VO[f]=(Voice){&SN[id],0,loop,1,vol}; SDL_UnlockAudioDevice(adev); }
static void stop_music(void){ SDL_LockAudioDevice(adev); for(int v=0;v<16;v++) if(VO[v].on&&VO[v].s==&SN[S_TITLE])VO[v].on=0; SDL_UnlockAudioDevice(adev); musicOn=0; }
static void audio_init(void){ SDL_AudioSpec want,have; SDL_zero(want); want.freq=44100; want.format=AUDIO_S16SYS; want.channels=1; want.samples=1024; want.callback=mixcb;
    adev=SDL_OpenAudioDevice(NULL,0,&want,&have,0); if(!adev)return;
    for(int i=0;i<S_N;i++){ char p[128]; snprintf(p,sizeof p,ROM"sfx/%s.wav",SNAME[i]); SDL_AudioSpec ws; Uint8*b; Uint32 l;
        if(!SDL_LoadWAV(p,&ws,&b,&l)) continue;
        SDL_AudioCVT cvt; if(SDL_BuildAudioCVT(&cvt,ws.format,ws.channels,ws.freq,AUDIO_S16SYS,1,44100)>0){ cvt.len=l; cvt.buf=malloc(l*cvt.len_mult); memcpy(cvt.buf,b,l); SDL_ConvertAudio(&cvt); SDL_FreeWAV(b); SN[i].buf=cvt.buf; SN[i].len=cvt.len_cvt; }
        else { SN[i].buf=b; SN[i].len=l; } }
    SDL_PauseAudioDevice(adev,0); }

// ---------- drawing ----------
static void rect(int x,int y,int w,int h,int r,int g,int b,int a){ SDL_SetRenderDrawColor(ren,r,g,b,a); SDL_Rect q={x,y,w,h}; SDL_RenderFillRect(ren,&q); }
static void disc(int cx,int cy,int rad,int r,int g,int b){ SDL_SetRenderDrawColor(ren,r,g,b,255); for(int dy=-rad;dy<=rad;dy++){ int dx=(int)sqrt((double)(rad*rad-dy*dy)); SDL_RenderDrawLine(ren,cx-dx,cy+dy,cx+dx,cy+dy);} }
static void text(const char*s,int x,int y,int sc,int r,int g,int b){ for(;*s;s++){ int c=*s; if(c>='a'&&c<='z')c-=32; if(c>=32&&c<96){ const char*gl=FONT3x5[c-32];
        for(int i=0;i<15;i++) if(gl[i]=='1') rect(x+(i%3)*sc,y+(i/3)*sc,sc,sc,r,g,b,255); } x+=4*sc; } }
static int twidth(const char*s,int sc){ return (int)strlen(s)*4*sc; }
static void ctext(const char*s,int y,int sc,int r,int g,int b){ text(s,(SW-twidth(s,sc))/2,y,sc,r,g,b); }
static const int FC[4][3]={{230,50,60},{245,205,40},{250,140,30},{170,80,210}};
static int fidx(char c){ return c=='a'?0:c=='b'?1:c=='o'?2:3; }
static void fruit(int cx,int cy,int sz,char c,int flash){ int k=fidx(c); int rad=sz*42/100; if(flash){ disc(cx,cy,rad+4,255,255,255); }
    disc(cx,cy,rad,FC[k][0],FC[k][1],FC[k][2]); disc(cx-rad/3,cy-rad/3,rad/4,255,255,255);
    int m=rad/3; SDL_SetRenderDrawColor(ren,40,30,30,255);
    if(k==0){ rect(cx-2,cy-rad-4,4,10,60,150,50,255); }
    if(k==1){ rect(cx-m,cy+rad/4,2*m,3,40,30,30,255); }
    if(k==2){ rect(cx-2,cy-rad-3,4,8,60,150,50,255); rect(cx-m,cy+rad/4,m*2,3,200,90,10,255); }
    if(k==3){ rect(cx-m,cy+rad/5,3,3,255,255,255,255); rect(cx+m,cy+rad/5,3,3,255,255,255,255); } }

// ---------- game state ----------
enum{ST_TITLE,ST_MODE,ST_LEVELS,ST_PLAY,ST_WIN,ST_LOSE};
static int st=ST_TITLE, mode=0 /*0 arcade 1 tight turn*/, world=0, cursel=0, cur=0, unlocked[100], bestT[100], bestScore[100];
static Grid G; static int gdir, turns, score, chain, phase /*0 idle 1 falling 2 flash*/, tick, flashN, hits, tleft_ms, lowSecs;
static char fl[GH][GW]; static Uint32 lastT;

static void load_levels(void){ char p[128]; snprintf(p,sizeof p,ROM"levels.txt"); FILE*f=fopen(p,"r"); if(!f){NL=0;return;} if(fscanf(f,"%d",&NL)!=1){NL=0;fclose(f);return;}
    for(int i=0;i<NL;i++){ if(fscanf(f,"%d %d %31s",&LV[i].world,&LV[i].limit,LV[i].name)!=3)break; for(int y=0;y<GH;y++) if(fscanf(f,"%10s",LV[i].g.c[y])!=1)break; for(char*q=LV[i].name;*q;q++) if(*q=='_')*q=' '; LV[i].world-=1; if(LV[i].world<0)LV[i].world=0; }
    fclose(f); }
static void save(void){ FILE*f=fopen(SAVE,"wb"); if(!f)return; fwrite(unlocked,sizeof unlocked,1,f); fwrite(bestT,sizeof bestT,1,f); fwrite(bestScore,sizeof bestScore,1,f); fclose(f); }
static void loadsave(void){ FILE*f=fopen(SAVE,"rb"); if(!f)return; if(fread(unlocked,sizeof unlocked,1,f)!=1)memset(unlocked,0,sizeof unlocked); if(fread(bestT,sizeof bestT,1,f)!=1)memset(bestT,0,sizeof bestT); if(fread(bestScore,sizeof bestScore,1,f)!=1)memset(bestScore,0,sizeof bestScore); fclose(f); }
static int widx(int w,int i){ int n=0; for(int k=0;k<NL;k++) if(LV[k].world==w){ if(n==i)return k; n++; } return -1; }
static int wcount(int w){ int n=0; for(int k=0;k<NL;k++) n+=(LV[k].world==w); return n; }
static void start_level(int idx){ cur=idx; G=LV[idx].g; gdir=0; turns=0; score=0; chain=0; phase=1; tick=0; hits=0; tleft_ms=LV[idx].limit*1000; lowSecs=99; st=ST_PLAY; }
static void do_rotate(int cw){ if(phase!=0)return; gdir=(gdir+(cw?3:1))&3; turns++; phase=1; tick=0; chain=0; play(cw?S_ROT1:S_ROT2,0.6f,0); }
static void step_game(Uint32 dt){
    if(st!=ST_PLAY)return;
    if(mode==0 && phase!=3){ tleft_ms-=dt; int s=tleft_ms/1000; if(s<5&&s<lowSecs&&tleft_ms>0){ lowSecs=s; play(S_TIMER,0.7f,0);} if(tleft_ms<=0){ tleft_ms=0; st=ST_LOSE; play(S_NOWIN,0.8f,0); return; } }
    tick+=dt; if(phase==1){ if(tick>=55){ tick=0; if(!fall_step(&G,gdir)){ if(find_matches(&G,fl)){ phase=2; flashN=0; play(S_HIT1+(hits++%3),0.7f,0); } else { phase=0; if(!fruit_left(&G)) goto won; if(mode==0&&0){} } } } }
    else if(phase==2){ if(tick>=260){ tick=0; int n=clear_matches(&G); chain++; score+=10*n*chain; if(chain>=2) play(S_COMBO1+((chain-2)>6?6:(chain-2)),0.7f,0); phase=1; } }
    return;
won: st=ST_WIN; play(S_BONUS,0.9f,0); { int bonus=mode==0? tleft_ms/1000*5:0; score+=bonus;
    if(cur>=0){ int k=0,w=LV[cur].world; for(int i=0;i<wcount(w);i++) if(widx(w,i)==cur)k=i; if(unlocked[w]<k+1) unlocked[w]=k+1;
        if(!bestT[cur]||turns<bestT[cur])bestT[cur]=turns; if(score>bestScore[cur])bestScore[cur]=score; save(); } } }

// ---------- rendering ----------
static void board_xy(int x,int y,int*u,int*v){ switch(gdir){case 0:*u=x;*v=y;break;case 1:*u=y;*v=GW-1-x;break;case 2:*u=GW-1-x;*v=GH-1-y;break;default:*u=GH-1-y;*v=x;} }
static void draw_play(void){
    rect(0,0,SW,SH,20,24,50,255); int cs=62, bx=(SW-cs*GW)/2-200, by=(SH-cs*GH)/2; rect(bx-8,by-8,cs*GW+16,cs*GH+16,10,12,30,255);
    for(int y=0;y<GH;y++) for(int x=0;x<GW;x++){ int u,v; board_xy(x,y,&u,&v); int px=bx+u*cs, py=by+v*cs; char c=G.c[y][x];
        if(c=='x') { rect(px,py,cs,cs,90,95,130,255); rect(px+3,py+3,cs-6,cs-6,70,75,110,255); }
        else { if((x+y)&1) rect(px,py,cs,cs,34,38,72,255); else rect(px,py,cs,cs,30,34,66,255); if(is_fruit(c)) fruit(px+cs/2,py+cs/2,cs,c,phase==2&&fl[y][x]&&((tick/70)&1)); } }
    int tx=bx+cs*GW+60; char b[64]; text(LV[cur].name,tx,70,5,255,255,255); snprintf(b,sizeof b,"SCORE %d",score); text(b,tx,150,4,255,230,120);
    snprintf(b,sizeof b,"TURNS %d",turns); text(b,tx,210,4,200,230,255);
    if(mode==0){ snprintf(b,sizeof b,"TIME %d",tleft_ms/1000); text(b,tx,270,4,tleft_ms<5000?255:200,tleft_ms<5000?90:230,90); }
    snprintf(b,sizeof b,"FRUIT %d",fruit_left(&G)); text(b,tx,330,4,200,255,200);
    text("L/R OR LEFT/RIGHT: ROTATE",tx,440,2,200,200,220); text("X: RESTART  Y: SKIP  B: MENU",tx,470,2,200,200,220);
    int ax=tx+60, ay=560; rect(ax-3,ay-30,6,40,255,255,255,255); rect(ax-14,ay+4,28,6,255,255,255,255); text("DOWN",tx+110,ay-10,3,255,255,255); }
static void draw_title(void){ rect(0,0,SW,SH,25,20,60,255); for(int i=0;i<12;i++){fruit(120+i*95,120+(int)(20*sin(SDL_GetTicks()/400.0+i)),90,"abos"[i%4],0); }
    ctext("FRUITFALL",260,14,255,240,120); ctext("PRESS A TO START",470,5,255,255,255); ctext("FAN REMAKE FOR SWITCH",600,3,170,180,220); (void)0; }
static void draw_menu(const char*title,const char**items,int n,int sel){ rect(0,0,SW,SH,25,20,60,255); ctext(title,80,8,255,240,120);
    for(int i=0;i<n;i++){ int y=240+i*90; if(i==sel) rect(300,y-14,680,74,70,60,140,255); ctext(items[i],y,6,i==sel?255:180,i==sel?255:180,i==sel?255:200);} ctext("A: SELECT   B: BACK",650,3,170,180,220); }
static void draw_levels(void){ rect(0,0,SW,SH,25,20,60,255); const char*wn[]={"GHOST WORLD","SEA WORLD","SPACE WORLD"}; ctext(wn[world],40,7,255,240,120);
    int n=wcount(world); for(int i=0;i<n;i++){ int c=i%5,r=i/5,x=340+c*120,y=150+r*100; int lk=(i>unlocked[world]+0 && i>unlocked[world]); int k=widx(world,i);
        rect(x,y,100,80,i==cursel?255:60,i==cursel?220:60,i==cursel?120:110,255); char b[8]; snprintf(b,8,"%d",i+1); text(b,x+14,y+14,5,i==cursel?20:230,i==cursel?20:230,i==cursel?20:230);
        if(k>=0&&bestT[k]){ snprintf(b,8,"%d",bestT[k]); text(b,x+14,y+56,2,120,255,120);} (void)lk; }
    ctext("A: PLAY   B: BACK   L/R: WORLD",650,3,170,180,220); }
static void draw_overlay(const char*t,const char*s){ rect(240,220,800,280,0,0,0,220); ctext(t,250,9,255,240,120); ctext(s,400,3,220,220,255); }

// ---------- input ----------
static int pressed[16], kb[8];
enum{B_A,B_B,B_X,B_Y,B_L=6,B_R,B_ZL,B_ZR,B_PLUS,B_MINUS,B_LEFT,B_UP,B_RIGHT,B_DOWN};
static int hit(int b){ int r=pressed[b]; return r; }
static void handle(void){
    int A=hit(B_A),B=hit(B_B),U=hit(B_UP),D=hit(B_DOWN),Lf=hit(B_LEFT),Rt=hit(B_RIGHT),L=hit(B_L)||hit(B_ZL),R=hit(B_R)||hit(B_ZR);
    switch(st){
    case ST_TITLE: if(!musicOn&&adev){ play(S_TITLE,0.5f,1); musicOn=1; } if(A||hit(B_PLUS)){ play(S_SELECT,0.7f,0); st=ST_MODE; cursel=mode; } break;
    case ST_MODE: { if(U||D){ mode^=1; play(S_CURSOR,0.7f,0);} if(A){ play(S_SELECT,0.7f,0); st=ST_LEVELS; cursel=0; } if(B){ play(S_BACK,0.7f,0); st=ST_TITLE; } break; }
    case ST_LEVELS: { int n=wcount(world); if(Lf&&cursel>0){cursel--;play(S_CURSOR,0.7f,0);} if(Rt&&cursel<n-1){cursel++;play(S_CURSOR,0.7f,0);} if(U&&cursel>=5){cursel-=5;play(S_CURSOR,0.7f,0);} if(D&&cursel+5<n){cursel+=5;play(S_CURSOR,0.7f,0);}
        if(L){world=(world+2)%3;cursel=0;play(S_CURSOR,0.7f,0);} if(R){world=(world+1)%3;cursel=0;play(S_CURSOR,0.7f,0);}
        if(A){ int k=widx(world,cursel); if(k>=0){ stop_music(); play(S_SELECT,0.7f,0); start_level(k);} } if(B){ play(S_BACK,0.7f,0); st=ST_MODE; cursel=mode; } break; }
    case ST_PLAY: if(L||Lf) do_rotate(0); if(R||Rt) do_rotate(1); if(hit(B_X)||hit(B_PLUS)) start_level(cur); if(hit(B_Y)){ int nx=cur+1; if(nx<NL&&LV[nx].world==LV[cur].world) start_level(nx); else play(S_ERROR,0.7f,0);} if(B){ play(S_BACK,0.7f,0); st=ST_LEVELS; } break;
    case ST_WIN: if(A){ int nx=cur+1; if(nx<NL&&LV[nx].world==LV[cur].world) start_level(nx); else { st=ST_LEVELS; cursel=0; } } if(B){ st=ST_LEVELS; } break;
    case ST_LOSE: if(A) start_level(cur); if(B){ st=ST_LEVELS; } break; }
    if(st==ST_LEVELS||st==ST_MODE||st==ST_TITLE){ if(!musicOn&&adev){ play(S_TITLE,0.5f,1); musicOn=1; } } }

int main(int argc,char**argv){ (void)argc;(void)argv;
#ifdef __SWITCH__
    romfsInit(); mkdir("sdmc:/switch",0777);
#endif
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK|SDL_INIT_TIMER);
    win=SDL_CreateWindow("Fruitfall",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SW,SH,0); ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND); SDL_RenderSetLogicalSize(ren,SW,SH);
    SDL_Joystick*js=NULL; for(int i=0;i<SDL_NumJoysticks();i++){ js=SDL_JoystickOpen(i); }
    load_levels(); loadsave(); audio_init(); lastT=SDL_GetTicks();
    const char*modes[]={"ARCADE","TIGHT TURN"}; int run=1;
    while(run){ memset(pressed,0,sizeof pressed); SDL_Event e;
        while(SDL_PollEvent(&e)){ if(e.type==SDL_QUIT)run=0;
            if(e.type==SDL_JOYBUTTONDOWN){ if(e.jbutton.button<16)pressed[e.jbutton.button]=1; if(e.jbutton.button==B_MINUS)run=0; }
            if(e.type==SDL_KEYDOWN&&!e.key.repeat){ switch(e.key.keysym.sym){case SDLK_RETURN:case SDLK_z:pressed[B_A]=1;break;case SDLK_ESCAPE:case SDLK_x:pressed[B_B]=1;break;case SDLK_r:pressed[B_X]=1;break;case SDLK_n:pressed[B_Y]=1;break;case SDLK_LEFT:pressed[B_LEFT]=1;break;case SDLK_RIGHT:pressed[B_RIGHT]=1;break;case SDLK_UP:pressed[B_UP]=1;break;case SDLK_DOWN:pressed[B_DOWN]=1;break;case SDLK_a:pressed[B_L]=1;break;case SDLK_s:pressed[B_R]=1;break;} } }
        Uint32 now=SDL_GetTicks(),dt=now-lastT; lastT=now; if(dt>100)dt=100;
        handle(); step_game(dt);
        switch(st){ case ST_TITLE: draw_title(); break; case ST_MODE: draw_menu("SELECT MODE",modes,2,mode); break; case ST_LEVELS: draw_levels(); break;
            case ST_PLAY: draw_play(); break;
            case ST_WIN: { draw_play(); char b[64]; snprintf(b,64,"SCORE %d   TURNS %d",score,turns); draw_overlay("LEVEL CLEAR",b); ctext("A: NEXT   B: LEVELS",440,3,255,255,255); break; }
            case ST_LOSE: draw_play(); draw_overlay("TIME UP","A: RETRY   B: LEVELS"); break; }
        SDL_RenderPresent(ren); }
    save();
#ifdef __SWITCH__
    romfsExit();
#endif
    SDL_Quit(); return 0; }
