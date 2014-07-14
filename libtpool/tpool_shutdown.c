#include "tpool.h"

/*
**  TPOOL_SHUTDOWN -- Run a worker
**
**	Parameters:
**		ctx        -- worker pool context
**		ebuf	   -- error buffer
**		elen	   -- error buffer size
**	Returns:
**		NULL always
**		non-zero errno on failure
**	Side Effects:
**		Frees memory
**		Shuts down the threads
**	Notes:
*/
TPOOL_CTX *
tpool_shutdown(TPOOL_CTX *ctx, char *ebuf, size_t elen)
{
	int xerror;
	int i;

	if (tpool_is_debug() == TRUE)
	{
		printf("DEBUG: %s(%d): tpool_shutdown() called\n", __FILE__, __LINE__);
	}

	if (ebuf == NULL || elen == 0)
	{
		ebuf = alloca(BUFSIZ);
		elen = BUFSIZ;
	}

	if (ctx == NULL)
	{
		xerror = EINVAL;
		(void) snprintf(ebuf, elen, "%s(%d)[%d]: received NULL context",
			__FILE__, __LINE__, (int)pthread_self());
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG: %s(%d): tpool_shutdown(): %s\n", __FILE__, __LINE__, ebuf);
		}
		errno = xerror;
		return NULL;
	}

	ctx->state = TPOOL_WORKER_STATE_EXIT;
	(void) pthread_mutex_lock(&(ctx->curworkers_mutex));
	 ctx->curworkers = 0;
	(void) pthread_mutex_unlock(&(ctx->curworkers_mutex));
	// (void) pthread_mutex_destroy(&(ctx->curworkers_mutex));

	/*
	 * Tell all the workers to exit. Signal each incase
	 * any are blocked in tpool_abide().
	 */
	for (i = 0; i < ctx->numworkers; i++)
		(void) pthread_cond_signal(&(ctx->cond));

	for (i = 0; i < ctx->numworkers; i++)
		ctx->workers[i].state = TPOOL_WORKER_STATE_EXIT;

	/*
	 * Join each incase any need to be allowed to complete.
	 */
	for (i = 0; i < ctx->numworkers; i++)
		pthread_join(ctx->workers[i].tid, NULL);

	(void) free(ctx->workers);

	(void) pthread_mutex_lock(&(ctx->tasks_mutex));
	 if (ctx->tasks != NULL)
	 {
		 for (i = 0; i < ctx->numworkers; i++)
		 {
			if ((ctx->tasks)[i] != NULL)
			{
				(void) free((ctx->tasks)[i]);
				(ctx->tasks)[i] = NULL;
			}
		 }
		 (void) free(ctx->tasks);
		 ctx->tasks = NULL;
	 }
	(void) pthread_mutex_unlock(&(ctx->tasks_mutex));

	ctx->numworkers = 0;

	if (ctx != NULL)
	{
		(void) free(ctx);
		ctx = NULL;
	}
	if (tpool_is_debug() == TRUE)
	{
		printf("DEBUG: %s(%d): tpool_shutdown(): done\n", __FILE__, __LINE__);
	}

	return NULL;
}

