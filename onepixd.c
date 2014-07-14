/********************************************************
** $Id: onepixd.c,v 1.13 2010/08/11 17:03:00 bcx Exp $
*********************************************************/

#include "onepixd.h"
#include "libtpool/tpool.h"

int Global_Die = FALSE;

/**********************************************
** Catch a signal to stop running. Process it
** and return. Just sets a flag.
***********************************************/
void
catch_sig(int sig)
{
	struct timespec  rqtp;
	static time_t was = 0;
	time_t now;

	/*
	 * One kill causes a gracefull shutdown.
	 * Two kills more than a second apart cause
	 * an immediate shutdown (with possibly lost
	 * data. This later should only be done if
	 * the damon becomes wedged.
	 */
	if (was == 0)
	{
		(void) time(&was);
		(void) signal(sig, SIG_IGN);
	}
	(void) time(&now);
	if (now - was > 1)
		(void) signal(sig, SIG_DFL);

	/*
	 * Let everyone know we are shutting down.
	 */
	Global_Die = TRUE;

	/*
	 * Sleep a second and return.
	 */
	rqtp.tv_sec  = 1;
	rqtp.tv_nsec = 0;
	(void) nanosleep(&rqtp, NULL);

	return;
}

/*
 * How to specify debugging by name.
 */
typedef struct {
	char *name;
	int   flag;
} DEBUGS;
static DEBUGS DebugNames[] = {
	{"config",	BUG_CONF},
	{"threads",	BUG_THREADS},
	{"main",	BUG_APP},
	{"log",		BUG_LOGS},
	{"gif",		BUG_GIF},
	{"io",		BUG_IO},
	{"all",		BUG_ALL},
	{ NULL,		0},
};

/*************************************************************
** MAIN
**	Reads the config file. Binds to a port. Launches the
**	worker and other threads. Listens for requests.
*/
int 
main(int argc, char **argv)
{
	char *		conf	= NULL;
	int	 	nthreads;
	int	 	c, ret;
	char**	 	ary;
	char*	 	prog;
	u_int		numfds		= 0;
	int	 	xerror;
	int	 	bg_flag	= TRUE;
	int	 	only_check_config;
	char		ebuf[BUFSIZ];
	char		nbuf[64];
	u_char *	gifimage	= NULL;
	u_char *	favicon		= NULL;
	int 		gifimagelen	= 0;
	int 		faviconlen	= 0;
	TPOOL_CTX *	tpool_ctx	= NULL;
	DEBUGS *	dp;
	LISTEN_CTX	*gctx		= NULL;
	LISTEN_CTX	*dctx		= NULL;
	LISTEN_CTX	*jctx		= NULL;
	pthread_t	gif_tid;
	pthread_t	data_tid;
	pthread_t	upload_tid;
#if HAVE_RESOURCE_H || HAVE_SYS_RESOURCE_H
	struct rlimit	rl;
#endif
	time_t		servertimeout;
	 
	prog = basename(argv[0]);
	if (prog == NULL)
		prog = argv[0];

	only_check_config = FALSE;
	servertimeout = 20;

	while ((c = getopt(argc, argv, "C:d:c:f")) != -1)
	{
		switch (c)
		{
			case 'f':
				bg_flag = FALSE;
				break;
			case 'C':
				only_check_config = TRUE;
				/*FALLTHROUGH*/
			case 'c':
				conf = optarg;
				break;
			case 'd':
				/*
				 * Debugging turned of if in background 
				 */
				if (bg_flag == TRUE)
					break;
				for (dp = DebugNames; dp->name != NULL; dp++)
				{
					if (strncasecmp(dp->name, optarg, strlen(optarg)) == 0)
					{
						setdebug(dp->flag);
						if (dp->flag == BUG_THREADS
							    || dp->flag == BUG_ALL)
							tpool_debug_set(TRUE);
						break;
					}
				}
				if (dp->name != NULL)
					break;
				printf("Unknown debug flag: %s, select from:\n", optarg);
				for (dp = DebugNames; dp->name != NULL; dp++)
					printf("\t%s\n", dp->name);
				return 0;
			case '?':
			default:
			usage:
				printf("Usage: %s "
					"[-c /path/onepixd.conf "
					"or "
					"-C /path/onepixd.conf] "
					"-d what "
					"[-f]\n",
					prog);
				return 0;
		}
	}
	if (argc != optind)
		goto usage;

	if (conf == NULL || strlen(conf) == 0)
	{
		(void) fprintf(stderr,
			"Required Parameter \"-c configfile\" missing.\n");
		goto usage;
	}
	ret = config_read_file(conf);
	if (ret != 0)
		return 0;

	/*
	 * All errors are logged, so we need to set up
	 * logging before checking the config file.
	 */
	ary = config_lookup(CONF_LOG_FACILITY);
	if (ary == NULL)
		ret = log_init(DEFAULT_LOG_FACILITY, prog);
	else
		ret = log_init(ary[0], prog);
	if (ret != 0)
		return 0;

	ret = config_validate();
	if (ret != 0)
		return 0;
	if (ret == 0 && only_check_config == TRUE)
		(void) printf("%s: FILE IS OKAY TO USE\n", conf);
	if (only_check_config == TRUE)
		return 0;


	ary = config_lookup(CONF_HOME_DIR);
	if (ary != NULL)
	{
		if (chdir(ary[0]) != 0)
		{
			xerror = errno;
			(void) snprintf(ebuf, sizeof ebuf,
				"Attempt to chdir(\"%s\"): %s",
				ary[0], strerror(xerror));
			log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
			goto shutdown_server;
		}
	}

	if (util_bg(bg_flag) != 0)
	{
		(void) snprintf(ebuf, sizeof ebuf, "fork(): %s\n", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}


#if HAVE_RESOURCE_H || HAVE_SYS_RESOURCE_H
#ifdef RLIMIT_AS
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_AS, &rl); 	/* max size virtual memory */
#endif
#ifdef RLIMIT_CPU
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_CPU, &rl);	/* unlimited CPU usage */
#endif
#ifdef RLIMIT_DATA
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_DATA, &rl);	/* unlimited memory */
#endif
#ifdef RLIMIT_FSIZE
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_FSIZE, &rl);	/* unlimited file sizes */
#endif
#ifdef RLIMIT_STACK
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_STACK, &rl);	/* unlimited stack */
#endif
#ifdef RLIMIT_CORE
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_CORE, &rl);	/* allow core dumps */
#endif
#ifdef RLIMIT_NOFILE
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_NOFILE, &rl);	/* maximum file descriptors */
	(void) getrlimit(RLIMIT_NOFILE, &rl);
	numfds = (rl.rlim_cur);

#endif
#endif /* HAVE_RESOURCE_H */

	ary = config_lookup(CONF_BECOME_USER);
	if (ary != NULL)
	{
		/*
		 * Note that setrunasuser() logs its own errors 
		 */
		if (setrunasuser(ary[0]) != 0)
			goto shutdown_server;
	}

	ary = config_lookup(CONF_NUMTHREADS);
	if (ary == NULL)
		nthreads = DEFAULT_THREADS;
	else
	{
		nthreads = strtoul(ary[0], NULL, 10);
		if (nthreads <= 0)
		{
			(void) fprintf(stderr,
				"Configuration item \"%s\" illegal value: %d.\n", 
					CONF_NUMTHREADS, nthreads);
			return 0;
		}
	}
	if (numfds == 0 || numfds > (nthreads * 3))
		numfds = nthreads * 3;
	(void) io_init(numfds);

	gifimage = gif_get1x1gif(&gifimagelen);
	if (gifimage == NULL)
	{
		(void) snprintf(ebuf, sizeof ebuf, "Load gif image: %s\n", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}
	favicon  = gif_getfavicon_ico(&faviconlen);

	/*
	 * Prepare to launch the thread to listen for inbound
	 * gif requests.
	 */
	gctx = str_alloc(sizeof(LISTEN_CTX), 1, __FILE__, __LINE__);
	if (gctx == NULL)
	{
		(void) snprintf(ebuf, sizeof ebuf, "alloc(): %s\n", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}
	gctx->type = LISTEN_TYPE_GIF;
	/* gif image only, no favicon */
	gctx->gifimage = gifimage;
	gctx->gifimagelen = gifimagelen;
	gctx->favicon  = NULL;
	gctx->faviconlen = 0;
	ary = config_lookup(CONF_GIF_PORT);
	if (ary == NULL)
		(void) strlcpy(gctx->port, "80", sizeof gctx->port);
	else
		(void) strlcpy(gctx->port, ary[0], sizeof gctx->port);

	ary = config_lookup(CONF_GIF_INTERFACE);
	if (ary == NULL)
		(void) strlcpy(gctx->interface, "INADDR_ANY", sizeof gctx->interface);
	else
		(void) strlcpy(gctx->interface, ary[0], sizeof gctx->interface);

	/*
	 * Everything from here down must be thread safe.
	 */

	tpool_ctx = tpool_init(nthreads, ebuf, sizeof ebuf);
	if (tpool_ctx == NULL)
		goto shutdown_server;
	gctx->tpool_ctx = tpool_ctx;


	/*
	 * The GIF server may have to bind to a privaliged port, such
	 * as port 80, so we launch it and expect it to change the
	 * user id after its bind.
	 */
	ret = pthread_create(&gif_tid, NULL, thread_listener, (void *)gctx);
	if (ret != 0)
	{
	}

	/*
	 * Prepare to launch the thread to listen for inbound
	 * data requests.
	 */
	dctx = str_alloc(sizeof(LISTEN_CTX), 1, __FILE__, __LINE__);
	if (dctx == NULL)
	{
		(void) snprintf(ebuf, sizeof ebuf, "alloc(): %s\n", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}
	dctx->type = LISTEN_TYPE_DATA;
	/* a favicon only, no gif */
	dctx->gifimage = NULL;
	dctx->favicon  = favicon;
	dctx->gifimagelen = 0;
	dctx->faviconlen  = faviconlen;
	ary = config_lookup(CONF_DATA_PORT);
	if (ary == NULL)
		(void) strlcpy(dctx->port, "8100", sizeof dctx->port);
	else
		(void) strlcpy(dctx->port, ary[0], sizeof dctx->port);

	ary = config_lookup(CONF_DATA_INTERFACE);
	if (ary == NULL)
		(void) strlcpy(dctx->interface, "INADDR_ANY", sizeof dctx->interface);
	else
		(void) strlcpy(dctx->interface, ary[0], sizeof dctx->interface);

	dctx->tpool_ctx = tpool_ctx;

	ret = pthread_create(&data_tid, NULL, thread_listener, (void *)dctx);
	if (ret != 0)
	{
	}

	/*
	 * Prepare to launch the thread to listen for uploads of more.
	 */
	(void) verify_threads_locking_init();
	jctx = str_alloc(sizeof(LISTEN_CTX), 1, __FILE__, __LINE__);
	if (jctx == NULL)
	{
		(void) snprintf(ebuf, sizeof ebuf, "alloc(): %s\n", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}
	jctx->type = LISTEN_TYPE_UPLOAD;
	/* a favicon only, no gif */
	jctx->gifimage = NULL;
	jctx->favicon  = favicon;
	jctx->gifimagelen = 0;
	jctx->faviconlen  = faviconlen;
	ary = config_lookup(CONF_UPLOAD_PORT);
	if (ary == NULL)
		(void) strlcpy(jctx->port, "8101", sizeof jctx->port);
	else
		(void) strlcpy(jctx->port, ary[0], sizeof jctx->port);

	ary = config_lookup(CONF_UPLOAD_INTERFACE);
	if (ary == NULL)
		(void) strlcpy(jctx->interface, "INADDR_ANY", sizeof jctx->interface);
	else
		(void) strlcpy(jctx->interface, ary[0], sizeof jctx->interface);

	jctx->tpool_ctx = tpool_ctx;

	ret = pthread_create(&upload_tid, NULL, thread_listener, (void *)jctx);
	if (ret != 0)
	{
	}

	ary = config_lookup(CONF_PIDFILE);
	if (ary != NULL)
	{
		/*
		 * Note that write_pid_file() logs its own errors 
		 */
		if (write_pid_file(ary[0]) != 0)
			goto shutdown_server;
	}

	(void) strlcpy(ebuf, "Startup: version=", sizeof ebuf);
	(void) strlcat(ebuf, VERSION, sizeof ebuf);
	(void) strlcat(ebuf, ", gif_interface=", sizeof ebuf);
	(void) strlcat(ebuf, gctx->interface, sizeof ebuf);
	(void) strlcat(ebuf, ", gif_port=", sizeof ebuf);
	(void) strlcat(ebuf, gctx->port, sizeof ebuf);
	(void) strlcat(ebuf, ", data_interface=", sizeof ebuf);
	(void) strlcat(ebuf, dctx->interface, sizeof ebuf);
	(void) strlcat(ebuf, ", data_port=", sizeof ebuf);
	(void) strlcat(ebuf, dctx->port, sizeof ebuf);
	(void) strlcat(ebuf, ", threads=", sizeof ebuf);
	(void) str_ultoa(nthreads, nbuf, sizeof nbuf);
	(void) strlcat(ebuf, nbuf, sizeof ebuf);
	log_emit(LOG_INFO, NULL, __FILE__, __LINE__, ebuf);


	Global_Die = FALSE;

	(void) signal(SIGINT,  catch_sig);
	(void) signal(SIGQUIT, catch_sig);
	(void) signal(SIGKILL, catch_sig);
	(void) signal(SIGTERM, catch_sig);
	(void) signal(SIGPIPE, SIG_IGN);

	if (Global_Die == TRUE)
		goto shutdown_server;
	(void) file_prune_garbage(&Global_Die);


shutdown_server:
	Global_Die = TRUE;

	(void) pthread_join(data_tid, NULL);
	(void) pthread_join(gif_tid, NULL);
	(void) pthread_join(upload_tid, NULL);

	(void) verify_threads_locking_shutdown();

	if (tpool_ctx != NULL)
		tpool_ctx = tpool_shutdown(tpool_ctx, ebuf, sizeof ebuf);

	if (gctx != NULL)
		gctx = str_free(gctx, __FILE__, __LINE__);
	if (dctx != NULL)
		dctx = str_free(dctx, __FILE__, __LINE__);
	if (jctx != NULL)
		jctx = str_free(jctx, __FILE__, __LINE__);
	if (gifimage != NULL)
		gifimage = str_free(gifimage, __FILE__, __LINE__);
	if (favicon != NULL)
		favicon = str_free(favicon, __FILE__, __LINE__);

	ary = config_lookup(CONF_PIDFILE);
	if (ary != NULL)
		(void) unlink(ary[0]);

	io_shutdown();
	config_shutdown();

	(void) strlcpy(ebuf, "Shutdown: version=", sizeof ebuf);
	(void) strlcat(ebuf, VERSION, sizeof ebuf);
	log_emit(LOG_INFO, NULL, __FILE__, __LINE__, ebuf);
	/*
	 * This str_shutdown must always be last.
	 */
	str_shutdown();

	return 0;
}
