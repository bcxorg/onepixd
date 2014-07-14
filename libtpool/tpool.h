#ifndef TPOOL_H
#define TPOOL_H 1
#include "../config.h"

#if HAVE_PTHREAD_H
# include "stdio.h"
#endif

#if HAVE_PTHREAD_H
# include "stdlib.h"
#endif

#if HAVE_PTHREAD_H
# include "string.h"
#endif

#if HAVE_PTHREAD_H
# include "errno.h"
#endif

#if HAVE_PTHREAD_H
# include "pthread.h"
#endif

#if HAVE_PTHREAD_H
# include "alloca.h"
#endif

#ifndef TRUE
# define TRUE (1)
#endif
#ifndef FALSE
# define FALSE (0)
#endif

typedef struct {
	void		*payload;		/* The payload                        */
	void		*(*call_funct)(void *, int *);
	void		 (*free_funct)(void *);	/* Function to free payload           */
} TPOOL_TASK_T;

typedef struct {
	pthread_t	  tid;			/* Thread identifier for this worker  */
	int		  state;
} TPOOL_WORKER_CTX;

#define TPOOL_WORKER_STATE_RUN	(0)	/* run normally when awakened         */
#define TPOOL_WORKER_STATE_EXIT	(1)	/* shutdown and return, when awakened */

typedef struct _gm_tpool_ctx {
	int			 curworkers;
	pthread_mutex_t		 curworkers_mutex;	
	int			 numworkers;
	TPOOL_WORKER_CTX	*workers;
	pthread_cond_t		 cond;
	pthread_mutex_t		 cond_mutex;	
	TPOOL_TASK_T **		 tasks;
	pthread_mutex_t		 tasks_mutex;	
	int			 tasks_head;
	int			 tasks_tail;
	int			 state;
} TPOOL_CTX;


/*
** Public prototypes
*/
int         tpool_delta(TPOOL_CTX *ctx, int delta);
TPOOL_CTX * tpool_init(int numworkers, char *ebuf, size_t elen);
TPOOL_CTX * tpool_shutdown(TPOOL_CTX *, char *ebuf, size_t elen);
int         tpool_push_task(TPOOL_CTX *ctx, void *call_funct(void *, int *), void free_funct(void *), void *payload, char *ebuf, size_t elen);
void        tpool_debug_set(int state);

/*
** Private prototypes
*/
TPOOL_TASK_T * tpool_pop_task(TPOOL_CTX *ctx, char *ebuf, size_t elen);
void *         tpool_worker(void *);
int            tpool_is_debug(void);

#endif /* TPOOL_H */
