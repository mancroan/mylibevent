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
#ifndef EVENT2_EVENT_STRUCT_H_INCLUDED_
#define EVENT2_EVENT_STRUCT_H_INCLUDED_

/** @file event2/event_struct.h

  Structures used by event.h.  Using these structures directly WILL harm
  forward compatibility: be careful.

  No field declared in this file should be used directly in user code.  Except
  for historical reasons, these fields would not be exposed at all.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#ifdef EVENT__HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* For int types. */
#include <event2/util.h>

/* For evkeyvalq */
#include <event2/keyvalq_struct.h>

//�����¼�״̬��־ 
//
// �¼���time min_heap����
#define EVLIST_TIMEOUT	    0x01
// �¼�����ע���¼�������
#define EVLIST_INSERTED	    0x02
// Ŀǰδʹ��
#define EVLIST_SIGNAL	    0x04
// �¼��ڼ���������
#define EVLIST_ACTIVE	    0x08
// �ڲ�ʹ�ñ��
#define EVLIST_INTERNAL	    0x10
// �¼�����һ�μ���������
#define EVLIST_ACTIVE_LATER 0x20
// �¼��Ѿ���ֹ
#define EVLIST_FINALIZING   0x40
// �¼���ʼ����ɣ������Ķ�������
#define EVLIST_INIT	    0x80
// ���������¼�״̬�������жϺϷ��Ե�
#define EVLIST_ALL          0xff

/* Fix so that people don't have to run with <sys/queue.h> */
#ifndef TAILQ_ENTRY
#define EVENT_DEFINED_TQENTRY_
#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}
#endif /* !TAILQ_ENTRY */

#ifndef TAILQ_HEAD
#define EVENT_DEFINED_TQHEAD_
#define TAILQ_HEAD(name, type)			\
struct name {					\
	struct type *tqh_first;			\
	struct type **tqh_last;			\
}
#endif

/* Fix so that people don't have to run with <sys/queue.h> */
#ifndef LIST_ENTRY
#define EVENT_DEFINED_LISTENTRY_
#define LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}
#endif /* !LIST_ENTRY */

#ifndef LIST_HEAD
#define EVENT_DEFINED_LISTHEAD_
#define LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;  /* first element */			\
	}
#endif /* !LIST_HEAD */

struct event;

struct event_callback {

     //��һ���ص��¼�
	TAILQ_ENTRY(event_callback) evcb_active_next;
	//�ص��¼���״̬��ʶ������Ϊ��
     //           #define EVLIST_TIMEOUT 0x01 // event��time���У�min_heap
     //           #define EVLIST_INSERTED 0x02 // event����ע���¼������У�event_base��queue��
     //           #define EVLIST_SIGNAL 0x04 // δ��ʹ��
     //           #define EVLIST_ACTIVE 0x08 // event�ڼ��������У�event_base��active_queue��
     //           #define EVLIST_INTERNAL 0x10 // �ڲ�ʹ�ñ��
     //           #define EVLIST_ACTIVE_LATER 0x20 event����һ�μ���������
     //           #define EVLIST_INIT     0x80 // event�ѱ���ʼ��
      //           #define EVLIST_ALL          0xff // ��Ҫ�����ж��¼�״̬�ĺϷ���
	short evcb_flags;

     // �ص����������ȼ���ԽС���ȼ�Խ��
	ev_uint8_t evcb_pri;	/* smaller numbers are higher priority */
 // ִ�в�ͬ�Ļص�����
/** @name Event closure codes
    Possible values for evcb_closure in struct event_callback
    @{

/** A regular event. Uses the evcb_callback callback

//�����ǳ���Ķ�д�¼���ִ��evcb_callback�ص�
#define EV_CLOSURE_EVENT 0
/** A signal event. Uses the evcb_callback callback 
// �������źţ�ִ��evcb_callback�ص�
#define EV_CLOSURE_EVENT_SIGNAL 1
/** A persistent non-signal event. Uses the evcb_callback callback 
// �����������Եķ��ź��¼���ʹ��evcb_callback�ص�
#define EV_CLOSURE_EVENT_PERSIST 2
/** A simple callback. Uses the evcb_selfcb callback. 
// �����Ǽ򵥵Ļص���ʹ��evcb_selfcb�ص�
#define EV_CLOSURE_CB_SELF 3
/** A finalizing callback. Uses the evcb_cbfinalize callback. 
// �����ǽ����ͻص���ʹ��evcb_cbfinalize�ص�
#define EV_CLOSURE_CB_FINALIZE 4
/** A finalizing event. Uses the evcb_evfinalize callback. 
// �����ǽ����¼���ʹ��evcb_evfinalize�ص�
#define EV_CLOSURE_EVENT_FINALIZE 5
/** A finalizing event that should get freed after. Uses the evcb_evfinalize
 * callback. 
// ���ڽ������¼�����Ӧ���ͷţ�ʹ��evcb_evfinalize�ص�
#define EV_CLOSURE_EVENT_FINALIZE_FREE 6
/** @} */
	ev_uint8_t evcb_closure;
	/* allows us to adopt for different types of events */
     // ���������Զ����䲻ͬ���͵Ļص��¼�
        union {
		void (*evcb_callback)(evutil_socket_t, short, void *);
		void (*evcb_selfcb)(struct event_callback *, void *);
		void (*evcb_evfinalize)(struct event *, void *);
		void (*evcb_cbfinalize)(struct event_callback *, void *);
	} evcb_cb_union;
     // �ص�����
	void *evcb_arg;
};

struct event_base;
struct event {
// event�Ļص���������event_base����
     // ���嶨��������
     // ����ΪһЩ���õĺ궨��
     // #define ev_pri ev_evcallback.evcb_pri
     // #define ev_flags ev_evcallback.evcb_flags
     // #define ev_closure ev_evcallback.evcb_closure
     // #define ev_callback ev_evcallback.evcb_cb_union.evcb_callback
     // #define ev_arg ev_evcallback.evcb_arg
	struct event_callback ev_evcallback;

	/* for managing timeouts */
     // ��������ʱ�¼�
	union {
		// ���ó�ʱ����
		TAILQ_ENTRY(event) ev_next_with_common_timeout;
		 // min_heap��С������
		int min_heap_idx;
	} ev_timeout_pos;
	// �����I/O�¼���ev_fdΪ�ļ���������������źţ�ev_fdΪ�ź�
	evutil_socket_t ev_fd;
	// libevent�����ÿ���¼����ᱣ��һ�ݾ��
	struct event_base *ev_base;
	// �ù�������ͬʱ����IO�¼����ź�
     // ����ΪһЩ������õĺ궨��
     /* mutually exclusive */
     // #define ev_signal_next    ev_.ev_signal.ev_signal_next
     // #define ev_io_next    ev_.ev_io.ev_io_next
     // #define ev_io_timeout    ev_.ev_io.ev_timeout
 
     /* used only by signals */
     // #define ev_ncalls    ev_.ev_signal.ev_ncalls
     // #define ev_pncalls    ev_.ev_signal.ev_pncalls
	union {
		/* used for io events */
		struct {
			 // ��һ��io�¼�
			LIST_ENTRY (event) ev_io_next;
			// �¼���ʱʱ�䣨�ȿ��������ʱ�䣬Ҳ�����Ǿ���ʱ�䣩
			struct timeval ev_timeout;
		} ev_io;

		/* used by signal events */
		struct {
			// ��һ���ź�
			LIST_ENTRY (event) ev_signal_next;
			 // �ź�׼������ʱ������ev_callback�Ĵ���
			short ev_ncalls;
			/* Allows deletes in callback */
			// ͨ��ָ�� ev_ncalls����NULL
			short *ev_pncalls;
		} ev_signal;
	} ev_;

	// �¼����ͣ��������������������ͣ�����io�¼����ź��޷�ͬʱ����
     // io�¼��� EV_READ,EV_WRITE
     // ��ʱ�¼���EV_TIMEOUT
     // �źţ�EV_SIGNAL
     // �����Ǹ���ѡ����Ժ��κ��¼�ͬʱ����
     // EV_PERSIST��������һ�������¼�����ʾִ����ϲ����Ƴ����粻�ӣ���ִ�����֮����Զ��Ƴ�
     // EV_ET: ���ش����������̨�������õĻ����Ϳ���ʹ�ã�ע������ˮƽ����
     // EV_FINALIZE��ɾ���¼�ʱ�Ͳ��������ˣ�����ȵ��ص�����ִ����ϣ�Ϊ���ڶ��߳��а�ȫʹ�ã���Ҫʹ��
     // event_finalize()����event_free_finalize()
     // EV_CLOSED�� �����Զ����رյ����ӣ�Ȼ�������ȡδ������ݣ����ǲ������к�̨������֧�����ѡ��
    short ev_events;
	// ��¼��ǰ�����¼�������
	short ev_res;		/* result passed to event callback */
	// �����¼��ĳ�ʱʱ��
	struct timeval ev_timeout;
};

TAILQ_HEAD (event_list, event);

#ifdef EVENT_DEFINED_TQENTRY_
#undef TAILQ_ENTRY
#endif

#ifdef EVENT_DEFINED_TQHEAD_
#undef TAILQ_HEAD
#endif

LIST_HEAD (event_dlist, event); 

#ifdef EVENT_DEFINED_LISTENTRY_
#undef LIST_ENTRY
#endif

#ifdef EVENT_DEFINED_LISTHEAD_
#undef LIST_HEAD
#endif

#ifdef __cplusplus
}
#endif

#endif /* EVENT2_EVENT_STRUCT_H_INCLUDED_ */
