#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>

#define JOIN_REQUEST 1000
#define JOIN_RESPONSE 2000

struct mesg_buffer {
  long mtype;
  char mtext[100];
};

void get_ids(char text[], int *sending_id, int *receiving_id){
  int n = strlen(text);
  int i=0, j=0;
  char number1[3], number2[3];
  while(text[i]!=' '){
    number1[j] = text[i];
    i++;
    j++;
  }
  *sending_id = atoi(number1);
  i++;
  j=0;
  while(i<n){
    number2[j]=text[i];
    i++;
    j++;
  }
  *receiving_id = atoi(number2);
}

int main(void){
  struct mesg_buffer message;
  int msgid, sending_id, receiving_id;
  char name[30];
  printf("name: ");
  fgets(name, sizeof(name), stdin);
  strcpy(message.mtext, name);
  msgid = msgget(1, IPC_EXCL);
  message.mtype = JOIN_REQUEST;
  if(msgsnd(msgid, &message, sizeof(message), 0)<-1){
    perror("server queue msgsnd");
    exit(1);
  }
  if(msgrcv(msgid, &message, sizeof(message), JOIN_RESPONSE, 0)<-1){
    perror("server queue msgrcv");
    exit(1);
  }
  if(strcmp(message.mtext, "full")==0){
    perror("Number of users are full");
    exit(1);
  }
  get_ids(message.mtext, &sending_id, &receiving_id);
  printf("receiving id: %d, sending id: %d\n", receiving_id, sending_id);
  pid_t pid = fork();
  if(pid==0){
    //child process
    while(1){
      if(msgrcv(msgid, &message, sizeof(message), receiving_id, 0)>=0){
        printf("\n%s",message.mtext);
        fflush(stdout);
      }
    }
  } else {
    while(1){
      printf("Write message: ");
      fgets(message.mtext, sizeof(message.mtext), stdin);
      message.mtype=sending_id;
      msgsnd(msgid, &message, sizeof(message), 0);
      if(strcmp(message.mtext, "leave\n")==0){
        kill(pid, SIGKILL);
        break;
      }
    }
  }
}