# Terminal_game
## Compilation instruction:

* gcc server.c

* gcc client.c -lncurses


Run every client with its own parameter from set {0, 1, 2}.
Game runs properly only if you connect 3 players and don't change default terminal window size.

## Basic actions
To train units, press t, then the type of unit to train from set {0, 1, 2, 3}, where:
* 0 - light infantry
* 1 - heavy infantry
* 2 - cavalry
* 3 - workers

and finally type number of units to train.

To attack, press a, then select number of units to send and finally select player to attack from set {1, 2, 3}.

Game finishes after reaching 5 victories or pressing q / ctrl + c by any of the players.


## Files' contents:
* mq_utils.c - functions related to message queues, mostly wrappers; definition of message, mq_buf, shm and shm_int structs
* sem_utils.c - functions related to semaphores, mostly wrappers; definition of sembuf, p and v initialization
* shm_utils.c - functions related to shared memory, mostly wrappers; definition of player and mq_player_buf structs
* client.c - the whole client program - connecting with server and user interface
* server.c - the whole server program, processing the game logic and communicating with clients
