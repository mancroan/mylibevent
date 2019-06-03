/*
 * Copyright (c) 2000-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EVENT_INTERNAL_H_INCLUDED_
#define EVENT_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include "evconfig-private.h"

#include <time.h>
#include <sys/queue.h>
#include "event2/event_struct.h"
#include "minheap-internal.h"
#include "evsignal-internal.h"
#include "mm-internal.h"
#include "defer-internal.h"

/* map union members back */

/* mutually exclusive */
#define ev_signal_next	ev_.ev_signal.ev_signal_next
#define ev_io_next	ev_.ev_io.ev_io_next
#define ev_io_timeout	ev_.ev_io.ev_timeout

/* used only by signals */
#define ev_ncalls	ev_.ev_signal.ev_ncalls
#define ev_pncalls	ev_.ev_signal.ev_pncalls

#define ev_pri ev_evcallback.evcb_pri
#define ev_flags ev_evcallback.evcb_flags
#define ev_closure ev_evcallback.evcb_closure
#define ev_callback ev_evcallback.evcb_cb_union.evcb_callback
#define ev_arg ev_evcallback.evcb_arg

//�¼��ر�ʱ�Ļص�����ģʽ���� 
/** @name Event closure codes

    Possible values for evcb_closure in struct event_callback

    @{
 */
/** A regular event. Uses the evcb_callback callback */
// �����¼���ʹ��evcb_callback�ص�
#define EV_CLOSURE_EVENT 0
/** A signal event. Uses the evcb_callback callback */
// �ź��¼���ʹ��evcb_callback�ص�
#define EV_CLOSURE_EVENT_SIGNAL 1
/** A persistent non-signal event. Uses the evcb_callback callback */
// �����Է��ź��¼���ʹ��evcb_callback�ص�
#define EV_CLOSURE_EVENT_PERSIST 2
/** A simple callback. Uses the evcb_selfcb callback. */
// �򵥻ص���ʹ��evcb_selfcb�ص�
#define EV_CLOSURE_CB_SELF 3
/** A finalizing callback. Uses the evcb_cbfinalize callback. */
// �����Ļص���ʹ��evcb_cbfinalize�ص�
#define EV_CLOSURE_CB_FINALIZE 4
/** A finalizing event. Uses the evcb_evfinalize callback. */
// �����¼��ص���ʹ��evcb_evfinalize�ص�
#define EV_CLOSURE_EVENT_FINALIZE 5
/** A finalizing event that should get freed after. Uses the evcb_evfinalize
 * callback. */
// �����¼�֮��Ӧ���ͷţ�ʹ��evcb_evfinalize�ص�
#define EV_CLOSURE_EVENT_FINALIZE_FREE 6
/** @} */

/** Structure to define the backend of a given event_base. */
struct eventop {
	/** The name of this backend. */
	// ��̨�������֣���epoll��select��poll��
	const char *name;
	/** Function to set up an event_base to use this backend.  It should
	 * create a new structure holding whatever information is needed to
	 * run the backend, and return it.  The returned pointer will get
	 * stored by event_init into the event_base.evbase field.  On failure,
	 * this function should return NULL. */
	// ����libevent���event_baseʹ�õ�ǰ��̨��������Ӧ�ô����µ����ݽṹ��
     // �����˺�̨���������������Ϣ��Ȼ�󷵻���Щ��Ϣ�Ľṹ�壬Ϊ��֧�ֶ���
     // �ṹ�壬��˷���void*�����ص�ָ�뽫������event_base.evbase�У����ʧ�ܣ�
     // ������NULL
	void *(*init)(struct event_base *);
	/** Enable reading/writing on a given fd or signal.  'events' will be
	 * the events that we're trying to enable: one or more of EV_READ,
	 * EV_WRITE, EV_SIGNAL, and EV_ET.  'old' will be those events that
	 * were enabled on this fd previously.  'fdinfo' will be a structure
	 * associated with the fd by the evmap; its size is defined by the
	 * fdinfo field below.  It will be set to 0 the first time the fd is
	 * added.  The function should return 0 on success and -1 on error.
	 */

     // ʹ�������ļ������������źű�ÿɶ����߿�д����events���������ǳ�����ӵ�
     // �¼����ͣ�һ�����߸����EV_READ,EV_WRITE,EV_SIGNAL,EV_ET����old������Щ�¼�
     // ��ǰ���¼����ͣ���fdinfo������fd��evmap�еĸ����ṹ����Ϣ�����Ĵ�С�������
     // fdinfo_len������fd��һ�����ʱ������Ϊ0.�ɹ��򷵻�0��ʧ���򷵻أ�1
	int (*add)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);

	/** As "add", except 'events' contains the events we mean to disable. */
     // ɾ���¼�
	int (*del)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	/** Function to implement the core of an event loop.  It must see which
	    added events are ready, and cause event_active to be called for each
	    active event (usually via event_io_active or such).  It should
	    return 0 on success and -1 on error.
	 */
 // event_loopʵ�ֵĺ��Ĵ��롣����������Щ��ӵ��¼��Ѿ�׼���ã�Ȼ�󴥷�ÿ��
     // ��Ծ�¼��������ã�ͨ����ͨ��event_io_active�����������������ɹ�����0��ʧ����1
	int (*dispatch)(struct event_base *, struct timeval *);
	/** Function to clean up and free our data from the event_base. */
	// ���event_base���ͷ�����
	void (*dealloc)(struct event_base *);
	/** Flag: set if we need to reinitialize the event base after we fork.
	 */

     // ��ִ��fork֮���Ƿ���Ҫ���³�ʼ���ı�ʶλ
	int need_reinit;
    /** Bit-array of supported event_method_features that this backend can
     * provide. */
     // ��̨���������ṩ������
     // enum event_method_feature {
     // ���ش���
     // EV_FEATURE_ET = 0x01,
     // Ҫ���º�̨�����ڵ��Ⱥܶ��¼�ʱ��ԼΪO(1)������select��poll�޷��ṩ����������
     // �����ַ�������N���¼�ʱ�������ṩO(N)����
    // EV_FEATURE_O1 = 0x02,
 
     // ��̨�������Դ�������ļ�������������������sockets
    // EV_FEATURE_FDS = 0x04,
    /** Require an event method that allows you to use EV_CLOSED to detect
     * connection close without the necessity of reading all the pending data.
     *
     * Methods that do support EV_CLOSED may not be able to provide support on
     * all kernel versions.
     **/
     // Ҫ���̨��������ʹ��EV_CLOSED������������Ƿ��жϣ�������Ҫ��ȡ
     // ����δ�����ݣ����ǲ��������ں˶����ṩ��������
    // EV_FEATURE_EARLY_CLOSE = 0x08

	enum event_method_feature features;
	/** Length of the extra information we should record for each fd that
	    has one or more active events.  This information is recorded
	    as part of the evmap entry for each fd, and passed as an argument
	    to the add and del functions above.
	 */

     // Ӧ��Ϊÿ���ļ������������Ķ�����Ϣ���ȣ�������Ϣ���ܰ���һ�����߶��
     // ��Ծ�¼��������Ϣ�Ǵ洢��ÿ���ļ���������evmap�У�Ȼ��ͨ����������
     // �������add��del�����С�
	size_t fdinfo_len;
};

#ifdef _WIN32
/* If we're on win32, then file descriptors are not nice low densely packed
   integers.  Instead, they are pointer-like windows handles, and we want to
   use a hashtable instead of an array to map fds to events.
*/
#define EVMAP_USE_HT
#endif

/* #define HT_CACHE_HASH_VALS */

#ifdef EVMAP_USE_HT
#define HT_NO_CACHE_HASH_VALUES
#include "ht-internal.h"
struct event_map_entry;
HT_HEAD(event_io_map, event_map_entry);
#else
#define event_io_map event_signal_map
#endif


//���濴struct event_signal_map����
/* Used to map signal numbers to a list of events.  If EVMAP_USE_HT is not
   defined, this structure is also used as event_io_map, which maps fds to a
   list of events.
*/
// �����洢�ź����ֺ�һϵ���¼�֮���ӳ�䡣���EVMAP_USE_HTû�ж��壬��������win32�£�
// struct event_io_map��struct event_signal_mapһ�£�event_io_map�ǽ�fds���¼�ӳ�䵽һ��
struct event_signal_map {
	/* An array of evmap_io * or of evmap_signal *; empty entries are
	 * set to NULL. */
	void **entries;
	/* The number of entries available in entries */
	int nentries;
};

/* A list of events waiting on a given 'common' timeout value.  Ordinarily,
 * events waiting for a timeout wait on a minheap.  Sometimes, however, a
 * queue can be faster.
 **/
struct common_timeout_list {
	/* List of events currently waiting in the queue. */
	struct event_list events;
	/* 'magic' timeval used to indicate the duration of events in this
	 * queue. */
	struct timeval duration;
	/* Event that triggers whenever one of the events in the queue is
	 * ready to activate */
	struct event timeout_event;
	/* The event_base that this timeout list is part of */
	struct event_base *base;
};

/** Mask used to get the real tv_usec value from a common timeout. */
#define COMMON_TIMEOUT_MICROSECONDS_MASK       0x000fffff

struct event_change;

/* List of 'changes' since the last call to eventop.dispatch.  Only maintained
 * if the backend is using changesets. */
// �о��Դ���һ��eventop.dispatch����֮��ĸı��б�ֻ���ں�̨ʹ�øı伯��ʱ�Ż�ά������б�����ά����
struct event_changelist {
	struct event_change *changes;
	int n_changes;
	int changes_size;
};

#ifndef EVENT__DISABLE_DEBUG_MODE
/* Global internal flag: set to one if debug mode is on. */
extern int event_debug_mode_on_;
#define EVENT_DEBUG_MODE_IS_ON() (event_debug_mode_on_)
#else
#define EVENT_DEBUG_MODE_IS_ON() (0)
#endif

TAILQ_HEAD(evcallback_list, event_callback);

/* Sets up an event for processing once */
struct event_once {
	LIST_ENTRY(event_once) next_once;
	struct event ev;

	void (*cb)(evutil_socket_t, short, void *);
	void *arg;
};

struct event_base {
	/** Function pointers and other data to describe this event_base's
	 * backend. */
	// ʵ��ʹ�ú�̨�����ľ����ʵ����ָ����Ǿ�̬ȫ���������
     // ���Ӿ�̬ȫ�ֱ���eventops��ѡ��
	const struct eventop *evsel;
	/** Pointer to backend-specific data. */
	// ָ���̨�ض������ݣ�����evsel->init���صľ��
     // ʵ�����Ƕ�ʵ�ʺ�̨�����������ݵķ�װ��void���ڼ����Կ���

	void *evbase;

	/** List of changes to tell backend about at next dispatch.  Only used
	 * by the O(1) backends. */
     // ���ߺ�̨������һ�ε��ȵı仯�б�
	struct event_changelist changelist;

	/** Function pointers used to describe the backend that this event_base
	 * uses for signals */
     // ����������ǰevent_base�����źŵĺ�̨����
	const struct eventop *evsigsel;
	/** Data to implement the common signal handelr code. */
    // ����ʵ�ֹ����źž���Ĵ���
	struct evsig_info sig;

	/** Number of virtual events */
	// �����¼�������
	int virtual_event_count;
	/** Maximum number of virtual events active */
	// �����¼����������
	int virtual_event_count_max;
	/** Number of total events added to this event_base */
	 // ��ӵ�event_base���¼�����
	int event_count;
	/** Maximum number of total events added to this event_base */
	// ��ӵ�event_base�ϵ�������
	int event_count_max;
	/** Number of total events active in this event_base */
	 // ��ǰevent_base�л�Ծ�¼��ĸ���
	int event_count_active;
	/** Maximum number of total events active in this event_base */
	// ��ǰevent_base�л�Ծ�¼���������
	int event_count_active_max;

	/** Set if we should terminate the loop once we're done processing
	 * events. */
	// һ��������ɴ����¼��ˣ��������Ӧ����ֹloop�������������
	int event_gotterm;
	/** Set if we should terminate the loop immediately */
	 // �����Ҫ��ֹloop�����������������
	int event_break;
	/** Set if we should start a new instance of the loop immediately. */
    // ���������ʵ����loop�������������
	int event_continue;

	/** The currently running priority of events */
	// ��ǰ�����¼������ȼ�
	int event_running_priority;

	/** Set if we're running the event_base_loop function, to prevent
	 * reentrant invocation. */
	 // ��ֹevent_base_loop�����
	int running_loop;

	/** Set to the number of deferred_cbs we've made 'active' in the
	 * loop.  This is a hack to prevent starvation; it would be smarter
	 * to just use event_config_set_max_dispatch_interval's max_callbacks
	 * feature */
	 // �����Ѿ���loop������Ϊ��active����deferred_cbs�ĸ���������Ϊ�˱���
     // ������hack������ֻ��Ҫʹ��event_config_set_max_dispatch_interval��s��
     // max_callbacks�����Ϳ��Ա�ĸ�����
	int n_deferreds_queued;

	/* Active event management. */
	/** An array of nactivequeues queues for active event_callbacks (ones
	 * that have triggered, and whose callbacks need to be called).  Low
	 * priority numbers are more important, and stall higher ones.
	 */
	// �洢�����¼���event_callbacks�Ķ��У���Щevent_callbacks����Ҫ���ã�
     // ����ԽС���ȼ�Խ��
	struct evcallback_list *activequeues;
	/** The length of the activequeues array */
	// ��Ծ���еĳ���
	int nactivequeues;
	/** A list of event_callbacks that should become active the next time
	 * we process events, but not this time. */
	// ��һ�λ��ɼ���״̬�Ļص��������б����ǵ�ǰ��β������
	struct evcallback_list active_later_queue;

	/* common timeout logic */
	// ���ó�ʱ�߼�

	/** An array of common_timeout_list* for all of the common timeout
	 * values we know. */
	// ���ó�ʱ�¼��б����Ƕ���ָ�룬ÿ��Ԫ�ض��Ǿ���ͬ����ʱ
     // ʱ���¼����б�	
	struct common_timeout_list **common_timeout_queues;
	/** The number of entries used in common_timeout_queues */
	// ���ó�ʱ�����е���Ŀ����
	int n_common_timeouts;
	/** The total size of common_timeout_queues. */
	// ���ó�ʱ���е��ܸ���
	int n_common_timeouts_allocated;

	/** Mapping from file descriptors to enabled (added) events */
	// �ļ����������¼�֮���ӳ���
	struct event_io_map io;

	/** Mapping from signal numbers to enabled (added) events. */
	 // �ź����ֺ��¼�֮��ӳ���
	struct event_signal_map sigmap;

	/** Priority queue of events with timeouts. */
	// �¼���ʱ�����ȼ����У�ʹ����С��ʵ��
	struct min_heap timeheap;

	/** Stored timeval: used to avoid calling gettimeofday/clock_gettime
	 * too often. */
	// �洢ʱ�䣺��������Ƶ������gettimeofday/clock_gettime
	struct timeval tv_cache;

	// monotonic��ʽ��ʱ��
	struct evutil_monotonic_timer monotonic_timer;

	/** Difference between internal time (maybe from clock_gettime) and
	 * gettimeofday. */
	 // �ڲ�ʱ�䣨���Դ�clock_gettime��ȡ����gettimeofday֮��Ĳ���
	struct timeval tv_clock_diff;
	/** Second in which we last updated tv_clock_diff, in monotonic time. */
	// �����ڲ�ʱ��ļ������
	time_t last_updated_clock_diff;

#ifndef EVENT__DISABLE_THREAD_SUPPORT
	/* threading support */
	/** The thread currently running the event_loop for this base */
	unsigned long th_owner_id;
	/** A lock to prevent conflicting accesses to this event_base */
	void *th_base_lock;
	/** A condition that gets signalled when we're done processing an
	 * event with waiters on it. */
	void *current_event_cond;
	/** Number of threads blocking on current_event_cond. */
	int current_event_waiters;
#endif
	/** The event whose callback is executing right now */
	// ��ǰִ�еĻص�����
	struct event_callback *current_event;

#ifdef _WIN32
	/** IOCP support structure, if IOCP is enabled. */
	struct event_iocp_port *iocp;
#endif

	/** Flags that this base was configured with */
/** Flags that this base was configured with */
     // event_base���õ�����ֵ
     // ���̵߳����ǲ���ȫ�ģ����̷߳�����ģʽ
    // EVENT_BASE_FLAG_NOLOCK = 0x01,
     // ���Լ��EVENT_*�Ȼ�������
    // EVENT_BASE_FLAG_IGNORE_ENV = 0x02,
     // ֻ����windows
    // EVENT_BASE_FLAG_STARTUP_IOCP = 0x04,
     // ��ʹ�û����ʱ�䣬ÿ�λص������ȡϵͳʱ��
    // EVENT_BASE_FLAG_NO_CACHE_TIME = 0x08,
     // ���ʹ��epoll��������ʹ��epoll�ڲ���changelist
    // EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST = 0x10,
     // ʹ�ø���ȷ��ʱ�䣬���ǿ������ܻή��
    // EVENT_BASE_FLAG_PRECISE_TIMER = 0x20

	enum event_base_config_flag flags;
	// ������ʱ����
	struct timeval max_dispatch_time;
	 // �����ȵĻص���������
	int max_dispatch_callbacks;
	// ���ȼ�����֮�󣬶��ڻ�Ծ�������Ӷ��и���������
     // ���ǵ��Ӷ��и��������������֮�󣬻���ʵ�ʵĻص���������Ϊ׼
	int limit_callbacks_after_prio;

	/* Notify main thread to wake up break, etc. */
	/** True if the base already has a pending notify, and we don't need
	 * to add any more. */
	 // ���event_base�Ѿ��й���δ���¼���֪ͨ����ô���ǾͲ���Ҫ�ٴ������
	int is_notify_pending;
	/** A socketpair used by some th_notify functions to wake up the main
	 * thread. */
	 // ĳЩth_notify�������ڻ������̵߳�socket pair��0��1д
	evutil_socket_t th_notify_fd[2];
	/** An event used by some th_notify functions to wake up the main
	 * thread. */
	// ����th_notify�����������̵߳��¼�
	struct event th_notify;
	/** A function used to wake up the main thread from another thread. */
	int (*th_notify_fn)(struct event_base *base);

	/** Saved seed for weak random number generator. Some backends use
	 * this to produce fairness among sockets. Protected by th_base_lock. */
// ����������������������ӡ�ĳЩ��̨������ʹ�������������ƽ��ѡ��
     // sockets��
	struct evutil_weakrand_state weakrand_seed;

	/** List of event_onces that have not yet fired. */
	LIST_HEAD(once_event_list, event_once) once_events;

};

struct event_config_entry {
	// ��һ�����εĺ�̨������
	TAILQ_ENTRY(event_config_entry) next;
	// ���εĺ�̨������
	const char *avoid_method;
};

/** Internal structure: describes the configuration we want for an event_base
 * that we're about to allocate. */
// �ڲ��ṹ�壬����������event_base�������õ�������Ϣ
struct event_config {
     // ���εĺ�̨�����б�
	TAILQ_HEAD(event_configq, event_config_entry) entries;

	  // cpu��������������Ƕ�event_base�Ľ��飬����ǿ�Ƶ�
	int n_cpus_hint;
	// �����ִ�����¼�飬Ĭ������£�event_base�ᰴ�յ�ǰ���ȼ����е�˳��һֱ�������ȼ�
     // ���е��¼�ִ�����֮��Ż������¼��������ĺô����������󣬵����ڵ����ȼ����бȽϳ�ʱ��
     // �ᵼ��ĳЩ�����ȼ�һֱ�ڵȴ�ִ�У��޷���ռcpu
    // event_base��event_loop�����μ�����¼�֮��ִ�лص�������ʱ����
    // ��Ҫÿ��ִ����ص�����֮�󶼽��м��

	struct timeval max_dispatch_interval;
	 // event_base��event_loop�����μ�����¼�֮��ִ�е��Ȼص�������������
     // ��Ҫÿ��ִ����ص�����֮�󶼽��м��

	int max_dispatch_callbacks;
	// �������������������Ŀ��أ������0����ÿ��ִ����ϻص�����֮��ǿ�ƽ��м�飻
     // �����n����ֻ����ִ�����>=n�����ȼ��¼�֮��Ż�ǿ��ִ���������

	int limit_callbacks_after_prio;
// event_base��̨������Ҫ������
     // enum event_method_feature {
     // ���ش���
     // EV_FEATURE_ET = 0x01,
     // Ҫ���º�̨�����ڵ��Ⱥܶ��¼�ʱ��ԼΪO(1)������select��poll�޷��ṩ����������
     // �����ַ�������N���¼�ʱ�������ṩO(N)����
    // EV_FEATURE_O1 = 0x02,
 
     // ��̨�������Դ�������ļ�������������������sockets
    // EV_FEATURE_FDS = 0x04,
    /** Require an event method that allows you to use EV_CLOSED to detect
     * connection close without the necessity of reading all the pending data.
     *
     * Methods that do support EV_CLOSED may not be able to provide support on
     * all kernel versions.
     **/
     // Ҫ���̨��������ʹ��EV_CLOSED������������Ƿ��жϣ�������Ҫ��ȡ
     // ����δ�����ݣ����ǲ��������ں˶����ṩ��������
    // EV_FEATURE_EARLY_CLOSE = 0x08
    // };
	enum event_method_feature require_features;
// event_base���õ�����ֵ
     // ���̵߳����ǲ���ȫ�ģ����̷߳�����ģʽ
    // EVENT_BASE_FLAG_NOLOCK = 0x01,
     // ���Լ��EVENT_*�Ȼ�������
    // EVENT_BASE_FLAG_IGNORE_ENV = 0x02,
     // ֻ����windows
    // EVENT_BASE_FLAG_STARTUP_IOCP = 0x04,
     // ��ʹ�û����ʱ�䣬ÿ�λص������ȡϵͳʱ��
    // EVENT_BASE_FLAG_NO_CACHE_TIME = 0x08,
     // ���ʹ��epoll��������ʹ��epoll�ڲ���changelist
    // EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST = 0x10,
     // ʹ�ø���ȷ��ʱ�䣬���ǿ������ܻή��
    // EVENT_BASE_FLAG_PRECISE_TIMER = 0x20

	enum event_base_config_flag flags;
};

/* Internal use only: Functions that might be missing from <sys/queue.h> */
#ifndef TAILQ_FIRST
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#endif
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif
#ifndef TAILQ_NEXT
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST(head);					\
	     (var) != TAILQ_END(head);					\
	     (var) = TAILQ_NEXT(var, field))
#endif

#ifndef TAILQ_INSERT_BEFORE
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif

#define N_ACTIVE_CALLBACKS(base)					\
	((base)->event_count_active)

int evsig_set_handler_(struct event_base *base, int evsignal,
			  void (*fn)(int));
int evsig_restore_handler_(struct event_base *base, int evsignal);

int event_add_nolock_(struct event *ev,
    const struct timeval *tv, int tv_is_absolute);
/** Argument for event_del_nolock_. Tells event_del not to block on the event
 * if it's running in another thread. */
#define EVENT_DEL_NOBLOCK 0
/** Argument for event_del_nolock_. Tells event_del to block on the event
 * if it's running in another thread, regardless of its value for EV_FINALIZE
 */
#define EVENT_DEL_BLOCK 1
/** Argument for event_del_nolock_. Tells event_del to block on the event
 * if it is running in another thread and it doesn't have EV_FINALIZE set.
 */
#define EVENT_DEL_AUTOBLOCK 2
/** Argument for event_del_nolock_. Tells event_del to procede even if the
 * event is set up for finalization rather for regular use.*/
#define EVENT_DEL_EVEN_IF_FINALIZING 3
int event_del_nolock_(struct event *ev, int blocking);
int event_remove_timer_nolock_(struct event *ev);

void event_active_nolock_(struct event *ev, int res, short count);
int event_callback_activate_(struct event_base *, struct event_callback *);
int event_callback_activate_nolock_(struct event_base *, struct event_callback *);
int event_callback_cancel_(struct event_base *base,
    struct event_callback *evcb);

void event_callback_finalize_nolock_(struct event_base *base, unsigned flags, struct event_callback *evcb, void (*cb)(struct event_callback *, void *));
void event_callback_finalize_(struct event_base *base, unsigned flags, struct event_callback *evcb, void (*cb)(struct event_callback *, void *));
int event_callback_finalize_many_(struct event_base *base, int n_cbs, struct event_callback **evcb, void (*cb)(struct event_callback *, void *));


void event_active_later_(struct event *ev, int res);
void event_active_later_nolock_(struct event *ev, int res);
int event_callback_activate_later_nolock_(struct event_base *base,
    struct event_callback *evcb);
int event_callback_cancel_nolock_(struct event_base *base,
    struct event_callback *evcb, int even_if_finalizing);
void event_callback_init_(struct event_base *base,
    struct event_callback *cb);

/* FIXME document. */
void event_base_add_virtual_(struct event_base *base);
void event_base_del_virtual_(struct event_base *base);

/** For debugging: unless assertions are disabled, verify the referential
    integrity of the internal data structures of 'base'.  This operation can
    be expensive.

    Returns on success; aborts on failure.
*/
void event_base_assert_ok_(struct event_base *base);
void event_base_assert_ok_nolock_(struct event_base *base);


/* Helper function: Call 'fn' exactly once every inserted or active event in
 * the event_base 'base'.
 *
 * If fn returns 0, continue on to the next event. Otherwise, return the same
 * value that fn returned.
 *
 * Requires that 'base' be locked.
 */
int event_base_foreach_event_nolock_(struct event_base *base,
    event_base_foreach_event_cb cb, void *arg);

/* Cleanup function to reset debug mode during shutdown.
 *
 * Calling this function doesn't mean it'll be possible to re-enable
 * debug mode if any events were added.
 */
void event_disable_debug_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_INTERNAL_H_INCLUDED_ */
