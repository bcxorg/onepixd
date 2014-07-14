#include "onepixd.h"

const FORM_FLAGS_T Form_Flags[] = {
	{ FORM_FLAG_TOT, "tot", "total" },
	{ FORM_FLAG_S00, "s00", "sum in [0]" },
	{ FORM_FLAG_S01, "s01", "sum in [1]" },
	{ FORM_FLAG_S02, "s02", "sum in [2]" },
	{ FORM_FLAG_S03, "s03", "sum in [3]" },
	{ FORM_FLAG_S04, "s04", "sum in [4]" },
	{ FORM_FLAG_S05, "s05", "sum in [5]" },
	{ FORM_FLAG_S06, "s06", "sum in [6]" },
	{ FORM_FLAG_S07, "s07", "sum in [7]" },
	{ FORM_FLAG_S08, "s08", "sum in [8]" },
	{ FORM_FLAG_S09, "s09", "sum in [9]" },
	{ FORM_FLAG_S10, "s10", "sum in [10]" },
	{ FORM_FLAG_S11, "s11", "sum in [11]" },
	{ FORM_FLAG_S12, "s12", "sum in [12]" },
	{ FORM_FLAG_S13, "s13", "sum in [13]" },
	{ FORM_FLAG_S14, "s14", "sum in [14]" },
	{ FORM_FLAG_S15, "s15", "sum in [15]" },
	{ FORM_FLAG_P00, "p00", "% in [0]" },
	{ FORM_FLAG_P01, "p01", "% in [1]" },
	{ FORM_FLAG_P02, "p02", "% in [2]" },
	{ FORM_FLAG_P03, "p03", "% in [3]" },
	{ FORM_FLAG_P04, "p04", "% in [4]" },
	{ FORM_FLAG_P05, "p05", "% in [5]" },
	{ FORM_FLAG_P06, "p06", "% in [6]" },
	{ FORM_FLAG_P07, "p07", "% in [7]" },
	{ FORM_FLAG_P08, "p08", "% in [8]" },
	{ FORM_FLAG_P09, "p09", "% in [9]" },
	{ FORM_FLAG_P10, "p10", "% in [10]" },
	{ FORM_FLAG_P11, "p11", "% in [11]" },
	{ FORM_FLAG_P12, "p12", "% in [12]" },
	{ FORM_FLAG_P13, "p13", "% in [13]" },
	{ FORM_FLAG_P14, "p14", "% in [14]" },
	{ FORM_FLAG_P15, "p15", "% in [15]" },
	{ FORM_FLAG_LAST, NULL, "total" },
};

int
html_flag_getindex(char *name)
{
	int i;

	if (name == NULL)
		return -1;
	for (i = 0; i < FORM_FLAG_LAST; i++)
	{
		if (strcasecmp(name, Form_Flags[i].name) == 0)
			return i;
	}
	return -1;
}

char *
html_flag_printfromindex(int index)
{
	if (index < 0)
		index = 0;
	if (index >= FORM_FLAG_LAST)
		index = FORM_FLAG_LAST -1;
	return Form_Flags[index].printname;
}

char *
html_row_time(time_t time)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	time_t t;
	char nowbuf[128];
	char *n;
	struct tm tmbuf;

	t = time;

	(void) pthread_mutex_lock(&mutex);
	 (void) localtime_r(&t, &tmbuf);
	 (void) strftime(nowbuf, sizeof nowbuf, "%b %e %Y %T", &tmbuf);
	 n = str_dup(nowbuf, __FILE__, __LINE__);
	(void) pthread_mutex_unlock(&mutex);
	return n;
}

static char *
html_now()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	time_t t;
	char nowbuf[128];
	char *n;
	struct tm tmbuf;

	(void) time(&t);

	(void) pthread_mutex_lock(&mutex);
	 (void) localtime_r(&t, &tmbuf);
	 (void) strftime(nowbuf, sizeof nowbuf, "%a, %e %b %Y %T GMT", &tmbuf);
	 n = str_dup(nowbuf, __FILE__, __LINE__);
	(void) pthread_mutex_unlock(&mutex);
	return n;
}

void
html_prefix_len(char *type, char *code, size_t size, char *buf, size_t buflen)
{
	char xbuf[512];
	char nbuf[64];
	char *n;

	if (buf == NULL || buflen == 0)
	{
		buf = xbuf;
		buflen = sizeof xbuf;
	}
	if (code == NULL)
		code = "200 Success";

	(void) memset(buf, '\0', buflen);
	(void) strlcpy(buf, "HTTP/1.1 ", buflen);
	(void) strlcat(buf, code, buflen);
	(void) strlcat(buf, "\r\n", buflen);

	n = html_now();
	(void) strlcat(buf, "Date: ", buflen);
	(void) strlcat(buf, n, buflen);
	(void) strlcat(buf, "\r\n", buflen);
	n = str_free(n, __FILE__, __LINE__);

	(void) strlcat(buf, "Server: onepixd/", buflen);
	(void) strlcat(buf, VERSION, buflen);
	(void) strlcat(buf, " (Unix)\r\n", buflen);
	(void) strlcat(buf, "Connection: Close\r\n", buflen);
	(void) strlcat(buf, "Cache-Control: max-age=0, must-revalidate\r\n", buflen);
	(void) strlcat(buf, "Content-Type: ", buflen);
	(void) strlcat(buf, type, buflen);
	(void) strlcat(buf, "\r\n", buflen);
	if (size > 0)
	{
		(void) strlcat(buf, "Content-Length: ", buflen);
		(void) str_ultoa((int)size, nbuf, sizeof nbuf);
		(void) strlcat(buf, nbuf, buflen);
		(void) strlcat(buf, "\r\n", buflen);
	}
	(void) strlcat(buf, "\r\n", buflen);
	return;
}


int
html_out(FILE *fp, char *code, char *type, char *msg)
{
	char obuf[BUFSIZ];

	if (fp == NULL)
		return errno = EINVAL;
	if (msg == NULL)
		return errno = EINVAL;

	(void) html_prefix_len(type, code, strlen(msg), obuf, sizeof obuf);
	(void) fprintf(fp, "%s%s", obuf, msg);
	return errno;
}

void
html_insert_script(char *hout, size_t hlen)
{
	(void) strlcat(hout, "<script type=\"text/javascript\">\n", hlen);
	(void) strlcat(hout, "function display_c(){\n", hlen);
	(void) strlcat(hout, "var refresh=1000;\n", hlen);
	(void) strlcat(hout, "mytime=setTimeout('display_ct()',refresh)\n", hlen);
	(void) strlcat(hout, "}\n", hlen);
	(void) strlcat(hout, "function display_ct() {\n", hlen);
	(void) strlcat(hout, "var strcount\n", hlen);
	(void) strlcat(hout, "var x = new Date()\n", hlen);
	(void) strlcat(hout, "document.getElementById('ct').innerHTML = x;\n", hlen);
	(void) strlcat(hout, "tt=display_c();\n", hlen);
	(void) strlcat(hout, "}\n", hlen);
	(void) strlcat(hout, "</script>\n", hlen);
}
void
html_body(char *hout, size_t hlen)
{
	char *date;
	time_t t;

	(void) strlcat(hout, "<html>\r\n", hlen);
	(void) strlcat(hout, "<head>\r\n", hlen);
	(void) strlcat(hout, "<title>onepixd ", hlen);
	(void) strlcat(hout, VERSION, hlen);
	(void) strlcat(hout, " Data Report Interface</title>\r\n", hlen);
	html_insert_script(hout, hlen);
	(void) strlcat(hout, "</head>\r\n", hlen);
	(void) strlcat(hout, "<body bgcolor=\"lightgreen\" onload=display_ct();>\r\n", hlen);
	(void) strlcat(hout, "<table align=\"center\" border=\"0\"><tr><td align=\"center\"><font size=\"+3\">\n", hlen);
	(void) strlcat(hout, "Onepixd ", hlen);
	(void) strlcat(hout, VERSION, hlen);
	t = time(NULL);
	(void) strlcat(hout, " Data Report Interface</font><br>Server Snapshot Time: ", hlen);
	date = html_row_time(t);
	(void) strlcat(hout, date, hlen);
	date = str_free(date, __FILE__, __LINE__);
	(void) strlcat(hout, "<br>Local Time Now: <span id='ct' ></span>\r\n", hlen);
	(void) strlcat(hout, "</td></tr></table><p>\n", hlen);
	return;
}

void
html_endofbody(char *hout, size_t hlen)
{
	(void) strlcat(hout, "</body>\r\n", hlen);
	(void) strlcat(hout, "</html>\r\n", hlen);
	return;
}

char *
html_path_to_datetime(char *path)
{
	char *retp;
	char *cp;

	if (path == NULL)
		return NULL;
	retp = str_alloc(20, 1, __FILE__, __LINE__);
	if (retp == NULL)
		return NULL;
	(void) strlcpy(retp, path, 20);
	cp = strrchr(retp, '/');
	if (cp != NULL)
		*cp = ' ';
	(void) strlcat(retp, ":00:00", 20);
	return retp;
}

static int
html_dehexify(char *p)
{
	unsigned int num = 0;

	if (p == NULL)
		return -1;

	if (*p == '\0' || *(p+1) == '\0')
		return -1;

	switch ((int)*p)
	{
		case '0': num = 0; break;
		case '1': num = 0x10; break;
		case '2': num = 0x20; break;
		case '3': num = 0x30; break;
		case '4': num = 0x40; break;
		case '5': num = 0x50; break;
		case '6': num = 0x60; break;
		case '7': num = 0x70; break;
		case '8': num = 0x80; break;
		case '9': num = 0x90; break;
		case 'a': num = 0xa0; break;
		case 'A': num = 0xa0; break;
		case 'b': num = 0xb0; break;
		case 'B': num = 0xb0; break;
		case 'c': num = 0xc0; break;
		case 'C': num = 0xc0; break;
		case 'd': num = 0xd0; break;
		case 'D': num = 0xd0; break;
		case 'e': num = 0xe0; break;
		case 'E': num = 0xe0; break;
		case 'f': num = 0xf0; break;
		case 'F': num = 0xf0; break;
		default: return -1;
	}
	switch ((int)*(p+1))
	{
		case '0': num |= 0; break;
		case '1': num |= 0x01; break;
		case '2': num |= 0x02; break;
		case '3': num |= 0x03; break;
		case '4': num |= 0x04; break;
		case '5': num |= 0x05; break;
		case '6': num |= 0x06; break;
		case '7': num |= 0x07; break;
		case '8': num |= 0x08; break;
		case '9': num |= 0x09; break;
		case 'a': num |= 0x0a; break;
		case 'A': num |= 0x0a; break;
		case 'b': num |= 0x0b; break;
		case 'B': num |= 0x0b; break;
		case 'c': num |= 0x0c; break;
		case 'C': num |= 0x0c; break;
		case 'd': num |= 0x0d; break;
		case 'D': num |= 0x0d; break;
		case 'e': num |= 0x0e; break;
		case 'E': num |= 0x0e; break;
		case 'f': num |= 0x0f; break;
		case 'F': num |= 0x0f; break;
		default: return -1;
	}
	return num;
}

unsigned int
html_unpercenthex(char *buf, unsigned int size)
{
	char *op, *ip, *ep;
	int err;

	if (buf == NULL || size == 0)
		return size;

	op = buf;
	ep = buf + size;

	for (ip = buf; ip < ep; ++ip)
	{
		if (*ip == '%' && (ep - ip) > 3)
		{
			err = html_dehexify(ip+1);
			if (err >= 0)
			{
				*op++ = err;
				ip += 2;
				continue;
			}
		}
		*op++ = *ip;
	}
	*op = '\0';
	return (op - buf);
}

char *
html_month(char *numstr, int mon)
{
	static char *mons[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	int i;

	if (mon < 0)
		mon = 0;
	if (mon > 0)
	{
		if (mon > 11)
			mon = 11;
		return mons[mon];
	}
	if (numstr == NULL)
		return mons[0];
	i = strtoul(numstr, NULL, 10);
	if (i < 0)
		i = 0;
	if (i > 11)
		i = 11;
	return mons[i];
}

typedef struct {
	int secs;
	char *text;
} WIND_T;

void
html_form_send(FORMS_T *formp, char *hout, size_t houtlen, char *error)
{
	char **	ary;
	struct tm tm_now;
	time_t	now;
	char	nbuf[64];
	int	i, j, k;
	int     daymo[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int	days;
	time_t  window;
	WIND_T  win[] = {
		{ 60, "1 min" },
		{ 300, "5 min" },
		{ 600, "10 min" },
		{ 900, "15 min" },
		{ 1800, "30 min" },
		{ 3600, "1 hour" },
		{ 2*3600, "2 hour" },
		{ 6*3600, "6 hour" },
		{ 12*3600, "12 hour" },
		{ 24*3600, "1 day" },
		{ 7*24*3600, "1 week" },
		{ 30*24*3600, "1 month" },
		{ 0, NULL },
	};
	WIND_T  *winp;

	(void) time(&now);
	(void) localtime_r(&now, &tm_now);

	(void) strlcat(hout, "<form action=\"\" method=\"POST\">\n", houtlen);

	/*
	 * If any user/passphrase is required always promt for it.
	 */
	ary = config_lookup(CONF_DATA_PASSPHRASE);
	if (ary != NULL)
	{
		(void) strlcat(hout, "<table border=\"0\" width=\"95%\" align=\"center\">\n", houtlen);
		(void) strlcat(hout, "<tr>\n", houtlen);
		(void) strlcat(hout, "<td><p><input type=\"text\" size=\"20\" maxlength=\"128\" name=\"user\" value=\"", houtlen);
		(void) strlcat(hout, formp->user, houtlen);
		(void) strlcat(hout, "\"> User Name</td>\n", houtlen);
		(void) strlcat(hout, "<td><p>Pass Phrase <input type=\"password\" size=\"20\" maxlength=\"128\" name=\"pw\" value=\"", houtlen);
		(void) strlcat(hout, formp->passwd, houtlen);
		(void) strlcat(hout, "\"></td>\n", houtlen);
		(void) strlcat(hout, "<td align=\"right\"><input style=\"background-color:white\" type=\"submit\" name=\"submit\" value=\"submit\" ></td>\n", houtlen);
		(void) strlcat(hout, "</tr>\n", houtlen);
		if (error != NULL)
		{
			struct timespec  rqtp; 
			(void) strlcat(hout, "<p><font color=\"red\">", houtlen);
			(void) strlcat(hout, error, houtlen);
			(void) strlcat(hout, "</font>\n", houtlen);
			for (i = 0; i < 15; i++)
			{
				rqtp.tv_sec  = 1;
				rqtp.tv_nsec = 0;
				(void) nanosleep(&rqtp, NULL);
			}
			html_endofbody(hout, houtlen);
			return;
		}
	}
	else
		(void) strlcat(hout, "<table border=\"0\" width=\"95%\" align=\"center\">\n", houtlen);

	/*
	** Prompt for the start and end times and the window size
	*/
	(void) strlcat(hout, "<tr>\n", houtlen);
	(void) strlcat(hout, "<td><p>", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"startmonth\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->start.mon[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_mon, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(tm_now.tm_mon, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->start.mon, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		(void) strlcat(hout, html_month(nbuf, 0), houtlen);
		(void) strlcat(hout, "\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		days = daymo[j];
		for (i = 0; i < 12; i++)
		{
			if (i == j)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(i+1, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, html_month(NULL, i), houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"startday\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->start.day[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_mday, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(tm_now.tm_mday, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->start.day, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		for (i = 1; i <= days; i++)
		{
			if (i == j)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"startyear\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->start.year[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_year + 1900, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(tm_now.tm_year + 1900, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->start.year, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\n", houtlen);
		for (i = -1; i < 2; i++)
		{
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(tm_now.tm_year + 1900 + i, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"starthour\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->start.hour[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_hour, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(formp->start.tm.tm_hour, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->start.hour, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		if (strlen(nbuf) == 1)
			(void) strlcat(hout, "0", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, ":00\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		for (i = 0; i < 24; i++)
		{
			if (i == j)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			if (strlen(nbuf) == 1)
				(void) strlcat(hout, "0", houtlen);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, ":00\n", houtlen);
		}
		(void) strlcat(hout, "</select> Start At\n", houtlen);

	(void) strlcat(hout, "</td>\n", houtlen);
	(void) strlcat(hout, "<td><p>End At \n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"endmonth\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->end.mon[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_mon, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(formp->end.tm.tm_mon, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->end.mon, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		(void) strlcat(hout, html_month(nbuf, 0), houtlen);
		(void) strlcat(hout, "\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		days = daymo[j];
		for (i = 0; i < 12; i++)
		{
			if (i == j)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(i+1, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, html_month(NULL, i), houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"endday\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->end.day[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_mday, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(formp->end.tm.tm_mday, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->end.day, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		for (i = 1; i <= days; i++)
		{
			if (i == j)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"endyear\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->end.year[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_year + 1900, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(formp->end.tm.tm_year + 1900, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->end.year, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\n", houtlen);
		for (i = -1; i < 2; i++)
		{
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(tm_now.tm_year + 1900 + i, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"endhour\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->end.hour[0] == '\0')
		{
			(void) str_ultoa(tm_now.tm_hour, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
		}
		else
		{
			(void) str_ultoa(formp->end.tm.tm_hour, nbuf, sizeof nbuf);
			(void) strlcat(hout, formp->end.hour, houtlen);
		}
		(void) strlcat(hout, "\">", houtlen);
		if (strlen(nbuf) == 1)
			(void) strlcat(hout, "0", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, ":00\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		for (i = 0; i < 24; i++)
		{
			if (i == j)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(i, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			if (strlen(nbuf) == 1)
				(void) strlcat(hout, "0", houtlen);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, ":00\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

	(void) strlcat(hout, "</td>\n", houtlen);

	(void) strlcat(hout, "<td align=\"right\"><p>Window Width \n", houtlen);
		(void) strlcat(hout, "<select style=\"background-color:white\" name=\"windowsecs\">", houtlen);
		(void) strlcat(hout, "<option value=\"", houtlen);
		if (formp->window == 0)
			window = 3600;
		else
			window = formp->window;

		(void) str_ultoa(window, nbuf, sizeof nbuf);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\">", houtlen);
		for (winp = win; winp-> secs != 0; ++winp)
		{
			if (winp->secs == window)
			{
				(void) strlcat(hout, winp->text, houtlen);
				break;
			}
		}
		(void) strlcat(hout, "\n", houtlen);
		j = strtoul(nbuf, NULL, 10);
		for (winp = win; winp-> secs != 0; ++winp)
		{
			if (winp->secs == window)
				continue;
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) str_ultoa(winp->secs, nbuf, sizeof nbuf);
			(void) strlcat(hout, nbuf, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, winp->text, houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "</select>\n", houtlen);

	(void) strlcat(hout, "</td>\n", houtlen);
	(void) strlcat(hout, "</tr>\n", houtlen);
	(void) strlcat(hout, "</table>\n", houtlen);

	/*
	 * Draw the columns.
	 */
	(void) strlcat(hout, "<p>\n", houtlen);
	(void) strlcat(hout, "<table align=\"center\" border=\"0\" width=\"95%\">\n", houtlen);
	(void) strlcat(hout, "<tr>\n", houtlen);
	for (i = 0; i < formp->have_cols.nary; i++)
	{
		int j;

		(void) str_ultoa(i, nbuf, sizeof nbuf);
		(void) strlcat(hout, "<td align=\"right\">\n[", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "] <select style=\"background-color:white\" name=\"col", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\">\n", houtlen);
		if (i < formp->selected_cols.nary && *(formp->selected_cols.ary[i]) != '\0') 
		{
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) strlcat(hout, formp->selected_cols.ary[i], houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, formp->selected_cols.ary[i], houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		else
			(void) strlcat(hout, "<option value=\"none\">no column", houtlen);
		for (j = 0; j < formp->have_cols.nary; j++)
		{
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) strlcat(hout, formp->have_cols.ary[j], houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, formp->have_cols.ary[j], houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		if (i < formp->selected_cols.nary && *(formp->selected_cols.ary[i]) != '\0')
			(void) strlcat(hout, "<option value=\"none\">no column", houtlen);
		(void) strlcat(hout, "</select>\n", houtlen);

		(void) str_ultoa(i, nbuf, sizeof nbuf);
		(void) strlcat(hout, "<br><select style=\"background-color:white\" name=\"flag", houtlen);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\">\n", houtlen);
		if (formp->flag[i] != FORM_FLAG_TOT && i >= 0 && i < FORM_FLAG_LAST)
		{
			(void) strlcat(hout, "<option value=\"", houtlen);
			(void) strlcat(hout, Form_Flags[formp->flag[i]].name, houtlen);
			(void) strlcat(hout, "\">", houtlen);
			(void) strlcat(hout, Form_Flags[formp->flag[i]].printname, houtlen);
			(void) strlcat(hout, "\n", houtlen);
		}
		(void) strlcat(hout, "<option value=\"tot\">total\n", houtlen);
		for (j = 1; j <= formp->have_cols.nary; j++)
		{
			if (formp->flag[i] != j)
			{
				(void) strlcat(hout, "<option value=\"", houtlen);
				(void) strlcat(hout, Form_Flags[j].name, houtlen);
				(void) strlcat(hout, "\">", houtlen);
				(void) strlcat(hout, Form_Flags[j].printname, houtlen);
				(void) strlcat(hout, "\n", houtlen);
			}

			k = j + 16;
			if (formp->flag[i] != k && k < FORM_FLAG_LAST)
			{
				(void) strlcat(hout, "<option value=\"", houtlen);
				(void) strlcat(hout, Form_Flags[k].name, houtlen);
				(void) strlcat(hout, "\">", houtlen);
				(void) strlcat(hout, Form_Flags[k].printname, houtlen);
				(void) strlcat(hout, "\n", houtlen);
			}
		}
		(void) strlcat(hout, "</select>\n", houtlen);
		(void) strlcat(hout, "<br><input type=\"text\" size=\"11\" maxlength=\"127\" name=\"match", houtlen);
		(void) str_ultoa(i, nbuf, sizeof nbuf);
		(void) strlcat(hout, nbuf, houtlen);
		(void) strlcat(hout, "\" value=\"", houtlen);
		if (formp->match[i][0] != '\0')
			(void) strlcat(hout, formp->match[i], houtlen);
		else
			(void) strlcat(hout, "nomatch", houtlen);
		(void) strlcat(hout, "\" OnFocus=\"this.value=''\">\n", houtlen);

		(void) strlcat(hout, "</td>\n", houtlen);
	}
	(void) strlcat(hout, "</tr>\n", houtlen);
	(void) strlcat(hout, "</table>\n", houtlen);
	(void) strlcat(hout, "</form>\n", houtlen);
	
}
