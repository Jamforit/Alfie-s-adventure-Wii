#include <assert.h>
#include <stdio.h>
#include "../source/game.h"
int main(void){
    Game g;int i;
    game_reset(&g);assert(g.state==STATE_TITLE && g.level==0);
    game_button(&g,1);assert(g.state==STATE_PLAY && g.hearts==3);
    for(i=0;i<180;i++)game_tick(&g,0,0,0);
    assert(g.grounded && (int)g.y==GROUND_Y-39);
    game_tick(&g,0,1,0);assert(!g.grounded && g.vy<0);
    for(i=0;i<100;i++)game_tick(&g,0,0,0);
    assert(g.grounded);
    game_tick(&g,0,0,1);assert(g.sound==SOUND_BARK && g.bark_cooldown>0);
    game_restart_level(&g);assert(g.level==0);
    game_level(&g,0);g.x=level_data[0].poos[0].x;g.y=GROUND_Y-39;
    game_tick(&g,0,0,0);assert(g.poos==1 && g.boost>0);
    game_level(&g,0);g.x=level_data[0].bones[0].x;g.y=GROUND_Y-39;
    game_tick(&g,0,0,0);assert(g.bones==1);
    game_level(&g,0);g.x=WORLD_WIDTH-122;g.y=GROUND_Y-39;
    game_tick(&g,1,0,0);assert(g.state==STATE_PLAY);
    for(i=0;i<TREAT_GOAL;i++){g.grabbed[i]=1;g.bones++;}
    game_tick(&g,1,0,0);assert(g.state==STATE_LEVEL_DONE);
    game_button(&g,1);assert(g.state==STATE_PLAY && g.level==1);
    game_level(&g,2);g.x=WORLD_WIDTH-122;g.y=GROUND_Y-39;
    for(i=0;i<TREAT_GOAL;i++){g.grabbed[i]=1;g.bones++;}
    game_tick(&g,1,0,0);assert(g.state==STATE_GAME_DONE);
    puts("Alfie's Adventure v0.2 logic tests PASSED");
    return 0;
}
