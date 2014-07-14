#include "onepixd.h"

#if BUFSIZ >= 2048
# define IO_BUFSIZ BUFSIZ
#else
# define IO_BUFSIZ (2048)
#endif
#define IO_MAXSIZ (IO_BUFSIZ * 8)

/*
 * File descriptor locking needed by threads.
 */
static int NumFds = 0;
static pthread_mutex_t *SFD_mutex;
void
io_init(int numfds)
{
	int i;

	if (isdebug(BUG_IO))
	{
		(void) printf("DEBUG: %s(%d): Maximum File Descriptors = %d\n",
				__FILE__, __LINE__, numfds);
	}
	SFD_mutex = str_alloc(numfds, sizeof(pthread_mutex_t), __FILE__, __LINE__);
	if (SFD_mutex == NULL)
		return;
	for (i = 0; i < numfds; ++i)
		(void) pthread_mutex_init(&(SFD_mutex[i]), NULL);
	NumFds = numfds;
	return;
}

static void
io_lock_fd(int fd)
{
	if (SFD_mutex == NULL || NumFds == 0)
		return;
	if (fd < 0)
		return;
	if (fd >= NumFds)
		fd = 0;
	(void) pthread_mutex_lock(&(SFD_mutex[fd]));
	return;
}
static void
io_unlock_fd(int fd)
{
	if (SFD_mutex == NULL || NumFds == 0)
		return;
	if (fd < 0)
		return;
	if (fd >= NumFds)
		fd = 0;

	(void) pthread_mutex_unlock(&(SFD_mutex[fd]));
	return;
}

void
io_shutdown(void)
{
	int i;

	if (SFD_mutex == NULL || NumFds == 0)
		return;
	for (i = 0; i < NumFds; ++i)
		(void) pthread_mutex_destroy(&(SFD_mutex[i]));		
	SFD_mutex = str_free(SFD_mutex, __FILE__, __LINE__);
}

/**************************************************************
**  IO_BARE_FWRITE -- Send a string to the client or server.
**      Parameters:     
**		fd	  -- socket
**              msg       -- message to send
**      Returns:
**		Number bytes written on success
**		-1 on error.
**      Side Effects:
**		Performs a network write.
**      Notes:
**		None
*/
int
io_bare_fwrite(THREAD_CTX_T *ctx, char *msg, char *ebuf, size_t elen)
{
	int ret, len;

	errno = EINVAL;

	if (msg == NULL)
		return -1;

	if (ctx == NULL)
		return -1;

	errno = 0;
	len = strlen(msg);
	if (len == 0)
		return 0;

	ret = fwrite(msg, 1, len, ctx->fp);
	if (ret < 0)
	{
		int xerror;

		xerror = errno;
		(void) strlcpy(ebuf, "Write: ", elen);
		(void) strlcat(ebuf, msg, elen);
		(void) strlcat(ebuf, ": ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return -1;
	}
	return ret;
}

/* 
**  IO_FGETS -- Read an arbitrarily long line.
**      Parameters:     
**		ctx	-- The context
**		size	-- Address of int to store the size read
**      Returns:
**		The line on success
**		NULL on error and sets errno.
**      Side Effects:
**		Performs a network read.
**		Allocates memory.
**      Notes:
*/
char *
io_fgets(THREAD_CTX_T *ctx, int *size, int limit, char *file, size_t line)
{
	char *cp;
	char *buf, *tmp;
	int len;
	int newlen;
	int ch, cnt;
	int fd;
	int xerror;
	
	if (ctx == NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	fd = ctx->sock;
	if (fd < 0)
	{
		errno = EINVAL;
		return NULL;
	}
	if (ctx->fp == NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	if (size != NULL)
		*size = 0;

	len = IO_BUFSIZ;
	if ((buf = str_alloc(1, len + 3, file, line)) == NULL)
	{
		return NULL;
	}

	cp = buf;
	ch = cnt = 0;
	io_lock_fd(ctx->sock);
	while (ch != '\n')
	{
		ch = fgetc(ctx->fp);
		if (ch == EOF)
		{
			/* empty read or EOF */
			xerror = errno;
			buf = str_free(buf, file, line);
			io_unlock_fd(ctx->sock);
			if (size != NULL)
				*size = -1;
			errno = xerror;
			return NULL;
		}
		if (ch == '\n')
			break;
		if (ch == '\r')
			continue;
		if (cnt >= len)
		{
			if (len >= IO_MAXSIZ)
				break;
			newlen = len + IO_BUFSIZ;
			if ((tmp = str_realloc(buf, len, newlen+3, file, line, TRUE)) == NULL)
			{
				xerror = errno;
				io_unlock_fd(ctx->sock);
				errno = xerror;
				return NULL;
			}
			len = newlen;
			buf = tmp;
			cp = buf + cnt;
		}
		if (cnt < len)
		{
			*cp++ = ch;
			++cnt;
			if (limit > 0 && cnt >= limit)
			{
				break;
			}
		}
	}
	io_unlock_fd(ctx->sock);

	*cp = '\0';
	if (size != NULL)
		*size = cp - buf;

	if (isdebug(BUG_IO))
	{
		(void) printf("DEBUG: %s(%d): io_fgets() got: %s\n",
			__FILE__, __LINE__, buf);
	}
	errno = 0;
	return buf;
}


#define  NOT_ENCODED	(0)
#define   QP_ENCODED	(1)
#define  B64_ENCODED	(2)

/* 
**  IO_GETCOMMAND -- Get a request for a one pixel gif.
**      Parameters:     
**		ctx	-- the context
**		query	-- to log query
**		qlen	-- to log query
**		ebuf	-- for error return
**		elen	-- for error return
**      Returns:
**		HTTP_GOT_REPLY * on success.
**		NULL on error.
**      Side Effects:
**		Performs a network read.
*/
HTTP_GOT_REPLY *
io_getcommand(THREAD_CTX_T *ctx, char *query, size_t qlen, char *ebuf, size_t elen, char *file, size_t line)
{
	char		 *cp;
	char		 *ep;
	char		 *bp;
	char		 *qp;
	char		 *xp;
	HTTP_GOT_REPLY	 *retp = NULL;
	int		  cnt;
	int		  xerror;
	int		  do_post = FALSE;
	int		  got_lf = FALSE;
	int		  content_length = -1;
	char		  logbuf[BUFSIZ];

	if (ctx == NULL)
	{
		xerror = EINVAL;
		strlcpy(ebuf, "io_getcommand: Called with NULL context: ", elen);
		strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return NULL;
	}
	if (ebuf == NULL)
	{
		ebuf = logbuf;
		elen = sizeof logbuf;
	}

	cp = io_fgets(ctx, &cnt, 0, file, line);
	if (cp == NULL)
	{
		xerror = errno;
		strlcpy(ebuf, "io_getcommand: Connection dropped: ", elen);
		strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return NULL;
	}

	if (cnt == 0)
	{
		xerror = (errno == 0) ? EINVAL : errno;
		cp = str_free(cp, file, line);
		strlcpy(ebuf, "io_getcommand: Connection dropped: ", elen);
		strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return NULL;
	}

	if (query != NULL && qlen > 0)
	{
		(void) strlcpy(query, cp, qlen);
	}

	if (strncasecmp(cp, "GET", 3) != 0 && strncasecmp(cp, "POST", 4) != 0)
	{
		xerror = EINVAL;
		(void) strlcpy(ebuf, "Client ", elen);
		(void) strlcat(ebuf, cp, elen);
		(void) strlcat(ebuf, ": Unsupported HTTP Request", elen);
		errno = xerror;
		return NULL;
	}
	if (strncasecmp(cp, "POST", 4) == 0)
		do_post = TRUE;

	if (retp == NULL)
		retp = str_alloc(sizeof(HTTP_GOT_REPLY), 1,
			__FILE__, __LINE__);
	if (retp == NULL)
	{
		xerror = (errno == 0) ? ENOMEM : errno;
		cp = str_free(cp, file, line);
		strlcpy(ebuf, "io_getcommand: Allocate return memory:", elen);
		strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return NULL;
	}
	if (do_post == TRUE)
		retp->type = HTTP_GOT_REPLY_TYPE_POST;
	else
		retp->type = HTTP_GOT_REPLY_TYPE_GET;

	if (do_post == TRUE)
		bp = cp + 5;
	else
		bp = cp + 4;
	ep = strrchr(bp, ' ');
	if (ep != NULL)
	{
		if (strncasecmp(ep, " HTTP", 5) == 0)
			*(ep) = '\0';
	}
	(void) html_unpercenthex(bp, strlen(bp));
	xp = str_squeeze(bp, FALSE, file, line);
	if (xp == NULL)
	{
		xerror = (errno == 0) ? ENOMEM : errno;
		strlcpy(ebuf, "io_getcommand: Allocate return memory:", elen);
		strlcat(ebuf, strerror(xerror), elen);
		errno = xerror;
		return NULL;
	}
	qp = strrchr(xp, '?');
	if (qp != NULL)
	{
		*qp = '\0';
	}

	/*
	** Remove all leading slashes.
	** Remover an optional trailing slash
	** This can legally result in an empty (zero length) path.
	*/
	ep = xp;
	while (*ep == '/' && *ep != '\0')
		++ep;
	if (qp != NULL)
	{
		bp = qp -1;
		if (*bp == '/')
			*bp = '\0';
	}
	(void) strlcpy(retp->path, ep, sizeof retp->path);
	cp = str_free(cp, file, line);

	if (qp != NULL)
	{
		char *np = NULL;
		char *vp = NULL;
		int   ret;

		bp = qp + 1;
		do
		{
			HTTP_GOT_EQUATE *eqp;

			ep = strchr(bp, '&');
			if (ep != NULL)
				*ep = '\0';
			if (np != NULL)
				np = str_free(np, file, line);
			if (vp != NULL)
				vp = str_free(vp, file, line);
			ret = parse_equal(bp, &np, &vp);
			if (ret != 0)
				break;
			if (retp->equatep == NULL)
			{
				retp->equatep = str_alloc(sizeof(HTTP_GOT_EQUATE), 1, file, line);
				retp->nequates = 1;
			}
			else
			{
				retp->equatep = str_realloc(retp->equatep,
					sizeof(HTTP_GOT_EQUATE) * retp->nequates,
					sizeof(HTTP_GOT_EQUATE) * (retp->nequates + 1),
					file, line, TRUE);
				retp->nequates += 1;
			}
			if (retp->equatep == NULL)
			{
				xerror = (errno == 0) ? ENOMEM : errno;
				cp = str_free(cp, file, line);
				retp = str_free(retp, file, line);
				strlcpy(ebuf, "io_getcommand: Allocate return memory:", elen);
				strlcat(ebuf, strerror(xerror), elen);
				errno = xerror;
				return NULL;
			}
			eqp = &((retp->equatep)[retp->nequates -1]);
			if (np != NULL)
				(void) strlcpy(eqp->name, np, MAX_GOT_NAMELEN);
			if (vp != NULL)
				(void) strlcpy(eqp->value, vp, MAX_GOT_VALUELEN);
			bp = ep + 1;
		} while (ep != NULL);
		if (np != NULL)
			np = str_free(np, file, line);
		if (vp != NULL)
			vp = str_free(vp, file, line);
	}
	xp = str_free(xp, file, line);

	/*
	 * Gobble to the end of input.
	 */
	got_lf = FALSE;
	content_length = -1;
	for(;;)
	{
		cp = io_fgets(ctx, &cnt, 0, __FILE__, __LINE__);
		if (cp == NULL || cnt < 0)
		{
			if (cp != NULL)
				cp = str_free(cp, __FILE__, __LINE__);
			break;
		}
		if (strncasecmp(cp, "Content-Length", 14) == 0)
		{
			xp = cp + 14;
			for (; *xp != '\0'; ++xp)
			{
				if (*xp == ':')
					break;
			}
			xp += 1;
			for (; *xp != '\0'; ++xp)
			{
				if (*xp != ' ')
					break;
			}
			content_length = strtoul(xp, NULL, 10);
			cp = str_free(cp, __FILE__, __LINE__);
			continue;
		}
		if (do_post == FALSE)
		{
			cp = str_free(cp, __FILE__, __LINE__);
			if (cnt == 0)
				break;
			else
				continue;
		}
		if (got_lf == TRUE)
		{
			retp->postp = pushargv(cp, retp->postp, __FILE__, __LINE__);
			cp = str_free(cp, __FILE__, __LINE__);
			continue;
		}
		if (strlen(cp) == 0)
		{
			got_lf = TRUE;
		}
		cp = str_free(cp, __FILE__, __LINE__);
		if (content_length > 0)
		{
			cp = io_fgets(ctx, &cnt, content_length, file, line);
			(void) html_unpercenthex(cp, content_length);
			xp = cp;
			do
			{
				ep = strchr(xp, '&');
				if (ep != NULL)
					*ep = '\0';
				retp->postp = pushargv(xp, retp->postp, __FILE__, __LINE__);
				xp = ep + 1;
			} while (ep != NULL);
			cp = str_free(cp, __FILE__, __LINE__);
			break;
		}
	}
	if (cp != NULL)
		cp = str_free(cp, __FILE__, __LINE__);
	return retp;
}


char *
io_html_now()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	time_t t;
	char nowbuf[128];
	char *n;
	struct tm *tm;

	(void) time(&t);

	(void) pthread_mutex_lock(&mutex);
	 tm = localtime(&t);
	 /* strftime is not thread safe */
	 (void) strftime(nowbuf, sizeof nowbuf, "%a, %e %b %Y %T GMT", tm);
	 n = str_dup(nowbuf, __FILE__, __LINE__);
	(void) pthread_mutex_unlock(&mutex);
	return n;
}

void
io_html_prefix(char *code, char *msg, char *buf, size_t buflen)
{
	char xbuf[512];
	char *n;

	if (code == NULL)
		code = "500";
	if (msg == NULL)
		code = "Internal Server Error";

	if (buf == NULL || buflen == 0)
	{
		buf = xbuf;
		buflen = sizeof xbuf;
	}

	(void) memset(buf, '\0', buflen);
	(void) strlcpy(buf, "HTTP/1.1 ", buflen);
	(void) strlcat(buf, code, buflen);
	(void) strlcat(buf, " ", buflen);
	(void) strlcat(buf, msg, buflen);
	(void) strlcat(buf, "\r\n", buflen);

	n = io_html_now();
	(void) strlcat(buf, "Date: ", buflen);
	(void) strlcat(buf, n, buflen);
	(void) strlcat(buf, "\r\n", buflen);
	n = str_free(n, __FILE__, __LINE__);

	(void) strlcat(buf, "Server: logod/", buflen);
	(void) strlcat(buf, VERSION, buflen);
	(void) strlcat(buf, " (Unix)\r\n", buflen);
	(void) strlcat(buf, "Connection: Close\r\n", buflen);
	(void) strlcat(buf, "Cache-Control: max-age=0, must-revalidate\r\n", buflen);
	(void) strlcat(buf, "\r\n", buflen);

}


/*
 * Really misnamed because it is only used for errors.
 */
int
io_html_good(THREAD_CTX_T *ctx, char *code, char *msg)
{
	int ret;
	char buf[BUFSIZ];
	char ebuf[BUFSIZ];
	
	if (ctx == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	io_html_prefix(code, msg, buf, sizeof buf);

	ret = io_bare_fwrite(ctx, buf, ebuf, sizeof ebuf);
	if (ret <= 0)
	{
		errno = EIO;
		return -1;
	}
	return ret;
}

int
io_html_redirect(THREAD_CTX_T *ctx, char *url, char *ebuf, size_t elen)
{
	int ret;
	char *n;
	char buf[BUFSIZ];
	char xbuf[BUFSIZ];
	
	if (ctx == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if (ebuf == NULL)
	{
		ebuf = xbuf;
		elen = sizeof xbuf;
	}
	

	(void) memset(buf, '\0', sizeof buf);
	(void) strlcpy(buf, "HTTP/1.1 ", sizeof buf);
	(void) strlcat(buf, "302", sizeof buf);
	(void) strlcat(buf, " ", sizeof buf);
	(void) strlcat(buf, "Moved Temporarily", sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);

	n = io_html_now();
	(void) strlcat(buf, "Date: ", sizeof buf);
	(void) strlcat(buf, n, sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);
	n = str_free(n, __FILE__, __LINE__);

	(void) strlcat(buf, "Server: logod/", sizeof buf);
	(void) strlcat(buf, VERSION, sizeof buf);
	(void) strlcat(buf, " (Unix)\r\n", sizeof buf);

	(void) strlcat(buf, "Location: ", sizeof buf);
	(void) strlcat(buf, url, sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);

	(void) strlcat(buf, "Connection: Close\r\n", sizeof buf);
	(void) strlcat(buf, "Content-Type: text/html\r\n", sizeof buf);
	(void) strlcat(buf, "Content-Length: 0\r\n", sizeof buf);
	(void) strlcat(buf, "Cache-Control: max-age=0, must-revalidate\r\n", sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);

	ret = io_bare_fwrite(ctx, buf, ebuf, sizeof ebuf);
	if (ret <= 0)
	{
		errno = EIO;
		return -1;
	}
	return ret;
}

/*
 * Simple routine to validate that a directory exists and is
 * in-fact a directory, and that we have write permission to it.
 */
int
io_isdirok(char *dir, char *ebuf, size_t elen)
{
	struct stat statb;
	char xbuf[1028];
	int xerror;

	if (dir == NULL)
	{
		(void) strlcpy(ebuf, "isdirok: Got NULL directory name", elen);
		return -1;
	}
	if (stat(dir, &statb) != 0)
	{
		xerror = errno;
		(void) strlcpy(xbuf, dir, sizeof xbuf);
		(void) strlcat(xbuf, ": ", sizeof xbuf);
		(void) strlcat(xbuf, strerror(xerror), sizeof xbuf);
		(void) strlcpy(ebuf, xbuf, elen);
		errno = xerror;
		return -1;
	}
	if (! S_ISDIR(statb.st_mode))
	{
		xerror = errno;
		(void) strlcpy(xbuf, dir, sizeof xbuf);
		(void) strlcat(xbuf, ": Not a directory.", sizeof xbuf);
		(void) strlcpy(ebuf, xbuf, elen);
		errno = xerror;
		return -1;
	}
	if (access(dir, R_OK|W_OK|X_OK) != 0)
	{
		xerror = errno;
		(void) strlcpy(xbuf, dir, sizeof xbuf);
		(void) strlcat(xbuf, ": Required Permission Denied", sizeof xbuf);
		(void) strlcpy(ebuf, xbuf, elen);
		errno = xerror;
		return -1;
	}
	return 0;
}
