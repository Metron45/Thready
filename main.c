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


int *top_X, *top_Y, *bottom_X, *bottom_Y;
int *cursor_x, *cursor_y, *hit_counter;
pthread_mutex_t *mutexLockThreads, *mutexLockObstacles;
pthread_cond_t *condLock;
bool finish;

int check_collision(int * data){
    bool col_x = false,col_y=false;

    for(int i=0;i<data[BOARD_OBSTACLES];i++){
        if(top_X[i] >= data[X] && data[X] >= bottom_X[i] && top_Y[i] >= data[Y] && data[Y] >= bottom_Y[i]){
                return i;
        }
    }
    return -1;
}

int check_side(int * data, int obstacle_index){
    int x=0;
    int y=0;

    if(data[X] == top_X[obstacle_index] || data[X] == bottom_X[obstacle_index]){
        x=1;
    }

    if(data[Y] == top_Y[obstacle_index] || data[Y] == bottom_Y[obstacle_index]){
        y=2;
    }
    return x+y;
}

int move_position(int * data){

    switch(data[MOVEMENT]){
        case 0:
            data[X]++;
            if(data[X] > AREA_SIZE*2){
                return 1;
            }
        break; 
        case 1:
            data[X]++;
            data[Y]--;
            if(data[X] > AREA_SIZE*2 || data[Y] < 0){
                return 1;
            }
        break;
        case 2:
            data[Y]--;
            if(data[Y] < 0){
                return 1;
            }
        break; 
        case 3:
            data[X]--;
            data[Y]--;
            if(data[X] < 0 || data[Y] < 0){
                return 1;
            }
        break;
        case 4:
            data[X]--;
            if(data[X] < 0){
                return 1;
            }
        break; 
        case 5:
            data[X]--;
            data[Y]++;
            if(data[X] < 0 || data[Y] > AREA_SIZE){
                return 1;
            }
        break;
        case 6:
            data[Y]++;
            if(data[Y] > AREA_SIZE){
                return 1;
            }
        break; 
        case 7:
            data[X]++;
            data[Y]++;
            if(data[X] > AREA_SIZE*2 || data[Y] > AREA_SIZE){
                return 1;
            }
        break;
    }

    int col = check_collision(data);
    if(col != -1){
        col= check_side(data, col);
        hit_counter[data[ID]] += col;
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
    //int thread_id = atoi((char*) ptr);
   
    pthread_mutex_lock(&mutexLockThreads[thread_id]);
    hit_counter[thread_id] = 0;
    pthread_mutex_unlock(&mutexLockThreads[thread_id]);
    
    while(!finish){
        //Speed decision
        usleep((MAX_SPEED / data[SPEED]) * 150000);
        
        //movement function
        if(move_position(data) == 1){
            pthread_mutex_lock(&mutexLockThreads[thread_id]);
            pthread_cond_wait(&condLock[thread_id], &mutexLockThreads[thread_id]);
            pthread_mutex_unlock(&mutexLockThreads[thread_id]);
        }

        //write to critical section
        pthread_mutex_lock(&mutexLockThreads[thread_id]);
        cursor_x[thread_id] = data[X];
        cursor_y[thread_id] = data[Y];
        pthread_mutex_unlock(&mutexLockThreads[thread_id]);
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

        //draw obstacles
        for(int y = 0 ; y < obstacles ; y++){
            move(AREA_SIZE + 4 + threads + y, 0);
            pthread_mutex_lock(&mutexLockObstacles[y]);
            printw("Obstacle: %2d Top: %2d %2d Bottom: %2d %2d", y, top_X[y], top_Y[y], bottom_X[y], bottom_Y[y]);
            pthread_mutex_unlock(&mutexLockObstacles[y]);
            for(int i=0;i<obstacles;i++){
                for(int x = bottom_X[i]; x<=top_X[i];x++){
                    for(int y = bottom_Y[i]; y<=top_Y[i];y++){
                        move(y,x);
                        printw("X");
                    }
                }
            }
        }

        //draw top info
        move(AREA_SIZE + 2, 0);
        printw("Threads: %d Obstacles: %d Size: %d", threads, obstacles, AREA_SIZE);

        //draw thread info
        for(int y = 0 ; y < threads ; y++){
            move(AREA_SIZE + 3 + y, 0);
            pthread_mutex_lock(&mutexLockThreads[y]);
            printw("Thread: %2d Hit_counted: %2d Position X:%2d Y:%2d", y, hit_counter[y], cursor_x[y], cursor_y[y]);
            if(cursor_x[y] != -1 && cursor_y[y] != -1){
                move(cursor_y[y] ,cursor_x[y] );
                printw("o");
            }
            pthread_mutex_unlock(&mutexLockThreads[y]);
        }
        

        refresh();
    }
}

void create_obstacle(int obstacle_index){
    int board_part = obstacle_index % 4;
    pthread_mutex_lock(&mutexLockObstacles[obstacle_index]);
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

    pthread_mutex_unlock(&mutexLockObstacles[obstacle_index]);
}

int main(int argc, char *argv[]){ 
    pthread_t thread;
    int error;
    finish = false;

    //initialize random
    srand(time(NULL));

    //number of threads
    int threads = 7;
    if (argc > 1) {
        threads = atoi(argv[1]);
    }

    //allocate
    cursor_x = malloc(threads * sizeof(int));
    cursor_y = malloc(threads * sizeof(int));
    hit_counter = malloc(threads * sizeof(int));
    mutexLockThreads = malloc(threads * sizeof(pthread_mutex_t));
    condLock = malloc(threads * sizeof(pthread_cond_t)); 

    //initialize
    for(int i = 0;i < threads; i++){
        cursor_x[i] = -1;
        cursor_y[i] = -1;
        hit_counter[i] = -1;
        if (pthread_mutex_init(&mutexLockThreads[i], NULL) != 0){
            return 1;
        }
        if (pthread_cond_init(&condLock[i], NULL) != 0){
            return 1;
        }
    }

    //create obstacles
    int obstacles = 4;
    if (argc > 2) {
        obstacles = atoi(argv[2]);
    }
    top_X = malloc(obstacles * sizeof(int));
    top_Y = malloc(obstacles * sizeof(int));
    bottom_X = malloc(obstacles * sizeof(int));
    bottom_Y = malloc(obstacles * sizeof(int));
    mutexLockObstacles = malloc(obstacles * sizeof(pthread_mutex_t));

    for(int i = 0; i < obstacles;i++){
        create_obstacle(i);
        //printf("%d %d %d %d\n",top_X[i], top_Y[i], bottom_X[i], bottom_Y[i]);
    }



    //initialize messages with thread_ids
    char **message = malloc((threads+1) * sizeof(char));
    for(int i = 0;i <= threads; i++){
        message[i] = malloc(2 * sizeof(char));
        sprintf(message[i], "%d-%d", i, obstacles);
        //printf("%s\n", message[i]);
    }
    sprintf(message[threads], "%d-%d", threads,obstacles);
    //printf("%s\n", message[threads]);

 
    //initialize ncurses screen <============================================
    initscr();
    noecho();
    curs_set(FALSE);

    //initialize threads
    for(int i=0; i < threads ; i++){
        error = pthread_create( &thread, NULL, thread_function, (void*) message[i]);
        if(error){
            endwin();
            printf("Thread failed to start");
            return 1;
        }
    }
    //initialize draw thread
    error = pthread_create( &thread, NULL, thread_draw, (void*) message[threads]);
    if(error){
        endwin();
        printf("Draw thread failed to start");
        return 1;
    }

    //wait for input to end program
    getch();
    finish = true;

    //finish threads
    for(int i=0; i < threads + 1 ; i++){
        pthread_join( thread, NULL);
    }

    //ending mutexes and condition locks
    for(int i = 0;i < threads; i++){
        pthread_mutex_unlock(&mutexLockThreads[i]);
        pthread_cond_signal(&condLock[i]);
        pthread_mutex_destroy(&mutexLockThreads[i]);
        pthread_cond_destroy(&condLock[i]);
    }

    //ending ncurses
    endwin();

    //freeing space
    free(cursor_x);
    free(cursor_y);
    free(mutexLockThreads);
    free(message);

    printf("Aplication exited succesfully\n");

    exit(EXIT_SUCCESS);
    return 0;
} 