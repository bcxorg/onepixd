#include "../onepixd.h"

typedef struct {
        EVP_PKEY *      pkey;
	RSA *           rsa;
	u_char *        rsaout;
} RSA_T; 

int
sign_get(char *host, char *port, char *pkpath, char *path, char *eid, char *tz, char *ebuf, size_t elen)
{
	FILE *		fp	= NULL;
	int		xerror	= 0;
	int		ret;
	EVP_PKEY *	keyp	= NULL;
	RSA_T		rsa;
	SHA256_CTX	sha256_ctx;
	u_char		sha256_out[SHA256_DIGEST_LENGTH+1];
	char		data[BUFSIZ];
	char		query[BUFSIZ];
	u_char		b64hash[BUFSIZ];
	unsigned int	rsaout_len;
	char *		cp;
	char **		p	= NULL;
	int		cnt;
	int		nid;
	time_t		now;
	int		portn;
	int		sock;
	struct hostent *hp;
	struct sockaddr_in sin;
	socklen_t sock_len;
	in_addr_t	addr;

	if (ebuf == NULL)
	{
		ebuf = alloca(BUFSIZ);
		elen = BUFSIZ;
	}

	errno = 0;
	if (host == NULL)
	{
		(void) snprintf(ebuf, elen, "Error: Failed to specify host or ip address.");
		return errno = EINVAL;
	}
	if (port == NULL)
	{
		(void) snprintf(ebuf, elen, "Error: Failed to specify port number.");
		return errno = EINVAL;
	}
	if (port == NULL)
	{
		(void) snprintf(ebuf, elen, "Failed to specify path to private key file.");
		return errno = EINVAL;
	}
	if (path == NULL)
	{
		(void) snprintf(ebuf, elen, "Failed to specify path of \"GET /path/?\".");
		return errno = EINVAL;
	}
	if (eid == NULL)
	{
		(void) snprintf(ebuf, elen, "Failed to specify \"eid\" for envelope id.");
		return errno = EINVAL;
	}
	if (tz == NULL)
	{
		(void) snprintf(ebuf, elen, "Failed to specify \"tz\" for time zone.");
		return errno = EINVAL;
	}

	fp = fopen(pkpath, "r");
	if (fp == NULL)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s: %s", pkpath, strerror(xerror));
		return errno = xerror;
	}
	keyp = NULL;
	if (PEM_read_PrivateKey(fp, &keyp, NULL, NULL) == NULL)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s: %s", pkpath, strerror(xerror));
		(void) fclose(fp);
		return errno = xerror;
	}
	(void) fclose(fp);
	(void) memset(&rsa, '\0', sizeof(RSA_T));
	rsa.rsa = EVP_PKEY_get1_RSA(keyp);
	if (rsa.rsa == NULL)
	{
		int err;

		xerror = (errno == 0) ? EIO : errno;
		err = ERR_get_error();
		(void) ERR_error_string_n(err, ebuf, sizeof elen);
		return errno = xerror;
	}
	rsaout_len = RSA_size(rsa.rsa);
	rsa.rsaout = malloc(rsaout_len*2);
	if (rsa.rsaout == NULL)
	{
		int err;

		xerror = (errno == 0) ? EIO : errno;
		err = ERR_get_error();
		(void) ERR_error_string_n(err, ebuf, sizeof elen);
		return errno = xerror;
	}
	(void) memset((char *)(rsa.rsaout), '\0', rsaout_len*2);

	
	now = time(NULL);
	
	snprintf(data, sizeof data, "%ld,%s,%s", now, eid, tz);
	(void) memset(&sha256_ctx, '\0', sizeof(SHA256_CTX));
	SHA256_Init(&sha256_ctx);
	SHA256_Update(&sha256_ctx, data, strlen(data));
	SHA256_Final(sha256_out, &sha256_ctx);

	nid = NID_sha256;
	ret = RSA_sign(nid, sha256_out, SHA256_DIGEST_LENGTH, rsa.rsaout, &rsaout_len, rsa.rsa);
	if (ret == 0)
	{
		int err;

		xerror = (errno == 0) ? EIO : errno;
		err = ERR_get_error();
		(void) ERR_error_string_n(err, ebuf, sizeof elen);
		return errno = xerror;
	}
	(void) memset(b64hash, '\0', sizeof b64hash);
	ret = base64_encode(rsa.rsaout, rsaout_len, b64hash, sizeof b64hash);
	(void) free(rsa.rsaout);

	for (cp = host, cnt = 0; *cp != '\0'; ++cp)
	{
		if (*cp == '.')
		{
			++cnt;
			continue;
		}
		if (! isdigit((int)*cp))
			break;
	}
	if (*cp == '\0' && cnt == 3)
	{
		addr = inet_addr(host);
		hp = gethostbyaddr(&addr, 4, AF_INET);
	}
	else
		hp = gethostbyname(host);
	if (hp == NULL)
		return -1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;

	ret = EIO;
	for (p = hp->h_addr_list; *p != 0; ++p)
	{
		(void) memset((char *) &sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		portn = atoi(port);
		sin.sin_port = htons(portn);
		sock_len = sizeof(struct sockaddr);
		(void) memcpy(&(sin.sin_addr.s_addr), *p, sock_len);
		ret = connect(sock, (struct sockaddr *)(&sin), sock_len);
		if (ret != 0)
			continue;
	}
	if (ret != 0)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s: %s", pkpath, strerror(xerror));
		return errno = xerror;
	}
	(void) snprintf(query, sizeof query, "GET /%s/?sent_id=%s&sent_tz=%s&sig=%s HTTP/1.1\r\n\r\n",
			path, eid, tz, (char *)b64hash);

	ret = write(sock, query, strlen(query));
	if (ret < 0)
	{
		xerror = errno;
		(void) snprintf(ebuf, elen, "%s: %s", pkpath, strerror(xerror));
		return errno = xerror;
	}
	*ebuf = '\0';
	(void) read(sock, ebuf, elen);
	cp = strchr(ebuf, '\r');
	if (cp != NULL)
		*cp = '\0';
	(void) close(sock);
	return 0;
}

#if TEST_MAIN
int
main(int argc, char **argv)
{
	char *	host	= NULL;
	char *	port	= NULL;
	char *	pkpath	= NULL;
	char *	path	= NULL;
	char *	eid	= NULL;
	char *	tz	= NULL;
	int	c;
	int	ret;
	char	ebuf[BUFSIZ];

	while ((c = getopt(argc, argv, "i:p:P:h:e:t:")) != -1)
	{
	    switch ((int) c)
	    {
		case 'i': 
			host = optarg;
			break;
		case 'p': 
			port = optarg;
			break;
		case 'P': 
			pkpath = optarg;
			break;
		case 'h': 
			path = optarg;
			break;
		case 'e': 
			eid = optarg;
			break;
		case 't': 
			tz = optarg;
			break;
		case '?':
		default:
usage:
			printf("Usage: %s -i ip_or_host -p port -P pubkey -h httppath -e email_id -t timezone\n", argv[0]);
			return 0;
	    }
	}
	if (host == NULL)
	{
		printf("-i missing: IP address or host name is mandatory\n");
		goto usage;
	}
	if (port == NULL)
	{
		printf("-p missing: port number is mandatory\n");
		goto usage;
	}
	if (pkpath == NULL)
	{
		printf("-P missing: path to private key is mandatory\n");
		goto usage;
	}
	if (path == NULL)
	{
		printf("-h missing: html prefix is mandatory\n");
		goto usage;
	}
	if (eid == NULL)
	{
		printf("-e missing: email id is mandatory\n");
		goto usage;
	}
	if (tz == NULL)
	{
		printf("-t missing: timezone is mandatory\n");
		goto usage;
	}

	ret = sign_get(host, port, pkpath, path, eid, tz, ebuf, sizeof ebuf);
	if (ret != 0)
		printf("ret = %d: %s\n", ret, strerror(errno));
	printf("%s\n", ebuf);
	return ret;
}
#endif /* TEST_MAIN */
