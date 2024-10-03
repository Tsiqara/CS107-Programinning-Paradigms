#ifndef _BANK_H
#define _BANK_H

#include <semaphore.h>


typedef struct Bank {
  unsigned int numberBranches;
  int numberWorkers;
  int numberWorkersNotFinished;
  pthread_mutex_t checkIfLastToFinish;
  sem_t waitingToStartNextDay;
  pthread_mutex_t reportTransferLock;
  struct       Branch  *branches;
  struct       Report  *report;
  pthread_mutex_t anotherLock;
  
} Bank;

#include "account.h"

int Bank_Balance(Bank *bank, AccountAmount *balance);

Bank *Bank_Init(int numBranches, int numAccounts, AccountAmount initAmount,
                AccountAmount reportingAmount,
                int numWorkers);

int Bank_Validate(Bank *bank);
int Bank_Compare(Bank *bank1, Bank *bank2);



#endif /* _BANK_H */
