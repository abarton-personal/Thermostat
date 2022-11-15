#include "circular_buffer.h"



#define BUF_SIZE 4
static int buffer[BUF_SIZE];
static int current_position = 0;



void buffer_initialize(int init_avg){
    for (int i=0; i<BUF_SIZE; i++){
        buffer[i] = init_avg;
    }
}

void buffer_append(int new_num){
    buffer[current_position] = new_num;
    current_position++;
    if(current_position >= BUF_SIZE){
        current_position = 0;
    }
}



int buffer_get_avg(){
    int total = 0;
    for(int i=0; i<BUF_SIZE; i++){
        // * 10 so we don't lose granularity in integer divide
        total += buffer[i] * 10;
    }
    return (total / BUF_SIZE) / 10;
}
