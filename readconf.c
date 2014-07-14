#include "onepixd.h"

/* Add auto updating later */
static pthread_mutex_t Hconfmutex = PTHREAD_MUTEX_INITIALIZER;
static HASH_CTX *Hconfbak = NULL;
static HASH_CTX *Hconf = NULL;

static char *Cfilename = NULL;

time_t
config_stat_file()
{
	struct stat statb;
	int ret;

	if (Cfilename == NULL)
		return (time_t) 0;
	ret = lstat(Cfilename, &statb);
	if (ret != 0)
		return (time_t) 0;
	return statb.st_mtime;
}

int
config_read_file(char *path)
{
	HASH_CTX *hctx;
	int xerror;
	char ebuf[BUFSIZ];

	if (path == NULL)
	{
		if (Cfilename == NULL)
			return errno = EINVAL;
		else
			path = Cfilename;
	}
	else
		Cfilename = str_dup(path, __FILE__, __LINE__);

	hctx = conf_init(NULL, path, ebuf, sizeof ebuf);
	if (hctx == NULL)
	{
		xerror = (errno == 0) ? errno = ENOMEM : errno;
		(void) fprintf(stderr, "Read: %s\n", ebuf);
		return errno = xerror;
	}

	(void) pthread_mutex_lock(&Hconfmutex);
	 if (Hconfbak != NULL)
		Hconfbak = conf_shutdown(Hconfbak);
	 Hconfbak = Hconf;
	 Hconf = hctx;
	(void) pthread_mutex_unlock(&Hconfmutex);
	return errno=0;
}


char **
config_lookup(char *str)
{
	char **ary;

	if (str == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	(void) pthread_mutex_lock(&Hconfmutex);
	 ary = conf_getval(Hconf, str);
	(void) pthread_mutex_unlock(&Hconfmutex);
	return ary;
}

static int
config_errout(char *name, char *value, char *why, int max)
{
	if (name == NULL || why == NULL)
		return EINVAL;
	printf("Config item: \"%s\"", name);
	if (value != NULL)
		printf("=\"%s\" %s", value, why);
	else
		printf(" %s", why);
	if (max > 0)
		printf(": %d is maximum.", max);
	printf("\n");
	return EINVAL;
}

int
config_validate()
{
	char **ary	= NULL;
	char **app	= NULL;
	char  *cp	= NULL;
	char  *xp	= NULL;
	char   ebuf[MAX_EBUF_LEN];
	int    ret	= 0;
	uid_t  uid	= 0;

	if (Hconf == NULL)
	{
		(void) fprintf(stderr, "No configuration information to validate\n");
		return EINVAL;
	}

	cp = CONF_HOME_DIR;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		if (config_dir_must_exist(ary[0], ebuf, sizeof ebuf) != 0)
			ret = config_errout(cp, ary[0], ebuf, 0);
		if (ary != NULL && ary[1] != NULL)
			ret = config_errout(cp, NULL, "Too many of this item found", 1);
	}

	cp = CONF_DISK_ARCHIVE_DAYS;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contains a non-integer", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_DISK_ARCHIVE_PATH;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		if (config_dir_must_exist(ary[0], ebuf, sizeof ebuf) != 0)
			ret = config_errout(cp, ary[0], ebuf, 0);
		if (ary != NULL && ary[1] != NULL)
			ret = config_errout(cp, NULL, "Too many of this item found", 1);
	}

	cp = CONF_SEND_404_ON_ERROR;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		if (truefalse(ary[0]) == NEITHER)
			ret = config_errout(cp, ary[0], "Value is neither true nor false", 0);
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_GIF_INTERFACE;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp) && *xp != '.')
			{
				ret = config_errout(cp, ary[0], "Value contained other than integer or dot", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_GIF_PORT;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_GIF_FILE;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		ret = access(ary[0], R_OK);
		if (ret != 0)
		{
			ret = config_errout(cp, ary[0], strerror(errno), 0);
		}
		else if (ary != NULL && ary[1] != NULL)
			ret = config_errout(cp, NULL, "Too many of this item found", 1);
	}

	cp = CONF_GIF_TIMEOUT;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_GIF_PATH_COLUMNS;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (app = ary; app != NULL && *app != NULL; ++app)
		{
			char *ep;

			ep = strchr(*app, ':');
			if (ep == NULL)
			{
				ret = config_errout(cp, *app, "Missing mandaory colon.", 0);
				continue;
			}
			*ep = '\0';
			for (xp = *app; *xp != '\0'; ++xp)
			{
				if (! isascii((int) *xp) || (! isalnum((int) *xp) && *xp != '_') )
				{
					ret = config_errout(cp, *app, "Value not all alphanumeric or _ ", 0);
					break;
				}
			}
			*ep = ':';
			++ep;
			for (xp = ep; *xp != '\0'; ++xp)
			{
				if (! isascii((int) *xp) || (! isalnum((int) *xp) && *xp != '_' && *xp != ',') )
				{
					ret = config_errout(cp, *app, "Value not all alphanumeric or _ or ,", 0);
					break;
				}
			}
			if (strlen(*app) >= MAX_COLUMN_NAME_LEN-1)
				ret = config_errout(cp, *app, "String value too long", MAX_COLUMN_NAME_LEN-1);
		}
	}

	cp = CONF_DATA_INTERFACE;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp) && *xp != '.')
			{
				ret = config_errout(cp, ary[0], "Value contained other than integer or dot", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_DATA_PORT;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_DATA_TIMEOUT;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_UPLOAD_INTERFACE;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp) && *xp != '.')
			{
				ret = config_errout(cp, ary[0], "Value contained other than integer or dot", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_UPLOAD_PORT;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	cp = CONF_UPLOAD_TIMEOUT;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	/*
	 * more_post_path= may be undefined. But if it is defined
	 * no element may have a character that is other than alpha-numeric
	 * or the underscore. And each element must be < 127 characters in length.
	 * There may only be 16 path elements listed at most.
	 */
	cp = CONF_UPLOAD_PATH_COLUMN;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (app = ary; app != NULL && *app != NULL; ++app)
		{
			char *ep;

			ep = strchr(*app, ':');
			if (ep == NULL)
			{
				ret = config_errout(cp, *app, "Missing mandaory colon.", 0);
				continue;
			}
			*ep = '\0';
			for (xp = *app; *xp != '\0'; ++xp)
			{
				if (! isascii((int) *xp) || (! isalnum((int) *xp) && *xp != '_') )
				{
					ret = config_errout(cp, *app, "Value not all alphanumeric or _ ", 0);
					break;
				}
			}
			*ep = ':';
			++ep;
			for (xp = ep; *xp != '\0'; ++xp)
			{
				if (! isascii((int) *xp) || (! isalnum((int) *xp) && *xp != '_' && *xp != ',') )
				{
					ret = config_errout(cp, *app, "Value not all alphanumeric or _ or ,", 0);
					break;
				}
			}
			if (strlen(*app) >= MAX_COLUMN_NAME_LEN-1)
				ret = config_errout(cp, *app, "String value too long", MAX_COLUMN_NAME_LEN-1);
		}
	}

	/*
	 * numthreads= must be defined. It shouldn't be too small or performance
	 * will degrade. Between 500 and 1000 is a good starting range. If the
	 * value specified contains non-digits, it will be rejected. If the value
	 * is larger than 9996, it will be rejected as too big.
	 * This numthreads= may only be defined once.
	 */
	cp = CONF_NUMTHREADS;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		for (xp = ary[0]; *xp != '\0'; ++xp)
		{
			if (! isdigit((int) *xp))
			{
				ret = config_errout(cp, ary[0], "Value contained other than integers", 0);
				break;
			}
		}
		if (strtoul(ary[0], NULL, 10) > MAX_THREADS)
			ret = config_errout(cp, ary[0], "Too many threads requested", MAX_THREADS);
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	/*
	 * log_facility= is optional. If omitted, the facility becomes LOG_MAIL.
	 * Values for this entry must be one of the facilities defined in syslog.h
	 * with the leading LOG_ omitted. The value will be checked in a non-casesensitive
	 * manner. Useful alternatives are local1 through local8.
	 */
	cp = CONF_LOG_FACILITY;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		if (log_check_facility(ary[0]) < 0)
			ret = config_errout(cp, ary[0], "Not a known syslog facility", 0);
		if (ary != NULL && ary[1] != NULL)
			ret = config_errout(cp, NULL, "Too many of this item found", 1);
	}

	/*
	 * become_user= is Mandatory. It may be any user that exists in the password
	 * database, or may be an existing user-id numeric expression. The user specified
	 * must exist and must not be root.
	 */
	cp = CONF_BECOME_USER;
	ary = config_lookup(cp);
	if (ary == NULL)
		ret = config_errout(cp, NULL, "Mandatory item is missing", 0);
	else
	{
		if (strlen(ary[0]) == 0)
			ret = config_errout(cp, ary[0], "May not be an empty string", 0);
		if (ary[1] != NULL)
			ret = config_errout(cp, NULL, "Too many of this item found", 1);
		if ((uid = util_getuid(ary[0])) == 0)
			ret = config_errout(cp, ary[0], "You may specified a bad user.", 0);
	}

	/* If a pidfile= is defined it must not
	 * be an empty string. The pid file must not already
	 * exist. If it does, the file will not be overwritten.
	 */
	cp = CONF_PIDFILE;
	ary = config_lookup(cp);
	if (ary != NULL)
	{
		if (strlen(ary[0]) == 0)
		{
			ret = config_errout(cp, ary[0], "May not be an empty string", 0);
		}
	}
	if (ary != NULL && ary[1] != NULL)
		ret = config_errout(cp, NULL, "Too many of this item found", 1);

	return ret;
}

int
config_dir_must_exist(char *fname, char *ebuf, size_t elen)
{
	char **ary;
	struct stat statb;
	char  *cp;
	char   buf[MAXPATHLEN];
	uid_t  uid;
	int    ret;
	int    xerror;

	if (fname == NULL)
	{
		(void) strlcpy(ebuf, "File name was NULL.", elen);
		return EINVAL;
	}

	ary = config_lookup(CONF_HOME_DIR);

	ret = lstat(fname, &statb);
	if (ret != 0)
	{
		xerror = errno;
		if (ary != NULL)
		{
			(void) strlcpy(buf, ary[0], sizeof buf);
			(void) strlcat(buf, "/", sizeof buf);
			(void) strlcat(buf, fname, sizeof buf);
			ret = lstat(buf, &statb);
		}
		if (ret != 0)
		{
			if (errno != 0)
				xerror = errno;
			(void) strlcpy(ebuf, fname, elen);
			(void) strlcat(ebuf, ": ", elen);
			(void) strlcat(ebuf, strerror(xerror), elen);
			return xerror;
		}
		fname = buf;
	}
	if (S_ISLNK(statb.st_mode))
	{
		xerror = errno = ENOTDIR;
		(void) strlcpy(ebuf, fname, elen);
		(void) strlcat(ebuf, ": ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		return xerror;
	}

	if (! S_ISDIR(statb.st_mode))
	{
		xerror = errno = ENOTDIR;
		(void) strlcpy(ebuf, fname, elen);
		(void) strlcat(ebuf, ": ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		return xerror;
	}

	cp = CONF_BECOME_USER;
	ary = config_lookup(cp);
	if (ary == NULL)
	{
		xerror = errno = EPERM;
		(void) strlcpy(ebuf, fname, elen);
		(void) strlcat(ebuf, ": ", elen);
		(void) strlcat(ebuf, "No owner was specified with ", elen);
		(void) strlcat(ebuf, CONF_BECOME_USER, elen);
		(void) strlcat(ebuf, "=.", elen);
		return xerror;
	}
	if ((uid = util_getuid(ary[0])) == 0)
	{
		(void) strlcpy(ebuf, fname, elen);
		(void) strlcat(ebuf, ": User ", elen);
		(void) strlcat(ebuf, ary[0], elen);
		(void) strlcat(ebuf, " not allowed to own this directory.", elen);
		return errno = EPERM;
	}
	if (uid != statb.st_uid)
	{
		(void) strlcpy(ebuf, fname, elen);
		(void) strlcat(ebuf, ": User ", elen);
		(void) strlcat(ebuf, ary[0], elen);
		(void) strlcat(ebuf, " must own this directory but doesn't.", elen);
		return errno = EPERM;
	}
	if ((statb.st_mode & S_IRWXU) != S_IRWXU)
	{
		(void) strlcpy(ebuf, fname, elen);
		(void) strlcat(ebuf, ": User ", elen);
		(void) strlcat(ebuf, ary[0], elen);
		(void) strlcat(ebuf, " must have write permission but doesn't.", elen);
		return errno = EACCES;
	}
	return 0;
}

int
config_reset_entry(char *name)
{
	if (Hconf == NULL)
	{
		log_emit(LOG_ERR, "0.0.0.0", __FILE__, __LINE__,
				"No config items yet.");
		return EINVAL;
	}
	if (name == NULL)
	{
		log_emit(LOG_ERR, "0.0.0.0", __FILE__, __LINE__,
				"Null config name to reset.");
		return EINVAL;
	}
	(void) pthread_mutex_lock(&Hconfmutex);
	 (void) conf_drop(Hconf, name);
	(void) pthread_mutex_unlock(&Hconfmutex);
	return 0;
}

void
config_shutdown()
{
	(void) pthread_mutex_lock(&Hconfmutex);
	 if (Hconf != NULL)
		 Hconf = conf_shutdown(Hconf);
	 if (Hconfbak != NULL)
		 Hconfbak = conf_shutdown(Hconfbak);
	(void) pthread_mutex_unlock(&Hconfmutex);
	if (Cfilename != NULL)
		Cfilename = str_free(Cfilename, __FILE__, __LINE__);
	return;
}
