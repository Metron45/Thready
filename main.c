#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

//max definitions
#define MAX_SPEED 4
#define AREA_SIZE 30
#define START_AREA 8

//data * of ball definitions
#define X 0
#define Y 1
#define MOVEMENT 2
#define SPEED 3
#define BOARD_OBSTACLES 4
#define ID 5

int *top_X, *top_Y, *bottom_X, *bottom_Y; //obstacle info
int *cursor_x, *cursor_y, *hit_counter; //ball thread info
bool *is_asleep; //ball thread into
pthread_mutex_t mutexLockThreads, mutexLockObstacles;
pthread_cond_t *condLock;
bool finish; //check for the end of the program

int check_collision(int * data){
    for(int i=0;i<data[BOARD_OBSTACLES];i++){

        pthread_mutex_lock(&mutexLockObstacles);
        if(top_X[i] >= data[X] && data[X] >= bottom_X[i] && top_Y[i] >= data[Y] && data[Y] >= bottom_Y[i]){
            pthread_mutex_unlock(&mutexLockObstacles);
            return i;
        }
        pthread_mutex_unlock(&mutexLockObstacles);
    }
    return -1;
}

int check_side(int * data, int obstacle_index){
    int col=0;

    pthread_mutex_lock(&mutexLockObstacles);
    if(data[X] == top_X[obstacle_index]){
        col+=1;
    }
    else if(data[X] == bottom_X[obstacle_index]){
        col+=1;
    }

    if(data[Y] == top_Y[obstacle_index]){
        col+=2;
    }
    else if(data[Y] == bottom_Y[obstacle_index]){
        col+=2;
    }

    pthread_mutex_unlock(&mutexLockObstacles);
    return col;
}

int move_ball(int * data){
    switch (data[MOVEMENT]) {
        case 0:
            data[X]++;
            break;
        case 1:
            data[X]++;
            data[Y]--;
            break;
        case 2:
            data[Y]--;
            break;
        case 3:
            data[X]--;
            data[Y]--;
            break;
        case 4:
            data[X]--;
            break;
        case 5:
            data[X]--;
            data[Y]++;
            break;
        case 6:
            data[Y]++;
            break;
        case 7:
            data[X]++;
            data[Y]++;
            break;
    }
    if(data[X] < 0 || data[X] > AREA_SIZE * 2 || data[Y] > AREA_SIZE || data[Y] < 0){
        return 1;
    }

    return 0;
}

void change_movement(int * data, int col){

    data[MOVEMENT] = (data[MOVEMENT] + 4)%8;
    move_ball(data);
    data[MOVEMENT] = (data[MOVEMENT] + 4)%8;

    switch (data[MOVEMENT]) {
        case 0:
            data[MOVEMENT] = 3 + (rand() % 3);
            break;
        case 1:
            if (col == 1) {
                data[MOVEMENT] = 3 + (rand() % 2);
            } else if (col == 2) {
                data[MOVEMENT] = 6 + (rand() % 2);
            } else {
                data[MOVEMENT] = 4 + (rand() % 3);
            }
            break;
        case 2:
            data[MOVEMENT] = 5 + (rand() % 3);
            break;
        case 3:
            if (col == 1) {
                data[MOVEMENT] = (rand() % 2);
            } else if (col == 2) {
                data[MOVEMENT] = 5 + (rand() % 2);
            } else {
                data[MOVEMENT] = (6 + (rand() % 3)) % 8;
            }
            break;
        case 4:
            data[MOVEMENT] = (7 + rand() % 3) % 8;
        case 5:
            if (col == 1) {
                data[MOVEMENT] = (7 + (rand() % 2)) % 8;
            } else if (col == 2) {
                data[MOVEMENT] = 2 + (rand() % 2);
            } else {
                data[MOVEMENT] = (rand() % 3);
            }
            break;
        case 6:
            data[MOVEMENT] = 1 + (rand() % 3);
            break;
        case 7:
            if (col == 1) {
                data[MOVEMENT] = 4 + (rand() % 2);
            } else if (col == 2) {
                data[MOVEMENT] = 1 + (rand() % 2);
            } else {
                data[MOVEMENT] = 2 + (rand() % 3);
            }
            break;
        default:
            data[MOVEMENT] =  (data[MOVEMENT] + 4)%8;
        break;
    }
}

int move_position(int * data){

    //move ball and check for falling out of box
    int out = move_ball(data);
    if(out == 1){
        return 1;
    }

    //check for collision with obstacle
    int col = check_collision(data);
    if(col > -1){
        col = check_side(data, col);
        pthread_mutex_lock(&mutexLockThreads);
        hit_counter[data[ID]] ++;
        pthread_mutex_unlock(&mutexLockThreads);
        change_movement(data,col);
    }

    return 0;
}

void *thread_function( void * ptr ){
    int thread_id;
    int obstacles;
    sscanf(ptr, "%d-%d", &thread_id, &obstacles);

    int data[] = {
        AREA_SIZE - START_AREA/2 + (int)(rand() % START_AREA) + 1, //X starting position
        AREA_SIZE/2 - START_AREA/2 + (int)(rand() % START_AREA) + 1, //Y starting position
        (int)(rand() % 8), //Movement direction
        (int)(rand() % MAX_SPEED) + 1, //Speed
        obstacles, //local data of how many obstacles board has
        thread_id
    }; 
    //printf("Thread %d\n", thread_id);

    while(!finish){
        //Speed decision
        usleep((MAX_SPEED / data[SPEED]) * 150000);
        
        //movement function
        if(move_position(data) == 1){
            pthread_mutex_lock(&mutexLockThreads);
            is_asleep[thread_id] = true;
            pthread_cond_wait(&condLock[thread_id], &mutexLockThreads);
            is_asleep[thread_id] = false;

            data[X] = AREA_SIZE - START_AREA/2 + (int)(rand() % START_AREA) + 1; //X starting position
            data[Y] = AREA_SIZE/2 - START_AREA/2 + (int)(rand() % START_AREA) + 1; //Y starting position
            data[MOVEMENT] = (int)(rand() % 8); //Movement direction

            pthread_mutex_unlock(&mutexLockThreads);
        }

        //write to critical section
        pthread_mutex_lock(&mutexLockThreads);
        cursor_x[thread_id] = data[X];
        cursor_y[thread_id] = data[Y];
        pthread_mutex_unlock(&mutexLockThreads);
    }
}

void *thread_draw( void * ptr ){
    //read number of threads
    int threads;
    int obstacles;
    sscanf(ptr, "%d-%d", &threads, &obstacles);

    while(!finish){
        //wait for redraw
        usleep(100000);
        clear();
       
        //draw boundries
        for(int i = 0; i <= (AREA_SIZE*2+1); i++){
            move(AREA_SIZE + 1,i);
            printw("H");
        }
        for(int i = 0; i <= AREA_SIZE; i++){
            move(i,AREA_SIZE * 2 + 1);
            printw("H");
        }



        //draw top info
        move(AREA_SIZE + 2, 0);
        printw("Threads: %d Obstacles: %d Size: %d", threads, obstacles, AREA_SIZE);

        //draw obstacles
        for(int y = 0 ; y < obstacles ; y++){
            move(AREA_SIZE + 4 + threads + y, 0);
            pthread_mutex_lock(&mutexLockObstacles);
            printw("Obstacle: %2d Top: %2d %2d Bottom: %2d %2d", y, top_X[y], top_Y[y], bottom_X[y], bottom_Y[y]);
            
            for(int i=0;i<obstacles;i++){
                for(int x = bottom_X[i]; x<=top_X[i];x++){
                    for(int y = bottom_Y[i]; y<=top_Y[i];y++){
                        move(y,x);
                        printw("X");
                    }
                }
            }
            pthread_mutex_unlock(&mutexLockObstacles);
        }

        //draw thread info
        for(int y = 0 ; y < threads ; y++){
            move(AREA_SIZE + 3 + y, 0);
            pthread_mutex_lock(&mutexLockThreads);
            printw("Thread: %2d Hit_counted: %2d IsAsleep: %d Position X:%2d Y:%2d", y, hit_counter[y], is_asleep[y], cursor_x[y], cursor_y[y]);
            if(cursor_x[y] != -1 && cursor_y[y] != -1){
                move(cursor_y[y] ,cursor_x[y] );
                printw("o");
            }
            pthread_mutex_unlock(&mutexLockThreads);
        }
        
        

        refresh();
    }
}

void *thread_unlock( void * ptr ){
    int asleep_count = 0;
    int max_asleep; 
    int threads;
    sscanf(ptr, "%d-%d", &max_asleep, &threads);
    int thread_with_least_hits;
    while(!finish){

        usleep(50000);

        thread_with_least_hits = rand()%threads;
        asleep_count =0;
        for(int i = 0; i < threads; i++){
            pthread_mutex_lock(&mutexLockThreads);
                if(is_asleep[i] == 1){
                    asleep_count++;
                    if(hit_counter[i]<hit_counter[thread_with_least_hits]){
                        thread_with_least_hits = i;
                    }
                }
            pthread_mutex_unlock(&mutexLockThreads);
        }
        if(asleep_count > (threads - max_asleep + 1)){
            pthread_cond_signal(&condLock[thread_with_least_hits]);
        }
    }
}

void create_obstacle(int obstacle_index){
    int board_part = obstacle_index % 4;
    if(board_part == 0){
        top_X[obstacle_index]    = (int)(rand() % (AREA_SIZE*2 - ((AREA_SIZE*2-START_AREA)/2)));
        bottom_X[obstacle_index] = (int)(rand() % (AREA_SIZE*2 - ((AREA_SIZE*2-START_AREA)/2)));
        top_Y[obstacle_index]    = (int)(rand() % ((AREA_SIZE-START_AREA)/2));
        bottom_Y[obstacle_index] = (int)(rand() % ((AREA_SIZE-START_AREA)/2));
    }
    else if(board_part == 1){
        top_X[obstacle_index]    = (AREA_SIZE*2 - ((AREA_SIZE*2-START_AREA)/2)) + (int)(rand() % ((AREA_SIZE*2-START_AREA)/2));
        bottom_X[obstacle_index] = (AREA_SIZE*2 - ((AREA_SIZE*2-START_AREA)/2)) + (int)(rand() % ((AREA_SIZE*2-START_AREA)/2));
        top_Y[obstacle_index]    = (int)(rand() % (AREA_SIZE - ((AREA_SIZE-START_AREA)/2)));
        bottom_Y[obstacle_index] = (int)(rand() % (AREA_SIZE - ((AREA_SIZE-START_AREA)/2)));
    }
    else if(board_part == 2){
        top_X[obstacle_index]    = ((AREA_SIZE*2-START_AREA)/2) + (int)(rand() % (AREA_SIZE*2 - ((AREA_SIZE*2-START_AREA)/2)));
        bottom_X[obstacle_index] = ((AREA_SIZE*2-START_AREA)/2) + (int)(rand() % (AREA_SIZE*2 - ((AREA_SIZE*2-START_AREA)/2)));
        top_Y[obstacle_index]    = (AREA_SIZE - ((AREA_SIZE-START_AREA)/2)) + (int)(rand() % ((AREA_SIZE-START_AREA)/2));
        bottom_Y[obstacle_index] = (AREA_SIZE - ((AREA_SIZE-START_AREA)/2)) + (int)(rand() % ((AREA_SIZE-START_AREA)/2));
    }
    else if(board_part == 3){
        top_X[obstacle_index]    = (int)(rand() % ((AREA_SIZE*2-START_AREA)/2));
        bottom_X[obstacle_index] = (int)(rand() % ((AREA_SIZE*2-START_AREA)/2));
        top_Y[obstacle_index]    = ((AREA_SIZE-START_AREA)/2) + (int)(rand() % (AREA_SIZE - ((AREA_SIZE-START_AREA)/2)));
        bottom_Y[obstacle_index] = ((AREA_SIZE-START_AREA)/2) + (int)(rand() % (AREA_SIZE - ((AREA_SIZE-START_AREA)/2)));
    }

    int temp;
    if(top_X[obstacle_index] < bottom_X[obstacle_index]){
        temp = top_X[obstacle_index];
        top_X[obstacle_index] = bottom_X[obstacle_index];
        bottom_X[obstacle_index] = temp;
    }

    if(top_Y[obstacle_index] < bottom_Y[obstacle_index]){
        temp = top_Y[obstacle_index];
        top_Y[obstacle_index] = bottom_Y[obstacle_index];
        bottom_Y[obstacle_index] = temp;
    }
}

int main(int argc, char *argv[]){ 
    pthread_t thread;
    int error;
    finish = false;

    //initialize random
    srand(time(NULL));

    //number of threads
    int threads = 6;
    if (argc > 1) {
        threads = atoi(argv[1]);
    }
    int max_active_threads = 3;
    if (argc > 2) {
        max_active_threads = atoi(argv[2]);
    }

    //allocate
    cursor_x = malloc(threads * sizeof(int));
    cursor_y = malloc(threads * sizeof(int));
    hit_counter = malloc(threads * sizeof(int));
    is_asleep = malloc(threads * sizeof(bool));
    condLock = malloc(threads * sizeof(pthread_cond_t)); 

    //initialize
    for(int i = 0;i < threads; i++){
        //ball threads
        cursor_x[i] = -1;
        cursor_y[i] = -1;
        hit_counter[i] = 0;
        is_asleep[i] = false;
        //mutexes
        if (pthread_cond_init(&condLock[i], NULL) != 0){
            return 1;
        }
    }

    if (pthread_mutex_init(&mutexLockThreads, NULL) != 0){
            return 1;
    }

    //create obstacles
    int obstacles = 4;
    if (argc > 3) {
        obstacles = atoi(argv[3]);
    }

    top_X = malloc(obstacles * sizeof(int));
    top_Y = malloc(obstacles * sizeof(int));
    bottom_X = malloc(obstacles * sizeof(int));
    bottom_Y = malloc(obstacles * sizeof(int));

    for(int i = 0; i < obstacles;i++){
       
        create_obstacle(i);
    }

    if (pthread_mutex_init(&mutexLockObstacles, NULL) != 0){
        return 1;
    }

    //initialize messages
    char **message = malloc((threads + 2) * sizeof(char*));
    for(int i = 0;i < threads; i++){
        message[i] = malloc(10 * sizeof(char));
        sprintf(message[i], "%d-%d", i, obstacles);
    }
    message[threads] = malloc(10 * sizeof(char));
    sprintf(message[threads], "%d-%d", threads,obstacles);
    message[threads+1] = malloc(10 * sizeof(char));
    sprintf(message[threads+1], "%d-%d", max_active_threads, threads);
 
    //initialize ball threads
    for(int i=0; i < threads ; i++){
        error = pthread_create( &thread, NULL, thread_function, (void*) message[i]);
        if(error){
            printf("Thread failed to start");
            return 1;
        }
    }
    //initialize draw thread
    error = pthread_create( &thread, NULL, thread_draw, (void*) message[threads]);
    if(error){
        printf("Draw thread failed to start");
        return 1;
    }

    //initialize unlocking thread
    error = pthread_create( &thread, NULL, thread_unlock, (void*) message[threads + 1]);
    if(error){
        printf("Unlock thread failed to start");
        return 1;
    }

    //initialize ncurses screen
    //debug------------------------------------------------------------------------------------------------
    initscr();
    noecho();
    curs_set(FALSE);

    //wait for input to end programdraw
    getch();
    finish = true;

   

    //ending mutexes and condition locks
    for(int i = 0;i < threads; i++){
        pthread_mutex_unlock(&mutexLockThreads);
        pthread_cond_signal(&condLock[i]);
        pthread_cond_destroy(&condLock[i]);
    }
    pthread_mutex_unlock(&mutexLockThreads);
    pthread_mutex_destroy(&mutexLockThreads);

    pthread_mutex_unlock(&mutexLockObstacles);
    pthread_mutex_destroy(&mutexLockObstacles);

    //finish threads
    for(int i=0; i < threads + 2 ; i++){
        pthread_join( thread, NULL);
    }

    //ending ncurses
    endwin();

    //freeing space
    free(top_X);
    free(top_Y);
    free(bottom_X);
    free(bottom_Y);

    free(cursor_x);
    free(cursor_y);
    free(hit_counter);
    free(is_asleep);

    free(message);

    printf("Aplication exited succesfully\n");

    exit(EXIT_SUCCESS);
    return 0;
} 