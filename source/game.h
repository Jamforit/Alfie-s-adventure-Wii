#ifndef ALFIE_GAME_H
#define ALFIE_GAME_H

/* Wii-independent logic for Alfie's Adventure, version 0.1. */
#define GAME_WIDTH 640
#define GAME_HEIGHT 480
#define WORLD_WIDTH 1320
#define GROUND_Y 394
#define TREAT_COUNT 7
#define PLATFORM_COUNT 4
#define PUDDLE_COUNT 2

typedef struct { float x, y, w, h; } Rectangle;
typedef struct { float x, y; int collected; } Treat;
typedef struct {
    float x, y, vx, vy;
    int grounded, facing, collected, won, splashes, tick;
    float camera;
    Treat treats[TREAT_COUNT];
} Game;

static const Rectangle platforms[PLATFORM_COUNT] = {
    {180, 326, 130, 16}, {405, 282, 126, 16},
    {675, 323, 128, 16}, {890, 290, 150, 16}
};
static const Rectangle puddles[PUDDLE_COUNT] = {
    {340, 382, 77, 12}, {810, 382, 72, 12}
};
static const float treat_positions[TREAT_COUNT][2] = {
    {116, 355}, {250, 286}, {465, 244}, {595, 355},
    {740, 284}, {954, 251}, {1129, 355}
};

static void game_reset(Game *g) {
    int i;
    g->x = 40; g->y = GROUND_Y - 38;
    g->vx = 0; g->vy = 0;
    g->grounded = 1; g->facing = 1;
    g->collected = 0; g->won = 0;
    g->splashes = 0; g->tick = 0; g->camera = 0;
    for (i=0; i<TREAT_COUNT; i++) {
        g->treats[i].x=treat_positions[i][0];
        g->treats[i].y=treat_positions[i][1];
        g->treats[i].collected=0;
    }
}

static int rect_overlap(float x,float y,float w,float h,
                        float ox,float oy,float ow,float oh) {
    return x < ox+ow && x+w > ox && y < oy+oh && y+h > oy;
}

/* One fixed 60 Hz game tick. Input -1=left, 0=stop, +1=right. */
static void game_step(Game *g, int horizontal, int jump) {
    int i;
    float last_bottom;
    if (g->won) return;
    g->tick++;
    if (horizontal > 0) {g->vx=3.7f;g->facing=1;}
    else if (horizontal < 0) {g->vx=-3.7f;g->facing=-1;}
    else g->vx=0;
    if (jump && g->grounded) {g->vy=-10.8f;g->grounded=0;}
    g->x+=g->vx;
    if (g->x<0)g->x=0;
    if (g->x>WORLD_WIDTH-48)g->x=WORLD_WIDTH-48;
    last_bottom=g->y+38;
    g->vy+=0.52f;
    if (g->vy>12)g->vy=12;
    g->y+=g->vy;
    g->grounded=0;
    if (g->vy>=0) {
        for (i=0;i<PLATFORM_COUNT;i++) {
            const Rectangle *p=&platforms[i];
            if (g->x+40>p->x && g->x+7<p->x+p->w &&
                last_bottom<=p->y+4 && g->y+38>=p->y) {
                g->y=p->y-38;g->vy=0;g->grounded=1;
            }
        }
        if (last_bottom<=GROUND_Y && g->y+38>=GROUND_Y) {
            /* Ground is under every platform and wins when lowest surface. */
            if (!g->grounded) {g->y=GROUND_Y-38;g->vy=0;g->grounded=1;}
        }
    }
    for (i=0;i<TREAT_COUNT;i++) {
        Treat *t=&g->treats[i];
        if (!t->collected && rect_overlap(g->x,g->y,45,38,t->x-10,t->y-10,20,20)) {
            t->collected=1;g->collected++;
        }
    }
    for (i=0;i<PUDDLE_COUNT;i++) {
        const Rectangle *p=&puddles[i];
        if (g->y+38>=GROUND_Y-4 && rect_overlap(g->x+7,g->y+27,30,10,p->x,p->y,p->w,p->h)) {
            /* Gentle retry: splashes reset Alfie's position, not the treats. */
            g->x=(i==0)?165:660;g->y=GROUND_Y-38;
            g->vy=0;g->grounded=1;g->splashes++;
        }
    }
    if (g->x>=1195 && g->collected==TREAT_COUNT && g->grounded) g->won=1;
    g->camera=g->x-235;
    if(g->camera<0)g->camera=0;
    if(g->camera>WORLD_WIDTH-GAME_WIDTH)g->camera=WORLD_WIDTH-GAME_WIDTH;
}
#endif