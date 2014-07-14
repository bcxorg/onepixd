#include "onepixd.h"

static int Debug = BUG_NONE;

void
setdebug(int fac)
{
	Debug |= fac;
}

void
cleardebug(int fac)
{
	Debug &= fac;
}

void
zerodebug()
{
	Debug = 0;
}

int
isdebug(int fac)
{
	if ((fac & Debug) != 0)
		return TRUE;
	return FALSE;
}
