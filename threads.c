#include "onepixd.h"

extern int Global_Die;

void *
thread_listener(void *arg)
{
	int		sock;
	int		ret;
	int		cnt;
	int		tcount;
	int		xerror;
	char		ebuf[BUFSIZ];
	char		nbuf[64];
	struct pollfd	rds;
	THREAD_CTX_T *	ctx	= NULL;
	char **		ary	= NULL;
	struct sockaddr	sa;
	struct timeval	timo;
	unsigned short	portn;
	struct sockaddr_in	sin, *sinp;
	LISTEN_CTX *lctx = (LISTEN_CTX *)arg;
#ifdef SO_LINGER
	struct linger	ling;
#endif

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		(void) snprintf(ebuf, sizeof ebuf,
			"socket(): %s", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}

	ret = 1;
	(void) setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&ret, sizeof(int));

	(void) memset((char *) &sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	portn = atoi(lctx->port);
	sin.sin_port = htons(portn);

	if (strcmp(lctx->interface, "INADDR_ANY") == 0)
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		sin.sin_addr.s_addr = inet_addr(lctx->interface);
	if (sin.sin_addr.s_addr < 0)
	{
		(void) snprintf(ebuf, sizeof ebuf, "socket: %s\n", strerror(errno));
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}
	cnt = 0;
	while ((ret = bind(sock, (struct sockaddr *) &sin, sizeof(sin))) < 0)
	{
		(void) snprintf(ebuf, sizeof ebuf,
			"bind(%s): port=%s, %s, sleeping %d seconds\n",
			lctx->interface, lctx->port,
			strerror(errno), (cnt+1)*5);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);

		if (cnt++ > 5)
			goto shutdown_server;
		sleep(cnt * 5);
	}


	ret = listen(sock, 1024);
	if (ret < 0)
	{
		xerror = errno;
		(void) strlcpy(ebuf, __FILE__, sizeof ebuf);
		(void) strlcat(ebuf, "(", sizeof ebuf);
		(void) memset(nbuf, '\0', sizeof nbuf);
		(void) str_ultoa(__LINE__, nbuf, sizeof nbuf);
		(void) strlcat(ebuf, nbuf, sizeof ebuf);
		(void) strlcat(ebuf, "): listen(", sizeof ebuf);
		(void) strlcat(ebuf, lctx->interface, sizeof ebuf);
		(void) strlcat(ebuf, "): ", sizeof ebuf);
		(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
		log_emit(LOG_INFO, NULL, __FILE__, __LINE__, ebuf);
		goto shutdown_server;
	}

	tcount = 0;
	cnt = 0;

	/*
	 * The main loop.
	 */
	for(;;)
	{
		struct sockaddr addr;
		socklen_t sock_len;
		int n;
		int len;
		char *cp;

		if (Global_Die == TRUE)
			break;

		n = tpool_delta(lctx->tpool_ctx, 0);
		if (n == 0)
		{
			struct timespec  rqtp;

			if (Global_Die == TRUE)
				break;
			++tcount;
			if (lctx->type == LISTEN_TYPE_GIF)
				(void) strlcpy(ebuf, "Gif Server: ", sizeof ebuf);
			else
				(void) strlcpy(ebuf, "Data Server: ", sizeof ebuf);
			if (tcount > 60)
			{
				(void) strlcat(ebuf,
					"No threads for over a minute. Aborting.",
					sizeof ebuf);
				log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
				Global_Die = TRUE;
				break;
			}
			(void) strlcat(ebuf,
				"No threads available, sleeping 1 second.", sizeof ebuf);
			log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
			rqtp.tv_sec  = 1;
			rqtp.tv_nsec = 0;
			(void) nanosleep(&rqtp, NULL);
			if (Global_Die == TRUE)
				break;
			continue;
		}
		tcount = 0;
		if (Global_Die == TRUE)
			break;
		
		sock_len = sizeof(struct sockaddr);
		rds.fd = sock;
		rds.events = (POLLIN | POLLPRI);
		rds.revents = 0;
		/*
		 * Poll timeout is 3 seconds in millisecond notation
		 */
		ret = poll(&rds, 1, 3000);
		if (ret == 0)
		{
			/* timeout */
			if (Global_Die == TRUE)
				break;
			continue;
		}
		if (ret < 0)
		{
			xerror = errno;
			if (xerror == EINTR ||errno == 0)
				continue;
			if (++cnt > 10)
			{
				(void) strlcpy(ebuf, "Poll error: ", sizeof ebuf);
				(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
				log_emit(LOG_ERR, "0.0.0.0", __FILE__, __LINE__, ebuf);
				Global_Die = TRUE;
				break;
			}
			continue;
		}
		if ((rds.revents & (POLLIN | POLLPRI)) == 0)
		{
			xerror = errno;
			(void) strlcpy(ebuf, "Poll error: ", sizeof ebuf);
			(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
			log_emit(LOG_ERR, "0.0.0.0", __FILE__, __LINE__, ebuf);
			Global_Die = TRUE;
			break;
		}

		(void) memset((char *)&addr, '\0', sock_len);
		ctx = (THREAD_CTX_T *) str_alloc(1, sizeof(THREAD_CTX_T), __FILE__, __LINE__);
		if (ctx == NULL)
		{
			xerror = errno;
			(void) strlcpy(ebuf, "Memory error: ", sizeof ebuf);
			(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
			(void) strlcat(ebuf, ": Aborting", sizeof ebuf);
			log_emit(LOG_ERR, "0.0.0.0", __FILE__, __LINE__, ebuf);
			Global_Die = TRUE;
			break;
		}
		(void) memset((char *)ctx, '\0', sizeof(THREAD_CTX_T));
		ctx->sock = accept(sock, &addr, &sock_len);
		if (ctx->sock < 0)
		{
			if (Global_Die == TRUE)
				break;
			if (errno == EAGAIN)
			{
				struct timespec  rqtp;

				rqtp.tv_sec  = 1;
				rqtp.tv_nsec = 0;
				(void) nanosleep(&rqtp, NULL);
				if (Global_Die == TRUE)
					break;
				ctx = str_free(ctx, __FILE__, __LINE__);
				continue;
			}
			if (errno == ECONNABORTED ||
			    errno == EMFILE ||
			    errno == ENOMEM)
			{
				ctx = str_free(ctx, __FILE__, __LINE__);
			    	continue;
			}
			if (errno == EINTR)
			{
				Global_Die = TRUE;
				break;
			}
			(void) snprintf(ebuf, sizeof ebuf,
				"(%s,%d): listen(%s): %s",
				 __FILE__, __LINE__,
				lctx->interface, strerror(errno));
			log_emit(LOG_ERR, "0.0.0.0", __FILE__, __LINE__, ebuf);
			Global_Die = TRUE;
			break;
		}
		len = sizeof(struct sockaddr);
		ret = getpeername(ctx->sock, &sa, (socklen_t *)(&len));
		if (ret != 0)
		{
			xerror = errno;
			(void) close(ctx->sock);
			ctx = str_free(ctx, __FILE__, __LINE__);
			log_emit(LOG_WARNING, "0.0.0.0", __FILE__, __LINE__,
				"Could not get IP address of client. Reject.");
			continue;
		}
		sinp = (struct sockaddr_in *)(&sa);
		cp = inet_ntoa(sinp->sin_addr);
		if (cp != NULL)
			(void) strlcpy(ctx->ipaddr, cp, sizeof(ctx->ipaddr));
		else
			(void) strlcpy(ctx->ipaddr, "unknown", sizeof(ctx->ipaddr));
		if (Global_Die == TRUE)
			break;

		if (isdebug(BUG_APP))
		{
			(void) printf("DEBUG: %s(%d): Got connect from: %s\n",
				__FILE__, __LINE__, ctx->ipaddr);
		}

		(void) strlcpy(ctx->port, lctx->port, sizeof ctx->port);

		ctx->fp  = fdopen(ctx->sock, "r+");
		if (ctx->fp == NULL)
		{
			xerror = errno;
			(void) strlcpy(ebuf, "fdopen for read: ", sizeof ebuf);
			(void) strlcat(ebuf, strerror(xerror), sizeof ebuf);
			log_emit(LOG_ERR, ctx->ipaddr, __FILE__, __LINE__, ebuf);
			(void) close(ctx->sock);
			ctx = str_free(ctx, __FILE__, __LINE__);
			if (Global_Die == TRUE)
				break;
			continue;
		}

		cnt = 1;
		(void) setsockopt(ctx->sock, SOL_SOCKET, TCP_NODELAY,
				(char *)&cnt, sizeof(int));
#ifdef SO_LINGER
		ling.l_onoff=1;
		ling.l_linger=10;
		(void) setsockopt(ctx->sock, SOL_SOCKET, SO_LINGER,
				&ling, sizeof(struct linger));
#endif /* SO_LINGER */
#ifdef SO_RCVBUF
		cnt = 1500;
		(void) setsockopt(ctx->sock, SOL_SOCKET, SO_RCVBUF,
				(char *)&cnt, sizeof(cnt));
#endif /* SO_RCVBUF */
#ifdef SO_SNDBUF
		cnt = 1500;
		(void) setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF,
				(char *)&cnt, sizeof(cnt));
#endif /* SO_SNDBUF */

#ifdef SO_SNDTIMEO
		timo.tv_sec = lctx->timeout;
		timo.tv_usec = 0;
		(void) setsockopt(ctx->sock, SOL_SOCKET, SO_SNDTIMEO,
				&timo, sizeof(struct timeval));
#endif /* SO_SNDTIMEO */
#ifdef SO_RCVTIMEO
		timo.tv_sec = lctx->timeout;
		timo.tv_usec = 0;
		(void) setsockopt(ctx->sock, SOL_SOCKET, SO_RCVTIMEO,
				&timo, sizeof(struct timeval));
#endif /* SO_RCVTIMEO */

		ctx->gifimage = lctx->gifimage;
		ctx->gifimagelen = lctx->gifimagelen;
		ctx->favicon = lctx->favicon;
		ctx->faviconlen = lctx->faviconlen;

		if (lctx->type == LISTEN_TYPE_GIF)
		{
			ret = tpool_push_task(lctx->tpool_ctx,
				thread_handle_gif, NULL, ctx,
				ebuf, sizeof ebuf);
		}
		else if (lctx->type == LISTEN_TYPE_DATA)
		{
			ret = tpool_push_task(lctx->tpool_ctx,
				thread_handle_data, NULL, ctx,
				ebuf, sizeof ebuf);
		}
		else if (lctx->type == LISTEN_TYPE_UPLOAD)
		{
			ret = tpool_push_task(lctx->tpool_ctx,
				thread_handle_upload, NULL, ctx,
				ebuf, sizeof ebuf);
		}
		if (ret != 0)
		{
			/*
			 * We should have been able to run a thread here,
			 * but we cannot. Just log the error and
			 * keep running.
			 */
			if (ctx->fp != NULL)
			{
				(void) fclose(ctx->fp);
			}
			else
			{
				(void) close(ctx->sock);
			}
			ctx = str_free(ctx, __FILE__, __LINE__);
		}
		if (isdebug(BUG_APP))
		{
			(void) printf("DEBUG: %s(%d): Launched a thread.\n",
				__FILE__, __LINE__);
		}
		if (Global_Die == TRUE)
			break;
	}
shutdown_server:
	Global_Die = TRUE;

	if (ary != NULL)
	{
		(void) unlink(ary[0]);
	}

	if (sock >= 0)
		(void) close(sock);

	return NULL;
}

void *
thread_handle_gif(void *arg, int *state)
{
	THREAD_CTX_T *	ctx 		= arg;
	char		ebuf[1024];
	char		logbuf[BUFSIZ];
	char		query[1024];
	char		fname[MAXPATHLEN];
	char		datum[BUFSIZ];
	int		didclose	= FALSE;
	int		line		= 0;
	int		ret		= 0;
	int		i;
	int		xerror		= 0;
	HTTP_GOT_REPLY *gotp		= NULL;
	char **		ary		= NULL;
	HTTP_GOT_EQUATE *eqp		= NULL;
	time_t		 now;
	PATH_COLUMNS_T * p		= NULL;
	char		nbuf[64];

	(void) memset(ebuf,        '\0', sizeof ebuf);

	if (isdebug(BUG_THREADS))
	{
		(void) printf("DEBUG: %s(%d): Thread Handle Connection called\n",
			__FILE__, __LINE__);
	}

	/*
	 * Read the GET or POST
	 */
	(void) memset(query, '\0', sizeof query);
	gotp = io_getcommand(ctx, query, sizeof query, ebuf, sizeof ebuf, __FILE__, __LINE__);
	if (gotp == NULL)
	{
		xerror = errno;
		line = __LINE__;
		goto send404;
	}
	if (ctx->fp != NULL)
		(void) fflush(ctx->fp);

	/*
	 * Process the directory path following the GET.
	 * For example if the config file had 
	 *	gif_path_columns = foo/bar:email_id,email_tz
	 * And the path following the GET were either:
	 *	GET /foo/bar/?
	 *	GET /foo/bar?
	 * The query would be good. Other it fails.
	 */
	if (gotp->path[0] == '\0')
	{
		(void) strlcpy(ebuf, "No path in query.", sizeof ebuf);
		line = __LINE__;
		goto send404;
	}
	ary = config_lookup(CONF_GIF_PATH_COLUMNS);
	if (ary != NULL)
	{
		char **app;

		for (app = ary; *app != NULL; ++app)
		{
			p = str_parse_path_column(*app, __FILE__, __LINE__);
			if (p == NULL)
				continue;

			if (strcasecmp(gotp->path, p->path) == 0)
				break;

			p = str_free(p, __FILE__, __LINE__);
		}
		if (p == NULL)
		{
			(void) memset(ebuf, '\0', sizeof ebuf);
			(void) strlcat(ebuf, gotp->path, sizeof ebuf);
			(void) strlcat(ebuf, ": matched no configured path", sizeof ebuf);
			line = __LINE__;
			goto send404;
		}
	}

	/* 
	 * Following the path...? is a series of & delimited equates.
	 */
	if (gotp->equatep == NULL || gotp->nequates != p->columns.nary)
	{

		(void) strlcpy(ebuf, "Wrong number of columns in query, got ", sizeof ebuf);
		(void) str_ultoa(gotp->nequates, nbuf, sizeof nbuf);
		(void) strlcat(ebuf, nbuf, sizeof ebuf);
		(void) strlcat(ebuf, " but expected ", sizeof ebuf);
		(void) str_ultoa(p->columns.nary, nbuf, sizeof nbuf);
		(void) strlcat(ebuf, nbuf, sizeof ebuf);
		line = __LINE__;
		goto send404;
	}

	(void) memset(fname, '\0', sizeof fname);
	(void) memset(datum, '\0', sizeof datum);

	for (i = 0, eqp = gotp->equatep; i < p->columns.nary; ++eqp, ++i)
	{
		if (strcasecmp(eqp->name, p->columns.ary[i]) != 0)
			break;
	}
	if (i != p->columns.nary)
	{
		(void) strlcpy(ebuf, "Wrong columns in query, got ", sizeof ebuf);
		for (i = 0, eqp = gotp->equatep; i < gotp->nequates; ++eqp, ++i)
		{
			(void) strlcat(ebuf, eqp->name, sizeof ebuf);
			(void) strlcat(ebuf, ",", sizeof ebuf);
		}
		(void) strlcat(ebuf, " but expected ", sizeof ebuf);
		for (i = 0; i < p->columns.nary; ++i)
		{
			(void) strlcat(ebuf, p->columns.ary[i], sizeof ebuf);
			(void) strlcat(ebuf, ",", sizeof ebuf);
		}
		line = __LINE__;
		goto send404;
	}
	now = time(NULL);
	(void) str_ultoa(now, nbuf, sizeof nbuf);
	(void) strlcpy(datum, nbuf, sizeof datum);
	(void) strlcpy(fname, p->path, sizeof fname);
	(void) strlcat(datum, ",", sizeof datum);
	(void) strlcat(fname, ",", sizeof fname);

	for (i = 0, eqp = gotp->equatep; i < p->columns.nary; ++eqp, ++i)
	{
		(void) strlcat(fname, p->columns.ary[i], sizeof fname);
		(void) strlcat(datum, eqp->value, sizeof datum);
		if (i !=  1)
		{
			(void) strlcat(datum, ",", sizeof datum);
			(void) strlcat(fname, ",", sizeof fname);
		}
	}
	p = str_free(p, __FILE__, __LINE__);

	ret = file_write_datum(now, fname, datum, ebuf, sizeof ebuf);
	if (ret != 0)
	{
		line = __LINE__;
		goto send404;
	}
	line = 0;
send404:
	if (line != 0)
	{
		(void) memset(logbuf, '\0', sizeof logbuf);
		(void) strlcat(logbuf, "Error: query=\"", sizeof logbuf);
		(void) strlcat(logbuf, query, sizeof logbuf);
		(void) strlcat(logbuf, "\": error=", sizeof logbuf);
		(void) strlcat(logbuf, ebuf, sizeof logbuf);
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, line, logbuf);
	}

	ary = config_lookup(CONF_SEND_404_ON_ERROR);
	if (ary == NULL)
		ret = TRUE;
	else
		ret = truefalse(ary[0]);
	if (line != 0 && ret == TRUE)
	{
		(void) fprintf(ctx->fp, "HTTP/1.1 404 Not Found\r\n\r\n");
		(void) fflush(ctx->fp);
		goto closeandfinish;
	}

	(void) memset(ebuf, '\0', sizeof ebuf);
	ret = gif_send(ctx, "gif", ctx->gifimage, ctx->gifimagelen, ebuf, sizeof ebuf);
	if (ret != 0 && isdebug(BUG_THREADS))
	{
		(void) printf("DEBUG: %s(%d): gif_send: %s\n",
			 __FILE__, __LINE__, ebuf);
	}

closeandfinish:
	didclose = FALSE;

	if (gotp != NULL)
	{
		if (gotp->equatep != NULL)
			gotp->equatep = str_free(gotp->equatep, __FILE__, __LINE__);
		if (gotp->postp != NULL)
			gotp->postp = str_free(gotp->postp, __FILE__, __LINE__);
		gotp = str_free(gotp, __FILE__, __LINE__);
	}

	if (ctx->fp != NULL)
		(void) fflush(ctx->fp);
	if (ctx->fp != NULL)
	{
		(void) fflush(ctx->fp);
		(void) fclose(ctx->fp);
		ctx->fp = NULL;
		didclose = TRUE;
	}
	if (didclose == FALSE && ctx->sock > 0)
	{
		(void) close(ctx->sock);
		ctx->sock = -1;
	}
	ctx  = str_free(ctx,  __FILE__, __LINE__);
	return arg;
}

void *
thread_handle_data(void *arg, int *state)
{
	THREAD_CTX_T *	ctx 		= arg;
	int		didclose	= FALSE;
	HTTP_GOT_REPLY *gotp		= NULL;
	int		xerror		= 0;
	int		line		= 0;
	int		i;
	int		ret;
	char	 	ebuf[1024];
	char	 	query[1024];
	char		hout[BUFSIZ * 16];
	char		nbuf[MAX_ACCII_INT];
	char		sbuf[MAX_ACCII_INT];
	char **		ppp		= NULL;
	char **		ary		= NULL;
	char **		app		= NULL;
	QUERY_CTX	qctx;
	FORMS_T		form;
	char *		vp 		= NULL;
	char *		np 		= NULL;
	char *		error_string	= NULL;
	PATH_COLUMNS_T *pcp		= NULL;
	COL_RET_T *	colret		= NULL;
	time_t		wstart;
	time_t		wend;
	time_t		wcur		= 0;

	(void) memset(&form, '\0', sizeof form);
	(void) memset(ebuf,  '\0', sizeof ebuf);
	(void) memset(&qctx, '\0', sizeof(QUERY_CTX));


	if (isdebug(BUG_THREADS))
	{
		(void) printf("DEBUG: %s(%d): Thread Handle Data called\n",
			__FILE__, __LINE__);
	}

	/*
	 * Read the GET or POST
	 */
	(void) memset(query, '\0', sizeof query);
	gotp = io_getcommand(ctx, query, sizeof query, ebuf, sizeof ebuf, __FILE__, __LINE__);
	if (gotp == NULL)
	{
		xerror = errno;
		line = __LINE__;
		goto closeandfinish;
	}
	if (ctx->fp != NULL)
		(void) fflush(ctx->fp);

	if (strcasecmp(gotp->path, "favicon.ico") == 0)
	{
		if (ctx->favicon != NULL)
			(void) gif_send(ctx, "png", ctx->favicon, ctx->faviconlen, ebuf, sizeof ebuf);
		goto closeandfinish;
	}
	/*
	 * Fill out the list of known columns.
	 */
	(void) memset(&form, '\0', sizeof form);
	ary = config_lookup(CONF_GIF_PATH_COLUMNS);
	if (ary != NULL)
	{
		for (app = ary; *app != NULL; ++app)
		{
			if (form.have_cols.nary == MAX_COLUMNS)
				break;
			pcp = str_parse_path_column(*app, __FILE__, __LINE__);
			if (pcp != NULL)
			{
				for (i = 0; i < pcp->columns.nary; ++i)
				{
					(void) strlcpy(form.have_cols.ary[form.have_cols.nary], pcp->columns.ary[i], MAX_COLUMN_NAME_LEN);
					++form.have_cols.nary;
					if (form.have_cols.nary == MAX_COLUMNS)
						break;
				}
				pcp = str_free(pcp, __FILE__, __LINE__);
			}

		}
	}
	ary = config_lookup(CONF_UPLOAD_PATH_COLUMN);
	if (ary != NULL)
	{
		for (app = ary; *app != NULL; ++app)
		{
			if (form.have_cols.nary == MAX_COLUMNS)
				break;
			pcp = str_parse_path_column(*app, __FILE__, __LINE__);
			if (pcp != NULL)
			{
				for (i = 0; i < pcp->columns.nary; ++i)
				{
					(void) strlcpy(form.have_cols.ary[form.have_cols.nary], pcp->columns.ary[i], MAX_COLUMN_NAME_LEN);
					++form.have_cols.nary;
					if (form.have_cols.nary == MAX_COLUMNS)
						break;
				}
				pcp = str_free(pcp, __FILE__, __LINE__);
			}

		}
	}
	if (gotp->type == HTTP_GOT_REPLY_TYPE_GET)
	{
		error_string = NULL;
		/*
		 * An initial get clears the variables and
		 * requests a password to set a cookie.
		 */
resetforms:
		(void) memset(hout, '\0', sizeof hout);
		html_prefix_len("text/html", "200", 0, hout, sizeof hout);
		html_body(hout, sizeof hout);
		html_form_send(&form, hout, sizeof hout, error_string);
		html_endofbody(hout, sizeof hout);
		(void) fprintf(ctx->fp, "%s", hout);
		goto closeandfinish;
	}
	if (gotp->postp == NULL)
	{
		xerror = errno;
		line = __LINE__;
		goto closeandfinish;
	}

	for (ppp = gotp->postp; *ppp != NULL; ++ppp)
	{
		if (np != NULL)
			np = str_free(np, __FILE__, __LINE__);
		if (vp != NULL)
			vp = str_free(vp, __FILE__, __LINE__);
		ret = parse_equal(*ppp, &np, &vp);
		if (ret != 0)
			break;
		if (strcasecmp("pw", np) == 0)
		{
			(void) strlcpy(form.passwd, vp, sizeof form.passwd);
			continue;
		}
		if (strcasecmp("user", np) == 0)
		{
			(void) strlcpy(form.user, vp, sizeof form.user);
			continue;
		}
		/* startyear is 4 digit year */
		if (strcasecmp("startyear", np) == 0)
		{
			(void) strlcpy(form.start.year, vp, sizeof form.start.year);
			form.start.tm.tm_year = strtoul(vp, NULL, 10);
			form.start.tm.tm_year -= 1900;
			continue;
		}
		/* startmonth is 0 - 11 */
		if (strcasecmp("startmonth", np) == 0)
		{
			(void) strlcpy(form.start.mon, vp, sizeof form.start.mon);
			form.start.tm.tm_mon = strtoul(vp, NULL, 10);
			continue;
		}
		/* startday is 1 -  */
		if (strcasecmp("startday", np) == 0)
		{
			(void) strlcpy(form.start.day, vp, sizeof form.start.day);
			form.start.tm.tm_mday = strtoul(vp, NULL, 10);
			continue;
		}
		/* starthour is 0 - 23 */
		if (strcasecmp("starthour", np) == 0)
		{
			(void) strlcpy(form.start.hour, vp, sizeof form.start.hour);
			form.start.tm.tm_hour = strtoul(vp, NULL, 10);
			continue;
		}
		/* endyear is 4 digit year */
		if (strcasecmp("endyear", np) == 0)
		{
			(void) strlcpy(form.end.year, vp, sizeof form.end.year);
			form.end.tm.tm_year = strtoul(vp, NULL, 10);
			form.end.tm.tm_year -= 1900;
			continue;
		}
		/* endmonth is 0 - 11 */
		if (strcasecmp("endmonth", np) == 0)
		{
			(void) strlcpy(form.end.mon, vp, sizeof form.end.mon);
			form.end.tm.tm_mon = strtoul(vp, NULL, 10);
			continue;
		}
		/* endday is 1 -  */
		if (strcasecmp("endday", np) == 0)
		{
			(void) strlcpy(form.end.day, vp, sizeof form.end.day);
			form.end.tm.tm_mday = strtoul(vp, NULL, 10);
			continue;
		}
		/* endhour is 0 - 23 */
		if (strcasecmp("endhour", np) == 0)
		{
			(void) strlcpy(form.end.hour, vp, sizeof form.end.hour);
			form.end.tm.tm_hour = strtoul(vp, NULL, 10);
			continue;
		}
		/* windowsecs is 0 - 23 */
		if (strcasecmp("windowsecs", np) == 0)
		{
			form.window = strtoul(vp, NULL, 10);
			continue;
		}
		for (i = 0; i < form.have_cols.nary; i++)
		{
			(void) strlcpy(sbuf, "col", sizeof sbuf);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(sbuf, nbuf, sizeof sbuf);
			if (strcasecmp(sbuf, np) == 0)
			{
				if (strcasecmp(vp, "none") != 0)
				{
					(void) strlcpy(form.selected_cols.ary[form.selected_cols.nary], vp, MAX_GOT_NAMELEN); 
					++form.selected_cols.nary;
				}
			}
			(void) strlcpy(sbuf, "flag", sizeof sbuf);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(sbuf, nbuf, sizeof sbuf);
			if (strcasecmp(sbuf, np) == 0)
			{
				int j;

				j = html_flag_getindex(vp);
				if (j >= 0 && j < FORM_FLAG_LAST)
				{
					(void) strlcpy(form.flagstr[i], vp, 4);
					form.flag[i] = j;
				}
			}
			(void) strlcpy(sbuf, "match", sizeof sbuf);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(sbuf, nbuf, sizeof sbuf);
			if (strcasecmp(sbuf, np) == 0)
			{
				if (strlen(vp) > 0)
					(void)strlcpy(form.match[i], vp, MAX_GOT_NAMELEN);
			}
		}

	}
	if (np != NULL)
		np = str_free(np, __FILE__, __LINE__);
	if (vp != NULL)
		vp = str_free(vp, __FILE__, __LINE__);

	ary = config_lookup(CONF_DATA_PASSPHRASE);
	if (ary != NULL)
	{
		char *ep = NULL;
		char **app = NULL;

		for (app = ary; *app != NULL; ++app)
		{
			ep = strchr(*app, ':');
			if (ep == NULL)
				continue;
			*ep = '\0';
			if (strcmp(*app, form.user) == 0)
			{
				if (ep != NULL)
					*ep = ':';
				break;
			}
			if (ep != NULL)
				*ep = ':';
		}
		if (ep == NULL || *app == NULL)
			goto passwordfailed;
		++ep;
		if (strcmp(form.passwd, ep) != 0)
		{
passwordfailed:
			(void) strlcpy(ebuf, ctx->ipaddr, sizeof ebuf);
			(void) strlcat(ebuf, ": ERROR: Authorization Failed. Warning abuse of this site may be a crime in certain jurisdictions.", sizeof ebuf);
			error_string = ebuf;
			goto resetforms;
		}
	}

	/* 
	 * Adjust the times here so that the data selected display window
	 * is smaller than the slice window.
	 * TODO: Instead of just making the start smaller, perhaps make the
	 * start and end grow equally out from the middle in half steps?
	 */
	form.start.tm.tm_isdst = -1;
	form.start.secs = mktime(&(form.start.tm));
	form.end.tm.tm_isdst = -1;
	form.end.secs   = mktime(&(form.end.tm));

	while (form.end.secs - form.start.secs < form.window)
	{
		int y;

		form.start.secs -= form.window;
		(void) localtime_r(&(form.start.secs), &(form.start.tm));
		form.start.tm.tm_isdst = -1;
		(void) str_ultoa(form.start.tm.tm_hour, nbuf, sizeof nbuf);
		(void) strlcpy(form.start.hour, nbuf, sizeof form.start.hour);
		(void) str_ultoa(form.start.tm.tm_mday, nbuf, sizeof nbuf);
		(void) strlcpy(form.start.day, nbuf, sizeof form.start.day);
		(void) str_ultoa(form.start.tm.tm_mon, nbuf, sizeof nbuf);
		(void) strlcpy(form.start.mon, nbuf, sizeof form.start.mon);
		y = form.start.tm.tm_year + 1900;
		(void) str_ultoa(y, nbuf, sizeof nbuf);
		(void) strlcpy(form.start.year, nbuf, sizeof form.start.year);
	}

	/*
	 * Redraw the screen.
	 * and show the requested data here 
	 */
	(void) memset(hout, '\0', sizeof hout);
	html_prefix_len("text/html", "200", 0, hout, sizeof hout);
	html_body(hout, sizeof hout);
	html_form_send(&form, hout, sizeof hout, error_string);
	(void) fprintf(ctx->fp, "%s", hout);

	if (form.selected_cols.nary == 0)
	{
		(void)fprintf(ctx->fp, "<p><font color=\"red\">ERROR: No columns were selected.</font>\n");
		goto skiptofinish;
	}

	colret = file_fetch_col_files(&form);
	if (colret == NULL)
		(void) fprintf(ctx->fp, "<p> no data found.\n");

	(void) memset(hout, '\0', sizeof hout);
	(void) strlcat(hout, "<p><table align=\"center\" border=\"1\" bgcolor=\"white\">\n", sizeof hout);
	(void) strlcat(hout, "<tr>\n", sizeof hout);
	(void) strlcat(hout, "<td>Window Start Time</td>\n", sizeof hout);
	for (i = 0; i < form.selected_cols.nary; i++)
	{
		(void) strlcat(hout, "<td>", sizeof hout);
		(void) strlcat(hout, "[", sizeof hout);
		(void) str_ultoa(i, nbuf, sizeof nbuf);
		(void) strlcat(hout, nbuf, sizeof hout);
		(void) strlcat(hout, "] ", sizeof hout);
		(void) strlcat(hout, form.selected_cols.ary[i], sizeof hout);
		(void) strlcat(hout, "</td>\n", sizeof hout);
	}
	(void) strlcat(hout, "</tr>\n", sizeof hout);

	for (wstart = form.start.secs; wstart < form.end.secs; wstart += form.window) 
	{
		char *row_time;
		int   total[MAX_COLUMNS];

		wend = wstart + form.window;
		(void) strlcat(hout, "<tr>\n", sizeof hout);
		(void) strlcat(hout, "<td>\n", sizeof hout);
		row_time = html_row_time(wstart);
		(void) strlcat(hout, row_time, sizeof hout);
		row_time = str_free(row_time, __FILE__, __LINE__);
		(void) strlcat(hout, "\n</td>\n", sizeof hout);
		for (i = 0; i < form.selected_cols.nary; i++)
		{
			char **tt;
			char **vv;
			char **ti;
			char **vi;

			(void) strlcat(hout, "<td align=\"right\">\n", sizeof hout);
			if (form.flag[i] == FORM_FLAG_TOT)
			{

				total[i] = 0;
				if ((tt = colret->d[i].times) == NULL ||
					(vv = colret->d[i].values) == NULL)
				{
					(void) strlcat(hout, "0\n", sizeof hout);
				}
				else
				{
					for (tt = colret->d[i].times, vv = colret->d[i].values;
						*tt != NULL && *vv != NULL; ++tt, ++vv)
					{
						wcur = strtoul(*tt, NULL, 10);
						if (wcur < wstart || wcur > wend)
							continue;
						if (strcmp(form.match[i], "nomatch") != 0)
						{
							if (strcasecmp(form.match[i], *vv) != 0)
								continue;
						}
						++total[i];
					}
					(void) str_ultoa(total[i], nbuf, sizeof nbuf);
					(void) strlcat(hout, nbuf, sizeof hout);
					(void) strlcat(hout, "\n", sizeof hout);
				}
			}
			if (form.flag[i] != FORM_FLAG_TOT)
			{
				int flag;
				int percent;

				flag = form.flag[i] - 1;
				if (flag < 0)
					flag = 0;
				if (flag >= MAX_COLUMNS)
					flag = MAX_COLUMNS;
				if (flag > 15)
				{
					percent = TRUE;
					flag -= 16;
				}
				else
					percent = FALSE;
				total[i] = 0;
				if ((tt = colret->d[flag].times) == NULL ||
					(vv = colret->d[flag].values) == NULL)
				{
					(void) strlcat(hout, "0\n", sizeof hout);
				}
				else if ((ti = colret->d[i].times) == NULL ||
					(vi = colret->d[i].values) == NULL)
				{
					(void) strlcat(hout, "0\n", sizeof hout);
				}
				else
				{
					for (tt = colret->d[flag].times, vv = colret->d[flag].values;
						*tt != NULL && *vv != NULL; ++tt, ++vv)
					{
						for (ti = colret->d[i].times, vi = colret->d[i].values;
							*ti != NULL && *vi != NULL; ++ti, ++vi)
						{
							wcur = strtoul(*tt, NULL, 10);
							if (wcur < wstart || wcur > wend)
								continue;
							if (strcmp(*vi, *vv) != 0)
								continue;
							if (strcmp(form.match[i], "nomatch") != 0)
							{
								if (strcasecmp(form.match[i], *vv) != 0)
									continue;
							}
							++total[i];
						}
					}
					if (percent == TRUE)
					{
						if (total[flag] == 0)
						{
							(void) strlcat(hout, "0", sizeof hout);
						}
						else
						{
							percent = (100 * total[i]) / total[flag];
							if (percent > 100)
								percent = 100;
							(void) str_ultoa(percent, nbuf, sizeof nbuf);
							(void) strlcat(hout, nbuf, sizeof hout);
						}
						(void) strlcat(hout, "%", sizeof hout);
					}
					else
					{
						(void) str_ultoa(total[i], nbuf, sizeof nbuf);
						(void) strlcat(hout, nbuf, sizeof hout);
					}
					(void) strlcat(hout, "\n", sizeof hout);
				}
			}
			
			(void) strlcat(hout, "</td>\n", sizeof hout);
		}
		(void) strlcat(hout, "</tr>\n", sizeof hout);
	}
	(void) strlcat(hout, "</table>\n", sizeof hout);
	(void) fprintf(ctx->fp, "%s", hout);
	colret = file_colret_free(colret);


	/*
	 * Finish up the html
	 */
skiptofinish:
	(void) memset(hout, '\0', sizeof hout);
	html_endofbody(hout, sizeof hout);
	(void) fprintf(ctx->fp, "%s", hout);

closeandfinish:
	if (gotp != NULL)
	{
		if (gotp->equatep != NULL)
			gotp->equatep = str_free(gotp->equatep, __FILE__, __LINE__);
		if (gotp->postp != NULL)
			gotp->postp = clearargv(gotp->postp);
		gotp = str_free(gotp, __FILE__, __LINE__);
	}
	didclose = FALSE;
	if (ctx->fp != NULL)
		(void) fflush(ctx->fp);
	if (ctx->fp != NULL)
	{
		(void) fflush(ctx->fp);
		(void) fclose(ctx->fp);
		ctx->fp = NULL;
		didclose = TRUE;
	}
	if (didclose == FALSE && ctx->sock > 0)
	{
		(void) close(ctx->sock);
		ctx->sock = -1;
	}
	ctx  = str_free(ctx,  __FILE__, __LINE__);
	return arg;
}

/******************************************************
** More Interface
**
** Upload additional data using a POST or a GET. The data lines
** will appear in the order and form:
**
**	epoch_hour = YYYY/MM/DD/HH
**	rows = number of data rows
**	columns = name,name,...
**	data follows in form of
**	value,value,...
**	value,value,...
**	etc.
**
******************************************************/
void *
thread_handle_upload(void *arg, int *state)
{
	THREAD_CTX_T *	ctx 		= arg;
	int		didclose	= FALSE;
	HTTP_GOT_REPLY *gotp		= NULL;
	char **		ary;
	char **		gpp		= NULL;
	char **		tpp		= NULL;
	int		i;
	char		*error_404	= NULL;
	time_t		now;
	int		line 		= 0;
	char	 	query[MAX_HTTP_QUERY];
	char	 	ebuf[MAX_EBUF_LEN];
	char		 fname[MAXPATHLEN];
	PATH_COLUMNS_T * p		= NULL;
	char		nbuf[64];

	QUERY_CTX	qctx;

	(void) memset(ebuf,  '\0', sizeof ebuf);
	(void) memset(&qctx, '\0', sizeof(QUERY_CTX));

	if (isdebug(BUG_THREADS))
	{
		(void) printf("DEBUG: %s(%d): Thread Handle Data called\n",
			__FILE__, __LINE__);
	}

	/*
	 * Read the GET or POST
	 */
	(void) memset(query, '\0', sizeof query);
	gotp = io_getcommand(ctx, query, sizeof query, ebuf, sizeof ebuf, __FILE__, __LINE__);
	if (gotp == NULL)
	{
		error_404 = "Empty Data Received";
		line = __LINE__;
		goto closeandfinish;
	}
	if (ctx->fp != NULL)
		(void) fflush(ctx->fp);

	if (*(gotp->path) == '\0')
	{
		error_404 = "No Path In Query";
		line = __LINE__;
		goto closeandfinish;
	}
	if (strcasecmp(gotp->path, "favicon.ico") == 0)
	{
		int   ret;

		if (ctx->favicon != NULL)
		{
			ret = gif_send(ctx, "png", ctx->favicon, ctx->faviconlen, ebuf, sizeof ebuf);
			if (ret != 0 && isdebug(BUG_THREADS))
			{
				(void) printf("DEBUG: %s(%d): gif_send favicon.ico: %s\n",
					 __FILE__, __LINE__, ebuf);
			}
		}
		goto closeandfinish;
	}
	/*
	 * If a securty path is required, check it.
	 */
	ary = config_lookup(CONF_UPLOAD_PATH_COLUMN);
	if (ary != NULL)
	{
		char **app;

		for (app = ary; *app != NULL; ++app)
		{
			p = str_parse_path_column(*app, __FILE__, __LINE__);
			if (p == NULL)
				continue;

			if (strcasecmp(gotp->path, p->path) == 0)
				break;

			p = str_free(p, __FILE__, __LINE__);
		}
		if (p == NULL)
		{
			(void) memset(ebuf, '\0', sizeof ebuf);
			(void) strlcat(ebuf, gotp->path, sizeof ebuf);
			(void) strlcat(ebuf, ": matched no configured path", sizeof ebuf);
			line = __LINE__;
			goto closeandfinish;
		}
	}
	if (gotp->type == HTTP_GOT_REPLY_TYPE_GET)
	{
		char		 datum[BUFSIZ];
		HTTP_GOT_EQUATE *eqp		= NULL;
		int		 ret		= 0;
		char		sig[BUFSIZ];

		(void) memset(fname, '\0', sizeof fname);
		(void) memset(datum, '\0', sizeof datum);
		(void) memset(sig,   '\0', sizeof sig);

		if (gotp->equatep == NULL)
		{
			error_404 = "Got GET, but no equates";
			line = __LINE__;
			goto closeandfinish;
		}

		/*
		 * Snarf the signature if there is one.
		 */
		for (i = 0, eqp = gotp->equatep; i < gotp->nequates; ++eqp, ++i)
		{
			if (strcasecmp(eqp->name, "sig") != 0)
				continue;
			(void) strlcpy(sig, eqp->value, sizeof sig);
			gotp->nequates -= 1;
			break;
		}
		/* 
		 * Following the path...? is a series of & delimited equates.
		 */
		if (gotp->equatep == NULL || gotp->nequates != p->columns.nary)
		{

			error_404 = "Column Mismatch";
			(void) strlcpy(ebuf, "Wrong number of columns in query, got ", sizeof ebuf);
			(void) str_ultoa(gotp->nequates, nbuf, sizeof nbuf);
			(void) strlcat(ebuf, nbuf, sizeof ebuf);
			(void) strlcat(ebuf, " but expected ", sizeof ebuf);
			(void) str_ultoa(p->columns.nary, nbuf, sizeof nbuf);
			(void) strlcat(ebuf, nbuf, sizeof ebuf);
			line = __LINE__;
			goto closeandfinish;
		}

		(void) memset(fname, '\0', sizeof fname);
		(void) memset(datum, '\0', sizeof datum);

		for (i = 0, eqp = gotp->equatep; i < p->columns.nary; ++eqp, ++i)
		{
			if (strcasecmp(eqp->name, p->columns.ary[i]) != 0)
				break;
		}
		if (i != p->columns.nary)
		{
			error_404 = "Bad Column Name";
			(void) strlcpy(ebuf, "Wrong columns in query, got ", sizeof ebuf);
			for (i = 0, eqp = gotp->equatep; i < gotp->nequates; ++eqp, ++i)
			{
				(void) strlcat(ebuf, eqp->name, sizeof ebuf);
				(void) strlcat(ebuf, ",", sizeof ebuf);
			}
			(void) strlcat(ebuf, " but expected ", sizeof ebuf);
			for (i = 0; i < p->columns.nary; ++i)
			{
				(void) strlcat(ebuf, p->columns.ary[i], sizeof ebuf);
				(void) strlcat(ebuf, ",", sizeof ebuf);
			}
			line = __LINE__;
			goto closeandfinish;
		}
		now = time(NULL);
		(void) str_ultoa(now, nbuf, sizeof nbuf);
		(void) strlcpy(datum, nbuf, sizeof datum);
		(void) strlcpy(fname, p->path, sizeof fname);
		(void) strlcat(datum, ",", sizeof datum);
		(void) strlcat(fname, ",", sizeof fname);

		for (i = 0, eqp = gotp->equatep; i < p->columns.nary; ++eqp, ++i)
		{
			(void) strlcat(fname, p->columns.ary[i], sizeof fname);
			(void) strlcat(datum, eqp->value, sizeof datum);
			if (i != (p->columns.nary - 1))
			{
				(void) strlcat(datum, ",", sizeof datum);
				(void) strlcat(fname, ",", sizeof fname);
			}
		}

		if (strlen(sig) > 0)
		{
			char **datap;

			datap = pushargv(datum, NULL, __FILE__, __LINE__);
			if (verify_sig((u_char *)sig, strlen(sig), datap) != 0)
			{
				datap = clearargv(datap);
				error_404 = "Upload signature did not verify.";
				line = __LINE__;
				goto closeandfinish;
			}
			datap = clearargv(datap);
		}

		ret = file_write_datum(now, fname, datum, ebuf, sizeof ebuf);
		if (ret != 0)
		{
			line = __LINE__;
			goto closeandfinish;
		}
		line = 0;
	}
	else /* gotp->type == HTTP_GOT_REPLY_TYPE_POST */
	{
		char *		vp = NULL;
		char *		np = NULL;
		int		row_count;
		char		cols[MAX_COLUMN_NAME_LEN * MAX_COLUMNS];
		char		sig[BUFSIZ];

		if (gotp->postp == NULL)
		{
			error_404 = "Empty Data Received";
			line = __LINE__;
			goto closeandfinish;
		}

		row_count	 = 0;
		np	  	 = NULL;
		vp	  	 = NULL;
		(void) memset(cols, '\0', sizeof cols);
		(void) memset(sig, '\0', sizeof sig);
		(void) memset(fname, '\0', sizeof fname);

		for (gpp = gotp->postp; *gpp != NULL; ++gpp)
		{
			int ret;

			if (np != NULL)
				np = str_free(np, __FILE__, __LINE__);
			if (vp != NULL)
				vp = str_free(vp, __FILE__, __LINE__);
			ret = parse_equal(*gpp, &np, &vp);
			if (ret != 0)
				break;

			if (strcmp(np, CONF_UPLOAD_POST_COLS) == 0)
				(void) strlcpy(cols, vp, sizeof cols);
			else if (strcmp(np, CONF_UPLOAD_POST_ROWS) == 0)
				row_count = strtoul(vp, NULL, 10);
			else if (strcmp(np, CONF_UPLOAD_POST_SIG) == 0)
				(void) strlcpy(sig, vp, sizeof sig);
			else if (strcmp(np, CONF_UPLOAD_POST_DATUM) == 0)
				break;
		}
		if (np != NULL)
			np = str_free(np, __FILE__, __LINE__);
		if (vp != NULL)
			vp = str_free(vp, __FILE__, __LINE__);

		if (strlen(sig) > 0)
		{
			if (verify_sig((u_char *)sig, strlen(sig), gpp) != 0)
			{
				error_404 = "Upload signature did not verify.";
				line = __LINE__;
				goto closeandfinish;
			}
		}
		tpp = gpp;
		for (i = 0; *tpp != NULL; ++tpp)
		{
			if (strncasecmp(*tpp, CONF_UPLOAD_POST_DATUM, strlen(CONF_UPLOAD_POST_DATUM)) != 0)
				continue;
			if (strchr(*tpp, '=') == NULL)
				continue;
			i++;
		}
		if (i != row_count)
		{
			error_404 = "Rows specified did not match row count";
			line = __LINE__;
			goto closeandfinish;
		}
		(void)strlcpy(fname, p->path, sizeof fname);
		(void) strlcat(fname, ",", sizeof fname);
		for (i = 0; i < p->columns.nary; ++i)
		{
			(void) strlcat(fname, p->columns.ary[i], sizeof fname);
			if (i != (p->columns.nary - 1))
				(void) strlcat(fname, ",", sizeof fname);
		}
		/*
		 * file_write_array logs its own errors.
		 */
		if (gotp != NULL && gpp != NULL)
		{
			(void) file_write_array(gpp, fname);
		}
		goto closeandfinish;
	}

closeandfinish:
	if (error_404 == NULL)
	{
		(void) fprintf(ctx->fp, "HTTP/1.1 200 Success\r\n\r\n");
		(void) fflush(ctx->fp);
	}
	else
	{
		(void) fprintf(ctx->fp, "HTTP/1.1 404 %s\r\n\r\n", error_404);
		(void) fflush(ctx->fp);
	}
	if (line != 0)
	{
		char logbuf[BUFSIZ];

		*logbuf = '\0';
		if (error_404 != NULL)
		{
			(void) strlcat(logbuf, "error=", sizeof logbuf);
			(void) strlcat(logbuf, error_404, sizeof logbuf);
		}
		(void) strlcat(logbuf, ", query=", sizeof logbuf);
		(void) strlcat(logbuf, query, sizeof logbuf);
		if (*ebuf != '\0')
		{
			(void) strlcat(logbuf, ", ebuf=", sizeof logbuf);
			(void) strlcat(logbuf, ebuf, sizeof logbuf);
		}
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, line, logbuf);
	}

	if (p != NULL)
		p = str_free(p, __FILE__, __LINE__);
	if (gotp != NULL)
	{
		if (gotp->equatep != NULL)
			gotp->equatep = str_free(gotp->equatep, __FILE__, __LINE__);
		if (gotp->postp != NULL)
			gotp->postp = str_free(gotp->postp, __FILE__, __LINE__);
		gotp = str_free(gotp, __FILE__, __LINE__);
	}
	didclose = FALSE;
	if (ctx->fp != NULL)
		(void) fflush(ctx->fp);
	if (ctx->fp != NULL)
	{
		(void) fflush(ctx->fp);
		(void) fclose(ctx->fp);
		ctx->fp = NULL;
		didclose = TRUE;
	}
	if (didclose == FALSE && ctx->sock > 0)
	{
		(void) close(ctx->sock);
		ctx->sock = -1;
	}
	ctx  = str_free(ctx,  __FILE__, __LINE__);
	return arg;
}

