#include "tpool.h"

/*
**  TPOOL_LAUNCH -- Launch a thread pool.
**	The start size and max size are configurable.
**
**	Parameters:
**		numworkers  -- how many workers to create.
**		ebuf          -- error buffer
**		elen	      -- error buffer length
**	Returns:
**		a thread pool context on success
**		NULL on failure
**	Side Effects:
**		Allocates memory
**		Launches threads
**	Notes:
**		Each worker automatally blocks itself on
**		startup and awaits a signal from tpool_run().
*/
TPOOL_CTX *
tpool_init(int numworkers, char *ebuf, size_t elen)
{
	int i;
	int ret;
	int count;
	TPOOL_CTX *ctx = NULL;
	int xerror;

	if (tpool_is_debug() == TRUE)
	{
		printf("DEBUG: %s(%d): tpool_init() called: numworkers=%d\n", __FILE__, __LINE__, numworkers);
	}

	if (ebuf == NULL || elen == 0)
	{
		ebuf = alloca(BUFSIZ);
		elen = BUFSIZ;
	}

	if (numworkers <= 0)
	{
		xerror = EINVAL;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: numworkers illegal: %d",
			__FILE__, __LINE__, (int)pthread_self(),
			numworkers);
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		errno = xerror;
		return NULL;
	}

	ctx = calloc(1, sizeof(TPOOL_CTX));
	if (ctx == NULL)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: allocate context: %s",
			__FILE__, __LINE__, (int)pthread_self(),
			strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		errno = xerror;
		return NULL;
	}

	if (pthread_mutex_init(&(ctx->curworkers_mutex), NULL) != 0)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: initialize mutex: %s",
			__FILE__, __LINE__, (int)pthread_self(),
			strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		if (ctx != NULL)
		{
			(void) free(ctx);
			ctx = NULL;
		}
		errno = xerror;
		return NULL;
	}

	if (pthread_mutex_init(&(ctx->cond_mutex), NULL) != 0)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: initialize mutex: %s",
			__FILE__, __LINE__, (int)pthread_self(), strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		if (ctx != NULL)
		{
			(void) free(ctx);
			ctx = NULL;
		}
		errno = xerror;
		return NULL;
	}

	if (pthread_mutex_init(&(ctx->tasks_mutex), NULL) != 0)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: initialize mutex: %s",
			__FILE__, __LINE__, (int)pthread_self(), strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		if (ctx != NULL)
		{
			(void) free(ctx);
			ctx = NULL;
		}
		errno = xerror;
		return NULL;
	}

	ctx->workers      = NULL;
	ctx->curworkers   = 0;

	ctx->workers = calloc(numworkers, sizeof(TPOOL_WORKER_CTX));
	if (ctx->workers == NULL)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: initialize workers array: %s",
			__FILE__, __LINE__, (int)pthread_self(), strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		if (ctx != NULL)
		{
			(void) free(ctx);
			ctx = NULL;
		}
		errno = xerror;
		return NULL;
	}
	ctx->tasks = (TPOOL_TASK_T **)calloc(numworkers, sizeof(TPOOL_TASK_T *));
	if (ctx->tasks == NULL)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: initialize tasks array: %s",
			__FILE__, __LINE__, (int)pthread_self(), strerror(xerror));
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): %s\n", __FILE__, __LINE__, ebuf);
		}
		if (ctx != NULL)
		{
			if (ctx->workers != NULL)
				(void) free(ctx->workers);
			(void) free(ctx);
			ctx = NULL;
		}
		errno = xerror;
		return NULL;
	}
	ctx->tasks_head   = 0;
	ctx->tasks_tail   = 0;

	count = 0;
	for (i = 0; i < numworkers; i++)
	{
		ctx->workers[i].state = TPOOL_WORKER_STATE_RUN;
		ctx->tasks[i]         = NULL;
		ret = pthread_create(&(ctx->workers[i].tid), NULL, tpool_worker, (void *)ctx);
		/*
		 * Not sure what to do on error here.
		 */
		if (ret != 0)
		{
			if (tpool_is_debug() == TRUE)
			{
				xerror = errno;
				printf("DEBUG ERROR: %s(%d): pthread_create: %s: %s\n", __FILE__, __LINE__, ebuf, strerror(xerror));
				errno = xerror;
			}
			break;
		}
		++count;
	}
	ctx->numworkers = count;
	if (tpool_is_debug() == TRUE)
	{
		printf("DEBUG: %s(%d): tpool_init() exit: numworkers=%d\n", __FILE__, __LINE__, count);
	}
	return ctx;
}

