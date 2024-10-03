#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "account.c"
#include "error.h"
#include "debug.h"

/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  pthread_mutex_lock(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  pthread_mutex_lock(&(account->accountLock));

  Account_Adjust(bank,account, amount, 1);

  pthread_mutex_unlock(&(account->accountLock));
  pthread_mutex_unlock(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));

  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  pthread_mutex_lock(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  pthread_mutex_lock(&(account->accountLock));
  if (amount > Account_Balance(account)) {
    pthread_mutex_unlock(&(account->accountLock));
    pthread_mutex_unlock(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank,account, -amount, 1);
  pthread_mutex_unlock(&(account->accountLock));
  pthread_mutex_unlock(&(bank->branches[AccountNum_GetBranchID(accountNum)].branchLock));
  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if(srcAccount == dstAccount){
    return ERROR_SUCCESS;
  }
  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
  if(updateBranch == 0){
    if(dstAccountNum > srcAccountNum){
      pthread_mutex_lock(&(srcAccount->accountLock));
      pthread_mutex_lock(&(dstAccount->accountLock));
    }else{
      pthread_mutex_lock(&(dstAccount->accountLock));
      pthread_mutex_lock(&(srcAccount->accountLock));
    }
  
    if (amount > Account_Balance(srcAccount)) {
      pthread_mutex_unlock(&(srcAccount->accountLock));
      pthread_mutex_unlock(&(dstAccount->accountLock));
      return ERROR_INSUFFICIENT_FUNDS;
    }

    pthread_mutex_lock(&(bank->anotherLock));
    Account_Adjust(bank, srcAccount, -amount, updateBranch);
    Account_Adjust(bank, dstAccount, amount, updateBranch);
    pthread_mutex_unlock(&(bank->anotherLock));

    pthread_mutex_unlock(&(srcAccount->accountLock));
    pthread_mutex_unlock(&(dstAccount->accountLock));

    return ERROR_SUCCESS;
  }else{
    int sourceBranchID = AccountNum_GetBranchID(srcAccountNum);
    int destinationBranchID = AccountNum_GetBranchID(dstAccountNum);
    if(destinationBranchID > sourceBranchID){
      pthread_mutex_lock(&(bank->branches[sourceBranchID].branchLock));
      pthread_mutex_lock(&(bank->branches[destinationBranchID].branchLock));
      pthread_mutex_lock(&(srcAccount->accountLock));
      pthread_mutex_lock(&(dstAccount->accountLock));
    }else{
      pthread_mutex_lock(&(bank->branches[destinationBranchID].branchLock));
      pthread_mutex_lock(&(bank->branches[sourceBranchID].branchLock));
      pthread_mutex_lock(&(dstAccount->accountLock));
      pthread_mutex_lock(&(srcAccount->accountLock));
    }

    if (amount > Account_Balance(srcAccount)) {
      pthread_mutex_unlock(&(srcAccount->accountLock));
      pthread_mutex_unlock(&(dstAccount->accountLock));
      pthread_mutex_unlock(&(bank->branches[sourceBranchID].branchLock));
      pthread_mutex_unlock(&(bank->branches[destinationBranchID].branchLock));
      return ERROR_INSUFFICIENT_FUNDS;
    }
    pthread_mutex_lock(&(bank->anotherLock));
    Account_Adjust(bank, srcAccount, -amount, updateBranch);
    Account_Adjust(bank, dstAccount, amount, updateBranch);
    pthread_mutex_unlock(&(bank->anotherLock));

    pthread_mutex_unlock(&(srcAccount->accountLock));
    pthread_mutex_unlock(&(dstAccount->accountLock));
    pthread_mutex_unlock(&(bank->branches[sourceBranchID].branchLock));
    pthread_mutex_unlock(&(bank->branches[destinationBranchID].branchLock));
    return ERROR_SUCCESS;
  }
  
}
