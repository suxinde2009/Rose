#include "win32x.h"
#include <shlobj.h> // SHBrowseForFolder

#include "xfunc.h"
//
// ----------------treeview(������ͼ)------------------------
//

//
// �������б�����һ��Ҷ��
//
// �򸸽ڵ�����ӽڵ㡣���������ӽڵ��κ��ֶΣ��ӽڵ��ֶΣ�����ͼ�꣬�ַ���������SetItem�趨��
// �����ӽڵ���ӱ�
HTREEITEM TreeView_AddLeaf(HWND hwndTV, HTREEITEM hTreeParent)
{
    TV_INSERTSTRUCT tvins;
    HTREEITEM       hti;

    memset(&tvins, 0, sizeof(tvins));

    // Set the parent item
    tvins.hParent = hTreeParent;

    tvins.hInsertAfter = TVI_LAST;

    // no members are valid
    tvins.item.mask = 0;

    // Add the item to the tree-view control.
    hti = TreeView_InsertItem(hwndTV, &tvins);
    return hti;
}


// ����ĳһITEM��TV_ITEM�ṹ��Ϣ
// @mask�� �������롣
// @lParam��mask��TVIF_PARAM��־ʱ��Ч��
// @lImage��mask��TVIF_IMAGE��־ʱ��Ч��
// @lSelectedImage��mask��TVIF_SELECTEDIMAGE��־ʱ��Ч��
// @lpszText��mask��TVIF_TEXT��־ʱ��Ч��
void TreeView_SetItem1(HWND hwndTV, HTREEITEM hTreeItem, UINT mask, LPARAM lParam, int iImage, int iSelectedImage, int cChildren, LPCSTR lpszText, ...)
{
	TCHAR           szBuffer[1024];
    va_list         list;

	if (lpszText) {
		// Ҫ��lpszText����ΪNULL
		va_start(list, lpszText);
		vsprintf(szBuffer, lpszText, list);
		va_end(list);
	} else {
		szBuffer[0] = 0;
	}

	TreeView_SetItem2(hwndTV, hTreeItem, mask, lParam, iImage, iSelectedImage, cChildren, szBuffer);
}

void TreeView_SetItem2(HWND hwndTV, HTREEITEM hTreeItem, UINT mask, LPARAM lParam, int iImage, int iSelectedImage, int cChildren, LPSTR pszText)
{
	TVITEMEX        tvi;

	// �õ�һ��������tvi
	tvi.mask = TVIF_HANDLE;
    tvi.hItem = hTreeItem;

    TreeView_GetItem(hwndTV, &tvi);

	tvi.mask = mask;
	tvi.lParam = lParam;
	tvi.iImage = iImage;
	tvi.iSelectedImage = iSelectedImage;
	tvi.cChildren = cChildren;
	tvi.pszText =  pszText;

	TreeView_SetItem(hwndTV, &tvi);
}

// ����ĳһITEM��TV_ITEM�ṹ��Ϣ
// @mask�� �������롣
// @lParam��mask��TVIF_PARAM��־ʱ��Ч��
// @lImage��mask��TVIF_IMAGE��־ʱ��Ч��
// @lSelectedImage��mask��TVIF_SELECTEDIMAGE��־ʱ��Ч��
// @lpszText��mask��TVIF_TEXT��־ʱ��Ч��
HTREEITEM TreeView_AddLeaf1(HWND hwndTV, HTREEITEM hTreeParent, UINT mask, LPARAM lParam, int iImage, int iSelectedImage, int cChildren, LPSTR lpszText, ...)
{
    HTREEITEM       hti;

	TCHAR           szBuffer[1024];
    va_list         list;

	if (lpszText) {
		// Ҫ��lpszText����ΪNULL
		va_start(list, lpszText);
		vsprintf(szBuffer, lpszText, list);
		va_end(list);
	} else {
		szBuffer[0] = 0;
	}


	hti = TreeView_AddLeaf(hwndTV, hTreeParent);
	TreeView_SetItem1(hwndTV, hti, mask, lParam, iImage, iSelectedImage, cChildren, szBuffer);
    return hti;
}

void TreeView_ReleaseItem(HWND hctl, HTREEITEM hti, BOOL fDel)
{
	LPARAM		lparam = NULL;
	TreeView_GetItemParam(hctl, hti, lparam, LPARAM);
	if (lparam) {
		free((void*)lparam);
	}
	if (fDel) {
		TreeView_DeleteItem(hctl, hti);
	} else {
		TreeView_SetItemParam(hctl, hti, 0);
	}
	return;
}

void cb_treeview_walk_expand(HWND hctl, HTREEITEM hti, uint32_t* ctx)
{
	TreeView_Expand(hctl, hti, TVE_EXPAND);
}

// @fSubLeaf: �Ƿ�Ҫ�����¼��Լ����¼�Ҷ��
// ����(hctl, htvi)Ҷ���µ�����Ҷ��, �ֱ����fn.
// ע:
// 1. ���ú����в�Ҫȥɾ����Ҷ��,Ҫɾ��ʹ�ú�������fDelete��TRUE
void TreeView_Walk(HWND hctl, HTREEITEM htvi, BOOL fSubLeaf, fn_treeview_walk fn, uint32_t *ctx, BOOL fDelete)
{
	HTREEITEM			hChildTreeItem = NULL;
	HTREEITEM			hPrevChildTreeItem;

	hChildTreeItem = TreeView_GetChild(hctl, htvi);
	while (hChildTreeItem) {
		//
        // Call the fn on the node itself.
        //
		if (fSubLeaf) {
			TreeView_Walk(hctl, hChildTreeItem, fSubLeaf, fn, ctx);
		}
		if (fn) {
			fn(hctl, hChildTreeItem, ctx);
		}
		
        hPrevChildTreeItem = hChildTreeItem;
		hChildTreeItem = TreeView_GetNextSibling(hctl, hChildTreeItem);
		if (fDelete) {
			TreeView_DeleteItem(hctl, hPrevChildTreeItem);
		}
    }
	return;
}

void TreeView_GetItem1(HWND hctl, HTREEITEM hti, TVITEMEX *tvi, UINT mask, char *text)
{
	tvi->mask = mask;	
	tvi->hItem = hti;
	tvi->pszText = text;
	tvi->cchTextMax = _MAX_PATH;
	TreeView_GetItem(hctl, tvi);
}

// @pathprefix: �����ֵΪNULL/pathprefix[0]=='\0', ���γɵ�·��ǰǰ����һ��
char* TreeView_FormPath(HWND hctl, HTREEITEM htvi, char *pathprefix)
{
	static char		path[_MAX_PATH];
	char			text[_MAX_PATH];
	HTREEITEM		htvip;
	TVITEMEX		tvi;

	path[0] = '\0';
	TreeView_GetItem1(hctl, htvi, &tvi, TVIF_TEXT, path);
	while ((htvip = TreeView_GetParent(hctl, htvi)) && (htvip != TVI_ROOT)) {
		TreeView_GetItem1(hctl, htvip, &tvi, TVIF_TEXT, text);
		if (path[0]) {
            strcpy(path, formatstr("%s\\%s", text, path));
		} else {
			strcpy(path, text);
		}
		htvi = htvip;
	}
	if (!is_empty_str(pathprefix)) {
		if (path[0]) {
			strcpy(path, formatstr("%s\\%s", pathprefix, path));
		} else {
			strcpy(path, pathprefix);
		}
	}

	return path;
}

void TreeView_FormVector(HWND hctl, HTREEITEM htvi, std::vector<std::string>& vec)
{
	char text[_MAX_PATH];
	HTREEITEM htvip;
	TVITEMEX tvi;

	TreeView_GetItem1(hctl, htvi, &tvi, TVIF_TEXT, text);
	vec.push_back(text);
	while ((htvip = TreeView_GetParent(hctl, htvi)) && (htvip != TVI_ROOT)) {
		TreeView_GetItem1(hctl, htvip, &tvi, TVIF_TEXT, text);
		vec.push_back(text);
		htvi = htvip;
	}
}

void TreeView_FormVector(HWND hctl, HTREEITEM htvi, std::vector<std::pair<LPARAM, std::string> >& vec)
{
	char text[_MAX_PATH];
	HTREEITEM htvip;
	TVITEMEX tvi;

	TreeView_GetItem1(hctl, htvi, &tvi, TVIF_TEXT | TVIF_PARAM, text);
	vec.push_back(std::make_pair(tvi.lParam, text));
	while ((htvip = TreeView_GetParent(hctl, htvi)) && (htvip != TVI_ROOT)) {
		TreeView_GetItem1(hctl, htvip, &tvi, TVIF_TEXT | TVIF_PARAM, text);
		vec.push_back(std::make_pair(tvi.lParam, text));
		htvi = htvip;
	}
}

// ��һЩ��̬�������ܳ�ͻ,�ɺ��Ϊ����
// ��Ŀ¼��ɾ�����һ��Ҷ�Ӻ�, ֱ����TVIF_CHILDRENΪ0, ���ܳ�ȥǰ��-����. ����TreeView_Expand���ǲ���
void TreeView_SetChilerenByPath(HWND hctl, HTREEITEM htvi, char *path)
{
	// ����Ҫ��Ҫ��path���ļ��Ļ�(ԭ�����ǲ�Ӧ���ǳ��ֵ�,�����ó�����ܲ�С�ĳ���), is_empty_dir��Ҫ����TRUE,
	// ��������ļ�Ҷ��Ҫ������ı�cChildren��Ϊ1.
	if (is_empty_dir(path)) {
		TreeView_Expand(hctl, htvi, TVE_COLLAPSE);	
		TreeView_SetItem1(hctl, htvi, TVIF_CHILDREN, 0, 0, 0, 0, NULL);	
	} else {	
		TreeView_SetItem1(hctl, htvi, TVIF_CHILDREN, 0, 0, 0, 1, NULL);	
	}	
	return;
}

int CALLBACK fn_tvi_compare_sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	HWND				hctl = (HWND)lParamSort;
	TVITEMEX			tvi1, tvi2;
	int					tvi1_type, tvi2_type;
	char				name1[_MAX_PATH], name2[_MAX_PATH];
	
	memset(&tvi1, 0, sizeof(TVITEMEX));
	memset(&tvi2, 0, sizeof(TVITEMEX));
	TreeView_GetItem1(hctl, (HTREEITEM)lParam1, &tvi1, TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE, name1);
	TreeView_GetItem1(hctl, (HTREEITEM)lParam2, &tvi2, TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE, name2);

	tvi1_type = tvi1.iImage? 1: 0;
	tvi2_type = tvi2.iImage? 1: 0;
	if (tvi1_type < tvi2_type) {
		// tvi1��Ŀ¼, tvi2���ļ�
		return -1;
	} else if (tvi1_type > tvi2_type) {
		// tvi1���ļ�, tvi2��Ŀ¼
		return 1;
	} else {
		// lvi1��lvi2����Ŀ¼���ļ�,ֻ�����ַ����Ƚ�
		return strcasecmp(name1, name2);
	}
}

// --------------------------------------------------------------


#define FT2UT_INTVAL_100NS		116444736000000000I64
#define FileTimeToUnixTime(ft)	\
	((posix_mku64(ft.dwLowDateTime, ft.dwHighDateTime) - 116444736000000000I64) / 10000000)

// @rootdir: �������ĸ�Ŀ¼
// @subdir: �˴�Ҫ�����ĳ�ȥ��Ŀ¼�Ĳ��ֵ���Ŀ¼.
void walk_subdir_win32(const char* rootdir, char *subdir, int deepen, fn_walk_dir fn, uint32_t *ctx)
{
	HANDLE				hFind;
	WIN32_FIND_DATA		finddata;
	BOOL				fOk;
	int64_t				timestamp = 0;
	char				cFileNameComp[_MAX_PATH];
	
	hFind = FindFirstFile("*.*", &finddata);
	fOk = (hFind != INVALID_HANDLE_VALUE);

	while (fOk) {
		sprintf(cFileNameComp, "%s\\%s", subdir, finddata.cFileName);
		timestamp = FileTimeToUnixTime(finddata.ftLastWriteTime);
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Ŀ¼
			if ((strcmp(finddata.cFileName, ".") != 0) && (strcmp(finddata.cFileName, "..") != 0)) {
				if (deepen && fn) {
					// ��ǳ����: ���ȵ���fn,��ȥ�ڸ�Ŀ�ϵݹ�
					fn(cFileNameComp, FILE_ATTRIBUTE_DIRECTORY, posix_mku64(finddata.nFileSizeLow, finddata.nFileSizeHigh), timestamp, ctx);
				}
				SetCurrentDirectory(formatstr("%s\\%s\\", rootdir, cFileNameComp));
				walk_subdir_win32(rootdir, cFileNameComp, deepen, fn, ctx);
				if (!deepen && fn) {
					// ���ǳ: �����ڸ�Ŀ�ϵݹ�,��ȥ����fn
					fn(cFileNameComp, FILE_ATTRIBUTE_DIRECTORY, posix_mku64(finddata.nFileSizeLow, finddata.nFileSizeHigh), timestamp, ctx);
				}
			}
		} else {
			// �ļ�
			if (fn) {
				fn(cFileNameComp, 0, posix_mku64(finddata.nFileSizeLow, finddata.nFileSizeHigh), timestamp, ctx);
			}
		}
		fOk = FindNextFile(hFind, &finddata);
	}
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
	}

	return;
}

// @rootdir: �������ĸ�Ŀ¼
// @subfolders: �Ƿ�Ҫ������Ŀ¼. 0: ������
// @deepen: ����, ����fnʱ���Ĳ�����, 1: ǳ����, 0: ���ǳ
// ע:
//  1.������FindFirstFile����ɾ��Ŀ¼�ǲ��е�.����⵽��Ŀʱ,����RemoveDirectory����������ʧ��(������:32,ָʾ���н�����ʹ��)
//    FindClose��ſ���,����ʱcloseʱ�����ܵ���.ҪɾĿ¼�ĵ���SHFileOperation.
BOOL walk_dir_win32(const char* rootdir, int subfolders, int deepen, fn_walk_dir fn, uint32_t *ctx, int del)
{
	char				szCurrDir[_MAX_PATH], text[_MAX_PATH];
	HANDLE				hFind;
	WIN32_FIND_DATA		finddata;
	BOOL				fOk, fRet = TRUE;
	int64_t				timestamp = 0;
		
	GetCurrentDirectory(_MAX_PATH, szCurrDir);
	SetCurrentDirectory(appendbackslash(rootdir));
	hFind = FindFirstFile("*.*", &finddata);
	fOk = (hFind != INVALID_HANDLE_VALUE);

	while (fOk) {
		timestamp = FileTimeToUnixTime(finddata.ftLastWriteTime);
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Ŀ¼
			if ((strcmp(finddata.cFileName, ".") != 0) && (strcmp(finddata.cFileName, "..") != 0)) {
				if (deepen && fn) {
					// ��ǳ����: ���ȵ���fn,��ȥ�ڸ�Ŀ�ϵݹ�
					if (!fn(finddata.cFileName, FILE_ATTRIBUTE_DIRECTORY, posix_mku64(finddata.nFileSizeLow, finddata.nFileSizeHigh), timestamp, ctx)) {
						fRet = FALSE;
						break;
					}
				}
				sprintf(text, "%s\\%s\\", rootdir, finddata.cFileName);
				if (subfolders) {
					SetCurrentDirectory(text);
					walk_subdir_win32(rootdir, finddata.cFileName, deepen, fn, ctx);
				}
				if (!deepen && fn) {
					// ���ǳ: �����ڸ�Ŀ�ϵݹ�,��ȥ����fn
					if (!fn(finddata.cFileName, FILE_ATTRIBUTE_DIRECTORY, posix_mku64(finddata.nFileSizeLow, finddata.nFileSizeHigh), timestamp, ctx)) {
						fRet = FALSE;
						break;
					}
				}
				if (del) {
					RemoveDirectory(text);
					DWORD err = GetLastError();
				}
			}
		} else {
			// �ļ�
			if (fn) {
				if (!fn(finddata.cFileName, 0, posix_mku64(finddata.nFileSizeLow, finddata.nFileSizeHigh), timestamp, ctx)) {
					fRet = FALSE;
					break;
				}
			}
			if (del) {
				sprintf(text, "%s\\%s", rootdir, finddata.cFileName);
				delfile(text);
			}
		}
		fOk = FindNextFile(hFind, &finddata);
	}
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
	}

	SetCurrentDirectory(szCurrDir);
	return fRet;
}

BOOL is_empty_dir(const char* dir)
{
	HANDLE				hFind;
	WIN32_FIND_DATA		finddata;
	char				szCurrDir[_MAX_PATH];
	BOOL				fOk, fEmpty = TRUE;;

	if (!is_directory(dir)) {
		// ����Ŀ¼,����TRUE
		return TRUE;
	}
	GetCurrentDirectory(_MAX_PATH, szCurrDir);
	SetCurrentDirectory(appendbackslash(dir));

	hFind = FindFirstFile("*.*", &finddata);
	fOk = (hFind != INVALID_HANDLE_VALUE);
	
	while (fOk) {
		if ((strcmp(finddata.cFileName, ".") != 0) && (strcmp(finddata.cFileName, "..") != 0)) {
			fEmpty = FALSE;
			break;
		}
		fOk = FindNextFile(hFind, &finddata);
	}
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
	}

	

	SetCurrentDirectory(szCurrDir);
	return fEmpty;
}

BOOL is_directory(const char* fname)
{
	WIN32_FILE_ATTRIBUTE_DATA		fattrdata;
	BOOL fok = GetFileAttributesEx(fname, GetFileExInfoStandard, &fattrdata);
	if (!fok) return FALSE;
	return (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? TRUE: FALSE;
}

BOOL is_file(const char *fname)
{
	WIN32_FILE_ATTRIBUTE_DATA		fattrdata;
	BOOL fok = GetFileAttributesEx(fname, GetFileExInfoStandard, &fattrdata);
	if (!fok) return FALSE;
	return (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? FALSE: TRUE;
}

typedef struct {
	char			path[_MAX_PATH];
	fn_walk_dir		fn;
	uint32_t		*ctx;
} walkdirempty_t;

static void cb_walk_dir_empty(char *name, uint32_t flags, uint64_t len, int64_t lastWriteTime, walkdirempty_t *wdepy)
{
	char			path[_MAX_PATH];
	
	if (wdepy->fn) {
		wdepy->fn(name, flags, len, lastWriteTime, wdepy->ctx);
	}
	sprintf(path, "%s\\%s", wdepy->path, name);
	if (flags & FILE_ATTRIBUTE_DIRECTORY) {
		RemoveDirectory(appendbackslash(path));
		DWORD err = GetLastError();
	} else {
        delfile(path);
	}
	return;
}

BOOL empty_directory(const char *path, fn_walk_dir fn, uint32_t *ctx)
{
	walk_dir_win32(path, 1, 0, fn, ctx, 1);
	return TRUE;
}

// @fname: �����ļ�
BOOL delfile(char *fname)
{
	WIN32_FILE_ATTRIBUTE_DATA		fattrdata;

	// ȥ��ֻ������
	GetFileAttributesEx(fname, GetFileExInfoStandard, &fattrdata);
	SetFileAttributes(fname, fattrdata.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
	return DeleteFile(fname);
}

// @fname: �������ļ�Ҳ������Ŀ¼
BOOL delfile1(const char *fname)
{
	WIN32_FILE_ATTRIBUTE_DATA		fattrdata;
	BOOL							fok = TRUE;
	char							text[_MAX_PATH];

	fok = GetFileAttributesEx(fname, GetFileExInfoStandard, &fattrdata);
	if (!fok) {
		DWORD err = GetLastError();
		return err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND;
	}
	if (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// Ҫɾ����Ŀ¼
		// 1.���Ŀ¼
		SHFILEOPSTRUCT		fopstruct;

		memset(&fopstruct, 0, sizeof(SHFILEOPSTRUCT));
		fopstruct.hwnd = NULL;
		fopstruct.wFunc = FO_DELETE;
		strcpy(text, fname);
		text[strlen(text) + 1] = '\0';
		fopstruct.pFrom = text;
		fopstruct.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION; // ��Ҫ��ʾ����ɾ��, ��Ҫ��ʾȷ��Ҫ��Ҫɾ���Ի���
		fok = SHFileOperation(&fopstruct)? FALSE: TRUE;
		// empty_directory(fname, NULL, NULL);
	} else {
		// Ҫɾ�����ļ�
		// ȥ��ֻ������
		SetFileAttributes(fname, fattrdata.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
		fok = DeleteFile(fname);
	}

	return fok;
}


// GetCurrentExePath,���ص�ǰ����ִ�е�Ӧ�ó��������Ŀ¼.
// ���ص�szCurExeDir���������Ŀ¼��
void GetCurrentExePath(char *szCurExeDir)
{
	char szCurDir[MAX_PATH];
	char szCmdLine[MAX_PATH];
	char *pszTmp1, *pszTmp2;
	BOOL fFound = FALSE;
	// �������������,GetCurrentDirectory���ؿ���̨����
	// ��·��,GetCommandLine��������,��debug\tvinstall.exe
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);

    // GetCommandLine�Ǽ���" "���ŵ�,��Ϊ��������ȥ�������"��
	strcpy(szCmdLine, GetCommandLine());

	// 1.szCmdLine���ܺ���������,Ӧ�ó���Ͳ���.Ӧ�ó���϶�����,�������ܲ�����.
	// 2.��Ӧ�ó���,����м京�пռ�,�ض�����" "��Χ,���û�пո�,������" "��Χ.
	// 3.Ӧ�ó���Ͳ���֮���ÿո�ָ�.

	if (szCmdLine[0] == '\"') {
		// Ӧ�ó����м京�пո�,ֻ����" "֮������
		strcpy(szCmdLine, GetCommandLine() + 1);
		pszTmp1 = strchr(szCmdLine, '\"');
		if (pszTmp1) {
            *pszTmp1 = 0;
		}
	} else {
		// Ӧ�ó����м䲻���ո�,����п��ܾ��ҿո�,ȥ����������
        pszTmp1 = strchr(szCmdLine, ' ');
		if (pszTmp1) {
			*pszTmp1 = 0;
		}
	}

	// 1.ȥ�����������һ��,���һ�ھ��ǿ�ִ���ļ���,.exe���ܻ�û��
	pszTmp1 = pszTmp2 = szCmdLine;
	while (*pszTmp1) {
		if (*pszTmp1 == '\\') {
			pszTmp2 = pszTmp1;
		}
		pszTmp1 ++;
	}
	*pszTmp2 = 0;
	// 2.����������Ƿ�����������,����оͰ�����Ϊʱȫ·��,������
	// һ��������Ӧ�ó�����,ֻ��������û��.exe��չ��
	// Window�ļ��������ܰ���':'
	pszTmp1 = szCmdLine;
	while (*pszTmp1) {
		if (*pszTmp1 == ':') {
			fFound = TRUE;
			break;
		}
		pszTmp1 ++;
	}
    if (fFound) { 
		// һ��ȫ·����������,��������CurrentDirectory�������
		strcpy(szCurExeDir, szCmdLine);
	} else {
		wsprintf(szCurExeDir, "%s\\%s", szCurDir, szCmdLine);
	}
}

struct tcompare_dir_param
{
	tcompare_dir_param(const std::string& dir1, const std::string& dir2, int& recursion_count)
		: current_path1(dir1)
		, current_path2(dir2)
		, recursion_count(recursion_count)
	{
		std::replace(current_path1.begin(), current_path1.end(), '/', '\\');
		if (current_path1.at(current_path1.size() - 1) == '\\') {
			current_path1.erase(current_path1.size() - 1);
		}

		if (!dir2.empty()) {
			std::replace(current_path2.begin(), current_path2.end(), '/', '\\');
			if (current_path2.at(current_path2.size() - 1) == '\\') {
				current_path2.erase(current_path2.size() - 1);
			}
		}
	}

	std::string current_path1;
	std::string current_path2;
	int& recursion_count;
};

BOOL cb_compare_dir_explorer(char *name, uint32_t flags, uint64_t len, int64_t lastWriteTime, uint32_t* ctx)
{
	tcompare_dir_param& cdp = *(tcompare_dir_param*)ctx;
	bool compair = !cdp.current_path2.empty();
	cdp.recursion_count ++;
	std::string path2 = cdp.current_path2;
	if (compair) {
		path2.append("\\");
		path2.append(name);
	}
	
	// compair
	if (flags & FILE_ATTRIBUTE_DIRECTORY) {
		if (compair) {
			if (!is_directory(path2.c_str())) {
				return FALSE;
			}
		}

		tcompare_dir_param cdp2(cdp.current_path1 + "\\" + name, path2, cdp.recursion_count);
		if (!walk_dir_win32_deepen(cdp2.current_path1.c_str(), 0, cb_compare_dir_explorer, (uint32_t *)&cdp2)) {
			return FALSE;
		}

	} else if (compair) {
		if (!is_file(path2.c_str())) {
			return FALSE;
		}

	}
	return TRUE;
}

BOOL compare_dir(const std::string& dir1, const std::string& dir2)
{
	int recursion1_count = 0;
	tcompare_dir_param cdp1(dir1, dir2, recursion1_count);
	BOOL fok = walk_dir_win32_deepen(dir1.c_str(), 0, cb_compare_dir_explorer, (uint32_t *)&cdp1);
	if (!fok) {
		return FALSE;
	}

	int recursion2_count = 0;
	tcompare_dir_param cdp2(dir2, "", recursion2_count);
	walk_dir_win32_deepen(dir2.c_str(), 0, cb_compare_dir_explorer, (uint32_t *)&cdp2);

	if (cdp1.recursion_count != cdp2.recursion_count) {
		return FALSE;
	}
	return TRUE;
}

// @src: �������ļ�Ҳ������Ŀ¼
// @dst: �������ļ�Ҳ������Ŀ¼
// if copy directory, dst must be not exist.
bool copyfile(const char *src, const char *dst)
{
	WIN32_FILE_ATTRIBUTE_DATA		fattrdata;
	BOOL							fok = TRUE;
	char							text1[_MAX_PATH], text2[_MAX_PATH];

	fok = GetFileAttributesEx(src, GetFileExInfoStandard, &fattrdata);
	if (!fok) return false;
	if (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// Ҫ���Ƶ���Ŀ¼
		SHFILEOPSTRUCT		fopstruct;

		memset(&fopstruct, 0, sizeof(SHFILEOPSTRUCT));
		fopstruct.hwnd = NULL;
		fopstruct.wFunc = FO_COPY;

		strcpy(text1, src);
		text1[strlen(text1) + 1] = '\0';
		fopstruct.pFrom = text1;

		strcpy(text2, dst);
		text2[strlen(text2) + 1] = '\0';
		fopstruct.pTo = text2;

		fopstruct.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION; // ��Ҫ��ʾ����ɾ��, ��Ҫ��ʾȷ��Ҫ��Ҫɾ���Ի���
		fok = SHFileOperation(&fopstruct)? FALSE: TRUE;
		if (fok) {
			fok = compare_dir(src, dst);
		}
	} else {
		// Ҫ���Ƶ����ļ�
		fok = CopyFile(src, dst, FALSE);
	}

	return fok? true: false;
}

bool copy_root_files(const char* src, const char* dst)
{
	char				szCurrDir[_MAX_PATH], text1[_MAX_PATH], text2[_MAX_PATH];
	HANDLE				hFind;
	WIN32_FIND_DATA		finddata;
	BOOL				fok, fret = TRUE;
	
	if (!src || !src[0] || !dst || !dst[0]) {
		return false;
	}
		
	GetCurrentDirectory(_MAX_PATH, szCurrDir);
	SetCurrentDirectory(appendbackslash(src));
	hFind = FindFirstFile("*.*", &finddata);
	fok = (hFind != INVALID_HANDLE_VALUE);

	while (fok) {
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Ŀ¼
		} else {
			// �ļ�
			sprintf(text1, "%s\\%s", src, finddata.cFileName);
			sprintf(text2, "%s\\%s", dst, finddata.cFileName);
			fret = CopyFile(text1, text2, FALSE);
			if (!fret) {
				posix_print_mb("copy file from %s to %s fail", text1, text2);
				break;
			}
		}
		fok = FindNextFile(hFind, &finddata);
	}
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
	}

	SetCurrentDirectory(szCurrDir);
	return fret? true: false;
}

char* GetBrowseFilePath(HWND hdlgP)
{
	static char			gname[_MAX_PATH];
	LPITEMIDLIST		pidl;
	BROWSEINFO			bsif;

	bsif.hwndOwner = hdlgP;
    bsif.pidlRoot = NULL;
    bsif.pszDisplayName = gname;
    bsif.lpszTitle = NULL;
    bsif.ulFlags = 0;
    bsif.lpfn = NULL;
    bsif.lParam = NULL;
    bsif.iImage = 0;

	pidl = SHBrowseForFolder(&bsif);

	if (pidl) {
		SHGetPathFromIDList(pidl, gname);
		CoTaskMemFree(pidl);

		// �������,ȥ����β��'\'�ַ�
		if (gname[0] && (gname[strlen(gname) - 1] == '\\')) {
			gname[strlen(gname) - 1] = 0;
		}
		return gname;
	} else {
		return NULL;
	}
}

char* GetBrowseFileName(const char *strInintialDir, char *strFilter, BOOL fFileMustExist)
{
	static char		gname[_MAX_PATH];
    OPENFILENAME	ofn={0};

    // Reset filename
    *gname = 0;

    // Fill in standard structure fields
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = NULL;
    ofn.lpstrFilter       = strFilter;
//		"Video Files (*.asf; *.avi; *.qt; *.mov)\0*.asf; *.avi; *.qt; *.mov\0\0";
		
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = gname;
    ofn.nMaxFile          = _MAX_PATH;
    ofn.lpstrTitle        = TEXT("Open File...\0");
    ofn.lpstrFileTitle    = NULL;
    ofn.lpstrDefExt       = TEXT("*\0");
	ofn.Flags             = OFN_PATHMUSTEXIST;
	if (fFileMustExist) {
		ofn.Flags		 |= OFN_FILEMUSTEXIST;
	}
    
    // ofn.lpstrInitialDir = "\\\0";
	ofn.lpstrInitialDir = strInintialDir;
    
    // Create the standard file open dialog and return its result
	if (GetOpenFileName((LPOPENFILENAME)&ofn)) {
		return gname;
	} else {
		return NULL;
	}
}

char* GetBrowseFileName_title(const char* strInintialDir, char *strFilter, BOOL fFileMustExist, char *title)
{
	static char		gname[_MAX_PATH];
    OPENFILENAME	ofn={0};

    // Reset filename
    *gname = 0;

    // Fill in standard structure fields
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = NULL;
    ofn.lpstrFilter       = strFilter;
//		"Video Files (*.asf; *.avi; *.qt; *.mov)\0*.asf; *.avi; *.qt; *.mov\0\0";
		
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = gname;
    ofn.nMaxFile          = _MAX_PATH;
    ofn.lpstrTitle        = title;
    ofn.lpstrFileTitle    = NULL;
    ofn.lpstrDefExt       = TEXT("*\0");
	ofn.Flags             = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	if (fFileMustExist) {
		ofn.Flags		 |= OFN_FILEMUSTEXIST;
	}
    
    // ofn.lpstrInitialDir = "\\\0";
	ofn.lpstrInitialDir = strInintialDir;
    
    // Create the standard file open dialog and return its result
	if (GetOpenFileName((LPOPENFILENAME)&ofn)) {
		return gname;
	} else {
		return NULL;
	}
}


// Used to retrieve a value give the section and key
int CfgQueryValueWin(char *szFileName, char* szSection, char* szKeyName, char* szKeyValue)
{
	DWORD dwRetVal;

	// Get the info from the .ini file	
	dwRetVal = GetPrivateProfileString(szSection, szKeyName, "", szKeyValue, _MAX_PATH, szFileName);	

	//
	// ����ֵ:
	// 0: ʧ��,���key��ûֵ,��Ӧ��szKeyValue[0]�ᱻ��Ϊ'\0'
	// ����: keyֵ�ĳ���,������������,��ʼkey = sec, dwRetVal����3
	//

	// szKeyValue:��һЩ��ȥ���ո���ַ�
	return dwRetVal? 0: -1;
}

// Used to add or set a key value pair to a section
int CfgSetValueWin(char *szFileName, char* szSection, char* szKeyName, const char* szKeyValue)
{
	DWORD dwRetVal;

	dwRetVal = WritePrivateProfileString (szSection, szKeyName, szKeyValue, szFileName);
	//
	// ����ֵ:
	// nonzero:		�ɹ�
	// zero:		ʧ��
	//
	return dwRetVal? 0: -1;
}

BOOL MakeDirectory(const std::string& dd)
{
	HANDLE fFile; // File Handle
	WIN32_FIND_DATA fileinfo; // File Information Structure
	std::vector<std::string> m_arr; // CString Array to  hold Directory Structures
	BOOL tt; // BOOL used to test if Create Directory was successful
	size_t x1 = 0; // Counter
	std::string tem = ""; // Temporary std::string Object
    
	if (dd.empty()) {
		return FALSE;
	}

	fFile = FindFirstFile(dd.c_str(), &fileinfo);
    
	// if the file exists and it is a directory
	if ((fFile != INVALID_HANDLE_VALUE) && (fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)) {
		// Directory Exists close file and return
		FindClose(fFile);
		return FALSE;
	}
	// Close the file
	FindClose(fFile);

	m_arr.clear();
    
	for (x1 = 0; x1 < dd.size(); x1++ ) { // Parse the supplied CString Directory String
		if (dd.at(x1) != '\\') { // if the Charachter is not a \     
			tem += dd.at(x1); // add the character to the Temp String   
		} else {
			m_arr.push_back(tem); // if the Character is a \     
			tem += "\\"; // Now add the \ to the temp string
		}   
		if (x1 == dd.size()-1) { //   If   we   reached   the   end   of   the   String   
			m_arr.push_back(tem);
		}
	}   
    
	// Now lets cycle through the String Array and create each directory in turn   
	for (std::vector<std::string>::const_iterator itor = m_arr.begin() + 1; itor != m_arr.end(); ++ itor) {
		tt = CreateDirectory((*itor).c_str(), NULL);
    
		// If the Directory exists it will return a false
		if (tt) {
			// If we were successful we set the attributes to normal
			SetFileAttributes((*itor).c_str(), FILE_ATTRIBUTE_NORMAL);
		}
	}
	// Now lets see if the directory was successfully created
	fFile = FindFirstFile(dd.c_str(), &fileinfo);
    
	m_arr.clear();
	if (fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {   
		// Directory Exists close file and return
		FindClose(fFile);
		return TRUE;
	} else {   
		// For Some reason the Function Failed Return FALSE
		FindClose(fFile);
		return FALSE;
	}
}