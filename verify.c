#include "onepixd.h"

/*
** OpenSSL locking stuff needed for SSL
*/

static pthread_mutex_t *Mutex_buf= NULL;

static void 
verify_threads_locking_function(int mode, int n, const char * file, int line)
{
	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&(Mutex_buf[n]));
	else
		pthread_mutex_unlock(&(Mutex_buf[n]));
}

static unsigned long
verify_threads_id_function(void)
{
	return ((unsigned long)pthread_self());
}

int
verify_threads_locking_init()
{
	int i;

	Mutex_buf = str_alloc(CRYPTO_num_locks(), sizeof(pthread_mutex_t), __FILE__, __LINE__);
	if (Mutex_buf == NULL)
		return errno;

	for (i = 0;  i < CRYPTO_num_locks();  i++)
		pthread_mutex_init(&(Mutex_buf[i]), NULL);

	CRYPTO_set_id_callback(verify_threads_id_function);
	CRYPTO_set_locking_callback(verify_threads_locking_function);
	return errno = 0;
}

void
verify_threads_locking_shutdown(void)
{
	int i;

	if (Mutex_buf == NULL)
		return;

	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);

	for (i = 0;  i < CRYPTO_num_locks();  i++)
		pthread_mutex_destroy(&(Mutex_buf[i]));

	Mutex_buf = str_free(Mutex_buf, __FILE__, __LINE__);
	return;
}

int
verify_sig(u_char *sig, size_t siglen, char **gary)
{
	EVP_PKEY *       rsa_pkey = NULL;
	RSA *            rsa_rsa = NULL;
	u_char *         rsa_sig = NULL;
	size_t           rsa_siglen = 0;
	size_t           rsa_keysize = 0;
	SHA256_CTX       sha256_ctx;
	u_char           sha256_out[SHA256_DIGEST_LENGTH];
	u_char           b64buf[1028];
	u_char	*	 p;
	u_char  	 pubkey[BUFSIZ];
	size_t		 pubkeylen;
	int		 nid, ret;
	int		 xerror;
	char **		 ary		= NULL;
	char **		 app;

	if (sig == NULL || gary == NULL)
		return errno = EINVAL;

	(void) memset(&sha256_ctx, '\0', sizeof(SHA256_CTX));
	SHA256_Init(&sha256_ctx);
	for (app = gary; *app != NULL; ++app)
		SHA256_Update(&sha256_ctx, *app, strlen(*app));
	SHA256_Final(sha256_out, &sha256_ctx);

	ary = config_lookup(CONF_UPLOAD_PUBKEY);
	if (ary == NULL)
		return errno = EINVAL;
	pubkeylen = base64_decode((u_char *)ary[0], pubkey, sizeof pubkey);
	if (pubkeylen <= 0)
	{
		return errno = EINVAL;
	}
	
	p = pubkey;
	rsa_pkey =  d2i_PUBKEY(NULL, (const u_char **)&p, pubkeylen);
	if (rsa_pkey == NULL)
	{
		xerror = (errno == 0) ? ENOMEM : errno;
		return errno = xerror;
	}

	rsa_rsa = EVP_PKEY_get1_RSA(rsa_pkey);
	if (rsa_rsa == NULL)
	{
		xerror = (errno == 0) ? ENOMEM : errno;
		return errno = xerror;
	}
	rsa_keysize = RSA_size(rsa_rsa);

	ret = base64_decode((u_char *)sig, b64buf, sizeof b64buf);
	if (ret <= 0)
	{
		xerror = (errno == 0) ? ENOMEM : errno;
		return errno = xerror;
	}

	rsa_sig = b64buf;
	rsa_siglen = ret;
	nid = NID_sha256;
	ret = RSA_verify(nid, sha256_out, SHA256_DIGEST_LENGTH, 
		rsa_sig, rsa_siglen, rsa_rsa);

	if (ret != 1)
	{
		/*
		 * Signature failed. Be certain to log why it failed.
		 */
		xerror = (errno == 0) ? ENOMEM : errno;
		EVP_PKEY_free(rsa_pkey);
		RSA_free(rsa_rsa);
		errno = xerror;
	}

	RSA_free(rsa_rsa);
	EVP_PKEY_free(rsa_pkey);
	return errno = 0;
}
