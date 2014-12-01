#include "xfunc.h"

char *formatstr(char *formt, ...)
{
	static char formatstrbuf[256];
	va_list va_param;
			
	va_start(va_param, formt);
	vsprintf(formatstrbuf, formt, va_param);
	va_end(va_param);

	return formatstrbuf;
}

// ���pathnameû�з�����,���,�����,�������
// ������ı�pathname,���ó���Ӧ��ȷ��pathname����һ�����п�λ
char* appendbackslash(const char* pathname)
{
	static char		gname[_MAX_PATH];
	size_t			len = strlen(pathname);

	strcpy(gname, pathname);
	if (gname[len - 1] != '\\') {
		gname[len] = '\\';
		gname[len + 1] = 0;
	}
	return gname;
}

char* dirname(const char* fullfilename) 
{
	static char		gname[_MAX_PATH];
	const char			*p, *p1;

	p = p1 = fullfilename;
	while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '\\');
	}

	if (p) {
		strncpy(gname, fullfilename, p - fullfilename);
		gname[p - fullfilename] = 0;
		if (gname[1] != ':') {
			gname[0] = 0;
		}
	} else {
		gname[0] = 0;
	}
	return gname[0]? gname: NULL;
}

// ����·���е��ļ�������
// ����һ��������ָ��һ���ļ���ȫ·�����ַ��������������ػ������ļ�����
char* basename(const char* fullfilename) 
{
	static char		gname[_MAX_PATH];
	const char		*p, *p1;

	if ((strlen(fullfilename) == 2) && (fullfilename[1] == ':')) {
		strcpy(gname, fullfilename);
		return gname;
	}

	p = p1 = fullfilename;
	while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '\\');
	}

	if (p + 1) {
		strcpy(gname, p + 1);
	} else {
		gname[0] = 0;
	}
	return gname[0]? gname: NULL;
}

// ����ֵ:
//  ���᷵��NULL,��strcasecmpҪ����������NULL�ᱨ�Ƿ������ó��򾭳�����ֱ�����⺯������ֵȥ�Ƚ�
char* extname(const char* filename)
{
	static char		gname[_MAX_EXTNAME + 1];
	const char		*p, *p1;

	gname[0] = 0;

	p = p1 = filename;
	while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '\\');
	}
	// p����ָ���˶��ļ�ʱ���ļ�,���ļ���ʱ��'\'
	if (*p == '\\') {
		p = p + 1;
	}
	if (*p == 0) {
		return gname;
	}
	p1 = p;	// ���ļ����п��ܺ��ж��'.'������image.2510.img���˴�ֻ���������һ��'.'�����ݣ�img
	while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '.');
	}
	// p����ָ�������һ��'.',��ָ���ַ�����β��'\0'
	if (*p != '.') {	// ������ļ����о�û�е�
		// �������ļ����ģ�ֻ��û����չ��
		gname[0] = 0;
	} else {
		if (strlen(p + 1) > _MAX_EXTNAME) {
			return gname;	// ��չ������̫������Ϊ��Ч	
		}
		strcpy(gname, p + 1);	// p1ָ��'.'
	}

	return gname;
}


// 111222,333 KB
char *format_len_u32n(uint32_t len)
{
	static char		fmtstr[20];
	uint32_t		integer_K;

	integer_K = len / CONSTANT_1K + ((len % CONSTANT_1K)? 1: 0);
	if (integer_K >= 1000) {
		sprintf(fmtstr, "%u,%03u", integer_K / 1000, integer_K % 1000);
	} else {
		sprintf(fmtstr, "%u", integer_K);
	}

	return fmtstr;
}
// 111222,333 KB
char *format_len_u64n(uint64_t len)
{
	static char		fmtstr[20];
	uint32_t		integer_K;

	integer_K = (uint32_t)(len / CONSTANT_1K + ((len % CONSTANT_1K)? 1: 0));
	if (integer_K >= 1000) {
		sprintf(fmtstr, "%u,%03u", integer_K / 1000, integer_K % 1000);
	} else {
		sprintf(fmtstr, "%u", integer_K);
	}
	
	return fmtstr;
}

char *format_space_u64n(uint64_t space)
{
	static char		fmtstr[20];
	double			val;

	if (space > CONSTANT_1G) {
		// 4.98 GB
		val = 1.0 * space / CONSTANT_1G;
		sprintf(fmtstr, "%.02f GB", val);
	} else if (space > CONSTANT_1M) {
		// 4.98 MB
		val = 1.0 * space / CONSTANT_1M;
		sprintf(fmtstr, "%u MB", (uint32_t)(val));
	} else {
		// 4.98 KB
		val = 1.0 * space / CONSTANT_1K;
		sprintf(fmtstr, "%u KB", (uint32_t)(val));
	}
	return fmtstr;
}

// ע������������������socketĬ������Ӧ��
char *inet_u32toa(const uint32_t addr)
{
	static char szIpAddr[50];
	
	sprintf(szIpAddr, "%i.%i.%i.%i",
		((int)(addr>>24)&0xFF),
        ((int)(addr>>16)&0xFF),
        ((int)(addr>> 8)&0xFF),
        ((int)(addr>> 0)&0xFF));
	
	return szIpAddr;
}

uint32_t inet_atou32(const char *ipaddr)
{
	int			f1, f2, f3, f4, retval = 0;
	uint32_t	ipv4;

	if (ipaddr) {
		retval = sscanf(ipaddr, "%i.%i.%i.%i", &f1, &f2, &f3, &f4);
	}

	if (retval == 4) {
		ipv4 =  ((f1 & 0xFF) << 24) | ((f2 & 0xFF) << 16) | ((f3 & 0xFF) << 8) | (f4 & 0xFF); 

	} else {
		ipv4 = 0;
	}
	return ipv4;
}

char *updextname(char *filename, char *newextname)
{
	static char		gname[_MAX_PATH];
	char			*p, *p1;

	strcpy(gname, filename);
	p = p1 = gname;
    while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '\\');
	}

	// p����ָ���˶��ļ�ʱ���ļ�,���ļ���ʱ��'\'
	if (*p == '\\') {
		p = p + 1;
	}
	if (*p == 0) {
		return NULL;
	}
	p1 = strchr(p, '.');
	if (!p1) {
		// û�ҵ���չ��
		return NULL;
	}
	p1[1] = 0;	// ����չ��ÿ���ַ���Ϊ'\0'
	strcat(gname, newextname);

	return gname;
}

char *offextname(const char *filename)
{
	static char		gname[_MAX_PATH];
	char			*p, *p1;

	strcpy(gname, filename);
	p = p1 = gname;
    while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '\\');
	}

	// p����ָ���˶��ļ�ʱ���ļ�,���ļ���ʱ��'\'
	if (*p == '\\') {
		p = p + 1;
	}
	if (*p == 0) {
		return NULL;
	}
	p1 = strchr(p, '.');
	if (!p1) {
		// û�ҵ���չ��
		return NULL;
	}
	p1[0] = 0;	// ����չ����'.'��Ϊ'\0'

	return gname;
}

// -1: �Ҳ�����չ��
// 0: ����չ����ͬ
int cmpextname(char *filename, char *desiredextname)
{
	char			*p, *p1;

	p = p1 = filename;
    while (p1) {
		p = p1;
		p1 = strchr(p1 + 1, '\\');
	}

	// p����ָ���˶��ļ�ʱ���ļ�,���ļ���ʱ��'\'
	if (*p == '\\') {
		p = p + 1;
	}
	if (*p == 0) {
		return -1;
	}
	p1 = strchr(p, '.');
	if (!p1) {
		// û�ҵ���չ��
		return -1;
	}
	// p1[1]����չ����һ���ַ�
	return strcmp(p1 + 1, desiredextname);
}

char *u32tostr(uint32_t u32n)
{
	static char		text[12];
	sprintf(text, "%u", u32n);
	return text;
}

uint32_t strtou32(char *str, int hex)
{
	uint32_t	u32n;
	int			retval;

	if (hex) {
		retval = sscanf(str, "%x", &u32n);
	} else {
		retval = sscanf(str, "%u", &u32n);
	}
	return (retval == 1)? u32n: 0;
}

// �޳�str��β�Ŀո��
// ����ֵ: ���޸ĵ��ַ�������
// ע:���ܸı�str����
uint32_t eliminate_terminal_blank_chars(char *str)
{
	char			*ptr = str + strlen(str);

	if (!str || !str[0]) {
		return 0;
	}
	ptr = str + strlen(str) - 1;

	while(*ptr == ' ') {
		*ptr = 0;
		if (ptr == str) {
			break;
		}
		ptr = ptr - 1;
	}
	return *str? (uint32_t)(ptr - str + 1): 0;
}

//
// remove all the blank characters except those in the double quotes
// ���ַ�(blank characters): �ո�� �Ʊ�� ע: ����˫�����ڵ�
// ע:
//  �������Ҫ��ɾ�����ַ�,string���ڴ�ᱻ�͵ظĵ�
void trim_blank_characters(char* string)
{
	uint32_t	keyval_begin = 0, inside_quote = 0;
	uint32_t	length = (uint32_t)strlen(string);
	uint32_t	i, j;

	for (i = 0 , j = 0 ; i < length ; i ++) {
		switch ( string[i] ) {
			case ' ':
			case '\t':
				if ( inside_quote == 0 ) break;
				// fall through
			default:
				if ( keyval_begin ) {
					inside_quote = 1;
				}
				if ( string[i] == '=' ) {
					keyval_begin = 1;
				}
				if ( i != j ) {
					string[j] = string[i];
				}
				j ++;
				break;
		}
	}
	string[j] = 0;
}

//
// ɾ��ע�ͷ���ע�ͷ���������ַ�(�ڵ�һ��/�ַ������ַ���ֵ����0)
// ע�ͷ�(blank characters): //  ע: ����˫�����ڵ�
// 
void trim_comments(char* string)
{
	uint32_t	inside_quote = 0;
	uint32_t	backslash_count = 0;
	uint32_t	length = (uint32_t)strlen(string);
	uint32_t	i;

	for (  i = 0 ; i < length ; i ++ ) {
		switch (string[i]) {
			case '/':
				if (inside_quote == 0) backslash_count ++;
				if (backslash_count == 2) {
					string[i-1] = 0;
					return;
				}
				break;				
			case '"':
				inside_quote = 1 - inside_quote;
				break;
			default:
				backslash_count = 0;
				break;
		}
	}
}

#define COMPARE_BLOCK_SIZE	CONSTANT_64M

bool file_compare(char* fname1, char*fname2)
{
	posix_file_t fp1 = INVALID_FILE, fp2 = INVALID_FILE;
	uint32_t bytertd1, bytertd2, fsizelow1, fsizehigh1, fsizelow2, fsizehigh2;
	uint64_t pos, fsize1, fsize2;
	uint8_t *data1 = NULL, *data2 = NULL;
	bool equal = false;

	posix_fopen(fname1, GENERIC_READ, OPEN_EXISTING, fp1);
	if (fp1 == INVALID_FILE) {
		posix_print_mb("cann't open file: %s", fname1);
		goto exit;
	}

	posix_fopen(fname2, GENERIC_READ, OPEN_EXISTING, fp2);
	if (fp2 == INVALID_FILE) {
		posix_print_mb("cann't open file: %s", fname2);
		goto exit;
	}

	posix_fsize(fp1, fsizelow1, fsizehigh1);
	fsize1 = posix_mku64(fsizelow1, fsizehigh1);
	posix_fsize(fp2, fsizelow2, fsizehigh2);
	fsize2 = posix_mku64(fsizelow2, fsizehigh2);

	if (fsize1 != fsize2) {
		posix_print_mb("file size is different");
		goto exit;
	}

	data1 = (uint8_t*)malloc(COMPARE_BLOCK_SIZE);
	if (!data1) {
		goto exit;
	}
	data2 = (uint8_t*)malloc(COMPARE_BLOCK_SIZE);
	if (!data2) {
		goto exit;
	}

	pos = 0;
	posix_print("file1: %s, file2: %s, compare size: %08xh\n", fname1, fname2, fsizelow1);
	while (pos < fsize1) {
		posix_print("compare address: %08xh\n", pos);
		posix_fread(fp1, data1, COMPARE_BLOCK_SIZE, bytertd1);
		posix_fread(fp2, data2, COMPARE_BLOCK_SIZE, bytertd2);
		if (bytertd1 != bytertd2) {
			goto exit;
		}
		if (memcmp(data1, data2, bytertd1)) {
			posix_print_mb("S: %08xh, detect different data", pos);
			goto exit;
		}
		pos += bytertd1;
	}
	equal = true;
	
exit:
	if (data1) {
		free(data1);
	}
	if (data2) {
		free(data2);
	}
	if (fp1 != INVALID_FILE) {
		posix_fclose(fp1);
	}
	if (fp2 != INVALID_FILE) {
		posix_fclose(fp2);
	}

	posix_print_mb("file1: %s, file2: %s, compare result: %s\n", fname1, fname2, equal? "true": "false");

	return equal;
}
