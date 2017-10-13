#ifndef __GDEBUG_H
#define __GDEBUG_H 1


#define GCUBE_VERSION			"0.5"
#define GCUBE_DESCRIPTION	("gcube v" GCUBE_VERSION)

#ifdef GDEBUG
#define EVENT_INFO				0
#define EVENT_QUIT				1
#define EVENT_OSREPORT		2
#define EVENT_TRAP				3
#define EVENT_LOG					4
#define EVENT_EFATAL			8
#define EVENT_EMAJOR			9
#define EVENT_EMINOR			10
#define EVENT_LOG_HLE			11
#define EVENT_DEBUG_MSG		12
#define EVENT_EXCEPTION		100
#define EVENT_UHW_READ		101
#define EVENT_UHW_WRITE		102
#define EVENT_BREAKPOINT	120
#define EVENT_STOP				200
#define EVENT_LOG_INT			400
#define EVENT_LOG_AI			401
#define EVENT_LOG_DSP			402
#define EVENT_LOG_CP			403
#define EVENT_LOG_DI			404
#define EVENT_LOG_EXI			405
#define EVENT_LOG_MI			406
#define EVENT_LOG_PE			407
#define EVENT_LOG_PI			408
#define EVENT_LOG_SI			409
#define EVENT_LOG_VI			410
#define EVENT_LOG_GX			411
#define EVENT_LOG_GX_IMM	412
#define EVENT_LOG_CALLS		413
#define EVENT_LOG_DUMMY		999


#define DEBUG										gdebug_event
void gdebug_event (int event, const char *format, ...);
void gdebug_run (__u32 pc);
void gdebug_print_intmask (__u32 mask, char *msg);
void gdebug_mem_read (__u32 addr);
void gdebug_mem_write (__u32 addr);
void gdebug_hw_read (__u32 addr);
void gdebug_hw_write (__u32 addr);

#else

#define DEBUG(X,format,...)			({})
#define gdebug_print_intmask(mask,msg)		{}

#endif // GDEBUG

void gcube_quit (char *msg);
void gcube_pe_refresh (void);
void gcube_perf_vertices (int count);
void gcube_refresh_manual (void);
void gcube_os_report (char *msg, int newline);
void gcube_save_state (void);
void gcube_load_state (void);


#endif // __GCUBE_H
