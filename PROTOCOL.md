## Server
Server consists of 7 main processes:
* main process waiting for end of the game (e. g. SIGINT) - ppid
* three processes communicating with their client, sending him/her information and updating his/her resources - pid[3]
* three processes dedicated for getting client's input - pid_input[3]
Moreover, every fight and training triggers new process. 

## Message queues
### Structures:
* message structs 
    * winner
        * winner == 1 if you won the battle
        * winner == 0 if you lost
    * add_info
        * add_info == 1 means initial message
        * add_info == 2 means end of the game
        * add_info == 3 tells the player to display "text" field
    * text - text to display
    * action - action sent from player to server
    * unit_type - type of unit (needed to send information about training)
    * unit_number[4] - number of units of all types (needed to send information about training or fight)

```typedef struct {
    /* Message struct */
    int winner; 
    int your_units; 
    int enemy_units; 
    int add_info; 
    char text[30];
    char action;
    int unit_type;
    int unit_number[4];
}message;```    
  

    * type:
        * type == 7 means this message is game status (player info)
        * type == 1 means this is initial message
        * type == 3 means other info, e. g. battle info (player to attack)


```struct mq_buf {
    /* Redefined msgbuf stuct */
    long mtype; 
    message msg;
};```    

  
* player structs 
player - struct containing basic information about player
    * n - identifier of player in notation {1, 2, 3}
    * military - military units:
        * 0 - light infantry
        * 1 - heavy infantry
        * 2 - cavalry
    * workers - number of workers
    * resources_increase - resources increase speed per 1 second (it is increased automatically after training workers)
    * resources - number of resources
    * victories - number of won battles
  

```typedef struct {
    /* Player information struct. */
    int n;
    int military[3];
    int workers;
    int resources_increase; 
    int resources;
    int victories;
}player;```    

```struct mq_player_buf {
    /* Redefined msgbuf struct */
    long mtype;
    player play;
};```    
  

### Queues:
* mq0 - queue between server components
    * it is used to initialize the game - main process sends messages to every child process to start the game
    * every server process starts after getting initial message through this queue
* mq[3] - queue between dedicated server process and its client 
    * server sends updated player struct and message structs through those queues
    * players get through this queue battle info, notifications, number of resources and units
* mq_input[3] - queue between dedicated server process and its client
    * client sends requests through this queue
    * it is only used with messages of type message

## Shared memory
### Types
There are two types of data kept in shared memory:
* shm_int - integer with related data
```typedef struct {
    int id;
    int *addr;
    int semaphore;
}shm_int;```    
    * addr - pointer to integer
    * id - shared memory id
    * semaphore - semaphore related to this shared memory segment
* shm - type player with related data
```typedef struct {
    int id;
    player *addr;
    int semaphore;
}shm;```    
    * addr - pointer to player struct
    * id - shared memory id
    * semaphore - semaphore related to this shared memory segment
### Defined shared memory segments
* shm_int training_in_process[3] - array of shm_int for every player
    * if *(training_in_process[i].addr) == 0, player has no active training process
    * otherwise, if value is equal to 1, player has triggered a training process
* shm players[3] - array of shm for every player; it keeps basic and updated information about players
* shm_int all_ready - initially equal to 0, increments when player connects to server; game starts when reaches 3
* shm_int end_game - initially equal to 0, changes to 1 when 
    * someone reaches 5 victories
    * SIGINT is sent
    * player presses 'q'

## Summary
Connection is established when server receives initial message from client through proper mq[i]. 
Client then increments value of all_ready variable. When it reaches 3, main server process starts the game and sends another initial message (through mq0) to other server components.

During the game, client checks for player messages and other messages at mq[i] and sends user's input at mq_input[i]. 
