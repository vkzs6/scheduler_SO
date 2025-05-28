#ifndef SCHEDULE_EDF_H
#define SCHEDULE_EDF_H

#include "list.h" // Inclui task.h indiretamente


void edf_add(char *name, int priority, int burst, int deadline);

 
void edf_schedule();

#endif // SCHEDULE_EDF_H