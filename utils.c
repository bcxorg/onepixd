#include "onepixd.h"


/*****************************************************
**  UTIL_BG -- Go into the background
**
**	Parameters:
**		dofork	-- preform the fork if TRUE
**	Returns:
**		0 on success, otherwise errno
**	Side Effects:
**		Forks
**		Detatches from controlling terminal
**		Closes standard I/O
**		Gets rid of groups
**	Notes:
**		This has not been validated on all
**		operating systems.
*/
int
util_bg(int dofork)
{
	int fd, i;

	if (dofork == TRUE)
	{
		i = fork();
		if (i < 0)
			return errno;

		if (i != 0)
			exit(0);
	}

	fd = fileno(stdin);
	(void) fflush(stdout);
	(void) dup2(fd, fileno(stdout));
	(void) fflush(stderr);
	(void) dup2(fd, fileno(stderr));
	(void) close(fd);
	(void) setsid();
#if ! OS_DARWIN
	(void) setpgrp();
#endif
	return (errno = 0);
}

uid_t
util_getuid(char *user)
{
	struct passwd *pw = NULL;
	uid_t  uid;

	if (user == NULL)
		return 0;
	uid = strtoul(user, NULL, 10);
	if (uid > 0)
		pw = getpwuid(uid);
	if (pw == NULL)
		pw = getpwnam(user);
	if (pw == NULL)
		return 0;
	return pw->pw_uid;
}

/*****************************************************
**  SETRUNASUSER -- Become this user to run as
**
**	Parameters:
**		uid	-- login name or user-id
**	Returns:
**		0 on success, otherwise -1
**		On error logs a message.
**	Side Effects:
**		Changes the real user.
**	Notes:
**		Does not change the group or drop old groups.
**		Does not change the effective user-id
*/
int
setrunasuser(char *userid)
{
	pid_t 		uid;
	struct passwd *	pw;
	char		ebuf[BUFSIZ];

	if (userid != NULL)
	{
		if (isdigit((int)*userid))
		{
			uid = atoi(userid);
		}
		else
		{
			pw = getpwnam(userid);
			if (pw == NULL)
			{
				(void) snprintf(ebuf, sizeof ebuf, "%s: %s\n",
					userid, "No such user.");
				log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
				return -1;
			}
			uid = pw->pw_uid;
		}

		if (uid == 0)
			return -1;

		if (setuid(uid) != 0)
		{
			(void) snprintf(ebuf, sizeof ebuf, "setuid(%d): %s\n",
				(int)uid, strerror(errno));
			log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
			return -1;
		}
		return 0;
	}
	(void) snprintf(ebuf, sizeof ebuf, "NULL: Cannot set user-id to NULL.\n");
	log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
	return -1;
}

/*****************************************************
**  WRITE_PID_FILE -- Write a file with the PID number
**
**	Parameters:
**		pidfile	-- /path/filename
**	Returns:
**		0 on success, otherwise -1
**		On error, logs an error message.
**	Side Effects:
**		Creates and writes a file
**	Notes:
**		This file is written as the runasuser user.
**		May overwrite an existing file.
*/
int
write_pid_file(char *pidfile)
{
	FILE *fp	= NULL;
	char  ebuf[BUFSIZ];

	if (pidfile == NULL)
	{
		(void) snprintf(ebuf, sizeof ebuf, "NULL: /path/filename for PID file was NULL");
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		return -1;
	}

	if ((fp = fopen(pidfile, "w+")) != NULL)
	{
		(void) fprintf(fp, "%d\n", (int)getpid());
		(void) fclose(fp);
		return 0;
	}
	(void) snprintf(ebuf, sizeof ebuf, "%s: %s\n", pidfile, strerror(errno));
	log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
	return -1;
}

