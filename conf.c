#include "onepixd.h"


void
conf_datafree(void *data)
{
	CONF_ARGV_CTX *avp = (CONF_ARGV_CTX *)data;

	avp = conf_freeav(avp);
}

HASH_CTX *
conf_init(HASH_CTX *cctx, char *path, char *ebuf, size_t elen)
{
	int              xerror  = 0;
	register FILE   *fp      = NULL;
	char            *np1     = NULL;
	char            *np2     = NULL;
	char            *vp1     = NULL;
	char            *vp2     = NULL;
	HASH_CTX     *rctx    = NULL;
	char             buf[BUFSIZ];

	if (path == NULL)
	{
		(void) snprintf(ebuf, elen, "%s(%d): path was null",
			__FILE__, __LINE__);
		errno = EINVAL;
		return NULL;
	}

	rctx = cctx;
	if (rctx == NULL)
	{
		rctx = hash_init(0);
		if (rctx == NULL)
		{
			xerror = errno;
			(void) snprintf(ebuf, elen, "%s(%d): hash init: %s",
				__FILE__, __LINE__, strerror(xerror));
			errno = xerror;
			return NULL;
		}
		hash_set_callback(rctx, conf_datafree);
	}

	/*
	 * Any error opening the file uses a standard
	 * system error to report.
	 */
	fp = fopen(path, "r");
	if (fp == NULL)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s(%d): %s: %s",
			__FILE__, __LINE__, path, strerror(xerror));
		errno = xerror;
		return NULL;
	}

	(void) memset(buf, '\0', sizeof buf);
	/*
	 * Read the file a line at a time.
	 */
	np1 = vp1 = NULL;
	np2 = vp2 = NULL;
	(void) memset(buf, '\0', sizeof buf);
	while (fgets(buf, sizeof buf, fp) != NULL)
	{
		char *bp;

		/*
		 * Empty lines and those that begin with # are
		 * silently skipped.
		 */
		bp = buf;
		if (*bp == '0' || *bp == '#' || *bp == '\n' || *bp == '\r')
		{
			continue;
		}

		/*
		 * Find the values and remove surrounding spaces.
		 */
		np1 = buf;
		vp1 = strchr(np1, '=');
		if (vp1 == NULL)
		{
			continue;
		}
		np2 = vp1 -1;
		*vp1 = '\0';
		vp1++;
		if (np1 >= np2)
		{
			continue;
		}
		for(; np2 > np1; --np2)
		{
			if (isascii((int)*np2) && isspace((int)*np2))
				continue;
			break;
		}
		if (np1 >= np2)
		{
			continue;
		}
		for(; np1 < np2; ++np1)
		{
			if (isascii((int)*np1) && isspace((int)*np1))
				continue;
			break;
		}
		if (np1 >= np2)
		{
			continue;
		}
		++np2;
		if (*np2 != '\0')
			*np2 = '\0';

		for(vp2 = vp1; *vp2 != '\0'; ++vp2)
			continue;
		for(--vp2; vp2 > vp1; --vp2)
		{
			if (isascii((int)*vp2) && isspace((int)*vp2))
				continue;
			break;
		}
		if (vp1 >= vp2)
		{
			continue;
		}
		for(; vp1 < vp2; ++vp1)
		{
			if (isascii((int)*vp1) && isspace((int)*vp1))
				continue;
			break;
		}
		if (vp1 > vp2)
		{
			continue;
		}
		++vp2;
		if (*vp2 != '\0')
			*vp2 = '\0';

		rctx = conf_updateconfig(rctx, np1, vp1);
	}
	if (ferror(fp) && ! feof(fp))
	{
		xerror = errno;
		(void) fclose(fp);
		(void) snprintf(ebuf, elen, "%s(%d): %s: %s",
			__FILE__, __LINE__, path, strerror(xerror));
		errno = xerror;
		return NULL;
	}
	(void) fclose(fp);
	errno = 0;
	return rctx;
}

HASH_CTX *
conf_updateconfig(HASH_CTX *rctx, char *name, char *value)
{
	CONF_ARGV_CTX *avp    = NULL;

	if (name == NULL || value == NULL || rctx == NULL)
	{
		errno = EINVAL;
		return rctx;
	}
	avp = (CONF_ARGV_CTX *)hash_lookup(rctx, name, NULL, 0);
	if (avp == NULL)
	{
		avp = conf_initav();
		if (avp == NULL)
		{
			errno = ENOMEM;
			return rctx;
		}
		avp = conf_pushav(value, avp, __FILE__, __LINE__);
		(void) hash_lookup(rctx, name, avp, sizeof(CONF_ARGV_CTX));
		//avp = str_free(avp, __FILE__, __LINE__);
		return rctx;
	}
	avp = conf_pushav(value, avp, __FILE__, __LINE__);

	errno = 0;
	return rctx;
}

/*****************************************************
**  CONF_GETVAL -- Fetch a value by name
**
**	Parameters:
**		cctx	-- a hash table context
**		val	-- variable to look up.
**	Returns:
**		an arry of strings on success
**		NULL on error, sets errno
**	Notes:
**	Warning:
*/
char **
conf_getval(HASH_CTX *cctx, char *val)
{
	CONF_ARGV_CTX *avp = NULL;

	avp = (CONF_ARGV_CTX *)hash_lookup(cctx, val, NULL, 0);
	if (avp == NULL)
	{
		return NULL;
	}
	return avp->ary;
}

/*****************************************************
**  CONF_SHUTDOWN -- Free the configuration data
**
**	Parameters:
**		cctx	-- a hash table context
**		val	-- variable to look up.
**	Returns:
**		an arry of strings on success
**		NULL on error, sets errno
**	Notes:
**	Warning:
*/
HASH_CTX *
conf_shutdown(HASH_CTX *cctx)
{
	return hash_shutdown(cctx);
}

void
conf_drop(HASH_CTX *rctx, char *name)
{
	if (rctx != NULL && name != NULL)
		(void) hash_drop(rctx, name);
}


CONF_ARGV_CTX *
conf_initav(void)
{
	CONF_ARGV_CTX *avp;

	avp = str_alloc(sizeof(CONF_ARGV_CTX), 1, __FILE__, __LINE__);
	if (avp == NULL)
		return NULL;

	avp->ary = NULL;
	avp->num  = 0;

	return avp;
}

CONF_ARGV_CTX *
conf_pushav(char *str, CONF_ARGV_CTX *avp, char *file, int line)
{
	if (str == NULL || avp == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	avp->ary = conf_pushnargv(str, avp->ary, &(avp->num), file, line);
	if (avp->ary == NULL)
		return NULL;

	return avp;
}

CONF_ARGV_CTX *
conf_freeav(CONF_ARGV_CTX *avp)
{
	if (avp == NULL)
		return NULL;
	avp->ary = conf_freenargv(avp->ary, &(avp->num));
	avp = str_free(avp, __FILE__, __LINE__);

	return NULL;
}

char **
conf_freenargv(char **ary, int *num)
{
	if (ary != NULL)
	{
		char **ccp;

		for (ccp = ary; *ccp != NULL; ++ccp)
		{
			*ccp = str_free(*ccp, __FILE__, __LINE__);
		}
		ary = str_free(ary, __FILE__, __LINE__);
	}
	if (num != NULL)
		*num = 0;
	return NULL;
}

char **
conf_pushnargv(char *str, char **ary, int *num, char *file, int line)
{
	int    i;
	char **tmp;

	if (str != NULL)
	{
		if (ary == NULL)
		{
			ary = str_alloc(sizeof(char **), 2, file, line);
			if (ary == NULL)
			{
				if (num != NULL)
					*num = 0;
				return NULL;
			}
			*ary = str_dup(str, __FILE__, __LINE__);
			*(ary+1) = NULL;
			if (*ary == NULL)
			{
				ary= str_free(ary, __FILE__, __LINE__);
				if (num != NULL)
					*num = 0;
				return NULL;
			}
			if (num != NULL)
				*num = 1;
			return ary;
		}
		i = 0;
		if (num == NULL)
		{
			for (i = 0; ;i++)
			{
				if (ary[i] == NULL)
					break;
			}
		}
		else
			i = *num;
		tmp = str_realloc((void *)ary, sizeof(char **) * (i+1), sizeof(char **) * (i+2),  __FILE__, __LINE__, FALSE);
		if (tmp == NULL)
		{
			ary = conf_freenargv(ary, num);
			return NULL;
		}
		ary = tmp;
		ary[i] = str_dup(str, __FILE__, __LINE__);
		if (ary[i] == NULL)
		{
			ary = conf_freenargv(ary, num);
			return NULL;
		}
		++i;
		ary[i] = NULL;
		if (num != NULL)
			*num = i;
	}
	return ary;
}


