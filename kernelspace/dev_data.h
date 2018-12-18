#ifndef _DEV_DATA_H
#define _DEV_DATA_H

/*
Manages the /dev/rxdata & /dev/lockdata files
*/

#include "linux_includes.h"

//Creates & destroy /dev/rxdata
bool dev_rxdata_create(void);
void dev_rxdata_destroy(void);

//Creates & destroy /dev/lockdata
bool dev_lockdata_create(void);
void dev_lockdata_destroy(void);

#endif