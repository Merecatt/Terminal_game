# Terminal_game
Compilation instruction:
gcc server.c
gcc client.c -lncurses

To train units, press t, then the type of unit to train from set {0, 1, 2, 3}, where:
0 - light infantry
1 - heavy infantry
2 - cavalry
3 - workers
and finally type number of units to train.

To attack, press a, then select number of units to send and finally select player to attack from set {1, 2, 3}.

Game finishes after reaching 5 victories or pressing q by any of the players.
