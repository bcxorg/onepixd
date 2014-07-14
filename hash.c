#include "onepixd.h"

/********************************************************************
** HASH_SET_CALLBACK -- Set the callback for freeing the data
**
**	Parameters:
**		ctx		-- Hash table context
**		callback	-- address of freeing function
**	Returns:
**		void		-- nothing
**	Side Effects:
**		None.
**	Notes:
**		The free function must be declared as:
**			void *funct(void *arg);
*/
void
hash_set_callback(HASH_CTX *hctx, void (*callback)(void *))
{
	if (hctx == NULL)
		return;
	hctx->freefunct = callback;
	return;
}

/********************************************************************
** HASH_STRING -- Convert a string into its hash value.
**
**	Parameters:
**		string	-- string to hash
**		limit	-- size of the hash table
**	Returns:
**		unsigned integer of hash value
**		if str == NULL hashes ""
**	Side Effects:
**		None.
**	Notes:
**		Generally for internal use only.
*/
static size_t
hash_string(char *str, size_t limit)
{
	size_t  hash      = 0;
	size_t  highorder = 0;
	int 	c         = 0;
	char *	s         = NULL;

	if (str == NULL)
		s = "";
	else
		s = str;

	/*
	 * Changed to a more modern CRC hash.
	 */
	hash = 5381;
	highorder = hash & 0xf8000000;
	do
	{
		c = (int)(*s);
		if (c == 0)
			break;
		hash = hash << 5;
		hash = hash ^ (highorder >> 27);
		hash = hash ^ c;
		highorder = hash & 0xf8000000;
		++s;
	} while (c != 0);
	return hash % limit;
}

/********************************************************************
** HASH_INIT -- Allocate and receive a context pointer
**
**	Parameters:
**		tablesize	-- size of the internal hash table
**	Returns:
**		Address of type HASH_CTX *
**		NULL on error and sets errno.
**	Side Effects:
**		Allocates memory.
**		Initializes tablesize number of mutexes
**	Notes:
**		If tablesize is zero, defaults to (2048)
**		Tablesize should be a power of two, if not, it
**		is silently adjusted to a power of two.
**	If you want a callback to free your data, call
**	hash_set_callback() immediately after this call.
*/
HASH_CTX *
hash_init(size_t tablesize)
{
	size_t 		 i     = 0;
	unsigned int	 p2    = 0;
	HASH_CTX	*hctx  = NULL;

	hctx = str_alloc(sizeof(HASH_CTX), 1, __FILE__, __LINE__);
	if (hctx == NULL) 
	{
		if (errno == 0)
			errno = ENOMEM;
		return NULL;
	}
	(void) memset(hctx, '\0', sizeof(HASH_CTX));

	if (tablesize == 0)
		hctx->tablesize = DEFAULT_HASH_TABLESIZE;
	else
		hctx->tablesize = tablesize;

	hctx->freefunct = NULL;

	/* 
	 * If buckets is too small, make it min sized. 
	 */
	if (hctx->tablesize < HASH_MIN_SHELVES)
		hctx->tablesize = HASH_MIN_SHELVES;

	/* 
	 * If it's too large, cap it. 
	 */
	if (hctx->tablesize > HASH_MAX_SHELVES)
		hctx->tablesize = HASH_MAX_SHELVES;

	/* 
	 * If it's is not a power of two in size, round up. 
	 */
	if ((hctx->tablesize & (hctx->tablesize - 1)) != 0) 
	{
		for (p2 = 0; hctx->tablesize != 0; p2++)
			hctx->tablesize >>= 1;

		if (p2 <= HASH_MAX_SHELVES_LG2)
			hctx->tablesize = DEFAULT_HASH_TABLESIZE;
		else
			hctx->tablesize = 1 << p2;
	}

	hctx->table = str_alloc(hctx->tablesize, sizeof(HASH_SHELF), __FILE__, __LINE__);
	if (hctx->table == NULL) 
	{
		if (errno == 0)
			errno = ENOMEM;
		hctx = str_free(hctx, __FILE__, __LINE__);
		return NULL;
	}

	for (i = 0; i < hctx->tablesize; i++)
	{
		(void) pthread_mutex_init(&(hctx->table[i].mutex), NULL);
		hctx->table[i].bucket = NULL;
	}

	return hctx;
}

/********************************************************************
** HASH_FREEBUCKET -- Free a bucket.
**
**	Parameters:
**		b	-- pointer to a bucket
**	Returns:
**		NULL always.
**		errno is non-zero on error
**	Side Effects:
**		Frees memory.
**	Notes:
**		Intended for internal use only.
**		Does not unlink b from linked list.
**		NO NOT mutex lock here.
*/
static HASH_BUCKET *
hash_freebucket(HASH_CTX *hctx, HASH_BUCKET *b)
{
	if (b == NULL)
		return NULL;
	if (b->key != NULL)
	{
		b->key = str_free(b->key, __FILE__, __LINE__);
		b->key = NULL;
	}
	if (b->data != NULL)
	{
		if (hctx != NULL && hctx->freefunct != NULL)
		{
			(hctx->freefunct)(b->data);
			b->data = NULL;
		}
		else
		{
			b->data = str_free(b->data, __FILE__, __LINE__);
		}
	}
	b = str_free(b, __FILE__, __LINE__);
	return NULL;
}

/********************************************************************
** HASH_SHUTDOWN -- Give up and free a hash table.
**
**	Parameters:
**		hctx	-- A hash context from hash_init()
**	Returns:
**		NULL always.
**		errno is non-zero on error
**	Side Effects:
**		Frees memory.
**	Notes:
**		None
*/
HASH_CTX *
hash_shutdown(HASH_CTX *hctx)
{
	int             i = 0;
	HASH_BUCKET *t = NULL;
	HASH_BUCKET *b = NULL;

	if (hctx == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	if (hctx->table == NULL || hctx->tablesize == 0)
	{
		errno = EINVAL;
		return NULL;
	}

	for (i = 0; i < hctx->tablesize; i++) 
	{
		(void) pthread_mutex_destroy(&(hctx->table[i].mutex));
		if ((hctx->table[i].bucket) == NULL)
			continue;
		
		b = hctx->table[i].bucket;
		if (b == NULL)
			continue;
		do
		{
			t = b->next;
			b = hash_freebucket(hctx, b);
			b = t;
		} while (b != NULL);
	}
	hctx->table = str_free(hctx->table, __FILE__, __LINE__);
	hctx = str_free(hctx, __FILE__, __LINE__);
	errno = 0;
	return NULL;
}

/********************************************************************
** HASH_LOOKUP -- Look up a key and get its data
**
**	Parameters:
**		hctx	-- A hash context from hash_init()
**		string	-- The string to lookup
**		data	-- Data for update only (NULL for plain lookup)
**		datalen -- Size in bytes of the data blob
**	Returns:
**		Address of data on success (search or update)
**		NULL and sets non-zero errno on error
**	Side Effects:
**		Allocates memory on update.
**	Notes:
**		If data is NULL, just lookup string and return data if found.
**		If data not NULL, insert if string not found, but if found,
**			replace the old data with the new.
*/
void *
hash_lookup(HASH_CTX *hctx, char *string, void *data, size_t datalen)
{
	uint32_t         hashval = 0;
	HASH_BUCKET  *b       = NULL;
	HASH_BUCKET  *n       = NULL;

	if (data != NULL && datalen == 0)
	{
		errno = EINVAL;
		return NULL;
	}

	if (string == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	if (hctx == NULL || hctx->table == NULL || hctx->tablesize == 0)
	{
		errno = EINVAL;
		return NULL;
	}


	hashval = hash_string(string, hctx->tablesize);

	(void) pthread_mutex_lock(&(hctx->table[hashval].mutex));
	 b = hctx->table[hashval].bucket;
	 if (b != NULL)
	 {
		do
		{
			if (b->key != NULL && strcasecmp(string, b->key) == 0)
			{
				(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
				errno = 0;
				return b->data;
			}
			b = b->next;
		} while (b != NULL);
	 }
	 if (data == NULL)
	 {
		(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
	 	errno = 0;
	 	return NULL;
	 }

	 /*
	  * Not found, so we inert it.
	  */
	 n = str_alloc(1, sizeof(HASH_BUCKET), __FILE__, __LINE__);
	 if (n == NULL)
	 {
		(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
		errno = ENOMEM;
		return NULL;
	 }
	 n->next = n->previous = NULL;
	 n->key = str_dup(string, __FILE__, __LINE__);
	 if (n->key == NULL)
	 {
		n = str_free(n, __FILE__, __LINE__);
		(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
		errno = ENOMEM;
		return NULL;
	 }
	 n->data = data;
	 (void) time(&(n->timestamp));

	 b = hctx->table[hashval].bucket;
	 if (b == NULL)
	 {
	 	hctx->table[hashval].bucket = n;
		(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
		errno = 0;
	 	return n->data;
	 }
	 while (b->next != NULL)
	 	b = b->next;
	 b->next = n;
	 n->previous = b;
	 n->next = NULL;
	(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));

	errno = 0;
	return n->data;
}
 
/********************************************************************
** HASH_DROP -- Remove a key/data from the hash table
**
**	Parameters:
**		hctx	-- A hash context from hash_init()
**		string	-- The string to remove
**	Returns:
**		Zero on success
**		Returns non-zero errno on error
**	Side Effects:
**		Frees memory
**	Notes:
**		If string not in the table, returns zero anyway.
*/
int
hash_drop(HASH_CTX *hctx, char *string)
{
	uint32_t        hashval = 0;
	HASH_BUCKET *b       = NULL;

	if (string == NULL)
	{
		return errno = EINVAL;
	}

	if (hctx == NULL || hctx->table == NULL || hctx->tablesize == 0)
	{
		return errno = EINVAL;
	}

	hashval = hash_string(string, hctx->tablesize);

	(void) pthread_mutex_lock(&(hctx->table[hashval].mutex));
	 b = hctx->table[hashval].bucket;
	 if (b != NULL)
	 {
		do
		{
			if (b->key != NULL && strcasecmp(string, b->key) == 0)
			{
				 if (b->previous != NULL)
					b->previous->next = b->next;
				 if (b->next != NULL)
					b->next->previous = b->previous;
				 b = hash_freebucket(hctx, b);
				(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
				return errno = 0;
			}
			b = b->next;
		} while (b != NULL);
	 }
	(void) pthread_mutex_unlock(&(hctx->table[hashval].mutex));
	return errno = 0;
}
 
/********************************************************************
** HASH_EXPIRE -- Remove old data from the hash table
**
**	Parameters:
**		hctx	-- A hash context from hash_init()
**		age	-- Maximum age to retain
**	Returns:
**		Zero on success
**		Returns non-zero errno on error
**	Side Effects:
**		Frees memory
**	Notes:
**		The age is in seconds. All entries older than
**		age are removed from the table.
*/
int
hash_expire(HASH_CTX *hctx, time_t age)
{
	HASH_BUCKET *b   = NULL;
	HASH_BUCKET *t   = NULL;
	time_t 		now = 0;
	int		i   = 0;

	if (age == 0)
	{
		return errno = EINVAL;
	}

	if (hctx == NULL || hctx->table == NULL || hctx->tablesize == 0)
	{
		return errno = EINVAL;
	}

	(void) time(&now);
	for (i = 0; i < hctx->tablesize; i++)
	{

		(void) pthread_mutex_lock(&(hctx->table[i].mutex));
		 b = hctx->table[i].bucket;
		 if (b != NULL)
		 {
			do
			{
				t = b->next;
				if ((now - b->timestamp) > age)
				{
					if (b->previous != NULL)
						b->previous->next = b->next;
					if (b->next != NULL)
						b->next->previous = b->previous;
					if (b == hctx->table[i].bucket)
						hctx->table[i].bucket = t;
					b = hash_freebucket(hctx, b);
				}
				b = t;
			} while (b != NULL);
		 }
		(void) pthread_mutex_unlock(&(hctx->table[i].mutex));
	}
	return errno = 0;
}
 
