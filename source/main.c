/*
 * ALFIE'S ADVENTURE v0.1 -- Nintendo Wii Homebrew
 * Uses only devkitPro's libogc (no copied game assets).
 * Hold Wii Remote sideways: D-Pad for movement, 2 to jump, + restart, HOME exit.
 */
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

#define FIFO_SIZE (256 * 1024)
#define PI 3.14159265358979323846f

static GXRModeObj *video_mode;
static void *framebuffers[2];
static unsigned char *fifo;
static int front = 0;
static Mtx44 projection;
static Mtx model;

typedef struct { u8 r,g,b; } Color;
static const Color DARK = {30,43,61};
static const Color WHITE = {255,255,249};
static const Color GOLD = {252,192,67};
static const Color BROWN = {139,88,64};
static const Color GRASS = {94,183,103};

static void vtx(float x, float y, Color c) {
    GX_Position3f32(x,y,0.0f);
    GX_Color4u8(c.r,c.g,c.b,255);
}
static void rect(float x,float y,float w,float h,Color c) {
    if (w<=0 || h<=0) return;
    GX_Begin(GX_QUADS,GX_VTXFMT0,4);
    vtx(x,y,c);vtx(x+w,y,c);vtx(x+w,y+h,c);vtx(x,y+h,c);
    GX_End();
}
static void triangle(float x1,float y1,float x2,float y2,float x3,float y3,Color c) {
    GX_Begin(GX_TRIANGLES,GX_VTXFMT0,3);
    vtx(x1,y1,c);vtx(x2,y2,c);vtx(x3,y3,c);
    GX_End();
}
static void disc(float x,float y,float rad,Color c) {
    int i;
    GX_Begin(GX_TRIANGLEFAN,GX_VTXFMT0,22);
    vtx(x,y,c);
    for(i=0;i<=20;i++){
        float a=2.0f*PI*(float)i/20.0f;
        vtx(x+cosf(a)*rad,y+sinf(a)*rad,c);
    }
    GX_End();
}

typedef struct {char letter;u8 bits[7];} Glyph;
static const Glyph alphabet[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{31,4,4,4,4,4,31}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {'!',{4,4,4,4,4,0,4}}, {':',{0,4,4,0,4,4,0}},
 {'-',{0,0,0,31,0,0,0}}, {'+', {0,4,4,31,4,4,0}},
 {'/',{1,1,2,4,8,16,16}}, {'\'', {4,4,8,0,0,0,0}},
 {'.',{0,0,0,0,0,0,4}}, {'?',{14,17,1,2,4,0,4}}
};

static void draw_text(float x,float y,const char *s,float scale,Color c) {
    while (*s) {
        const u8 *bits=NULL;
        unsigned int k;int row,col;
        if (*s==' ') {x+=6*scale;s++;continue;}
        for(k=0;k<sizeof(alphabet)/sizeof(alphabet[0]);k++) {
            if(alphabet[k].letter==*s){bits=alphabet[k].bits;break;}
        }
        if(bits){
            for(row=0;row<7;row++)for(col=0;col<5;col++){
                if(bits[row] & (1<<(4-col))) rect(x+col*scale,y+row*scale,scale,scale,c);
            }
        }
        x+=6*scale;s++;
    }
}

static void dog(float x,float y,int facing,int tick) {
    Color fur={191,129,83},cream={232,198,147},ear={93,69,58},black={38,42,47};
    float headx=facing>0?x+25:x+1;
    /* wagging tail */
    if(facing>0) {
        rect(x-5,y+13,15,7,fur);
        triangle(x-6,y+12-(tick%14<7?3:0),x+5,y+11,x+2,y+22,fur);
    } else {
        rect(x+35,y+13,15,7,fur);
        triangle(x+52,y+12-(tick%14<7?3:0),x+39,y+11,x+43,y+22,fur);
    }
    rect(x+8,y+10,30,21,fur);
    rect(x+12,y+14,12,15,cream);
    rect(x+9,y+29,8,9,fur);rect(x+29,y+29,8,9,fur);
    rect(headx,y+2,21,25,fur);
    rect(facing>0?headx:headx+14,y+4,7,23,ear);
    rect(facing>0?headx+14:headx-5,y+17,11,10,cream);
    rect(facing>0?headx+17:headx-5,y+17,5,5,black);
    rect(facing>0?headx+12:headx+5,y+9,4,4,black);
    rect(x+18,y+10,7,8,ear);  /* Alfie's dark coat patch */
}
static void bone(float x,float y,int bob) {
    Color light={255,249,218},shadow={221,175,93};
    y+=bob;
    disc(x-6,y-4,5,shadow);disc(x-6,y+5,5,shadow);
    disc(x+13,y-4,5,shadow);disc(x+13,y+5,5,shadow);
    rect(x-6,y-5,20,12,light);
    disc(x-6,y-3,4,light);disc(x-6,y+5,4,light);
    disc(x+14,y-3,4,light);disc(x+14,y+5,4,light);
}

static void background(const Game *g) {
    int i;float cx=g->camera;
    Color sky={137,216,235},cloud={249,250,238},hill={119,192,145},hill2={101,178,143};
    Color trunk={139,104,78},tree={76,155,116},darkgrass={61,142,91},wood={165,115,74};
    rect(0,0,640,480,sky);
    disc(555-cx*.05f,92,37,(Color){255,224,119});
    for(i=0;i<5;i++){
        float bx=i*210-cx*.25f;
        disc(bx+30,255,105,cloud); /* distant rolling cloud-like hills */
        rect(bx-80,255,250,50,hill);
    }
    rect(0,304,640,100,hill);
    for(i=0;i<7;i++){
        float bx=i*205-cx*.37f;
        disc(bx,320,96,hill2);
    }
    for(i=0;i<11;i++){
        float tx=70+i*135-cx*.68f;
        if(tx<-60||tx>700)continue;
        rect(tx+16,273,13,120,trunk);
        disc(tx+22,264,35,tree);
        disc(tx+2,281,26,tree);
        disc(tx+42,285,27,tree);
    }
    rect(0,GROUND_Y,640,86,wood);
    rect(0,GROUND_Y,640,11,GRASS);
    rect(0,GROUND_Y+11,640,5,darkgrass);
    for(i=0;i<18;i++){
        float tx=i*78-cx;
        if(tx>-10&&tx<640)rect(tx,GROUND_Y+17,27,4,(Color){199,146,95});
    }
    for(i=0;i<PLATFORM_COUNT;i++){
        const Rectangle *p=&platforms[i];float px=p->x-cx;
        rect(px,p->y,p->w,p->h,BROWN);
        rect(px,p->y,p->w,5,GRASS);
        rect(px+5,p->y+9,p->w-10,3,(Color){206,156,104});
    }
    for(i=0;i<PUDDLE_COUNT;i++){
        const Rectangle *p=&puddles[i];float px=p->x-cx;
        disc(px+12,p->y+7,12,(Color){72,175,210});
        rect(px+12,p->y-4,p->w-24,p->h+12,(Color){72,175,210});
        disc(px+p->w-12,p->y+7,12,(Color){72,175,210});
        rect(px+12,p->y+1,p->w-30,3,(Color){177,230,241});
    }
    /* Doghouse: finish line. */
    {
        float hx=1188-cx;
        rect(hx+4,316,89,78,(Color){191,91,86});
        triangle(hx-7,323,hx+48,275,hx+104,323,(Color){129,64,70});
        rect(hx+31,345,32,49,(Color){70,54,67});
        disc(hx+47,349,16,(Color){70,54,67});
        draw_text(hx+20,317,"HOME",1.4f,WHITE);
    }
    for(i=0;i<TREAT_COUNT;i++){
        const Treat *t=&g->treats[i];
        if(!t->collected) bone(t->x-cx,t->y,(g->tick/15+i)%3-1);
    }
    dog(g->x-cx,g->y,g->facing,g->tick);
}

static void hud(const Game *g) {
    char count[8];
    count[0]='0'+g->collected;count[1]='/';count[2]='0'+TREAT_COUNT;count[3]=0;
    rect(0,0,640,66,DARK);
    draw_text(18,15,"ALFIE'S ADVENTURE",2.0f,WHITE);
    draw_text(431,14,"TREATS",1.6f,GOLD);
    draw_text(534,11,count,2.6f,WHITE);
    draw_text(18,45,"D-PAD MOVE   2 JUMP   + RESTART   HOME EXIT",1.1f,WHITE);
    if(g->won){
        rect(70,138,500,178,DARK);
        rect(76,144,488,166,(Color){239,197,114});
        rect(90,157,460,136,DARK);
        draw_text(144,180,"ALFIE DID IT!",3,WHITE);
        draw_text(150,226,"ALL TREATS FOUND",2,GOLD);
        draw_text(170,267,"PRESS + TO REPLAY",1.5f,WHITE);
    }
    else if(g->collected==TREAT_COUNT){
        draw_text(220,82,"FIND THE DOGHOUSE!",1.5f,DARK);
    }
    else if(g->tick<300){
        draw_text(175,83,"COLLECT 7 BONES TO GET HOME",1.35f,DARK);
    }
}

static void graphics_init(void) {
    float y_scale;u32 xfb_height;
    GXColor black={0,0,0,255};
    VIDEO_Init();
    video_mode=VIDEO_GetPreferredMode(NULL);
    framebuffers[0]=MEM_K0_TO_K1(SYS_AllocateFramebuffer(video_mode));
    framebuffers[1]=MEM_K0_TO_K1(SYS_AllocateFramebuffer(video_mode));
    VIDEO_Configure(video_mode);
    VIDEO_SetNextFramebuffer(framebuffers[0]);
    VIDEO_SetBlack(TRUE);
    VIDEO_Flush();VIDEO_WaitVSync();
    if(video_mode->viTVMode&VI_NON_INTERLACE)VIDEO_WaitVSync();
    fifo=memalign(32,FIFO_SIZE);
    if(!fifo)exit(1);
    memset(fifo,0,FIFO_SIZE);
    GX_Init(fifo,FIFO_SIZE);
    GX_SetCopyClear(black,0x00ffffff);
    GX_SetViewport(0,0,video_mode->fbWidth,video_mode->efbHeight,0,1);
    y_scale=GX_GetYScaleFactor(video_mode->efbHeight,video_mode->xfbHeight);
    xfb_height=GX_SetDispCopyYScale(y_scale);
    GX_SetScissor(0,0,video_mode->fbWidth,video_mode->efbHeight);
    GX_SetDispCopySrc(0,0,video_mode->fbWidth,video_mode->efbHeight);
    GX_SetDispCopyDst(video_mode->fbWidth,xfb_height);
    GX_SetCopyFilter(video_mode->aa,video_mode->sample_pattern,GX_TRUE,video_mode->vfilter);
    GX_SetFieldMode(video_mode->field_rendering,
        video_mode->viHeight==2*video_mode->xfbHeight?GX_ENABLE:GX_DISABLE);
    GX_SetPixelFmt(video_mode->aa?GX_PF_RGB565_Z16:GX_PF_RGB8_Z24,GX_ZC_LINEAR);
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_FALSE,GX_ALWAYS,GX_FALSE);
    GX_SetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_CLEAR);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetNumTevStages(1);
    GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,
                   GX_LIGHTNULL,GX_DF_NONE,GX_AF_NONE);
    GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORDNULL,GX_TEXMAP_NULL,GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS,GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0,GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
    guOrtho(projection,0,480,0,640,0,100);
    GX_LoadProjectionMtx(projection,GX_ORTHOGRAPHIC);
    guMtxIdentity(model);
    GX_LoadPosMtxImm(model,GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
    GX_SetDispCopyGamma(GX_GM_1_0);
    VIDEO_SetBlack(FALSE);VIDEO_Flush();
}

static void render(const Game *g) {
    /* Each frame writes a new image, then presents the buffer. */
    GX_SetViewport(0,0,video_mode->fbWidth,video_mode->efbHeight,0,1);
    background(g);hud(g);
    GX_DrawDone();
    GX_CopyDisp(framebuffers[front],GX_TRUE);
    VIDEO_SetNextFramebuffer(framebuffers[front]);
    VIDEO_Flush();VIDEO_WaitVSync();
    front^=1;
}
int main(int argc,char **argv) {
    Game game;
    (void)argc;(void)argv;
    graphics_init();WPAD_Init();WPAD_SetDataFormat(WPAD_CHAN_0,WPAD_FMT_BTNS);
    game_reset(&game);
    while (SYS_MainLoop()) {
        u32 held,pressed;int movement=0;
        WPAD_ScanPads();
        held=WPAD_ButtonsHeld(0);pressed=WPAD_ButtonsDown(0);
        if(pressed&WPAD_BUTTON_HOME) break;
        if(pressed&WPAD_BUTTON_PLUS)game_reset(&game);
        /* For a sideways Wii Remote, physical UP is left; DOWN is right. */
        if(held&(WPAD_BUTTON_UP|WPAD_BUTTON_LEFT)) movement=-1;
        if(held&(WPAD_BUTTON_DOWN|WPAD_BUTTON_RIGHT)) movement=1;
        game_step(&game,movement,(pressed&(WPAD_BUTTON_2|WPAD_BUTTON_A))!=0);
        render(&game);
    }
    WPAD_Shutdown();
    return 0;
}