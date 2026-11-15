#ifndef ALFIE_GAME_H
#define ALFIE_GAME_H
#include <math.h>

/* Wii-independent game code. All coordinates are in virtual 640x480 pixels. */
#define GAME_WIDTH 640
#define GAME_HEIGHT 480
#define WORLD_WIDTH 1760
#define GROUND_Y 395
#define LEVEL_COUNT 3
#define PLATFORM_COUNT 7
#define TREAT_COUNT 9
#define TREAT_GOAL 6
#define POO_COUNT 3
#define BALL_COUNT 3
#define PUDDLE_COUNT 3
#define ENEMY_COUNT 4

#define STATE_TITLE 0
#define STATE_PLAY 1
#define STATE_LEVEL_DONE 2
#define STATE_GAME_DONE 3
#define STATE_LOST 4

#define SOUND_NONE 0
#define SOUND_JUMP 1
#define SOUND_BONE 2
#define SOUND_POO 3
#define SOUND_BARK 4
#define SOUND_HURT 5
#define SOUND_WIN 6
#define SOUND_BALL 7
#define SOUND_STOMP 8

typedef struct { float x,y,w,h; } Rect;
typedef struct { float x,y; } Point;
typedef struct {
    const char *name;
    Rect platforms[PLATFORM_COUNT];
    Rect puddles[PUDDLE_COUNT];
    Point bones[TREAT_COUNT];
    Point poos[POO_COUNT];
    Point balls[BALL_COUNT];
    Point enemies[ENEMY_COUNT];
} LevelData;

static const LevelData level_data[LEVEL_COUNT] = {
    {"THE GARDEN",
        {{170,331,145,15},{355,296,140,15},{562,331,135,15},{795,309,150,15},{1040,325,135,15},{1242,287,142,15},{1444,323,145,15}},
        {{323,384,69,12},{715,384,77,12},{1344,384,66,12}},
        {{114,353},{230,290},{426,255},{606,291},{842,269},{993,353},{1116,286},{1310,246},{1516,284}},
        {{279,364},{915,362},{1483,367}},
        {{488,359},{1215,360},{1638,360}},
        {{520,360},{879,360},{1180,360},{1510,360}}},
    {"THE BIG PARK",
        {{174,310,139,15},{390,327,134,15},{617,281,147,15},{854,327,149,15},{1072,299,133,15},{1280,335,137,15},{1470,282,140,15}},
        {{329,384,60,12},{768,384,72,12},{1406,384,62,12}},
        {{118,351},{234,270},{458,287},{680,242},{921,287},{1137,259},{1234,352},{1354,298},{1539,244}},
        {{277,364},{976,362},{1563,367}},
        {{567,360},{1207,355},{1658,356}},
        {{505,360},{840,360},{1172,360},{1539,360}}},
    {"THE SUNNY BEACH",
        {{189,334,136,15},{397,309,149,15},{620,274,139,15},{842,326,150,15},{1050,298,145,15},{1249,328,142,15},{1483,287,135,15}},
        {{353,384,70,12},{781,384,65,12},{1420,384,75,12}},
        {{113,351},{256,294},{466,270},{693,234},{929,287},{1119,259},{1309,289},{1546,246},{1670,352}},
        {{306,363},{974,363},{1533,363}},
        {{571,358},{1186,355},{1634,357}},
        {{507,360},{871,360},{1237,360},{1588,360}}}
};

typedef struct {
    float x,y,vx,vy,camera,checkpoint;
    int grounded,facing,level,state,score,lives,hearts,bones,balls,poos;
    int tick,invincible,boost,bark_cooldown,bark_ring,poo_flash,combo;
    int grabbed[TREAT_COUNT],got_poo[POO_COUNT],got_ball[BALL_COUNT],enemy_gone[ENEMY_COUNT];
    int sound; /* Last sound event emitted by game_tick, cleared at start of next tick. */
} Game;

static int hitbox(float x,float y,float w,float h,float X,float Y,float W,float H) {
    return x<X+W && x+w>X && y<Y+H && y+h>Y;
}
static float enemy_x(const Game *g,int i){
    return level_data[g->level].enemies[i].x+sinf((g->tick+i*65)*0.028f)*27.0f;
}
static void game_level(Game *g,int n){
    int i;
    g->level=n; g->x=34;g->y=GROUND_Y-39;g->vx=0;g->vy=0;g->camera=0;
    g->checkpoint=34;g->grounded=1;g->facing=1;g->hearts=3;
    g->bones=0;g->balls=0;g->poos=0;g->tick=0;g->invincible=0;
    g->boost=0;g->bark_cooldown=0;g->bark_ring=0;g->poo_flash=0;
    g->state=STATE_PLAY;g->sound=SOUND_NONE;
    for(i=0;i<TREAT_COUNT;i++)g->grabbed[i]=0;
    for(i=0;i<POO_COUNT;i++)g->got_poo[i]=0;
    for(i=0;i<BALL_COUNT;i++)g->got_ball[i]=0;
    for(i=0;i<ENEMY_COUNT;i++)g->enemy_gone[i]=0;
}
static void game_reset(Game *g){
    g->state=STATE_TITLE;g->level=0;g->score=0;g->lives=0;g->combo=0;
    game_level(g,0);
    g->state=STATE_TITLE;
}
static void game_button(Game *g,int pressed2){
    if(!pressed2)return;
    if(g->state==STATE_TITLE){g->score=0;game_level(g,0);g->sound=SOUND_WIN;}
    else if(g->state==STATE_LEVEL_DONE){int n=g->level+1;game_level(g,n);g->sound=SOUND_WIN;}
    else if(g->state==STATE_GAME_DONE){game_reset(g);g->sound=SOUND_WIN;}
    else if(g->state==STATE_LOST){int n=g->level;game_level(g,n);g->sound=SOUND_WIN;}
}
static void game_restart_level(Game *g){
    int n=g->level;
    if(g->state==STATE_TITLE || g->state==STATE_GAME_DONE)game_reset(g);
    else game_level(g,n);
}
static void game_tick(Game *g,int movement,int jump,int bark){
    int i;float bottom;
    const LevelData *L=&level_data[g->level];
    g->sound=SOUND_NONE;
    if(g->state!=STATE_PLAY)return;
    g->tick++;
    if(g->invincible>0)g->invincible--;
    if(g->boost>0)g->boost--;
    if(g->bark_cooldown>0)g->bark_cooldown--;
    if(g->bark_ring>0)g->bark_ring--;
    if(g->poo_flash>0)g->poo_flash--;
    if(bark && !g->bark_cooldown){
        g->bark_cooldown=48;g->bark_ring=20;g->sound=SOUND_BARK;
        for(i=0;i<ENEMY_COUNT;i++)if(!g->enemy_gone[i] &&
           fabsf(g->x-enemy_x(g,i))<120 && fabsf(g->y-level_data[g->level].enemies[i].y)<70){
            g->enemy_gone[i]=1;g->score+=75;
        }
    }
    if(movement!=0){g->facing=movement>0?1:-1;g->vx=movement*(g->boost?5.5f:3.9f);}
    else g->vx=0;
    if(jump&&g->grounded){g->vy=g->boost?-13.3f:-11.5f;g->grounded=0;g->sound=SOUND_JUMP;}
    g->x+=g->vx;
    if(g->x<0)g->x=0;
    if(g->x>WORLD_WIDTH-46)g->x=WORLD_WIDTH-46;
    bottom=g->y+39;
    g->vy+=0.51f;
    if(g->vy>12)g->vy=12;
    g->y+=g->vy;
    g->grounded=0;
    if(g->vy>=0){
        /* First landing surface encountered by the feet wins. */
        float best=GROUND_Y;
        for(i=0;i<PLATFORM_COUNT;i++){
            const Rect *p=&L->platforms[i];
            if(g->x+42>p->x && g->x+6<p->x+p->w && bottom<=p->y+4 && g->y+39>=p->y && p->y<best)best=p->y;
        }
        if(bottom<=best+4 && g->y+39>=best){g->y=best-39;g->grounded=1;g->vy=0;}
    }
    for(i=0;i<TREAT_COUNT;i++){
        Point p=L->bones[i];
        if(!g->grabbed[i] && hitbox(g->x,g->y,45,39,p.x-12,p.y-13,24,26)){
            g->grabbed[i]=1;g->bones++;g->score+=100;g->sound=SOUND_BONE;
        }
    }
    for(i=0;i<BALL_COUNT;i++){
        Point p=L->balls[i];
        if(!g->got_ball[i] && hitbox(g->x,g->y,45,39,p.x-15,p.y-16,30,32)){
            g->got_ball[i]=1;g->balls++;g->score+=250;g->sound=SOUND_BALL;
        }
    }
    for(i=0;i<POO_COUNT;i++){
        Point p=L->poos[i];
        if(!g->got_poo[i] && hitbox(g->x,g->y,45,39,p.x-13,p.y-12,26,24)){
            g->got_poo[i]=1;g->poos++;g->boost=270;g->poo_flash=75;
            g->score+=50;g->sound=SOUND_POO;
        }
    }
    for(i=0;i<PUDDLE_COUNT;i++){
        const Rect *p=&L->puddles[i];
        if(g->y+39>=GROUND_Y-3 && hitbox(g->x+9,g->y+31,28,8,p->x,p->y,p->w,p->h)){
            /* Splash gives a harmless, short backward skid. */
            g->x-=g->facing*3.6f;
            if(g->x<0)g->x=0;
        }
    }
    for(i=0;i<ENEMY_COUNT;i++){
        float ex=enemy_x(g,i),ey=L->enemies[i].y;
        if(g->enemy_gone[i])continue;
        if(hitbox(g->x+7,g->y+6,34,32,ex-17,ey-24,35,28)){
            if(g->boost){g->enemy_gone[i]=1;g->score+=100;g->sound=SOUND_STOMP;}
            else if(g->vy>0 && bottom<ey-17){
                g->enemy_gone[i]=1;g->vy=-7.5f;g->score+=150;g->sound=SOUND_STOMP;
            }else if(!g->invincible){
                g->hearts--;g->invincible=100;g->sound=SOUND_HURT;
                g->x=(g->x>70)?g->x-55:25;g->y=GROUND_Y-39;g->vy=0;
                if(g->hearts<=0){g->state=STATE_LOST;return;}
            }
        }
    }
    if(g->x>560 && g->x>g->checkpoint+300)g->checkpoint=g->x-100;
    if(g->x>=WORLD_WIDTH-120 && g->bones>=TREAT_GOAL && g->grounded){
        g->score+=500+g->hearts*100;
        g->state=(g->level==LEVEL_COUNT-1)?STATE_GAME_DONE:STATE_LEVEL_DONE;
        g->sound=SOUND_WIN;
    }
    g->camera=g->x-230;
    if(g->camera<0)g->camera=0;
    if(g->camera>WORLD_WIDTH-GAME_WIDTH)g->camera=WORLD_WIDTH-GAME_WIDTH;
}
#endif
