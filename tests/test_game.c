#include <assert.h>
#include <stdio.h>
#include "../source/game.h"
int main(void) {
    Game g;
    int i;
    game_reset(&g);
    assert(g.x==40 && g.collected==0 && g.grounded);
    for(i=0;i<600;i++)game_step(&g,0,0);
    assert(g.grounded && g.y==GROUND_Y-38);
    game_step(&g,0,1);
    assert(!g.grounded && g.vy<0);
    for(i=0;i<90;i++)game_step(&g,0,0);
    assert(g.grounded && g.y==GROUND_Y-38);
    game_reset(&g);
    for(i=0;i<TREAT_COUNT;i++){g.treats[i].collected=1;g.collected++;}
    g.x=1205;g.y=GROUND_Y-38;g.grounded=1;
    game_step(&g,0,0);assert(g.won);
    printf("Game physics tests passed.\n");
    return 0;
}