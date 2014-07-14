#include "tpool.h"

/*
**  POOL_WORKER -- The common thread worker
**
**	Parameters:
**		ctx	  -- The thread context
**	Returns:
**		NULL always
**	Side Effects:
**		Blocks on a condition each time through the loop
**	Notes:
*/
void *
tpool_worker(void *arg)
{
	TPOOL_CTX *ctx;
	TPOOL_TASK_T	*task;
	int ret;
	int delta;

	ctx = (TPOOL_CTX *)arg;


	if (ctx == NULL)
		return NULL;

	for (;;)
	{
		if (ctx->state == TPOOL_WORKER_STATE_EXIT)
			break;

		delta = tpool_delta(ctx, 1);

		pthread_mutex_lock(&(ctx->cond_mutex));
		 ret = pthread_cond_wait(&(ctx->cond), &(ctx->cond_mutex));
		pthread_mutex_unlock(&(ctx->cond_mutex));
		delta = tpool_delta(ctx, -1);

		if (ctx->state == TPOOL_WORKER_STATE_EXIT)
			break;

		if (ret == ETIMEDOUT)
			continue;

		task = tpool_pop_task(ctx, NULL, 0);
		if (task == NULL)
		{
			continue;
		}
		if (task->call_funct == NULL)
		{
			continue;
		}

		/*
		 * Call the function that performs the work.
		 * Ignore any return value.
		 */
		(void) (task->call_funct)((void *)(task->payload), &(ctx->state));

		/*
		 * If they gave us a function to free the payload,
		 * free it now. Otherwise let the programmer beware
		 * that it must be freed later.
		 */
		if (task->payload != NULL && task->free_funct != NULL)
		{
			(void) (task->free_funct)((void *)(task->payload));
			task->payload = NULL;
		}
		(void) free(task);
		task = NULL;
		continue;
	}
	return NULL;
}

