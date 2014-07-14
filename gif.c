#include "onepixd.h"

int
gif_send(THREAD_CTX_T *ctx, char *type, u_char *gif, size_t size, char *ebuf, size_t elen)
{
	char	buf[4096];
	char	nbuf[128];
	char *	n = NULL;
	char *  cp = NULL;
	size_t	nlen;
	int	xerror;
	size_t	len;
	int	ret;

	if (isdebug(BUG_GIF) == TRUE)
		(void) printf("DEBUG: %s(%d): gif_send: Entered size=%d\n",
			__FILE__, __LINE__, (int)size);
	if (gif == NULL)
	{
		xerror = EINVAL;
		(void) strlcpy(ebuf, "Write NULL gif: ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, __LINE__, ebuf);
		errno = xerror;
		return -1;
	}
	if (ctx == NULL)
	{
		xerror = EINVAL;
		(void) strlcpy(ebuf, "Write gif NULL context: ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, __LINE__, ebuf);
		errno = xerror;
		return -1;
	}

	if (ctx->fp == NULL)
	{
		xerror = EINVAL;
		(void) strlcpy(ebuf, "Write gif NULL file pointer: ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, __LINE__, ebuf);
		errno = xerror;
		return -1;
	}

	(void) memset(buf, '\0', sizeof buf);
	(void) strlcpy(buf, "HTTP/1.1 200 OK\r\n", sizeof buf);

	(void) strlcat(buf, "Server: onepixd/", sizeof buf);
	(void) strlcat(buf, VERSION, sizeof buf);
	(void) strlcat(buf, " (Unix)\r\n", sizeof buf);

	(void) strlcat(buf, "Date: ", sizeof buf);
	n = io_html_now();
	(void) strlcat(buf, n, sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);
	n = str_free(n, __FILE__, __LINE__);

	(void) strlcat(buf, "Expires: Mon, 04 Dec 2006 12:00:00 GMT\r\n", sizeof buf);

	(void) strlcat(buf, "Content-Type: image/", sizeof buf);
	(void) strlcat(buf, type, sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);

	(void) strlcat(buf, "Connection: close\r\n", sizeof buf);
	(void) strlcat(buf, "Pragma: no-cache\r\n", sizeof buf);
	(void) strlcat(buf, "Cache-Control: max-age=0, must-revalidate\r\n", sizeof buf);

	(void) str_ultoa(size, nbuf, sizeof nbuf);
	(void) strlcat(buf, "Content-Length: ", sizeof buf);
	(void) strlcat(buf, nbuf, sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);
	(void) strlcat(buf, "\r\n", sizeof buf);

	len = strlen(buf);

	nlen = len + size + 2;
	n = str_alloc(1, nlen, __FILE__, __LINE__);
	if (n == NULL)
	{
		xerror = errno;
		(void) strlcpy(ebuf, "Write gif: ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, __LINE__, ebuf);
		errno = xerror;
		return -1;
	}

	(void) strlcpy(n, buf, nlen);
	for (cp = n; *cp != '\0'; ++cp)
		continue;
	(void)memcpy(cp, gif, size);
	cp += size;
	(void)memcpy(cp, "\r\n", 2);


	if (isdebug(BUG_GIF) == TRUE)
	{
		int i;
		(void) printf("DEBUG: %s(%d): gif_send: gif size=%s\n",
			__FILE__, __LINE__, nbuf);

		(void) printf("---------------------------------------\n");
		for (i = 0; i < nlen; i++)
		{
			if (n[i] == '\r')
				continue;
			if (isprint((int)(n[i])) || n[i] == '\n')
				fputc(n[i], stdout);
			else
				fputc('.', stdout);
		}
		(void) printf("---------------------------------------\n");
	}

	ret = fwrite(n, 1, nlen, ctx->fp);
	if (ret != nlen)
	{
		xerror = errno;
		n = str_free(n, __FILE__, __LINE__);
		(void) strlcpy(ebuf, "Write gif: ", elen);
		(void) strlcat(ebuf, strerror(xerror), elen);
		log_emit(LOG_ERR, ctx->ipaddr, __FILE__, __LINE__, ebuf);
		errno = xerror;
		return -1;
	}
	ret = fflush(ctx->fp);
	n = str_free(n, __FILE__, __LINE__);
	if (isdebug(BUG_GIF) == TRUE)
		(void) printf("DEBUG: %s(%d): gif_send: Success\n",
			__FILE__, __LINE__);
	return ret;
}

static u_char *
gif_file_image(char *image_name, int *len)
{
	struct stat statb;
	int	    ret;
	u_char *    retp;
	FILE *	    fp = NULL;

	if (len == NULL)
		return NULL;

	ret = stat(image_name, &statb);
	if (ret != 0)
		return NULL;

	if (statb.st_size == 0)
		return NULL;

	retp = str_alloc(statb.st_size, 1, __FILE__, __LINE__);
	if (retp == NULL)
		return NULL;
	fp = fopen(image_name, "r");
	if (fp == NULL)
	{
		retp = str_free(retp, __FILE__, __LINE__);
		return NULL;
	}
	ret = fread(retp, 1, statb.st_size, fp);
	if (ret != statb.st_size)
	{
		(void) fclose(fp);
		retp = str_free(retp, __FILE__, __LINE__);
		return NULL;
	}
	(void) fclose(fp);
	*len = statb.st_size;
	return retp;
}

u_char *
gif_get1x1gif(int *lenp)
{
	u_char *retp = NULL;
	static u_char gif[] = {
		0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x21, 0xF9, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x00,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3B,
	};
	char **ary = NULL;

	ary = config_lookup(CONF_GIF_FILE);
	if (ary != NULL)
	{
		retp = gif_file_image(ary[0], lenp);
	}
	if (retp == NULL)
	{
		retp = str_alloc(sizeof gif, 1, __FILE__, __LINE__);
		if (retp == NULL)
			return NULL;
		(void) memcpy(retp, gif, sizeof gif);
		*lenp = sizeof gif;
	}
	return retp;
}

u_char *
gif_getfavicon_ico(int *lenp)
{
	return gif_file_image("favicon.ico", lenp);
}

