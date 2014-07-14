
#include "tpool.h"

/*
**  TPOOL_PUSH_TASK -- Push a job onto the stack
**
**	Parameters:
**		ctx     	-- worker pool context
**		call_funct 	-- function to handle the task
**		free_funct 	-- function to free the data
**		payload		-- the data for the task
**		ebuf		-- error buffer
**		elen		-- error buffer size
**	Returns:
**		zero on success
**		non-zero on failure, and sets errno
**	Side Effects:
**		Allocates memory
**		Launches a thread
**	Notes:
**		The worker automatally blocks itself on
**		startup and awaits a signal.
*/
int
tpool_push_task(TPOOL_CTX *ctx, void *call_funct(void *, int *), void free_funct(void *), void *payload, char *ebuf, size_t elen)
{
	int		xerror;
	int		tmp;
	TPOOL_TASK_T *  task;

	if (ebuf == NULL || elen == 0)
	{
		ebuf = alloca(BUFSIZ);
		elen = BUFSIZ;
	}

	if (ctx == NULL)
	{
		xerror = EINVAL;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: received NULL context",
			__FILE__, __LINE__, (int)pthread_self());
		errno = xerror;
		return errno;
	}

	if (call_funct == NULL)
	{
		xerror = EINVAL;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: received NULL function pointer",
			__FILE__, __LINE__, (int)pthread_self());
		errno = xerror;
		return errno;
	}

	task = (TPOOL_TASK_T *)calloc(1, sizeof(TPOOL_TASK_T));
	if (task == NULL)
	{
		if (errno == 0)
			xerror = ENOMEM;
		else
			xerror = errno;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: allocate a task: %s",
			__FILE__, __LINE__, (int)pthread_self(), 
			strerror(xerror));
		errno = xerror;
		return errno;
	}

	task->call_funct    = call_funct;
	task->free_funct    = free_funct;
	task->payload       = payload;

	if (pthread_mutex_lock(&(ctx->tasks_mutex)) != 0)
	{
		if (errno == 0)
			xerror = EINVAL;
		else
			xerror = errno;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: tasks mutex lock failed: %s",
			__FILE__, __LINE__, (int)pthread_self(), 
			strerror(xerror));
		if (task != NULL)
		{
			(void) free(task);
			task = NULL;
		}
		errno = xerror;
		return errno;
	}
	 tmp = ctx->tasks_tail;
	 if (++tmp >= ctx->numworkers)
		tmp = 0;
	 if (ctx->tasks_head == ctx->tasks_tail 
	 	&& (ctx->tasks)[ctx->tasks_head] == NULL) /* empty task list */
	 {
		 ctx->tasks[ctx->tasks_head] =  task;
		 ctx->tasks_tail = tmp;
		(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
		goto signal_cond;
	 }

	 if (tmp == ctx->tasks_head)
	 {
		/* collision */
		if (errno == 0)
			xerror = EINVAL;
		else
			xerror = errno;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: tasks no room in stack: %s",
			__FILE__, __LINE__, (int)pthread_self(), 
			strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): tpool_pop_task(): %s\n", __FILE__, __LINE__, ebuf);
		}
		if (task != NULL)
		{
			(void) free(task);
			task = NULL;
		}
		(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
		errno = xerror;
		return errno;
	 }
	 ctx->tasks[ctx->tasks_tail] =  task;
	 ctx->tasks_tail = tmp;
	(void) pthread_mutex_unlock(&(ctx->tasks_mutex));

signal_cond:
	if (pthread_mutex_lock(&(ctx->tasks_mutex)) != 0)
	{
		if (errno == 0)
			xerror = EINVAL;
		else
			xerror = errno;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: tasks mutex lock failed: %s",
			__FILE__, __LINE__, (int)pthread_self(), 
			strerror(xerror));
		if (task != NULL)
		{
			(void) free(task);
			task = NULL;
		}
		errno = xerror;
		return errno;
	}
	if (pthread_cond_signal(&(ctx->cond)) != 0)
	{
		if (errno == 0)
			xerror = EINVAL;
		else
			xerror = errno;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: tasks cond signal failed: %s",
			__FILE__, __LINE__, (int)pthread_self(), 
			strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): tpool_pop_task(): %s\n", __FILE__, __LINE__, ebuf);
		}
		(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
		errno = xerror;
		return errno;
	}
	(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
	return (errno = 0);
}

/*
**  TPOOL_POP_TASK -- Pop a job from the stack
**
**	Parameters:
**		ctx     	-- worker pool context
**		ebuf		-- error buffer
**		elen		-- error buffer size
**	Returns:
**		task		-- if available
**		NULL		-- if nothing available
**	Side Effects:
**		Launches a thread
*/
TPOOL_TASK_T *
tpool_pop_task(TPOOL_CTX *ctx, char *ebuf, size_t elen)
{
	int xerror;
	int tmp			= 0;
	TPOOL_TASK_T *task	= NULL;

	if (ebuf == NULL || elen == 0)
	{
		ebuf = alloca(BUFSIZ);
		elen = BUFSIZ;
	}

	if (ctx == NULL)
	{
		xerror = EINVAL;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: received NULL context",
			__FILE__, __LINE__, (int)pthread_self());
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): tpool_pop_task(): %s\n", __FILE__, __LINE__, ebuf);
		}
		errno = xerror;
		return NULL;
	}

	if (pthread_mutex_lock(&(ctx->tasks_mutex)) != 0)
	{
		if (errno == 0)
			xerror = EINVAL;
		else
			xerror = errno;
		(void) snprintf(ebuf, elen,
			"%s(%d)[%d]: tasks mutex lock failed: %s",
			__FILE__, __LINE__, (int)pthread_self(), 
			strerror(xerror));
		if (task != NULL)
		{
			(void) free(task);
			task = NULL;
		}
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): tpool_pop_task(): %s\n", __FILE__, __LINE__, ebuf);
		}
		errno = xerror;
		return NULL;
	}
	 tmp = ctx->tasks_head;
	 if (++tmp >= ctx->numworkers)
	 	tmp = 0;
	 if (ctx->tasks_head == ctx->tasks_tail && (ctx->tasks)[ctx->tasks_head] != NULL)
	 {
		 /* only one task */
		 task = ctx->tasks[ctx->tasks_head];
		 ctx->tasks[ctx->tasks_head] = NULL;
		(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG: %s(%d): tpool_pop_task(): head=%d:%p tail=%d:%p task=%p\n", __FILE__, __LINE__, ctx->tasks_head, ctx->tasks[ctx->tasks_head], ctx->tasks_tail, ctx->tasks[ctx->tasks_tail], task);
		}
		errno = 0;
		return task;
	 }
	 if (ctx->tasks_head == ctx->tasks_tail)
	 {
		(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): tpool_pop_task(): head=%d:%p tail=%d:%p task=%p\n", __FILE__, __LINE__, ctx->tasks_head, ctx->tasks[ctx->tasks_head], ctx->tasks_tail, ctx->tasks[ctx->tasks_tail], task);
		}
		errno = 0;
	 	return NULL;
	 }
	 task = ctx->tasks[ctx->tasks_head];
	 ctx->tasks[ctx->tasks_head] = NULL;
	 ctx->tasks_head = tmp;
	(void) pthread_mutex_unlock(&(ctx->tasks_mutex));
	if (tpool_is_debug() == TRUE)
	{
		printf("DEBUG: %s(%d): tpool_pop_task(): head=%d:%p tail=%d:%p task=%p\n", __FILE__, __LINE__, ctx->tasks_head, ctx->tasks[ctx->tasks_head], ctx->tasks_tail, ctx->tasks[ctx->tasks_tail], task);
	}
	errno = 0;
	return task;
}

