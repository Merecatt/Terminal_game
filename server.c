#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h> //fifo
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "mq_utils.h"
#include "shm_utils.h"
#include "sem_utils.h"
#include <math.h>

typedef struct {
    int price[4];
    double attack[4];
    double defense[4];
    int production_time[4];
}units_stats;

units_stats G_units_stats = {{100, 250, 550, 150},
                             {1, 1.5, 3.5, 0}, 
                             {1.2, 3, 1.2, 0},
                             {2, 3, 5, 2}};

int interrupt;

int train_mq;
int train_pid[3];

void update_resources(int mq, player *play, int semaphore){
    printf("Updating resources.\n");
    sleep(1);
    sem_p(semaphore);
    play->resources += play->resources_increase; // CRITICAL SECTION
    mq_send_status(mq, play);
    sem_v(semaphore);
}

void remove_trash(shm players[], shm_int all_ready, shm_int end_game, int mq0, int mq[], int mq_input[]){
    /* Function removing created IPCs from the system */
    /* players' shared memory and semaphores */
    printf("Removing players' structures.\n");
    for (int i = 0; i < 3; i++){
        shm_detach(players[i].addr);
        shm_remove(players[i].id);
        sem_remove(players[i].semaphore);
    }

    /* all_ready and end_game shared memory and semaphores */
    printf("Removing all_ready.\n");
    shm_detach(all_ready.addr);
    shm_remove(all_ready.id);
    sem_remove(all_ready.semaphore);
    printf("Removing end_game.\n");
    shm_detach(end_game.addr);
    shm_remove(end_game.id);
    sem_remove(end_game.semaphore);

    /* message queues */
    printf("Removing message queues.\n");
    mq_remove(mq0);
    printf("Removing train_mq.\n");
    mq_remove(train_mq);
    for (int i = 0; i < 3; i++){
        mq_remove(mq[i]);
        mq_remove(mq_input[i]);
    }
}

int winner(shm players[]){
    /* Function returning id of the winner (in {0, 1, 2} notation). 
    If there is no winner, returns -1. */
    for (int i = 0; i < 3; i++){
        player *ptr = players[i].addr; // pointer to player's structure
        if ((*ptr).victories == 5){
            return i;
        }
    }
    return -1;
}

void add_unit(shm *player, int unit, shm_int end_game){
    /* Function adding single unit during training */
    sleep(G_units_stats.production_time[unit]);
    if (*(end_game.addr) == 1){
        printf("Training process number %d interrupted by end of the game.\n", (*(player->addr)).n-1);
        exit(0);
    }
    sem_p(player->semaphore);
    switch(unit){
        case 0:
            (*(player->addr)).military[0]++;
            break;
        case 1:
            (*(player->addr)).military[1]++;
            break;
        case 2:
            (*(player->addr)).military[2]++;
            break;
        case 3:
            (*(player->addr)).workers++;
            (*(player->addr)).resources_increase += 5;
            break;
        default:
            printf("Error while training.\n");
    }
    sem_v(player->semaphore);
}

int myceil(double x){
    /* implementation of ceiling function */
    int temp;
    temp = (int)x;
    if (x == (double)temp){
        return temp;
    }
    return temp+1;
}

int survivors_SASO (int units, double SA, double SO){
    //X * (SA/SO)
    int result;
    double factor;
    if (SO == 0.0 && SA == 0.0){
        return units;
    }
    if (SA == 0){
        return units;
    }
    if (units == 0){
        return 0;
    }
    factor = (double)SA/(SO+SA);
    result = units - myceil(units * factor);
    if (result > 0) 
        return result;
    else {
        printf("Error while counting survivors\n");
        return 0;
    }
}

int are_we_fighting(shm attacker, int units[], message *msg){
    /* Function checking if number of units sent to fight
    isn't greater than the number of units possessed by the player.
    If so, returns -1.
    If everything is okay, returns 0. */
    sem_p(attacker.semaphore);
    for (int i = 0; i < 3; i++){
        if ((*(attacker.addr)).military[i] < units[i]){
            msg->add_info = 3;
            strcpy(msg->text, "Incorrect number of units.");
            sem_v(attacker.semaphore);
            return -1;
        }
    }
    sem_v(attacker.semaphore);
    return 0;
}

void fight(shm attacker, shm defender, int units[], int mq[], shm_int end_game){
    if (fork() == 0){
        message msg;
        int n_attacker, n_defender;
        n_attacker = (*(attacker.addr)).n - 1;
        n_defender = (*(defender.addr)).n - 1;
        if (n_attacker == n_defender) {
            msg.add_info = 3;
            strcpy(msg.text, "You cannot kill that which has no life.");
            mq_send(mq[n_attacker], &msg, 3);
            exit(0);
        }
        else {
            if (are_we_fighting(attacker, units, &msg) == -1){
                mq_send(mq[n_attacker], &msg, 3);
            }
            else {

                sem_p(attacker.semaphore);
                for (int i = 0; i < 3; i++){
                    (*(attacker.addr)).military[i] -= units[i];
                }
                sem_v(attacker.semaphore);

                printf("Player %d attacks player %d\n", n_attacker, n_defender);
                sleep(5);
                int defenders[3]; // defender's units
                double attack_points = 0.0, defense_points = 0.0;
                sem_p(defender.semaphore);
                for (int i = 0; i < 3; i++){
                    defenders[i] = (*(defender.addr)).military[i];
                }
                sem_v(defender.semaphore);
                for (int i = 0; i < 3; i++){
                    attack_points += G_units_stats.attack[i] * units[i]; // unit attack points * number of units of that type
                }
                for (int i = 0; i < 3; i++){
                    defense_points += G_units_stats.defense[i] * defenders[i]; // unit defense points * number of units of that type
                }
                
                
                int attacker_survivors[3], defender_survivors[3];
                message msg_a, msg_d;

                if (attack_points > defense_points){ // attack is successful
                    printf("Player %d wins with %f attack against %f defense\n", n_attacker, attack_points, defense_points);
                    char temp[40]; 
                    sprintf(temp, "You won the battle with player %d", n_defender+1);
                    strcpy(msg_a.text, temp);
                    msg_a.add_info = 3;
                    sem_p(attacker.semaphore);
                    for (int i = 0; i < 3; i++){
                        attacker_survivors[i] = survivors_SASO(units[i], defense_points, attack_points);
                        (*(attacker.addr)).military[i] += attacker_survivors[i];
                        printf("Survivors of player %d type %d: %d\n", n_attacker, i, attacker_survivors[i]);
                    }
                    mq_send(mq[n_attacker], &msg_a, 3);
                    (*(attacker.addr)).victories++;
                    mq_send_status(mq[n_attacker], attacker.addr); 
                    sem_v(attacker.semaphore);
                    
                    sprintf(temp, "You lost the battle with player %d", n_attacker+1);
                    strcpy(msg_d.text, temp);
                    msg_d.add_info = 3;
                    sem_p(defender.semaphore);
                    for (int i = 0; i < 3; i++){
                        (*(defender.addr)).military[i] = 0;
                    }
                    mq_send(mq[n_defender], &msg_d, 3);
                    mq_send_status(mq[n_defender], defender.addr);
                    sem_v(defender.semaphore); 
                    if ((*(attacker.addr)).victories == 5){
                        sem_p(end_game.semaphore);
                        *(end_game.addr) = 1;
                        sem_v(end_game.semaphore);
                    }
                }
                else {
                    printf("Player %d defends with %f defense against %f attack\n", n_defender, defense_points, attack_points);
                    char temp[40];
                    sprintf(temp, "You lost the battle with player %d", n_defender+1);
                    strcpy(msg_a.text, temp);
                    msg_a.add_info = 3; 
                    sem_p(attacker.semaphore);
                    for (int i = 0; i < 3; i++){
                        attacker_survivors[i] = survivors_SASO(units[i], defense_points, attack_points);
                        (*(attacker.addr)).military[i] += attacker_survivors[i];
                        printf("Survivors of player %d type %d: %d\n", n_attacker, i, attacker_survivors[i]);
                    }
                    mq_send(mq[n_attacker], &msg_a, 3); 
                    mq_send_status(mq[n_attacker], attacker.addr);
                    sem_v(attacker.semaphore);

                    sprintf(temp, "You defended yourself from player %d", n_attacker+1);
                    strcpy(msg_d.text, temp);
                    msg_d.add_info = 3; // change to 4
                    sem_p(defender.semaphore);
                    for (int i = 0; i < 3; i++){
                        defender_survivors[i] = survivors_SASO(defenders[i], attack_points, defense_points);
                        (*(defender.addr)).military[i] += defender_survivors[i];
                        printf("Survivors of player %d type %d: %d\n", n_defender, i, defender_survivors[i]);
                    }
                    mq_send(mq[n_defender], &msg_d, 3);
                    mq_send_status(mq[n_defender], defender.addr);
                    sem_v(defender.semaphore); 
                    
                }
            }
            exit(0);
        }
    }
}

void assign_units (int *units, int *units2){
    for (int i = 0; i < 4; i++){
        units[i] = units2[i];
    }
}

void clear_message (message *msg){
    msg->add_info = -1;
    strcpy(msg->text, "");
    msg->unit_type = -1;
}

void train_units (int parent_pid, shm play, int mq_output, shm_int end_game){
    /* Function creating and maintaining training process.
    It is called only once (for each player) at the beginning, creates training process
    and triggers training for consecutive orders in the training queue. */
    int n = (*(play.addr)).n - 1;
    printf("Training process for player %d is started.\n", n);
    message msg;
    char temp[60];
    int unit_index;
    int units[4];
    int success_receive;
    while (*(end_game.addr) != 1){
        do {
            success_receive = mq_receive2(train_mq, &msg, n+1, IPC_NOWAIT);
        }
        while (success_receive == -1 && (*(end_game.addr)) != 1);
        if ((*(end_game.addr)) == 1){
            printf("Training process of player %d finishes.\n", n);
            exit(0);
        }
        unit_index = msg.unit_type;
        assign_units(units, msg.unit_number);
        sprintf(temp, "Training %d units of %d type in progress.", units[unit_index], unit_index);
        printf("%s\n", temp);
        msg.add_info = 3;
        strcpy(msg.text, temp);
        mq_send(mq_output, &msg, 3);

        while (units[unit_index] > 0){
            add_unit(&play, unit_index, end_game);
            mq_send_status(mq_output, (play.addr));
            units[unit_index]--;
        }
        msg.add_info = 3;
        strcpy(msg.text, "Training finished.");
        mq_send(mq_output, &msg, 3);
    }
    printf("Training process of player %d finishes.\n", n);
    exit(0);
}

void got_train_order (int units[], shm play, int mq_output){
    /* Function checking train order and queuing it if it's valid */
    int n = (*(play.addr)).n - 1;
    int cost = 0;
    int unit;
    int index = -1;
    message msg;
    for (int i = 0; i < 4; i++){
        if (units[i] != 0){
            index = i;
        }
    }
    if (index == -1){
        msg.add_info = 3;
        strcpy(msg.text, "You order to train no units.");
        mq_send(mq_output, &msg, 3);
    }
    else if (units[index] > 9 || units[index] < 0){
        msg.add_info = 3;
        strcpy(msg.text, "Incorrect number of units.");
        mq_send(mq_output, &msg, 3);
    }
    else {
        for (int i = 0; i < 4; i++){
            cost += G_units_stats.price[i] * units[i];
        }
        printf("Cost: %d\n", cost);
        sem_p(play.semaphore);
        int p_resources = (*(play.addr)).resources;
        if (p_resources < cost){
            sem_v(play.semaphore);
            msg.add_info = 3; // tell client to print the message
            strcpy(msg.text, "Not enough resources.");
            mq_send(mq_output, &msg, 3);
        }
        else {
            (*(play.addr)).resources -= cost;
            sem_v(play.semaphore);
            message msg;
            msg.add_info = 3;
            char temp[60];

            for (int i = 0; i < 4; i++){
                if (units[i] != 0){
                    unit = i; // this is the unit to train
                    sprintf(temp, "You ordered to train %d units of %d type.", units[i], i);
                    break;
                }
            }
            strcpy(msg.text, temp);
            mq_send(mq_output, &msg, 3);
            assign_units(msg.unit_number, units);
            msg.unit_type = unit;
            mq_send(train_mq, &msg, n+1);
            printf("Training %d units of type %d for player %d queued.\n", units[unit], unit, n);
        }
    }
}

void get_input(int mq, shm play, int mq_output, shm players[], int mqs[], shm_int end_game){
    int status;
    message msg;
    status = mq_receive2(mq, &msg, 0, IPC_NOWAIT); // MUST BE NOWAIT so the process can end in case of end of the game
    if (status != -1){
        printf("msg.action: %c\n", msg.action);
        switch(msg.action) {
            case 't':
                got_train_order(msg.unit_number, play, mq_output);
                break;
            case 'a':
                fight(players[(*(play.addr)).n - 1], players[msg.add_info - 1], msg.unit_number, mqs, end_game);
                break;
            case 'q':
                sem_p(end_game.semaphore);
                *(end_game.addr) = 1;
                sem_v(end_game.semaphore);
                break;
            default:
                printf("Player sent illegal message.\n");
        }
    }
}

void got_signal(){
    interrupt = 1;
    printf("Got SIGINT, goodbye cruel world.\n");
}

int main()
{   
    interrupt = 0;
    signal(SIGINT, got_signal);

    /* initialize players' structures with semaphores and shared memory */
    shm players[3];
    
    for (int i = 0; i < 3; i++){
        players[i].semaphore = sem_create(2137+i);      // create semaphores
        sem_initialize(players[i].semaphore);           // initialize semaphores
        players[i].id = shm_create(sizeof(player));     // create memory segment for a player
        players[i].addr = shm_attach(players[i].id);    // attach memory segment
        shm_players_init(players[i].addr, i);           // initialize players' structures
    }

    int ppid; 
    ppid = getpid(); // main server process' pid

    /* initialize all_ready and its semaphore */
    shm_int all_ready;
    all_ready.id = shm_create(sizeof(int));
    all_ready.addr = shm_attach(all_ready.id);
    *(all_ready.addr) = 0;  // initial number of ready players; start the game when reaches 3
    all_ready.semaphore = sem_create2();
    sem_initialize(all_ready.semaphore);

    /* initialize end_game and its semaphore */
    shm_int end_game;
    end_game.id = shm_create3(sizeof(int), 1945);
    end_game.addr = shm_attach(end_game.id);
    *(end_game.addr) = 0;  // when it reaches 1, it's time to finish
    end_game.semaphore = sem_create2();
    sem_initialize(end_game.semaphore);

    /* initialize message queue to communicate with child processes */
    int mq0;
    mq0 = mq_create();
    /* initialize message queue for training */
    train_mq = mq_create();

    /* initialize message queues and processes for communication with players */
    int pid[3];          // server processes dedicated for players
    int mq[3];           // server->player message queues' id
    int pid_input[3];    // dedicated for getting players' input processes
    int mq_input[3];     // message queues for getting players' input
    for (int i = 0; i < 3; i++){
        mq[i] = mq_open(2137 + i);
        mq_input[i] = mq_open(2140+i);
    }

    /* connect with clients */
    mq_init(0, &pid[0], mq[0], all_ready); // get player 0
    if (getpid() == ppid){ // only the main process calls fork
        mq_init(1, &pid[1], mq[1], all_ready); // get player 1
    }
    if (getpid() == ppid){ // only the main process calls fork
        mq_init(2, &pid[2], mq[2], all_ready); // get player 2
    }

    /* fork to get processes dedicated for user's input */
    for (int i = 0; i < 3; i++){
        if (getpid() == ppid){
            if ((pid_input[i] = fork()) == -1) {
                perror("preparing to get input - fork");
            }
        }
    }

    /* Prepare train processes */
    for (int i = 0; i < 3; i++){
        if (getpid() == ppid){
            train_pid[i] = fork();
            if (train_pid[i] == 0){
                train_units(ppid, players[i], mq[i], end_game);
            }
        }
    }

    /* start the game */
    if (getpid() == ppid){ // parent process
    while (*(all_ready.addr) != 3){ // wait to start the game
        sleep(1);
    }

    printf("\nLet's start the game!\n");
    
    /* send initial message to child processes to start the game */
    message init_message = {-1, -1, -1, 1, "", ' '};
    for (int i = 0; i < 6; i++) //send six messages for two processes of every player
        mq_send(mq0, &init_message, 1);
    } 

    message start_game;

    /* server child processes should wait until they get message from parent */

    /* Update resources until the end of the game */
    for (int i = 0; i < 3; i++) {
        if (pid[i] == 0){
            mq_receive(mq0, &start_game, 1); // wait for the initial message
                if (start_game.add_info == 1){ // if you got the initial message
                    while (*(end_game.addr) != 1){
                        update_resources(mq[i], players[i].addr, players[i].semaphore);
                    }
                    int who_won = winner(players);
                    message msg_end_game;
                    msg_end_game.add_info = 2; // set info that the game is over
                    /* set text field so that is says who the winner is */
                    if (who_won == -1){
                        strcpy(msg_end_game.text, "Nobody won.");
                    }
                    else {
                        char str[20], temp[10];
                        strcpy(str, "Player ");
                        sprintf(temp, "%d", who_won+1);
                        strcat(str, temp);
                        strcat(str, " won.");
                        strcpy(msg_end_game.text, str);
                    }
                    mq_send(mq[i], &msg_end_game, 3);
                    printf("Output process %d finishes.\n", i);
                    exit(0);
                }
        }
    }

    /* Get user's input until the end of the game */
    for (int i = 0; i < 3; i++) {
        if (pid_input[i] == 0){
            signal(SIGCLD, SIG_IGN); // ignore SIGCLD not to make a zombie
            mq_receive(mq0, &start_game, 1); // wait for the initial message
            if (start_game.add_info == 1){ // if you got the initial message
                while (*(end_game.addr) != 1){
                    get_input(mq_input[i], players[i], mq[i], players, mq, end_game);
                }
                printf("Input process %d finishes.\n", i);
                exit(0);
            }
        }
    }

    if (getpid() == ppid){ 
        
        while (*(end_game.addr) != 1){ // main server process sleeps until the end of the game
            if (interrupt == 1){
                sem_p(end_game.semaphore);
                *(end_game.addr) = 1;
                sem_v(end_game.semaphore);
                sleep(1);
            }
            sleep(1);
        }
        
        /* delete all the trash from system */
        printf("Waiting for children to end.\n");
        for (int i = 0; i < 9; i++){
            wait(NULL);
        }
        sleep(3);
        remove_trash(players, all_ready, end_game, mq0, mq, mq_input);
        printf("Trash removed successfully.\n");
    }

    exit(0);
}
