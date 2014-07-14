#include "onepixd.h"

typedef struct {
	char cols[MAX_COLUMNS][MAX_COLUMN_NAME_LEN];
	int ncols;
} COLS;
typedef struct {
	FILE *fps[MAX_COL_FILES_PER_HR];
	int   nfps;
	char  fname[MAX_COL_FILES_PER_HR][MAXPATHLEN];
	COLS  c[MAX_COL_FILES_PER_HR];
} FILES;


char *
file_time_to_day(time_t when, char *ebuf, size_t elen)
{
	time_t  now	= 0;
	int	days	= 0;
	time_t  secs	= 0;
	char   *retp	= NULL;
	char    tbuf[MAXPATHLEN];
	char	nbuf[64];
	char  **ary	= NULL;
	struct   tm *lt	= NULL;
	char	*cp;
	int	 xerror;
#if HAVE_LOCALTIME_R
	struct   tm tm;
#else
	static pthread_mutex_t localtime_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
	
	/*
	 * If the record is for a time that is more in 
	 * the past then we archive for, we return NULL
	 * to supress the write.
	 */
	ary = config_lookup(CONF_DISK_ARCHIVE_DAYS);
	if (ary == NULL)
		days = 3;	/* 3 days is the default */
	else
		days = strtoul(ary[0], NULL, 10);
	secs = days * 24 * 60 * 60;
	now = time(NULL);
	if ((now - when) > secs)
	{
		(void) strlcpy(ebuf, "Data write for time ", elen);
		(void) strlcat(ebuf, ctime(&when), elen);
		cp = strrchr(ebuf, '\n');
		if (cp != NULL)
			*cp = '\0';
		(void) strlcat(ebuf, ": Too old: ", elen);
		(void) str_ultoa(days, nbuf, sizeof nbuf);
		(void) strlcat(ebuf, nbuf, elen);
		(void) strlcat(ebuf, " days Maximum", elen);
		errno = EINVAL;
		return NULL;
	}

	/*
	 * localtime() is not always thread safe, and there
	 * is not POSIX requrement for a localtime_r() so
	 * we use a mutex instead.
	 */
#if HAVE_LOCALTIME_R
	lt = localtime_r(&when, &tm);
#else
	(void) pthread_mutex_lock(&localtime_mutex);
	lt = localtime(&when);
#endif

	(void) strftime(tbuf, sizeof tbuf, "%j", lt);

#if ! HAVE_LOCALTIME_R
	(void) pthread_mutex_unlock(&localtime_mutex);
#endif
	retp = str_dup(tbuf, __FILE__, __LINE__);
	if (retp == NULL)
	{
		xerror = errno;
		(void) strlcpy(ebuf, "Data write for time ", elen);
		(void) strlcat(ebuf, ctime(&when), elen);
		cp = strrchr(ebuf, '\r');
		if (cp != NULL)
			*cp = '\0';
		(void) strlcat(ebuf, ": Memory allocation failure: ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return NULL;
	}
	return retp;
}

int
file_make_path_exist(char *path, char *ebuf, size_t elen)
{
	char		 buf[MAXPATHLEN];
	char		*cp = NULL;
	char		*ep = NULL;
	int		 ret;
	int		 xerror;
	struct stat	 statb;

	cp = path;
	if (*cp == '/')
		++cp;
	do
	{
		ep = strchr(cp, '/');
		if (ep != NULL)
			*ep = '\0';
		(void) memset(buf, '\0', sizeof buf);
		(void) strlcpy(buf, path, sizeof buf);

		ret = stat(buf, &statb);
		if (ret < 0 && errno != ENOENT)
		{
			xerror = errno;
			(void) strlcpy(ebuf, "Data stat() path: ", elen);
			(void) strlcat(ebuf, buf, elen);
			(void) strlcat(ebuf, ":  ", elen);
			(void) strlcat(ebuf, strerror(xerror), elen);
			return errno = xerror;
		}
		if (ret < 0 && errno == ENOENT)
		{
			ret = mkdir(buf, 0775);
			if (ret < 0)
			{
				xerror = errno;
				(void) strlcpy(ebuf, "Data mkdir() path: ", elen);
				(void) strlcat(ebuf, buf, elen);
				(void) strlcat(ebuf, ":  ", elen);
				(void) strlcat(ebuf, strerror(xerror), elen);
				return errno = xerror;
			}
		}

		if (ep != NULL)
		{
			*ep = '/';
			cp = ep + 1;
		}
		else
			cp = NULL;
	} while (cp != NULL && *cp != '\0');
	return errno = 0;
}

char *
file_iso_to_path(char *iso)
{
	return NULL;
}

int
file_write_datum(time_t when, char *fname, char *datum, char *ebuf, size_t elen)
{
	char    fbuf[MAXPATHLEN];
	char   *path	= NULL;
	char   *cp	= NULL;
	char  **ary	= NULL;
	FILE *	fp	= NULL;
	int     ret;
	int	xerror;

	path = file_time_to_day(when, ebuf, elen);
	if (path == NULL)
		return errno;

	ary = config_lookup(CONF_DISK_ARCHIVE_PATH);
	if (ary == NULL)
		(void) strlcpy(fbuf, DEFAULT_DISK_ARCHIVE_PATH, sizeof fbuf);
	else
		(void) strlcpy(fbuf, ary[0], sizeof fbuf);
	cp = fbuf + strlen(fbuf) + 1;
	if (*cp != '/')
		(void) strlcat(fbuf, "/", sizeof fbuf);
	(void) strlcat(fbuf, path, sizeof fbuf);
	path = str_free(path, __FILE__, __LINE__);

	ret = file_make_path_exist(fbuf, ebuf, elen);
	if (ret != 0)
		return ret;

	(void) strlcat(fbuf, "/", sizeof fbuf);
	(void) strlcat(fbuf, fname, sizeof fbuf);

	fp = fopen(fbuf, "a");
	if (fp == NULL)
	{
		xerror = errno;
		(void) strlcpy(ebuf, fbuf, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		return errno = xerror;
	}
	ret = fprintf(fp, "%s\n", datum);
	if (ret < 0)
	{
		xerror = errno;
		(void) strlcpy(ebuf, fbuf, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		return errno = xerror;
	}
	ret = fclose(fp);
	if (ret == EOF)
	{
		xerror = errno;
		(void) strlcpy(ebuf, fbuf, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		return errno = xerror;
	}
	return errno = 0;
}

void
file_write_array(char **datap, char *fname)
{
	char    fbuf[MAXPATHLEN];
	char    ebuf[MAX_EBUF_LEN];
	char	nbuf[MAX_ACCII_INT];
	char   *path	= NULL;
	char   *cp	= NULL;
	char  **ary	= NULL;
	char  **app	= NULL;
	FILE *	fp	= NULL;
	int	nbad	= 0;
	int     ret;
	time_t	curday;
	time_t  nextday;
	int	xerror;

	if (datap == NULL || fname == NULL)
	{
		xerror = errno;
		(void) strlcpy(fbuf, "Write more data failed. Empty data.", sizeof fbuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, fbuf);
		return;
	}

	curday = nextday = 0;
	app = datap;

	while (1)
	{
		if (fp == NULL)
		{
			ary = config_lookup(CONF_DISK_ARCHIVE_PATH);
			if (ary == NULL)
				(void) strlcpy(fbuf, DEFAULT_DISK_ARCHIVE_PATH, sizeof fbuf);
			else
				(void) strlcpy(fbuf, ary[0], sizeof fbuf);
			cp = fbuf + strlen(fbuf) + 1;
			if (*cp != '/')
				(void) strlcat(fbuf, "/", sizeof fbuf);

			if (app == NULL || *app == NULL)
				break;
			cp = strchr(*app, '=');
			++cp;
			curday = strtoul(cp, NULL, 10);
			path = file_time_to_day(curday, ebuf, sizeof ebuf);
			(void) strlcat(fbuf, path, sizeof fbuf);
			path = str_free(path, __FILE__, __LINE__);

			/*
			 * Make sure the directory exits.
			 */
			ret = file_make_path_exist(fbuf, ebuf, sizeof ebuf);
			if (ret != 0)
			{
				xerror = errno;
				(void) strlcpy(fbuf, "Create Path failed: ", sizeof fbuf);
				(void) strlcat(fbuf, ebuf, sizeof fbuf);
				log_emit(LOG_ERR, NULL, __FILE__, __LINE__, fbuf);
				return;
			}

			(void) strlcat(fbuf, "/", sizeof fbuf);
			(void) strlcat(fbuf, fname, sizeof fbuf);

			fp = fopen(fbuf, "a");
			if (fp == NULL)
			{
				xerror = errno;
				(void) strlcpy(ebuf, fbuf, sizeof ebuf);
				(void) strlcat(ebuf, ": ", sizeof ebuf);
				(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
				log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
				return;
			}
		}
		for (app = datap; *app != NULL; app++)
		{
			cp = strchr(*app, '=');
			++cp;
			nextday = strtoul(cp, NULL, 10);
			if (curday != nextday)
			{
				break;
			}
			ret = fprintf(fp, "%s\n", cp);
			if (ret < 0)
			{
				xerror = errno;
				(void) strlcpy(ebuf, fbuf, sizeof ebuf);
				(void) strlcat(ebuf, ": ", sizeof ebuf);
				(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
				log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
				return;
			}
		}
		ret = fclose(fp);
		if (ret == EOF)
		{
			xerror = errno;
			(void) strlcpy(ebuf, fbuf, sizeof ebuf);
			(void) strlcat(ebuf, ": ", sizeof ebuf);
			(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
			log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
			return;
		}
		if (*app == NULL)
			break;
	}
	if (nbad != 0)
	{
		xerror = errno;
		(void) strlcpy(ebuf, fbuf, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) str_ultoa(nbad, nbuf, sizeof nbuf);
		(void) strlcat(ebuf, nbuf, sizeof ebuf);
		(void) strlcat(ebuf, " badly formed line in this upload.", sizeof ebuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		return;
	}
	return;
}

int
file_prune(time_t secs)
{
	return 0;
}

#if 0
char **
file_help(char *ip)
{
	char **	dirlist		= NULL;
	char **	collist		= NULL;
	char **	cpp		= NULL;
	char *	currdir		= NULL;
	DIR *dirp		= NULL;
	struct dirent *d	= NULL;
	char ** ary		=NULL;
	int	xerror;
	struct stat statp;
	char path[MAXPATHLEN];
	char fullname[BUFSIZ];
	char ebuf[MAX_EBUF_LEN];

	(void) memset(path, '\0', sizeof path);
	ary = config_lookup(CONF_DISK_ARCHIVE_PATH);
	if (ary == NULL)
		(void) strlcpy(path, DEFAULT_DISK_ARCHIVE_PATH, sizeof path);
	else
		(void) strlcpy(path, ary[0], sizeof path);
	dirlist = pushargv(path, dirlist, __FILE__, __LINE__);

	while (1)
	{
		if (dirlist == NULL)
			break;
		if (currdir != NULL)
			currdir = str_free(currdir, __FILE__, __LINE__);
		currdir = popargv(dirlist);
		if (currdir == NULL)
		{
			dirlist = NULL;
			break;
		}
		if (stat(currdir, &statp) != 0)
		{
			xerror = errno;
			(void) strlcpy(ebuf, currdir, sizeof ebuf);
			(void) strlcat(ebuf, ": ", sizeof ebuf);
			(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
			log_emit(LOG_ERR, ip, __FILE__, __LINE__, ebuf);
			currdir = str_free(currdir, __FILE__, __LINE__);
			continue;
		}
		if (! S_ISDIR(statp.st_mode))
		{
			currdir = str_free(currdir, __FILE__, __LINE__);
			continue;
		}

		dirp = opendir(currdir);
		while ((d = readdir(dirp)) != NULL)
		{
			if ((d->d_name)[0] == '.')
				continue;
			(void) strlcpy(fullname, currdir,    sizeof fullname);
			(void) strlcat(fullname, "/",       sizeof fullname);
			(void) strlcat(fullname, d->d_name, sizeof fullname);

			if (stat(fullname, &statp) != 0)
			{
				fprintf(stderr, "%s(%d): %s: %s\n", __FILE__, __LINE__, fullname, strerror(errno));
				continue;
			}
			if (S_ISLNK(statp.st_mode))
				continue;
			if (S_ISDIR(statp.st_mode))
			{
				dirlist = pushargv(fullname, dirlist, __FILE__, __LINE__);
				continue;
			}
			if (collist == NULL)
			{
				collist = pushargv(d->d_name, collist, __FILE__, __LINE__);
				continue;
			}
			for (cpp = collist; *cpp != NULL; ++cpp)
			{
				if (strcmp(*cpp, d->d_name) == 0)
					break;
				continue;
			}
			if (*cpp == NULL)
			{
				collist = pushargv(d->d_name, collist, __FILE__, __LINE__);
			}
			continue;
		}
		(void) closedir(dirp);
		currdir = str_free(currdir, __FILE__, __LINE__);
	}
	if (dirlist != NULL)
		dirlist = clearargv(dirlist);
	return collist;
}
#endif /* 0 */

time_t
file_path_to_time(char *path)
{
	char *cp	= NULL;
	char *ep	= NULL;
	struct   tm tm;

	if (path == NULL)
		return time(NULL);
	(void) memset(&tm, '\0', sizeof(struct tm));
	cp = path;
	if (*cp == '/')
		++cp;
	tm.tm_year = strtoul(cp, &ep, 10);
	tm.tm_year -= 1900;
	cp = ep+1;
	tm.tm_mon = strtoul(cp, &ep, 10);
	tm.tm_mon -= 1;
	cp = ep+1;
	tm.tm_mday = strtoul(cp, &ep, 10);
	cp = ep+1;
	tm.tm_hour = strtoul(cp, &ep, 10);
	return mktime(&tm);
}

char **
file_decommaize(char *columns)
{
	char **	ary	= NULL;
	char *	cp	= NULL;
	char *	ep	= NULL;
	char	copy[BUFSIZ];

	if (columns == NULL)
		return NULL;

	(void) strlcpy(copy, columns, sizeof copy);
	cp = copy;
	do
	{
		ep = strchr(cp, ',');
		if (ep != NULL)
			*ep = '\0';
		ary = pushargv(cp, ary, __FILE__, __LINE__);
		if (ep != NULL)
			cp = ep+1;
		else
			break;
	} while(cp != NULL && *cp != '\0');
	return ary;
}

void
file_data_add(time_t start, time_t end, char *fname, int fromcol, COL_FILE_RET_T *colp)
{
	FILE *fp =		NULL;
	time_t 	timestamp =	0;
	char *	comma =		NULL;
	char **	gotcols =	NULL;
	char ** gpp =		NULL;
	char *	cp;
	int	i;
	char  	buf[BUFSIZ];

	/**** Should probably log these errors ******/
	if (fname == NULL)
		return;
	if (colp == NULL)
		return;
	if (fromcol < 0 || fromcol >= MAX_COLUMNS)
		return;

	fp = fopen(fname, "r");
	if (fp == NULL)
		return;
	while (fgets(buf, sizeof buf, fp) != NULL)
	{
		cp = strrchr(buf, '\r');
		if (cp != NULL)
			*cp = '\0';
		cp = strrchr(buf, '\n');
		if (cp != NULL)
			*cp = '\0';
		comma = NULL;
		timestamp = strtoul(buf, &comma, 10);
		if (timestamp < start || timestamp > end)
			continue;
		if (comma != NULL)
			*comma = '\0';
		else
			continue;
		gotcols = file_decommaize(comma+1);
		if (gotcols == NULL)
			continue;
		i = 0;
		for (gpp = gotcols; *gpp != NULL; ++gpp)
		{
			if (i == fromcol)
				break;
			++i;
		}
		if (*gpp == NULL)
			continue;
		colp->times = pushargv(buf, colp->times, __FILE__, __LINE__);
		colp->values = pushargv(*gpp, colp->values, __FILE__, __LINE__);

		gotcols = clearargv(gotcols);
	}
	if (ferror(fp))
		return;
}

COL_RET_T *
file_colret_free(COL_RET_T *colret)
{
	int i;

	if (colret == NULL)
		return NULL;
	for (i = 0; i < MAX_COLUMNS; i++)
	{
		if (colret->d[i].times != NULL)
			colret->d[i].times = clearargv(colret->d[i].times);
		if (colret->d[i].values != NULL)
			colret->d[i].values = clearargv(colret->d[i].values);
	}
	colret = str_free(colret, __FILE__, __LINE__);
	return NULL;
}

COL_RET_T *
file_fetch_col_files(FORMS_T *formp)
{
	char ** gotcols		= NULL;
	char **	gpp		= NULL;
	int	i;
	int	k;
	DIR *dirp		= NULL;
	struct dirent *d	= NULL;
	char	*dir		= NULL;
	char ** ary		= NULL;
	int	xerror;
	time_t	start;
	struct stat statp;
	char newdir[MAXPATHLEN];
	char path[MAXPATHLEN];
	char fullname[BUFSIZ];
	char ebuf[MAX_EBUF_LEN];
	COL_RET_T  *data	= NULL;

	if (formp == NULL)
		return NULL;


	(void) memset(path, '\0', sizeof path);
	ary = config_lookup(CONF_DISK_ARCHIVE_PATH);
	if (ary == NULL)
		(void) strlcpy(path, DEFAULT_DISK_ARCHIVE_PATH, sizeof path);
	else
		(void) strlcpy(path, ary[0], sizeof path);

	if (stat(path, &statp) != 0)
	{
		xerror = errno;
		(void) strlcpy(ebuf, path, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		errno = xerror;
		return NULL;
	}
	if (! S_ISDIR(statp.st_mode))
	{
		xerror = errno;
		(void) strlcpy(ebuf, path, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		return NULL;
	}
	dir = NULL;
	start = formp->start.secs;
	do
	{
		dir = file_time_to_day(start, ebuf, sizeof ebuf);
		if (dir != NULL)
			break;
		start += (3600 * 24);
		if (start > formp->end.secs)
			return NULL;
	} while (dir == NULL);
	data = str_alloc(sizeof(COL_RET_T), 1, __FILE__, __LINE__);
	if (data == NULL)
	{
		xerror = errno;
		(void) strlcpy(ebuf, dir, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		return NULL;
	}
	(void) memset(data, '\0', sizeof(COL_RET_T));
	while (1)
	{
		(void) strlcpy(newdir, path, sizeof newdir);
		(void) strlcat(newdir, "/", sizeof newdir);
		(void) strlcat(newdir, dir, sizeof newdir);
		dirp = opendir(newdir);
		if (dirp == NULL)
		{
			if (errno != ENOENT)
			{
				xerror = errno;
				(void) strlcpy(ebuf, newdir, sizeof ebuf);
				(void) strlcat(ebuf, ": ", sizeof ebuf);
				(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
				log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
			}
			goto nextdir;
		}
		while ((d = readdir(dirp)) != NULL)
		{
			if ((d->d_name)[0] == '.')
				continue;
			(void) strlcpy(fullname, newdir,    sizeof fullname);
			(void) strlcat(fullname, "/",       sizeof fullname);
			(void) strlcat(fullname, d->d_name, sizeof fullname);

			if (stat(fullname, &statp) != 0)
			{
				fprintf(stderr, "%s(%d): %s: %s\n", __FILE__, __LINE__, fullname, strerror(errno));
				continue;
			}
			if (S_ISLNK(statp.st_mode))
				continue;
			if (! S_ISREG(statp.st_mode))
				continue;

			gotcols = file_decommaize(d->d_name);
			if (gotcols == NULL)
				continue;

			for (k = 0, gpp = gotcols; *gpp != NULL; gpp++, k++)
			{
				for (i = 0; i < formp->selected_cols.nary; i++)
				{
					if (strcasecmp(formp->selected_cols.ary[i], *gpp) != 0)
						continue;
					(void) file_data_add(formp->start.secs,
						formp->end.secs, fullname, k-1, &(data->d[i]));
				}
			}
			gotcols = clearargv(gotcols);
		}
		(void) closedir(dirp);
nextdir:
		dir = str_free(dir, __FILE__, __LINE__);
		start += (3600 * 24);
		if (start > formp->end.secs)
			break;
		dir = file_time_to_day(start, ebuf, sizeof ebuf);
		if (dir == NULL)
			break;
		continue;
	} /* while (1) scan directory */

	/* may need to sort the results into time order */

	if (gotcols != NULL)
		gotcols = clearargv(gotcols);
	return data;
}


/********************************************************************
** GARBAGE COLLECTION for old data files
** main() becomes this routine and runs until the program
** is signaled to exit.
********************************************************************/

/*
** Return a list of all the directories in the data directory.
*/
char **
file_list_dirs(void)
{
	char **	dirlist	= NULL;
	char **	ary	= NULL;
	DIR *	dirp	= NULL;
	char **	retp	= NULL;
	char *	currdir	= NULL;
	int    	xerror;
	char   	ebuf[MAX_EBUF_LEN];
	char   	fullname[BUFSIZ];
	char   	basedir[MAXPATHLEN];
	struct 	stat statb;
	struct 	dirent *d = NULL;

	ary = config_lookup(CONF_DISK_ARCHIVE_PATH);
	if (ary == NULL)
		(void) strlcpy(basedir, DEFAULT_DISK_ARCHIVE_PATH, sizeof basedir);
	else
		(void) strlcpy(basedir, ary[0], sizeof basedir);

	dirlist = pushargv(basedir, dirlist, __FILE__, __LINE__);
	currdir  = NULL;
	while (1)
	{
		if (dirlist == NULL)
			break;
		if (currdir != NULL)
			currdir = str_free(currdir, __FILE__, __LINE__);
		currdir = popargv(dirlist);
		if (currdir == NULL)
		{
			dirlist = NULL;
			break;
		}
		if (stat(currdir, &statb) != 0)
		{
			xerror = errno;
			(void) strlcpy(ebuf, currdir, sizeof ebuf);
			(void) strlcat(ebuf, ": ", sizeof ebuf);
			(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
			log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
			currdir = str_free(currdir, __FILE__, __LINE__);
			continue;
		}
		if (! S_ISDIR(statb.st_mode))
		{
			currdir = str_free(currdir, __FILE__, __LINE__);
			continue;
		}
		retp = pushargv(currdir, retp, __FILE__, __LINE__);
		dirp = opendir(currdir);
		while ((d = readdir(dirp)) != NULL)
		{
			if ((d->d_name)[0] == '.')
				continue;
			(void) strlcpy(fullname, currdir,    sizeof fullname);
			(void) strlcat(fullname, "/",       sizeof fullname);
			(void) strlcat(fullname, d->d_name, sizeof fullname);

			if (stat(fullname, &statb) != 0)
			{
				fprintf(stderr, "%s(%d): %s: %s\n", __FILE__, __LINE__, fullname, strerror(errno));
				continue;
			}
			if (S_ISLNK(statb.st_mode))
				continue;
			if (S_ISDIR(statb.st_mode))
			{
				dirlist = pushargv(fullname, dirlist, __FILE__, __LINE__);
				continue;
			}
			/* were are at the file level */
			continue;
		}
		(void) closedir(dirp);
		currdir = str_free(currdir, __FILE__, __LINE__);
	}
	if (dirlist != NULL)
		dirlist = clearargv(dirlist);
	return retp;
}

#define FILE_TYPE_FILE (1)
#define FILE_TYPE_DIR  (2)

static void
file_unlink(char *path, int type)
{
	char   ebuf[MAX_EBUF_LEN];
	int    xerror;
	int    ret;

	if (path == NULL)
		return;

	if (type == FILE_TYPE_FILE)
		ret = unlink(path);
	else if (type == FILE_TYPE_DIR)
		ret = rmdir(path);
	else
		return;

	if (ret != 0)
	{
		xerror = errno;
		(void) strlcpy(ebuf, path, sizeof ebuf);
		(void) strlcat(ebuf, ": ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
	}
}

static void
file_dir_trim(char *path, time_t expire_secs)
{
	struct stat statdir, statfile;
	int ret;
	time_t now;
	time_t then;
	DIR *  dirp	= NULL;
	struct dirent *d = NULL;
	char   fullname[MAXPATHLEN];
	int    count;

	if (path == NULL || expire_secs == 0)
		return;

	ret = stat(path, &statdir);
	if (ret != 0)
		return;
	if (! S_ISDIR(statdir.st_mode))
		return;

	count = 0;
	dirp = opendir(path);
	while ((d = readdir(dirp)) != NULL)
	{
		if (strcmp(d->d_name, ".") == 0)
			continue;
		if (strcmp(d->d_name, "..") == 0)
			continue;

		(void) strlcpy(fullname, path, sizeof fullname);
		(void) strlcat(fullname, "/", sizeof fullname);
		(void) strlcat(fullname, d->d_name, sizeof fullname);
		ret = stat(fullname, &statfile);
		if (ret != 0)
			continue;
		if (S_ISDIR(statfile.st_mode))
		{
			count = 1;
			break;
		}

		now = time(NULL);
		then = now - expire_secs;
		if (statfile.st_mtime < then)
			(void) file_unlink(fullname, FILE_TYPE_FILE);
		else
			count += 1;
		continue;
	}
	(void) closedir(dirp);
	if (count == 0)
	{
		now = time(NULL);
		now -= expire_secs;
		if (statdir.st_mtime < now)
			(void) file_unlink(path, FILE_TYPE_DIR);
	}
	return;
}

void
file_prune_garbage(int *shutdown)
{
	char **dirs = NULL;
	char **dpp  = NULL;
	time_t expire = 0;
	time_t now;
	char **ary	= NULL;
	struct timespec  rqtp;

	ary = config_lookup(CONF_DISK_ARCHIVE_DAYS);
	if (ary == NULL)
		expire = strtoul(DEFAULT_DISK_ARCHIVE_PATH, NULL, 10);
	else
		expire = strtoul(ary[0], NULL, 10);
	expire = expire * 24 * 3600; /* convert days to seconds */

	while (*shutdown != TRUE)
	{
		dirs = file_list_dirs();
		if (dirs != NULL)
		{
			for (dpp = dirs; *dpp != NULL; ++dpp)
			{
				file_dir_trim(*dpp, expire);
			}
		}
		dirs = clearargv(dirs);
		while(*shutdown != TRUE)
		{
			/*
			 * Wake up to garbage clean once each hour
			 * on the hour.
			 */
			rqtp.tv_sec  = 1;
			rqtp.tv_nsec = 0;
			(void) nanosleep(&rqtp, NULL);
			now = time(NULL);
			if ((now % 3600) == 0)
				break;

		}
	}
	return;
}
