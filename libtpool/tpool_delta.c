#include "tpool.h"

/*
** TPOOL_DELTA -- Change or show the number of workers free
**
**	Parameters:
**		ctx        -- worker pool context
**		delta	   -- how much to change
**	Returns:
**		return number of worker available after delta applied
**	Notes:
*/

int
tpool_delta(TPOOL_CTX *ctx, int delta)
{
	int ret = 0;

	if (ctx == NULL)
	{
		if (tpool_is_debug() == TRUE)
		{
			printf("DEBUG ERROR: %s(%d): ctx is NULL\n", __FILE__, __LINE__);
		}
		return 0;
	}
	if (ctx != NULL && ctx->state == TPOOL_WORKER_STATE_EXIT)
	{
		return 0;
	}
	if (delta == 0)
	{
		(void) pthread_mutex_lock(&(ctx->curworkers_mutex));
		 ret = ctx->curworkers;
		(void) pthread_mutex_unlock(&(ctx->curworkers_mutex));
		return ret;
	}

	(void) pthread_mutex_lock(&(ctx->curworkers_mutex));
	 ctx->curworkers += delta;
	 ret = ctx->curworkers;
	(void) pthread_mutex_unlock(&(ctx->curworkers_mutex));

	if (ctx->state == TPOOL_WORKER_STATE_EXIT)
		return 0;
	return ret;
}

