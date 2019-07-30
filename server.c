#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define JOIN_REQUEST 1000
#define JOIN_RESPONSE 2000

struct mesg_buffer {
  long mtype;
  char mtext[100];
} message;

typedef struct node {
  int data;
  struct node *next;
};

int curr_id = 0;
struct node* userList = NULL;
int num_users = 0;

void append_node(int data){
  // adds new node to end of list
  struct node *root = userList;
  if(root==NULL){
    struct node *n = (struct node*)malloc(sizeof(struct node));
    n->data = data;
    n->next = NULL;
    userList = n;
  } else {
    while(root->next!=NULL){
      root = root->next;
    }
    struct node *n= (struct node*)malloc(sizeof(struct node));
    n->data = data;
    n->next = NULL;
    root->next = n;
  }
}

void remove_node(int data){
  // removes node from list
  struct node* root = userList;
  if(root->data==data){
    userList = userList->next;
    free(root);
  } else {
    while(root->next->data!=data){
      root=root->next;
    }
    struct node* temp = root->next;
    root->next = root->next->next;
    free(temp);
  }
}

// what will happen if two processes request to join at same time
void *messageThread(void *vargp){
  // thread to handle messages from a user
  int msgid = msgget(1, IPC_EXCL);
  char name[30];
  strcpy(name, message.mtext);
  name[strlen(name)-1]='\0';
  int sending_id = curr_id-1;
  int receiving_id = curr_id;
  num_users++;
  append_node(sending_id);
  sprintf(message.mtext, "%d %d", receiving_id, sending_id);
  message.mtype = JOIN_RESPONSE;
  if(msgsnd(msgid, &message, sizeof(message), 0)<0){
    perror("join response cannot be sent");
    exit(1);
  };
  printf("new user registered: %s\n", name);
  fflush(stdout);
  while(1){
    if(msgrcv(msgid, &message, sizeof(message), receiving_id, 0)>=0){
      message.mtext[strlen(message.mtext)-1]='\0';
      printf("%s sent message: %s\n", name, message.mtext);
      if(strcmp(message.mtext, "leave\n")==0){
        // remove user
        remove_node(sending_id);
        printf("user left: %s\n", name);
      } else {
        // send message to every current user
        char temp[30];
        temp[0] = '\0';
        strcat(temp, name);
        strcat(temp, ": ");
        strcat(temp, message.mtext);
        strcpy(message.mtext, temp);
        struct node* root = userList;
        while(root!=NULL){
          if(root->data!=sending_id){
            message.mtype = root->data;
            msgsnd(msgid, &message, sizeof(message), 0);
          }
          root=root->next;
        }
      }
      fflush(stdout);
    }
  }
}

int main(void){
  int msgid = msgget(1, 0666 | IPC_CREAT);
  pthread_t tids[100];
  int k=0;
  while(1){
    if(msgrcv(msgid, &message, sizeof(message), JOIN_REQUEST, 0)>=0){
      curr_id+=2;
      pthread_create(&tids[k], NULL, messageThread, NULL);
      k++;
    }
  }
  for(int i=0;i<k;i++){
    pthread_join(tids[i], NULL);
  }
  msgctl(msgid, IPC_RMID, NULL);
}