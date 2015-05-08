/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * mono-mutex.h: Portability wrappers around POSIX Mutexes
 *
 * Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 * Copyright 2002 Ximian, Inc. (www.ximian.com)
 */

#ifndef __MONO_MUTEX_H__
#define __MONO_MUTEX_H__

#include <config.h>

#include <glib.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <time.h>

#ifdef HOST_WIN32
#include <winsock2.h>
#include <windows.h>
#endif

G_BEGIN_DECLS

#ifndef HOST_WIN32

typedef struct {
	pthread_mutex_t mutex;
	gboolean complete;
} mono_once_t;

#define MONO_ONCE_INIT { PTHREAD_MUTEX_INITIALIZER, FALSE }

int mono_once (mono_once_t *once, void (*once_init) (void));

typedef pthread_mutex_t mono_mutex_t;
typedef pthread_cond_t mono_cond_t;

#define mono_mutex_init(mutex) pthread_mutex_init (mutex, NULL)
#define mono_mutex_lock(mutex) pthread_mutex_lock (mutex)
#define mono_mutex_trylock(mutex) pthread_mutex_trylock (mutex)
#define mono_mutex_timedlock(mutex,timeout) pthread_mutex_timedlock (mutex, timeout)
#define mono_mutex_unlock(mutex) pthread_mutex_unlock (mutex)
#define mono_mutex_destroy(mutex) pthread_mutex_destroy (mutex)

#define mono_cond_init(cond,attr) pthread_cond_init (cond,attr)
#define mono_cond_wait(cond,mutex) pthread_cond_wait (cond, mutex)
#define mono_cond_timedwait(cond,mutex,timeout) pthread_cond_timedwait (cond, mutex, timeout)
#define mono_cond_signal(cond) pthread_cond_signal (cond)
#define mono_cond_broadcast(cond) pthread_cond_broadcast (cond)
#define mono_cond_destroy(cond)

/*
 * This should be used instead of mono_cond_timedwait, since that function is not implemented on windows.
 */
int mono_cond_timedwait_ms (mono_cond_t *cond, mono_mutex_t *mutex, int timeout_ms);

/* This is a function so it can be passed to pthread_cleanup_push -
 * that is a macro and giving it a macro as a parameter breaks.
 */
G_GNUC_UNUSED
static inline int mono_mutex_unlock_in_cleanup (mono_mutex_t *mutex)
{
	return(mono_mutex_unlock (mutex));
}

/* Returns zero on success. */
static inline int
mono_mutex_init_recursive (mono_mutex_t *mutex)
{
	int res;
	pthread_mutexattr_t attr;

	pthread_mutexattr_init (&attr);
	pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
	res = pthread_mutex_init (mutex, &attr);
	pthread_mutexattr_destroy (&attr);

	return res;
}

#else

typedef CRITICAL_SECTION mono_mutex_t;
typedef CONDITION_VARIABLE mono_cond_t;

#define mono_mutex_init(mutex) (InitializeCriticalSection((mutex)), 0)
#define mono_mutex_init_recursive(mutex) (InitializeCriticalSection((mutex)), 0)
#define mono_mutex_lock(mutex) EnterCriticalSection((mutex))
#define mono_mutex_trylock(mutex) (!TryEnterCriticalSection((mutex)))
#define mono_mutex_unlock(mutex)  LeaveCriticalSection((mutex))
#define mono_mutex_destroy(mutex) DeleteCriticalSection((mutex))

static inline int
mono_cond_init (mono_cond_t *cond, int attr)
{
	InitializeConditionVariable (cond);
}

static inline int
mono_cond_wait (mono_cond_t *cond, mono_mutex_t *mutex)
{
	int res;

	res = SleepConditionVariableCS (cond, mutex, INFINITE);
	if (res)
		/* Success */
		return 0;
	else
		return 1;
}

static inline int
mono_cond_timedwait (mono_cond_t *cond, mono_mutex_t *mutex, struct timespec *timeout)
{
	// FIXME:
	g_assert_not_reached ();
	return 0;
}

static inline int
mono_cond_signal (mono_cond_t *cond)
{
	WakeConditionVariable (cond);
}

static inline int
mono_cond_broadcast (mono_cond_t *cond)
{
	WakeAllConditionVariable (cond);
}

static inline int
mono_cond_destroy (mono_cond_t *cond)
{
}

static inline int
mono_cond_timedwait_ms (mono_cond_t *cond, mono_mutex_t *mutex, int timeout_ms)
{
	int res;

	res = SleepConditionVariableCS (cond, mutex, timeout_ms);
	if (res)
		/* Success */
		return 0;
	else
		return 1;
}

#endif

int mono_mutex_init_suspend_safe (mono_mutex_t *mutex);

G_END_DECLS

#endif /* __MONO_MUTEX_H__ */
