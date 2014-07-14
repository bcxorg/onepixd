#ifndef DAOPGD_H
#define DAOPGD_H

#include "config.h"

#if HAVE_STDIO_H
# include <stdio.h>
#endif

#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_ERRNO_H
# include <errno.h>
#endif

#if HAVE_SIGNAL_H
# include <signal.h>
#endif

#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

#if HAVE_STDARG_H 
# include <stdarg.h>
#endif

#if HAVE_LIBGEN_H  
# include <libgen.h> 
#endif

#if HAVE_STDLIB_H  
# include <stdlib.h> 
#endif

#if HAVE_STRING_H  
# include <string.h>
#endif

#if HAVE_POLL_H
# include <poll.h>
#endif

#if HAVE_STRINGS_H
# include <strings.h>
#endif

#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#if HAVE_SYS_INTTYPES_H
# include <sys/inttypes.h>
#endif

#if HAVE_TIME_H
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
#endif

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_UTIME_H
# include <utime.h>
#endif
#ifndef ULONG_MAX
# define ULONG_MAX ((unsigned long)(~0))
#endif
#ifndef UCHAR_MAX
# define UCHAR_MAX ((unsigned char)(~0))
#endif
#ifndef CHAR_BIT
# define CHAR_BIT (8)
#endif

#if HAVE_CTYPE_H
# include <ctype.h>
#endif

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#if HAVE_IDNA_H
# include <idna.h>
#endif

#if HAVE_PTHREAD_H
# include <pthread.h>
#endif

#if HAVE_SYSLOG_H
# include <syslog.h>
#endif
#if HAVE_DIRENT_H
# include <dirent.h>
#endif
#if HAVE_ASSERT_H
# include <assert.h>
#endif
#if HAVE_NETDB_H
# include <netdb.h>
#endif
#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#if HAVE_NETINET_IN_H
# include <netinet/tcp.h>
#endif
#if HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#if HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#if HAVE_RESOLV_H
# include <resolv.h>
#endif
#if HAVE_PWD_H
# include <pwd.h>
#endif

#if HAVE_RESOURCE_H 
# include <resource.h>
#endif
#if HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#if HAVE_OPENSSL_ASN1_H
# include <openssl/asn1.h>
#endif

#if HAVE_OPENSSL_BIO_H
# include <openssl/bio.h>
#endif

#if HAVE_OPENSSL_ERR_H
# include <openssl/err.h>
#endif

#if HAVE_OPENSSL_EVP_H
# include <openssl/evp.h>
#endif

#if HAVE_OPENSSL_SHA_H
# include <openssl/sha.h>
#endif

#if HAVE_OPENSSL_SSL_H
# include <openssl/ssl.h>
#endif

#if HAVE_OPENSSL_X509V3_H
# include <openssl/x509v3.h>
#endif

#if HAVE_OPENSSL_RAND_H
# include <openssl/rand.h>
#endif

#if HAVE_OPENSSL_OPENSSLV_H
# include <openssl/opensslv.h>
#endif

#if HAVE_OPENSSL_PEM_H
# include <openssl/pem.h>
#endif

#if HAVE_OPENSSL_RSA_H
# include <openssl/rsa.h>
#endif

#if HAVE_OPENSSL_MD5_H
# include <openssl/md5.h>
#endif

#if ! HAVE_OPENSSL_SHA_H
# error Openssl  must be available to build
#endif


#ifndef TRUE
# define TRUE (1)
#endif
#ifndef FALSE
# define FALSE (0)
#endif
#ifndef MAYBE
# define MAYBE (-1)
# define NEITHER MAYBE
#endif
#ifndef TEMPFAIL
# define TEMPFAIL (-2)
#endif
#ifndef ERRPARAM
# define ERRPARAM (-3)
#endif
 
#define MAX_COLUMN_NAME_LEN	(128)
#define MAX_COLUMNS		(16)
#define MAX_COL_FILES_PER_HR	(8)
#define MAX_ACCII_INT		(64)
#define MAX_EBUF_LEN		(1024)
#define MAX_HTTP_QUERY		(4096)
#define MAX_THREADS		(9996)

#include "libtpool/tpool.h"
typedef struct {
	TPOOL_CTX *     tpool_ctx;
	char 		interface[MAXHOSTNAMELEN];
	char		port[MAX_ACCII_INT];
	int		timeout;
	int 		type;
	u_char * 	gifimage;	/* shared memory, don't free */
	int		gifimagelen;
	u_char *	favicon;	/* shared memory, don't free */
	int		faviconlen;
} LISTEN_CTX;
#define LISTEN_TYPE_GIF		(0)
#define LISTEN_TYPE_DATA	(1)
#define LISTEN_TYPE_UPLOAD	(2)

typedef struct {
	char ary[MAX_COLUMNS][MAX_COLUMN_NAME_LEN];
	int nary;
} FIXED_ARRAY_T;

typedef struct {
	char		path[MAX_COLUMN_NAME_LEN];
	FIXED_ARRAY_T	columns;
} PATH_COLUMNS_T;

/*****************************************************************
** Function Prototypes
*/

typedef struct {
	u_char * 	gifimage;	/* shared memory, don't free */
	int		gifimagelen;
	u_char *	favicon;	/* shared memory, don't free */
	int		faviconlen;
	int sock;
	pthread_t tid;
	char ipaddr[128];
	char port[32];
	FILE *fp;
#define THREAD_CTX_REFERERSIZE (256)
	char referer[THREAD_CTX_REFERERSIZE];
} THREAD_CTX_T;

#define MAX_GOT_NAMELEN		(255)
#define MAX_GOT_VALUELEN	(1024)
typedef struct {
	char name[MAX_GOT_NAMELEN];
	char value[MAX_GOT_VALUELEN];
} HTTP_GOT_EQUATE;

typedef struct {
	char path[MAX_COLUMN_NAME_LEN];
	HTTP_GOT_EQUATE *equatep;
	int    nequates;
	char **postp;
	int    type;
#define HTTP_GOT_REPLY_TYPE_GET  (0)
#define HTTP_GOT_REPLY_TYPE_POST (1)
} HTTP_GOT_REPLY;

/* io.c */
#define IO_OKAY		(0)
#define IO_ERROR	(1)
#define IO_SYNTAX	(2)
void  	io_init(int);
void    io_shutdown(void);
char *	io_fgets(THREAD_CTX_T *, int *, int limit, char *file, size_t line);
HTTP_GOT_REPLY *io_getcommand(THREAD_CTX_T *, char *, size_t, char *, size_t, char *file, size_t line);
char *	io_html_now(void);
void	io_html_prefix(char *, char *, char *, size_t);
int	io_html_good(THREAD_CTX_T *, char *, char *);
int	io_html_redirect(THREAD_CTX_T *ctx, char *url, char *, size_t);
int	io_bare_fwrite(THREAD_CTX_T *, char *, char *, size_t);
int	isdirok(char *dir, char *ebuf, size_t elen);

/* log.c */
int	log_check_facility(char *facility);
int	log_init(char *, char *);
void	log_emit(int, char *, char *, int, char *);

/* str.c */
char * str_squeeze(char *str, int freeflag, char *file, int line);
void * str_alloc(size_t size, size_t count, char *file, int line);
void * str_free(void *, char *, int);
char * str_dup(char *, char *, int);
void * str_realloc(void *oldptr, size_t oldsize, size_t size, char *file, int line, int flag);
char * str_ultoa(unsigned long, char *, size_t);
void   str_shutdown(void);
int    str_parse_to_ary(char *str, int ch, FIXED_ARRAY_T *cols);
PATH_COLUMNS_T *str_parse_path_column(char *str, char *file, int line);
char * strcasestr(char *hay, char *needle);
char **pushargv(char *str, char **ary, char *file, size_t line);
char * popargv(char **ary);
char **clearargv(char **ary);
#if ! HAVE_STRLCAT
size_t strlcat(char *dst, char *src, size_t size);
#endif
#if ! HAVE_STRLCPY
size_t strlcpy(char *dst, char *src, size_t size);
#endif


/* utils.c */
int    util_bg(int);
int    setrunasuser(char *);
uid_t  util_getuid(char *user);
int    write_pid_file(char *);
int    sizeofargv(char **ary);
int    parse_equal(char *string, char **namep, char **valuep);
int    truefalse(char *str);

typedef struct {
	char ** times;
	char ** values;
} COL_FILE_RET_T;

typedef struct {
	COL_FILE_RET_T d[MAX_COLUMNS];
} COL_RET_T;

typedef struct {
	struct tm tm;
	time_t	secs;
	char year[5];
	char mon[3];
	char day[3];
	char hour[3];
} FORMS_TIME_T;
typedef struct {
	char    user[MAX_GOT_NAMELEN];
	char    passwd[MAX_GOT_NAMELEN];
	FIXED_ARRAY_T have_cols;
	FIXED_ARRAY_T selected_cols;
	FORMS_TIME_T start;
	FORMS_TIME_T end;
	char	flagstr[MAX_COLUMNS][4];
	int	flag[MAX_COLUMNS];
	char	match[MAX_COLUMNS][MAX_GOT_NAMELEN];
	time_t window;
} FORMS_T;

typedef struct {
	int index;
	char *name;
	char *printname;
} FORM_FLAGS_T;

#define FORM_FLAG_TOT	(0)
#define FORM_FLAG_S00	(1)
#define FORM_FLAG_S01	(2)
#define FORM_FLAG_S02	(3)
#define FORM_FLAG_S03	(4)
#define FORM_FLAG_S04	(5)
#define FORM_FLAG_S05	(6)
#define FORM_FLAG_S06	(7)
#define FORM_FLAG_S07	(8)
#define FORM_FLAG_S08	(9)
#define FORM_FLAG_S09	(10)
#define FORM_FLAG_S10	(11)
#define FORM_FLAG_S11	(12)
#define FORM_FLAG_S12	(13)
#define FORM_FLAG_S13	(14)
#define FORM_FLAG_S14	(15)
#define FORM_FLAG_S15	(16)
#define FORM_FLAG_P00	(17)
#define FORM_FLAG_P01	(18)
#define FORM_FLAG_P02	(19)
#define FORM_FLAG_P03	(20)
#define FORM_FLAG_P04	(21)
#define FORM_FLAG_P05	(22)
#define FORM_FLAG_P06	(23)
#define FORM_FLAG_P07	(24)
#define FORM_FLAG_P08	(25)
#define FORM_FLAG_P09	(26)
#define FORM_FLAG_P10	(27)
#define FORM_FLAG_P11	(28)
#define FORM_FLAG_P12	(29)
#define FORM_FLAG_P13	(30)
#define FORM_FLAG_P14	(31)
#define FORM_FLAG_P15	(32)
#define FORM_FLAG_LAST	(33)


/* threads.c */
void *thread_listener(void*);
void *thread_handle_gif(void *, int *);
void *thread_handle_data(void *, int *);
void *thread_handle_upload(void *, int *);

/* gif.c */
int    gif_send(THREAD_CTX_T *ctx, char *type, u_char *gif, size_t size, char *ebuf, size_t elen);
u_char * gif_get1x1gif(int *len);
u_char * gif_getfavicon_ico(int *len);

typedef struct entry_bucket {
	struct entry_bucket *previous;
	struct entry_bucket *next;
	char  *key;
	void  *data;
	time_t timestamp;
} HASH_BUCKET;

typedef struct {
	HASH_BUCKET   *bucket;
	pthread_mutex_t  mutex;
} HASH_SHELF;

#define	HASH_MIN_SHELVES_LG2	4
#define	HASH_MIN_SHELVES	(1 << HASH_MIN_SHELVES_LG2)

/*
 * max * sizeof internal_entry must fit into size_t.
 * assumes internal_entry is <= 32 (2^5) bytes.
 */
#define	HASH_MAX_SHELVES_LG2	(sizeof (size_t) * 8 - 1 - 5)
#define	HASH_MAX_SHELVES	((size_t)1 << HASH_MAX_SHELVES_LG2)

typedef struct {
	HASH_SHELF *table;
	size_t         tablesize;
	void (*freefunct)(void *);
} HASH_CTX;

#define DEFAULT_HASH_TABLESIZE	(2048)

HASH_CTX * hash_init(size_t tablesize);
HASH_CTX * hash_shutdown(HASH_CTX *hctx);
void	   hash_set_callback(HASH_CTX *hctx, void (*callback)(void *));
void *     hash_lookup(HASH_CTX *hctx, char *string, void *data, size_t datalen);
int        hash_drop(HASH_CTX *hctx, char *string);
int        hash_expire(HASH_CTX *hctx, time_t age);


void       conf_datafree(void *data);
HASH_CTX * conf_updateconfig(HASH_CTX *rctx, char *name, char *value);
HASH_CTX * conf_init(HASH_CTX *cctx, char *path, char *ebuf, size_t elen);
char **    conf_getval(HASH_CTX *cctx, char *val);
HASH_CTX * conf_shutdown(HASH_CTX *cctx);
void       conf_drop(HASH_CTX *rctx, char *name);

typedef struct {
        char **ary;
	int    num;
} CONF_ARGV_CTX;

CONF_ARGV_CTX * conf_initav(void);
CONF_ARGV_CTX * conf_pushav(char *str, CONF_ARGV_CTX *avp, char *file, int line);
CONF_ARGV_CTX * conf_freeav(CONF_ARGV_CTX *avp);
char **         conf_freenargv(char **ary, int *num);
char **         conf_pushnargv(char *str, char **ary, int *num, char *file, int line);

/* readconf.c */
int     config_read_file(char *path);
char ** config_lookup(char *str);
int     config_dir_must_exist(char *fname, char *ebuf, size_t elen);
int     config_validate(void);
void	config_shutdown(void);

#define CONF_HOME_DIR		"home_dir"
#define DEFAULT_THREADS		(100)
#define CONF_NUMTHREADS		"numthreads"
#define CONF_BECOME_USER	"become_user"
#define CONF_PIDFILE		"pidfile"
#define DEFAULT_LOG_FACILITY	"mail"
#define CONF_LOG_FACILITY	"log_facility"
#define DEFAULT_DISK_ARCHIVE_DAYS	"30"
#define CONF_DISK_ARCHIVE_DAYS	"disk_archive_days"
#define CONF_DISK_ARCHIVE_PATH	"disk_archive_path"
#define DEFAULT_DISK_ARCHIVE_PATH	"data"
#define CONF_SEND_404_ON_ERROR	"send_404_on_error"

#define CONF_GIF_INTERFACE	"gif_interface"
#define CONF_GIF_PORT		"gif_port"
#define CONF_GIF_TIMEOUT	"gif_timeout"
#define CONF_GIF_FILE		"gif_file"
#define CONF_GIF_PATH_COLUMNS	"gif_path_columns"

#define CONF_DATA_INTERFACE	"data_interface"
#define CONF_DATA_PORT		"data_port"
#define CONF_DATA_TIMEOUT	"data_timeout"
#define CONF_DATA_PASSPHRASE	"data_passphrase"

#define CONF_UPLOAD_INTERFACE	"upload_interface"
#define CONF_UPLOAD_PORT	"upload_port"
#define CONF_UPLOAD_TIMEOUT	"upload_timeout"
#define CONF_UPLOAD_PATH_COLUMN	"upload_path_columns"
#define CONF_UPLOAD_PUBKEY	"upload_pubkey"
#define CONF_UPLOAD_POST_COLS	"cols"
#define CONF_UPLOAD_POST_ROWS	"rows"
#define CONF_UPLOAD_POST_DATUM	"datum"
#define CONF_UPLOAD_POST_SIG	"sig"

/* in debug.c */
#define BUG_NONE	(0)
#define BUG_CONF	(0x0001)	/* debug config.c   */
#define BUG_THREADS	(0x0002)	/* debug threads.c  */
#define BUG_APP		(0x0004)	/* debug main()     */
#define BUG_LOGS	(0x0008)	/* debug log.c      */
#define BUG_GIF		(0x0010)	/* debug gif.c      */
#define BUG_IO		(0x0020)	/* debug io.c       */
#define BUG_ALL		(0xffff)	/* debug everything */
#define BUG_ANY		BUG_ALL
void setdebug(int fac);
void cleardebug(int fac);
void zerodebug(void);
int  isdebug(int fac);


typedef struct _server_t {
	pthread_t		 tid;
	pthread_mutex_t		 mutex;
	pthread_cond_t		 cond;
	struct _gm_tpool_ctx	*tctx;
	int			 run;
	int			 kind;
	int			 die;
} SERVER_T;

#define QUERY_NONE (0)
#define QUERY_HTML (1)
#define QUERY_XML  (2)
#define QUERY_CSV  (2)
 
#define UNITS_HOURS     (0)
#define UNITS_DAYS      (1)
#define UNITS_MONTHS    (2) 
#define UNITS_YEARS     (3)
	 
#define MODE_LIST       (0)
#define MODE_TOTAL      (1)
			 
typedef struct {
	int query;
	int units;
	int limit;
	int mode;
	int dupes;
	char columns[BUFSIZ];
} QUERY_CTX;

/* files.c */
char *       file_time_to_day(time_t when, char *ebuf, size_t elen);
int          file_write_datum(time_t when, char *fname, char *datum, char *ebuf, size_t elen);
void         file_write_array(char **ary, char *row_name);
char **	     file_help(char *ip);
char **      file_fetch_data(QUERY_CTX *qctx);
time_t	     file_path_to_time(char *path);
void	     file_prune_garbage(int *shutdown);
COL_RET_T *  file_fetch_col_files(FORMS_T *formp);
COL_RET_T *  file_colret_free(COL_RET_T *colret);

/* html.c */
char *	     html_row_time(time_t time);
int          html_flag_getindex(char *name);
char *       html_flag_printfromindex(int index);
void         html_prefix_len(char *type, char *code, size_t size, char *buf, size_t buflen);
int          html_out(FILE *fp, char *code, char *type, char *msg);
void         html_body(char *hout, size_t hlen);
void 	     html_endofbody(char *hout, size_t hlen);
char *       html_path_to_datetime(char *path);
unsigned int html_unpercenthex(char *buf, unsigned int size);   
void	     html_form_send(FORMS_T *formp, char *hout, size_t houtlen, char *errror);

/* xml.c */
void xml_header(char *x, size_t xlen);
void xml_trailer(char *x, size_t slen);
char * xml_path_to_datetime(char *path);

/* verify.c */
int     verify_sig(u_char *sig, size_t siglen, char **gary);
int	verify_threads_locking_init();
void	verify_threads_locking_shutdown(void);

/* base64.c */
int	base64_encode(u_char *data, size_t datalen, u_char *buf, size_t buflen);
int	base64_decode(u_char *str, u_char *buf, size_t buflen);

#endif /* DAOPGD_H */
