#ifndef _MODULE_LOADER_H_
#define _MODULE_LOADER_H_

/*
Kernel modules loading and unloading
*/

//Loads a module from a file path. 
//Param values cannot be NULL but can be an empty string.
bool module_load(const char* filename, const char* param_values);

//Unloads a module from its name.
bool module_unload(const char* filename);

#endif 

