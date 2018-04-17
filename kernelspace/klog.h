#ifndef _KLOG_H_
#define _KLOG_H_

/*
Kernel logging macros.
*/

#include "linux_includes.h"
#include "config.h"

//convert int to string
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

//These macros fill in filename, function and line automatically.
//It is still needed to add the '\n' at the end of the message, contrary to userspace log_xxx macros.
#if VERBOSE
	#define klog_debug(...)		printk(KERN_DEBUG	MODULE_NAME " [" __FILE__ ":" STR(__LINE__) "]" __VA_ARGS__)
	#define klog_info(...)		printk(KERN_INFO	MODULE_NAME " [" __FILE__ ":" STR(__LINE__) "]" __VA_ARGS__)
#else
	#define klog_debug(...)	
	#define klog_info(...)	
#endif

#define klog_warning(...)	printk(KERN_WARNING	MODULE_NAME " [" __FILE__ ":" STR(__LINE__) "]" __VA_ARGS__)
#define klog_error(...)		printk(KERN_ERR		MODULE_NAME " [" __FILE__ ":" STR(__LINE__) "]" __VA_ARGS__)

#endif