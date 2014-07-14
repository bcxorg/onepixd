#include "onepixd.h"


/********************************************************************
** STR_SQUEEZE -- Remove surrounding whitespace
**
**      Parameters:
**              str	-- String to squeeze and duplicate
**		freeflag -- Free str on success.
**		file	-- caller
**		line	-- caller;
**      Returns:
**              Address of duplicate on success
**              NULL on failure
**      Side Effects:
**		Allocates memory.
**      Notes:
**	 	Used to parse html returns such as Host:foo
**		That might show up hostile as "Host  :   foo"
*/
char *
str_squeeze(char *str, int freeflag, char *file, int line)
{
	char *new, *np, *cp;
	int len;
	
	if (str == NULL)
		return NULL;

	len = strlen(str);
	if (len == 0)
		return NULL;

	new = str_alloc(len+1, 1, file, line);
	if (new == NULL)
		return NULL;
	for (cp = str, np = new; *cp != '\0'; ++cp)
	{
		if (! isspace((int) *cp))
			*np++ = *cp;
	}
	*np = '\0';
	if (freeflag == TRUE)
		str = str_free(str, __FILE__, __LINE__);
	return new;
}

/*****************************************************
** A simple way to track allocs and frees.
*/
#define MAX_ALLOCS BUFSIZ
static int NumAllocs = 0;
typedef struct {
	void * addr;
	char   file[256];
	int    line;
} ALLOC_T;
static ALLOC_T AllocList[MAX_ALLOCS];
pthread_mutex_t Alloc_Mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************
**  STR_ALLOC -- Self correcting alloc
**
**  Will not attempt to allocate zero bytes
**  Always zeros the alocated memory.
**
**	Parameters:
**		size	-- item size
**		count	-- number of items
**		file    -- file that called this
**		line	-- line number in file
**	Returns:
**		Address of allocated memory
**	Side Effects:
**		Allocates memory.
*/
void *
str_alloc(size_t size, size_t count, char *file, int line)
{
	int bytes;
	void *retp;

	if (file == NULL)
		file = __FILE__;
	if (line <= 0)
		line =  __LINE__;

	bytes = size * count;
	if (bytes < 1)
		bytes = 1;

	retp = malloc(bytes);
	if (retp == NULL)
	{
		return NULL;
	}
	(void) memset(retp, '\0', bytes);
	(void) pthread_mutex_lock(&Alloc_Mutex);
	 if (NumAllocs < MAX_ALLOCS)
	 {
		AllocList[NumAllocs].addr = retp;
		AllocList[NumAllocs].line = line;
		(void)strlcpy(AllocList[NumAllocs].file, file, 255);
	 }
	 ++NumAllocs;
	 if (NumAllocs >= MAX_ALLOCS)
		--NumAllocs;
	(void) pthread_mutex_unlock(&Alloc_Mutex);
	return retp;
}

/*****************************************************
**  STR_DUP -- Self correcting strdup
**
**	Parameters:
**		str	-- string to duplicate
**		file    -- file that called this
**		line	-- line number in file
**	Returns:
**		Address of allocated memory
**	Side Effects:
**		Allocates memory.
*/
char *
str_dup(char *str, char *file, int line)
{
	size_t bytes;
	char  *retp;

	if (file == NULL)
		file = __FILE__;
	if (line <= 0)
		line =  __LINE__;

	if (str == NULL)
		return NULL;
	bytes = strlen(str);
	if (bytes < 1)
		bytes = 1;

	bytes += 1;
	retp = str_alloc(bytes, 1,  file, line);
	if (retp == NULL)
	{
		return NULL;
	}
	(void) memset(retp, '\0', bytes);
	(void) strlcpy(retp, str, bytes);
	return retp;
}

/*****************************************************
**  STR_REALLOC -- Self correcting realloc
**
**	Parameters:
**		oldptr	-- old pointer
**		oldsize -- old buffer size
**		size	-- new item size
**		file    -- file that called this
**		line	-- line number in file
**		flag	-- free oldp on failure if TRUE
**	Returns:
**		Address of allocated memory
**	Side Effects:
**		Allocates memory.
*/
void *
str_realloc(void *oldp, size_t oldsize, size_t newsize, char *file, int line, int flag)
{
	void *newp;

	if (oldp == NULL)
		return NULL;
	if (file == NULL)
		file = __FILE__;
	if (line <= 0)
		line =  __LINE__;

	if (newsize < 1)
		newsize = 1;
	if (newsize <= oldsize)
		return oldp;

	newp = str_alloc(newsize, 1, file, line);
	if (newp == NULL)
	{
		if (flag == TRUE)
			oldp = str_free(oldp, __FILE__, __LINE__);
		return NULL;
	}
	(void) memset(newp, '\0', newsize);
	if (oldsize > 0)
		(void) memcpy(newp, oldp, oldsize);
	if (newsize > oldsize)
		(void) memset(newp+oldsize, '\0', newsize - oldsize);
	oldp = str_free(oldp, __FILE__, __LINE__);

	return newp;
}

/*****************************************************
**  STR_FREE -- Self correcting free
**
**  Will not attempt to free NULL.
**  Will not attempt to free a static variable.
**
**	Parameters:
**		arg	-- Pointer to free
**		file    -- file that called this
**		line	-- line number in file
**	Returns:
**		NULL always
**	Side Effects:
**		Frees memory
*/
void *
str_free(void *arg, char *file, int line)
{
	char *c = "constant";
	size_t diff;
	int i;

	if (file == NULL)
		file = __FILE__;
	if (line <= 0)
		line =  __LINE__;

	if (arg == NULL)
	{
		return NULL;
	}

	if (arg > (void *)c)
		diff = arg - (void *)c;
	else
		diff = (void *)c - arg;
	if (diff < 32000)
	{
		return NULL;
	}
	(void) pthread_mutex_lock(&Alloc_Mutex);
	 for (i = 0; i < NumAllocs; i++)
	 {
		if (AllocList[i].addr == arg)
			break;
	 }
	 if (i < NumAllocs)
	 {
		int j;
		if (i + 1 == NumAllocs)
			--NumAllocs;
		else
		{
			for (j = i+1; j < NumAllocs; i++, j++)
			{
				if (i < 0 || i >= MAX_ALLOCS)
					continue;
				if (j < 0 || j >= MAX_ALLOCS)
					continue;
				AllocList[i].addr = AllocList[j].addr;
				AllocList[i].line = AllocList[j].line;
				(void) strlcpy(AllocList[i].file, AllocList[j].file, 255);
			}
			--NumAllocs;
		}
	 }
	(void) pthread_mutex_unlock(&Alloc_Mutex);

	(void) free(arg);
	return NULL;
}

void
str_shutdown(void)
{
	int i;
	char	nbuf[MAX_ACCII_INT];
	char ebuf[MAX_EBUF_LEN];

	if (NumAllocs <= 0)
		return;

	for (i = 0; i < NumAllocs; i++)
	{

		(void) strlcpy(ebuf, "Possible memory leak found at exit: ", sizeof ebuf);
		(void) strlcat(ebuf, AllocList[i].file, sizeof ebuf);
		(void) strlcat(ebuf, " at line ", sizeof ebuf);
		(void) str_ultoa(AllocList[i].line, nbuf, sizeof nbuf);
		(void) strlcat(ebuf, nbuf, sizeof ebuf);
		(void) strlcat(ebuf, ".", sizeof ebuf);
		log_emit(LOG_ERR, NULL, __FILE__, __LINE__, ebuf);
	}
	return;
}

/*
** Convert a decimal unsigned long interger into a string.
** Returns a pointer to the passed buffer.
*/
char *
str_ultoa(unsigned long val, char *buffer, size_t bufferlen)
{
	register char  *b = buffer;
	register size_t l = bufferlen;
	register unsigned long    v = val;
	register long  mod, d, digit;
#define MAXDIGITS (32)
	int digits[MAXDIGITS];

	if (b == NULL || l < 2)
		return NULL;

	if (v == 0)
	{
		*b++ = '0';
		*b = '\0';
		return buffer;
	}
	digit = 0;
	do
	{
		mod = v % 10;
		v = v / 10;
		digits[digit] = mod;
		++digit;
		if (digit >= MAXDIGITS)
			break;
	} while(v != 0);

	for (d = digit-1; d >= 0; --d)
	{
		switch (digits[d])
		{
		    case 0: *b++ = '0'; --l; break;
		    case 1: *b++ = '1'; --l; break;
		    case 2: *b++ = '2'; --l; break;
		    case 3: *b++ = '3'; --l; break;
		    case 4: *b++ = '4'; --l; break;
		    case 5: *b++ = '5'; --l; break;
		    case 6: *b++ = '6'; --l; break;
		    case 7: *b++ = '7'; --l; break;
		    case 8: *b++ = '8'; --l; break;
		    case 9: *b++ = '9'; --l; break;
		}
		if (l == 1)
			break;
	}
	*b = '\0';
	return buffer;
}

char *
strcasestr(char *hay, char *needle)
{
	int c1, c2;
	char *cp, *np;
	int len;

	if (hay == NULL || needle == NULL)
		return NULL;
	len = strlen(needle);
	if (len == 0)
		return NULL;

	c1 = (int)*needle;
	if (isalpha(c1))
	{
		if (isupper(c1))
			c2 = tolower(c1);
		else
			c2 = toupper(c1);
	}
	else
		c2 = c1;

	cp = hay;
	while ((np = strchr(cp, c1)) != NULL)
	{
		if (strncasecmp(np, needle, len) == 0)
			return np;
		cp = np+1;
	}
	cp = hay;
	while ((np = strchr(cp, c2)) != NULL)
	{
		if (strncasecmp(np, needle, len) == 0)
			return np;
		cp = np+1;
	}
	return NULL;
}

/*****************************************************
**  PUSHARGV -- Add to and array of strings.
**
**	Parameters:
**		str	-- The string to add.
**		ary	-- The array to extend.
**	Returns:
**		ary on success.
**		NULL on error and sets errno.
**	Side Effects:
**		Allocates and reallocates memory.
*/
char **
pushargv(char *str, char **ary, char *file, size_t line)
{
	if (str != NULL)
	{
		int    i;
		char **tmp;

		if (ary == NULL)
		{
			ary = str_alloc(sizeof(char **), 2, file, line);
			if (ary == NULL)
			{
				return NULL;
			}
			ary[0] = str_dup(str, file, line);
			ary[1] = NULL;
			if (ary[0] == NULL)
			{
				ary = str_free(ary, file, line);
				return NULL;
			}
			return ary;
		}
		for (i = 0; ;i++)
		{
			if (ary[i] == NULL)
				break;
		}
		tmp = str_realloc((void *)ary, sizeof(char **) * (i+1), sizeof(char **) * (i+2), file, line, FALSE);
		if (tmp == NULL)
		{
			ary = clearargv(ary);
			return NULL;
		}
		ary = tmp;
		ary[i] = str_dup(str, file, line);
		if (ary[i] == NULL)
		{
			ary = clearargv(ary);
			return NULL;
		}
		ary[i+1] = NULL;
	}
	return ary;
}

char *
popargv(char **ary)
{
	char **ccp;
	char  *cp;

	if (ary == NULL)
		return NULL;
	for (ccp = ary; ; ++ccp)
	{
		if (*ccp == NULL)
			break;
	}
	if (ccp == ary)
	{
		(void) str_free(ary, __FILE__,  __LINE__);
		return NULL;
	}
	--ccp;
	cp = *ccp;
	*ccp = NULL;
	return cp;
}

int
sizeofargv(char **ary)
{
	char **ap;
	int  cnt;

	if (ary == NULL)
		return 0;

	cnt = 0;
	for (ap = ary; *ap != NULL; ++ap)
		++cnt;

	return cnt;
}

int
str_parse_to_ary(char *str, int ch, FIXED_ARRAY_T *cols)
{
	char *	cp	= NULL;
	char *	ep	= NULL;

	if (str == NULL || cols == NULL)
		return -1;

	cp = str;
	cols->nary = 0;
	do
	{
		ep = strchr(cp, ch);
		if (ep != NULL)
			*ep = '\0';
		(void) strlcpy(cols->ary[cols->nary], cp, MAX_COLUMN_NAME_LEN);
		cols->nary += 1;
		if (ep != NULL)
			cp = ep + 1;
	} while (ep != NULL);
	return cols->nary;
}


/********************************************************************
** STR_PARSE_PATH_COLUMNS -- Parse out the configured settings
**
**	Parameters:
**		str	-- the string to parsse
**		file	-- passed __FILE__ 
**		line	-- passed __LINE__
**	Returns:
**		pointer to a filled out PATH_COLUMNS_T
**		NULL on failue
**
** Note that column paths are in the format: path:column,list,...
********************************************************************/
PATH_COLUMNS_T *
str_parse_path_column(char *str, char *file, int line)
{
	PATH_COLUMNS_T	*pcp;
	char		*cp;
	char		*ep;
	int		 ret;
	char		 copy[MAX_COLUMN_NAME_LEN];

	if (str == NULL)
		return NULL;
	(void) strlcpy(copy, str, sizeof copy);

	ep = strchr(copy, ':');
	if (ep == NULL)
		return NULL;

	*ep = '\0';
	++ep;
	if (*ep == '\0')
	{
		return NULL;
	}

	cp = str_squeeze(copy, FALSE, file, line);
	if (cp == NULL)
		return NULL;

	pcp = str_alloc(sizeof(PATH_COLUMNS_T), 1, file, line);
	if (pcp == NULL)
	{
		cp = str_free(cp, __FILE__, __LINE__);
		return NULL;
	}

	(void) strlcpy(pcp->path, cp, sizeof pcp->path); 
	cp = str_free(cp, __FILE__, __LINE__);

	ret = str_parse_to_ary(ep, ',', &(pcp->columns));
	if (ret <= 0)
	{
		pcp = str_free(pcp, file, line);
		return NULL;
	}
	return pcp;
}

/********************************************************************
** PARSE_EQUAL -- Find and return name and value in name=value
**
**	Parameters:
**		str	-- string containing one expression
**		namep	-- pointer to name variable
**		valuep	-- pointer to value variable
**	Returns:
**		0 on success
**		non-zero on failure and sets errno
**	Side Effects:
**		Allocates memory
**	Notes:
*/

int
parse_equal(char *string, char **namep, char **valuep)
{
	register char **np = namep;
	register char **vp = valuep;
	register char *c0, *e0, *c1, *e1;
	char 		s[MAX_GOT_NAMELEN * MAX_COLUMNS];

	if (string == NULL || strlen(string) < 3)  /* minimum "a=b" */
		return (errno = EINVAL);
	if (np == NULL)
		return (errno = EINVAL);
	if (vp == NULL)
		return (errno = EINVAL);

	(void) strlcpy(s, string, sizeof s);
	/*
	 * Find the name
	 */
	e0 = c1 = strchr(s, '=');
	if (e0 == NULL)
		return (errno = EINVAL);

	/* split name at the equal into two strings */
	*e0 = '\0';
	--e0;
	/* missing name (e.g. "=b") caught here */
	if (e0 == s)
		return (errno = EINVAL);
	++c1;
	/* strip leading spaces */
	for (c0 = s; c0 < e0; ++c0)
	{
		if (isspace((int)*c0))
			continue;
		break;
	}
	/* empty name */
	if (c0 == e0)
		return (errno = EINVAL);
	/* strip trailing spaces */
	for (; e0 > c0; --e0)
	{
		if (isspace((int)*e0))
		{
			*e0 = '\0';
			continue;
		}
		break;
	}
	/* empty name */
	if (c0 == e0)
		return (errno = EINVAL);

	/*
	 * c0 now points to the name.
	 * parse the value.
	 */
	e1 = c1 + strlen(c1);
	if (e1 == c1)
		return (errno = EINVAL);
	/* strip leading spaces */
	for (; c1 < e1; ++c1)
	{
		if (isspace((int)*c1))
			continue;
		break;
	}
	/* empty value */
	if (c1 == e1)
		return (errno = EINVAL);
	/* strip trailing spaces */
	for (; e1 > c1; --e1)
	{
		if (isspace((int)*e1) || *e1 == '\0')
		{
			*e1 = '\0';
			continue;
		}
		break;
	}
	++e1;
	/* empty name */
	if (c1 == e1)
		return (errno = EINVAL);

	/*
	 * c0 points to name, c1 points to value. Neither is
	 * zero size or it would have been rejected.
	 */
	*np = str_dup(c0, __FILE__, __LINE__);
	if (*np == NULL)
		return (errno == 0 ? ENOMEM : errno);
	*vp = str_dup(c1, __FILE__, __LINE__);
	if (*vp == NULL)
	{
		*np = str_free(*np, __FILE__, __LINE__);
		return (errno == 0 ? ENOMEM : errno);
	}
	return 0;
}



/*****************************************************
**  CLEARARGV -- Free the argv array
**
**	Parameters:
**		ary	-- Pointer to array to free
**	Returns:
**		NULL always
**	Side Effects:
**		Allocates memory.
*/
char **
clearargv(char **ary)
{
	if (ary != NULL)
	{
		char **ccp;

		for (ccp = ary; *ccp != NULL; ++ccp)
		{
			*ccp = str_free(*ccp, __FILE__, __LINE__);
		}
		ary = str_free(ary, __FILE__, __LINE__);
	}
	return NULL;
}

int
truefalse(char *str)
{
	if (str == NULL)
		return NEITHER;
	if (strlen(str) == 0)
		return NEITHER;
	switch ((int)*str)
	{
		case '1':
		case 'T':
		case 'Y':
		case 't':
		case 'y':
		case 'S':
		case 's':
			return TRUE;
		case '0':
		case 'F':
		case 'N':
		case 'f':
		case 'n':
			return FALSE;
	}
	return NEITHER;
}

#if ! HAVE_STRLCPY
/*
**  STRLCPY -- size bounded string copy
**
**	This is a bounds-checking variant of strcpy.
**	If size > 0, copy up to size-1 characters from the nul terminated
**	string src to dst, nul terminating the result.  If size == 0,
**	the dst buffer is not modified.
**	Additional note: this function has been "tuned" to run fast and tested
**	as such (versus versions in some OS's libc).
**
**	The result is strlen(src).  You can detect truncation (not all
**	of the characters in the source string were copied) using the
**	following idiom:
**
**		char *s, buf[BUFSIZ];
**		...
**		if (strlcpy(buf, s, sizeof(buf)) >= sizeof(buf))
**			goto overflow;
**
**	Parameters:
**		dst -- destination buffer
**		src -- source string
**		size -- size of destination buffer
**
**	Returns:
**		strlen(src)
*/

size_t
strlcpy(char *dst, char *src, size_t size)
{
	size_t i;

	if (src == NULL)
		return 0;
	if (dst == NULL)
		return 0;
	if (size-- <= 0)
		return strlen(src);
	for (i = 0; i < size && (dst[i] = src[i]) != 0; i++)
		continue;
	dst[i] = '\0';
	if (src[i] == '\0')
		return i;
	else
		return i + strlen(src + i);
}
#endif /* ! HAVE_STRLCPY */

#if ! HAVE_STRLCAT
/*
**  STRLCAT -- size bounded string concatenation
**
**	This is a bounds-checking variant of strcat.
**	If strlen(dst) < size, then append at most size - strlen(dst) - 1
**	characters from the source string to the destination string,
**	nul terminating the result.  Otherwise, dst is not modified.
**
**	The result is the initial length of dst + the length of src.
**	You can detect overflow (not all of the characters in the
**	source string were copied) using the following idiom:
**
**		char *s, buf[BUFSIZ];
**		...
**		if (strlcat(buf, s, sizeof(buf)) >= sizeof(buf))
**			goto overflow;
**
**	Parameters:
**		dst -- nul-terminated destination string buffer
**		src -- nul-terminated source string
**		size -- size of destination buffer
**
**	Returns:
**		total length of the string tried to create
**		(= initial length of dst + length of src)
*/

size_t
strlcat(char *dst, char *src, size_t size)
{
	size_t i, j, o;

	if (src == NULL)
		return 0;
	if (dst == NULL)
		return 0;
	o = strlen(dst);
	if (size < o + 1)
		return o + strlen(src);
	size -= o + 1;
	for (i = 0, j = o; i < size && (dst[j] = src[i]) != 0; i++, j++)
		continue;
	dst[j] = '\0';
	if (src[i] == '\0')
		return j;
	else
		return j + strlen(src + i);
}
#endif /* ! HAVE_STRLCAT */

