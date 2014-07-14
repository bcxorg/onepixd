/********************************************************
** LOG -- All logging of errors is handled here.
**
** Initially uses only syslog for logging.
** May add logging to files later.
*/

#include "onepixd.h"

typedef struct {
	char *name;
	int   value;
} SYSLOG_F;
static SYSLOG_F Facilitynames[] = {
#ifdef LOG_AUTH
	{ "auth", LOG_AUTH },
#endif
#ifdef LOG_AUTHPRIV
	{ "authpriv", LOG_AUTHPRIV },
#endif
#ifdef LOG_CRON
	{ "cron", LOG_CRON },
#endif
#ifdef LOG_DAEMON
	{ "daemon", LOG_DAEMON },
#endif
#ifdef LOG_FTP
	{ "ftp", LOG_FTP },
#endif
#ifdef LOG_KERN
	{ "kern", LOG_KERN },
#endif
#ifdef LOG_LPR
	{ "lpr", LOG_LPR },
#endif
#ifdef LOG_MAIL
	{ "mail", LOG_MAIL },
#endif
#ifdef LOG_NEWS
	{ "news", LOG_NEWS },
#endif
#ifdef LOG_USER
	{ "user", LOG_USER },
#endif
#ifdef LOG_UUCP
	{ "uucp", LOG_UUCP },
#endif
#ifdef LOG_LOCAL0
	{ "local0", LOG_LOCAL0 },
#endif
#ifdef LOG_LOCAL1
	{ "local1", LOG_LOCAL1 },
#endif
#ifdef LOG_LOCAL2
	{ "local2", LOG_LOCAL2 },
#endif
#ifdef LOG_LOCAL3
	{ "local3", LOG_LOCAL3 },
#endif
#ifdef LOG_LOCAL4
	{ "local4", LOG_LOCAL4 },
#endif
#ifdef LOG_LOCAL5
	{ "local5", LOG_LOCAL5 },
#endif
#ifdef LOG_LOCAL6
	{ "local6", LOG_LOCAL6 },
#endif
#ifdef LOG_LOCAL7
	{ "local7", LOG_LOCAL7 },
#endif
	{ NULL, -1 }
};

int
log_check_facility(char *facility)
{
	SYSLOG_F *fptr;

	for(fptr = Facilitynames; fptr->name != NULL; ++fptr)
	{
		if (strcasecmp(facility, fptr->name) == 0)
			return fptr->value;
	}
	return -1;
}

int
log_init(char *facility, char *prog)
{
	int fac = 0;

	errno = 0;
	if (prog == NULL)
		prog = "undefined";

	if (facility == NULL)
	{
		(void) openlog(prog, LOG_PID, LOG_MAIL);
		return 0;
	}

	fac = log_check_facility(facility);
	if (fac < 0)
	{
		(void) openlog(prog, LOG_PID, LOG_MAIL);
		return 0;
	}
	(void) openlog(prog, LOG_PID, fac);
	return 0;
}

void
log_emit(int level, char *ipnum, char *file, int line, char *msg)
{
	char *s;

	if (level == LOG_INFO)
		s = "status";
	else if (level == LOG_ERR)
		s = "error";
	else if (level == LOG_WARNING)
		s = "warning";
	else
		s= "notice";

	if (file == NULL)
		file = __FILE__;
	if (line <= 0)
		line = __LINE__;

	if (msg == NULL)
		msg = "Undefined Error";

	if (isdebug(BUG_LOGS) == TRUE)
	{
		printf("DEBUG: %s(%d): %s: %s\n", file, line, s, msg);
		return;
	}

	if (ipnum == NULL || strcmp(ipnum, "0.0.0.0") == 0)
		(void)syslog(level, "%s(%d): %s=%s", file, line, s, msg);
	else
		(void)syslog(level, "%s(%d): client_ip=%s, %s=%s", file, line, ipnum, s, msg);
	return;
}

