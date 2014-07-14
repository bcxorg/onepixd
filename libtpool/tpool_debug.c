#include "tpool.h"

static int debug = FALSE;

/*
**  TPOOL_DEBUG_SET -- Turn off/on debugging
**
**	Parameters:
**		set	  -- boolean to turn on or off
**	Returns:
**		void
**	Notes:
*/
void
tpool_debug_set(int state)
{
	debug = state;
	return;
}

/*
**  TPOOL_IS_DEBUG -- Is debugging on or off
**
**	Parameters:
**		void
**	Returns:
**		TRUE if on
**		FALSE if off
**	Notes:
*/
int
tpool_is_debug(void)
{
	if (debug == TRUE)
		return TRUE;
	return FALSE;
}

