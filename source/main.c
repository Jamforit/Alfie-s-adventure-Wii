/* ALFIE'S ADVENTURE v0.2 -- family-friendly original Wii homebrew.
 * Based on the real Alfie. For Kacey and Darcey!
 * Sideways Wii Remote: D-pad run, 2 jump, 1 bark, + restart, HOME exit. */
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "game.h"
#include "sprites.h"
#include <asndlib.h>

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
static const Color PINK = {255,140,186};
static const Color NAVY = {39,53,83};

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
 {'.',{0,0,0,0,0,0,4}}, {'?',{14,17,1,2,4,0,4}}, {'&',{12,18,20,8,21,18,13}},
 {'=',{0,0,31,0,31,0,0}}, {'*',{0,21,14,31,14,21,0}}
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


/* ===== Wii GX hardware sprite loading: 128x96 RGB5A3 tiled textures ===== */
static GXTexObj sprite_tex[4];
static void *tex_mem[4];
static void decode_sprite(int index,const unsigned char *src,int size){
    int i=0,j=0;
    unsigned char *dst=(unsigned char*)memalign(32,SPR_W*SPR_H*2);
    if(!dst)return;
    memset(dst,0,SPR_W*SPR_H*2);
    while(i<size && j<SPR_W*SPR_H*2){
        unsigned int count,tag=src[i++];
        if(tag>=128){
            unsigned char a,b;
            count=tag-127;
            if(i+1>=size)break;
            a=src[i++];b=src[i++];
            while(count-- && j+1<SPR_W*SPR_H*2){dst[j++]=a;dst[j++]=b;}
        }else{
            count=tag+1;
            while(count-- && i+1<size && j+1<SPR_W*SPR_H*2){dst[j++]=src[i++];dst[j++]=src[i++];}
        }
    }
    tex_mem[index]=dst;
    DCFlushRange(dst,SPR_W*SPR_H*2);
    GX_InitTexObj(&sprite_tex[index],dst,SPR_W,SPR_H,GX_TF_RGB5A3,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GX_InitTexObjFilterMode(&sprite_tex[index],GX_LINEAR,GX_LINEAR);
}
static void sprites_init(void){
    decode_sprite(0,sprite_idle_rle,SPR_IDLE_LEN);
    decode_sprite(1,sprite_run_rle,SPR_RUN_LEN);
    decode_sprite(2,sprite_jump_rle,SPR_JUMP_LEN);
    decode_sprite(3,sprite_power_rle,SPR_POWER_LEN);
}
static void sprite_vert(float x,float y,float u,float v){
    GX_Position3f32(x,y,0);
    GX_Color4u8(255,255,255,255);
    GX_TexCoord2f32(u,v);
}
static void draw_alfie(float x,float y,float w,float h,int pose,int facing){
    float u0=facing>0?0:1,u1=facing>0?1:0;
    if(pose<0 || pose>3 || !tex_mem[pose])return;
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
    GX_LoadTexObj(&sprite_tex[pose],GX_TEXMAP0);
    GX_SetVtxDesc(GX_VA_TEX0,GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GX_SetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);
    GX_Begin(GX_QUADS,GX_VTXFMT0,4);
    sprite_vert(x,y,u0,0);sprite_vert(x+w,y,u1,0);
    sprite_vert(x+w,y+h,u1,1);sprite_vert(x,y+h,u0,1);
    GX_End();
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS,GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0,GX_DIRECT);
    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORDNULL,GX_TEXMAP_NULL,GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
}

/* ===== Sound effects: small, original, synthesized waveforms ===== */
#define SFX_COUNT 9
#define SFX_RATE 16000
#define SFX_MAX 8000
static s16 sound_data[SFX_COUNT][SFX_MAX] ATTRIBUTE_ALIGN(32);
static int sound_len[SFX_COUNT];
static int voice_rotator=0;
static void sounds_init(void){
    int i,k;
    const int duration[SFX_COUNT]={0,105,190,350,250,310,540,180,150};
    const float freq[SFX_COUNT]={0,550,790,130,220,310,520,960,440};
    ASND_Init();ASND_Pause(0);
    for(i=1;i<SFX_COUNT;i++){
        int count=duration[i]*SFX_RATE/1000;
        float phase=0;
        if(count>SFX_MAX)count=SFX_MAX;
        sound_len[i]=count;
        for(k=0;k<count;k++){
            float t=(float)k/SFX_RATE,p=(float)k/count,env=(1-p)*(1-p);
            float f=freq[i]*(1.0f+((i==1)?0.7f:(i==3)?-0.6f:0.0f)*p);
            float v;
            phase+=2*PI*f/SFX_RATE;
            v=sinf(phase);
            if(i==4){v=0.60f*sinf(phase)+0.32f*sinf(phase*0.5f);}
            if(i==3){v=0.55f*sinf(phase)+0.3f*sinf(phase*0.25f)+0.14f*sinf(phase*6);}
            if(i==6){v=sinf(phase)+0.25f*sinf(phase*1.5f);env=0.55f*(1-p);}
            if(i==5){v=sinf(phase)+0.4f*sinf(phase*0.3f);}
            if(i==2||i==7){env=(1-p)*0.72f;}
            sound_data[i][k]=(s16)(v*env*14000);
        }
        /* ASND requires cacheline-aligned PCM, and lengths rounded to 32-byte boundaries. */
        while(sound_len[i]%16 && sound_len[i]<SFX_MAX)sound_data[i][sound_len[i]++]=0;
        DCFlushRange(sound_data[i],sound_len[i]*2);
    }
}
static void play_sound(int which){
    if(which<=0 || which>=SFX_COUNT)return;
    voice_rotator=(voice_rotator+1)%8;
    ASND_SetVoice(1+voice_rotator,VOICE_MONO_16BIT_BE,SFX_RATE,0,
                  sound_data[which],sound_len[which]*2,190,190,NULL);
}

/* ===== Game art ===== */
static void pill(float x,float y,float w,float h,Color c){
    rect(x+10,y,w-20,h,c);disc(x+10,y+h*.5f,h*.5f,c);disc(x+w-10,y+h*.5f,h*.5f,c);
}
static void sparkle(float x,float y,float s,Color c){
    triangle(x,y-s,x+s*.35f,y,x,y+s,c);
    triangle(x,y-s,x-s*.35f,y,x,y+s,c);
    triangle(x-s,y,x,y-s*.35f,x+s,y,c);
    triangle(x-s,y,x,y+s*.35f,x+s,y,c);
}
static void draw_bone(float x,float y,int tick){
    Color cream={255,246,208},border={192,139,81};
    y+=sinf(tick*.09f)*2;
    disc(x-8,y-5,5,border);disc(x-8,y+5,5,border);
    disc(x+8,y-5,5,border);disc(x+8,y+5,5,border);
    rect(x-9,y-4,18,9,cream);
    disc(x-8,y-5,4,cream);disc(x-8,y+5,4,cream);
    disc(x+8,y-5,4,cream);disc(x+8,y+5,4,cream);
}
static void draw_poo(float x,float y,int tick){
    Color rich={110,64,43},light={180,99,49};
    disc(x,y+4,12,rich);
    disc(x-4,y-3,9,light);
    disc(x+4,y-3,8,light);
    disc(x,y-11,6,rich);
    disc(x+2,y-15,3,light);
    disc(x-5,y-2,1,(Color){251,241,192});
    disc(x+5,y-3,1,(Color){251,241,192});
    if(tick%32<16) sparkle(x+18,y-13,4,GOLD);
}
static void draw_ball(float x,float y,int tick){
    disc(x,y,11,(Color){86,133,45});disc(x,y-2,9,(Color){192,232,54});
    rect(x-6,y-2,10,3,(Color){247,253,214});
    disc(x-3,y-6,2,WHITE);
    if(tick%24<12)sparkle(x+14,y-11,5,GOLD);
}
static void draw_flower(float x,float y,int j){
    Color pink=(j%3==0)?PINK:(Color){244,210,80};
    rect(x-1,y,2,13,(Color){50,130,66});
    disc(x-5,y-2,4,pink);disc(x+5,y-2,4,pink);
    disc(x,y-7,4,pink);disc(x,y+2,4,pink);disc(x,y-2,3,GOLD);
}
static void draw_cloud(float x,float y,float s){
    Color cloud={250,252,246};
    disc(x,y,23*s,cloud);disc(x+30*s,y-10*s,30*s,cloud);
    disc(x+67*s,y+2*s,24*s,cloud);
    rect(x-10*s,y,92*s,20*s,cloud);
}
static void draw_tree(float x,float bottom,int type,int tick){
    Color wood={121,76,58},green={66,150,92},green2={95,175,108};
    rect(x+5,bottom-125,18,125,wood);
    disc(x+10,bottom-143,45,green);
    disc(x-22,bottom-126,37,green2);
    disc(x+45,bottom-129,37,green2);
    if(type==0){
        disc(x+2,bottom-154,4,PINK);
        disc(x+36,bottom-140,3,PINK);
    }else if(tick%150<75){
        disc(x+18,bottom-170,5,(Color){248,214,81});
    }
}
static void draw_background(const Game *g){
    int level=g->level,i;float cam=g->camera;
    Color sky=level==2?(Color){120,207,252}:(Color){158,225,250};
    Color hill=level==2?(Color){113,196,211}:(Color){107,193,130};
    Color grass=level==2?(Color){251,222,136}:(Color){77,178,104};
    Color dirt=level==2?(Color){232,183,117}:(Color){142,97,66};
    rect(0,0,640,480,sky);
    rect(0,153,640,164,(Color){195,232,241});
    if(level==2){
        rect(0,224,640,154,(Color){72,181,213});
        for(i=0;i<13;i++){
            float bx=(i*127-cam*.15f);
            rect(bx,266+(i%3)*25,75,4,(Color){195,245,247});
        }
        disc(529-cam*.06f,110,42,(Color){255,226,117});
        for(i=0;i<5;i++)draw_cloud((i*260-cam*.15f),94+(i%2)*32,.65f);
        for(i=0;i<4;i++){
            float x=i*430-cam*.35f;
            triangle(x-30,279,x+80,160,x+174,281,(Color){92,170,160});
        }
    }else{
        disc(535-cam*.07f,101,39,(Color){255,226,139});
        for(i=0;i<5;i++)draw_cloud(i*235-cam*.16f,92+(i%2)*25,.65f);
        for(i=0;i<7;i++){
            float bx=i*200-cam*.28f;
            disc(bx,312,107,hill);
        }
        rect(0,303,640,92,(Color){127,204,130});
        if(level==0){
            /* Cream picket garden fence. */
            for(i=0;i<33;i++){
                float fx=i*57-cam*.68f;
                if(fx<-30||fx>670)continue;
                rect(fx,302,9,85,(Color){242,235,209});
                triangle(fx,302,fx+4,294,fx+9,302,(Color){242,235,209});
            }
            rect(0,339,640,7,(Color){224,226,195});
        }else{
            for(i=0;i<14;i++){
                float tx=i*125-cam*.64f;
                if(tx<-90||tx>700)continue;
                draw_tree(tx,395,1,g->tick);
            }
        }
    }
    /* Ground and turf. */
    rect(0,GROUND_Y,640,85,dirt);
    rect(0,GROUND_Y,640,12,grass);
    if(level!=2){
        rect(0,GROUND_Y+9,640,6,(Color){43,139,82});
        for(i=0;i<18;i++){
            float x=i*97-cam*.9f;
            if(x>-10&&x<650)draw_flower(x,GROUND_Y-12,i);
        }
        if(level==0)for(i=0;i<11;i++){
            float tx=i*195+65-cam*.56f;
            if(tx>-60&&tx<700)draw_tree(tx,386,0,g->tick);
        }
    }else{
        for(i=0;i<12;i++){
            float x=i*83-cam*.6f;
            disc(x+10,GROUND_Y+19,3,(Color){255,239,174});
        }
    }
    for(i=0;i<14;i++){
        float x=i*155-cam;
        if(x>-10 && x<650)rect(x,GROUND_Y+32+(i%3)*17,21,5,(Color){190,137,98});
    }
}
static void draw_level(const Game *g){
    const LevelData *L=&level_data[g->level];
    float cam=g->camera;int i;
    Color platform=g->level==2?(Color){189,142,96}:BROWN;
    Color top=g->level==2?(Color){255,232,166}:GRASS;
    draw_background(g);
    for(i=0;i<PLATFORM_COUNT;i++){
        Rect p=L->platforms[i];p.x-=cam;
        if(p.x>660||p.x+p.w<-10)continue;
        rect(p.x,p.y,p.w,p.h,platform);
        rect(p.x,p.y,p.w,7,top);
        rect(p.x+10,p.y+9,p.w-20,3,(Color){230,181,117});
        if(g->level!=2){
            draw_flower(p.x+24,p.y-12,i);
            draw_flower(p.x+p.w-19,p.y-12,i+3);
        }
    }
    /* Puddles are slippery, but not fatal. */
    for(i=0;i<PUDDLE_COUNT;i++){
        Rect p=L->puddles[i];float x=p.x-cam;
        if(x<-100||x>700)continue;
        disc(x+13,p.y+7,11,(Color){43,157,208});
        rect(x+12,p.y-4,p.w-22,22,(Color){43,157,208});
        disc(x+p.w-13,p.y+7,11,(Color){43,157,208});
        rect(x+13,p.y+1,p.w-29,3,(Color){180,237,252});
    }
    for(i=0;i<TREAT_COUNT;i++)if(!g->grabbed[i]){
        Point b=L->bones[i];
        draw_bone(b.x-cam,b.y,g->tick+i*8);
    }
    for(i=0;i<BALL_COUNT;i++)if(!g->got_ball[i]){
        Point b=L->balls[i];draw_ball(b.x-cam,b.y,g->tick+i*8);
    }
    for(i=0;i<POO_COUNT;i++)if(!g->got_poo[i]){
        Point p=L->poos[i];draw_poo(p.x-cam,p.y,g->tick+i*8);
    }
    /* Each level ends at an illustrated kennel / finish marker. */
    {
        float hx=WORLD_WIDTH-142-cam;
        rect(hx,311,85,84,(Color){184,101,87});
        triangle(hx-9,319,hx+43,270,hx+95,319,(Color){122,71,87});
        rect(hx+28,347,29,48,(Color){63,49,64});
        disc(hx+42,348,15,(Color){63,49,64});
        rect(hx+12,325,59,10,(Color){254,218,126});
        draw_text(hx+22,327,"HOME",1.35f,NAVY);
        if(g->level==2){
            disc(hx-20,380,17,(Color){238,220,166});
            rect(hx-34,365,27,22,(Color){237,194,113});
            triangle(hx-34,365,hx-20,348,hx-7,365,(Color){231,190,110});
        }
    }
    /* Cats in garden, squirrels in park, crabs on beach. */
    for(i=0;i<ENEMY_COUNT;i++){
        float x=enemy_x(g,i)-cam,y=L->enemies[i].y;
        if(g->enemy_gone[i]||x<-40||x>680)continue;
        if(g->level==2){
            Color red={244,103,79};
            disc(x,y-6,16,red);rect(x-25,y-5,50,7,red);
            disc(x-23,y-13,7,red);disc(x+23,y-13,7,red);
            triangle(x-27,y-9,x-32,y-24,x-17,y-17,red);
            triangle(x+27,y-9,x+32,y-24,x+17,y-17,red);
            disc(x-7,y-17,5,WHITE);disc(x+7,y-17,5,WHITE);
            disc(x-7,y-18,2,NAVY);disc(x+7,y-18,2,NAVY);
        }else{
            Color fur=g->level==0?(Color){112,79,93}:(Color){177,104,63};
            disc(x,y-9,16,fur);
            disc(x-12,y-23,10,fur);disc(x+10,y-23,10,fur);
            triangle(x-12,y-20,x-18,y-40,x-1,y-23,fur);
            triangle(x+10,y-20,x+17,y-40,x+2,y-23,fur);
            disc(x+6,y-20,4,WHITE);disc(x+7,y-20,2,NAVY);
            rect(x+12,y-8,15,6,fur);
            if(g->level==1)disc(x-16,y-20,12,fur);
        }
    }
    if(g->invincible==0 || g->tick%8<5){
        int pose=g->boost?3:(!g->grounded?2:(g->vx!=0 && g->tick%22<16?1:0));
        float x=g->x-cam-14,y=g->y-23;
        draw_alfie(x,y,83,64,pose,g->facing);
    }
    if(g->boost){
        for(i=0;i<4;i++){
            float t=g->tick*.19f+i*1.5f;
            sparkle(g->x-cam+20+cosf(t)*48,g->y+13+sinf(t)*24,4+(i%2)*2,GOLD);
        }
    }
    if(g->bark_ring>0){
        float r=22+(20-g->bark_ring)*3;
        sparkle(g->x-cam+22+g->facing*r,g->y+13,9,(Color){255,235,117});
        if(g->bark_ring>9)draw_text(g->x-cam+((g->facing>0)?56:-66),g->y-35,"WOOF!",1.4f,NAVY);
    }
}
static void overlay_panel(Color accent){
    rect(62,104,516,295,(Color){52,65,91});
    rect(70,112,500,279,(Color){255,249,225});
    rect(70,112,500,34,accent);
}
static void draw_interface(const Game *g){
    char b[64];int i;
    rect(0,0,640,59,NAVY);
    draw_text(16,9,"ALFIE'S ADVENTURE",2.2f,WHITE);
    draw_text(413,11,level_data[g->level].name,1.45f,GOLD);
    draw_text(17,39,"2 JUMP    1 BARK    + RESTART    HOME EXIT",1.2f,WHITE);
    rect(0,59,640,36,(Color){251,239,194});
    sprintf(b,"BONES %d/%d",g->bones,TREAT_GOAL);
    draw_text(15,72,b,1.7f,NAVY);
    sprintf(b,"SCORE %d",g->score);
    draw_text(223,72,b,1.5f,NAVY);
    draw_text(490,70,"LIVES",1.6f,NAVY);
    for(i=0;i<g->hearts;i++){
        disc(583+i*16,77,6,PINK);disc(593+i*16,77,6,PINK);
        triangle(577+i*16,79,599+i*16,79,588+i*16,92,PINK);
    }
    if(g->boost){
        pill(232,101,183,26,(Color){255,223,123});
        draw_text(249,108,"POO POWER!",1.8f,(Color){115,65,45});
    }else if(g->state==STATE_PLAY && g->tick<280){
        draw_text(140,103,"COLLECT BONES AND FIND HOME!",1.5f,NAVY);
    }
    if(g->state==STATE_TITLE){
        overlay_panel((Color){239,176,109});
        draw_text(109,121,"ALFIE'S ADVENTURE",2.55f,NAVY);
        draw_alfie(183,144,273,150,0,1);
        draw_text(139,316,"FOR KACEY & DARCEY",2.15f,NAVY);
        draw_text(154,352,"PRESS 2 TO START!",1.95f,(Color){150,92,63});
    }else if(g->state==STATE_LEVEL_DONE){
        overlay_panel((Color){255,203,125});
        draw_text(125,121,"LEVEL COMPLETE!",2.6f,NAVY);
        draw_alfie(196,145,247,144,3,1);
        draw_text(133,303,"WELL DONE KACEY & DARCEY!",1.56f,NAVY);
        draw_text(160,349,"PRESS 2 FOR NEXT LEVEL",1.65f,(Color){160,86,55});
    }else if(g->state==STATE_GAME_DONE){
        overlay_panel((Color){253,195,123});
        draw_text(125,118,"ALFIE SAVED THE DAY!",2.2f,NAVY);
        draw_alfie(196,148,251,133,3,1);
        draw_text(121,289,"KACEY & DARCEY ARE STARS!",1.75f,NAVY);
        sprintf(b,"YOUR SCORE %d",g->score);
        draw_text(199,323,b,1.8f,(Color){156,85,65});
        draw_text(190,358,"PRESS 2 TO REPLAY",1.8f,NAVY);
    }else if(g->state==STATE_LOST){
        overlay_panel(PINK);
        draw_text(159,126,"OOPS! TRY AGAIN!",2.5f,NAVY);
        draw_alfie(202,155,240,145,0,1);
        draw_text(151,320,"ALFIE NEEDS YOUR HELP!",1.75f,NAVY);
        draw_text(180,357,"PRESS 2 TO RETRY",1.8f,(Color){156,85,65});
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


static void render(const Game *g){
    GX_SetViewport(0,0,video_mode->fbWidth,video_mode->efbHeight,0,1);
    draw_level(g);
    draw_interface(g);
    GX_DrawDone();
    GX_CopyDisp(framebuffers[front],GX_TRUE);
    VIDEO_SetNextFramebuffer(framebuffers[front]);
    VIDEO_Flush();VIDEO_WaitVSync();front^=1;
}
int main(int argc,char **argv){
    Game g;
    (void)argc;(void)argv;
    graphics_init();
    sprites_init();
    WPAD_Init();WPAD_SetDataFormat(WPAD_CHAN_0,WPAD_FMT_BTNS);
    sounds_init();
    game_reset(&g);
    while(SYS_MainLoop()){
        u32 held,pressed;int movement=0;
        WPAD_ScanPads();held=WPAD_ButtonsHeld(0);pressed=WPAD_ButtonsDown(0);
        if(pressed&WPAD_BUTTON_HOME)break;
        if(pressed&WPAD_BUTTON_PLUS)game_restart_level(&g);
        /* Sideways Remote: physical UP acts as left; DOWN acts as right. */
        if(held&(WPAD_BUTTON_UP|WPAD_BUTTON_LEFT))movement=-1;
        if(held&(WPAD_BUTTON_DOWN|WPAD_BUTTON_RIGHT))movement=1;
        if(g.state==STATE_PLAY)
            game_tick(&g,movement,(pressed&(WPAD_BUTTON_2|WPAD_BUTTON_A))!=0,
                      (pressed&WPAD_BUTTON_1)!=0);
        else game_button(&g,(pressed&(WPAD_BUTTON_2|WPAD_BUTTON_A))!=0);
        play_sound(g.sound);
        render(&g);
    }
    ASND_End();WPAD_Shutdown();
    return 0;
}
