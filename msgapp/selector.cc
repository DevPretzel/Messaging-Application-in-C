#include "selector.h"
#include <stdio.h>
#include <poll.h>
#include <sys/time.h>


typedef struct pollfd PollInfo;


InputSelector::InputSelector(int maxSize) 
{
  MaxSize = maxSize;
  ElementList = (void *) new PollInfo[MaxSize];
  NumElems = 0; 
  
   /* Used to store the set of active file descriptors. */
  ActiveSet = new int[MaxSize + 1]; 
}

InputSelector::~InputSelector()
{
  PollInfo *theList = (PollInfo *)ElementList;    
  delete theList;
  delete ActiveSet;
}

bool
InputSelector::add(int fd)
{
   /* Make sure there is room in the set for the fd. */
  if( NumElems == MaxSize ) 
    return false;
  
   /* Make sure the fd is not already in the set. */
  PollInfo *theList = (PollInfo *)ElementList;   
  for( int i = 0; i < NumElems; i++ )
    if(fd == theList[i].fd) 
      return false;
    
   /* Add the fd. */
  PollInfo elem = {.fd = fd, .events = POLLRDNORM | POLLIN, .revents = 0};
  theList[NumElems] = elem;
  NumElems++;
  
  return true;
}

bool
InputSelector::remove(int fd)
{
  PollInfo *theList = (PollInfo *)ElementList;  
  for(int i = 0; i < NumElems; i++)
    if(fd == theList[i].fd) {
      theList[i] = theList[NumElems - 1];
      NumElems--;
      return true;
    }
    
  return false;
}


void
InputSelector::clear()
{
  NumElems = 0;
}


int *
InputSelector::select()
{
  PollInfo *theList = (PollInfo *)ElementList;  
  for(int i = 0; i < NumElems; i++)
    theList[i].revents = 0;
  
  ActiveSet[0] = -1;
  
   /* Wait for input on one of the file descriptors without timing out. */
  int result = poll(theList, NumElems, -1);
  
  if(result <= 0)
    return ActiveSet;
    
   /* Traverse the list to find the FDs that have active input. */
  int pos = 0;
  for(int i = 0; i < NumElems; i++) {
    if(theList[i].revents != 0) {
      ActiveSet[pos] = theList[i].fd;
      pos++;
    }
  }
  
  ActiveSet[pos] = -1;
  return ActiveSet;
}

