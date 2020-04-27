#include <stdio.h>
#include <stdlib.h>

// struct inode{
// int blocks[5];
// };

// void main(){
// int i = 0;
// struct inode *in;
// in = (struct inode *) malloc(sizeof(struct inode));
// for( int i = 0;i<5;i++){
//     // in.blocks[i] = 1;
//     printf("%d ",in->blocks[i]);
//     }
// // printf("%d",i);
// }

void sa(int *a, int b){
    *a = b;
}

void main(){
    int *a, b =5;
    sa(a,b);
    printf("alll done\n");
}