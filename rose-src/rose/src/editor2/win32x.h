/*
 tree view(������ͼ)
*/

#ifndef __WIN32X_H_
#define __WIN32X_H_

#undef _UNICODE
#undef UNICODE

#ifndef _ENABLE_PRINT
#define _ENABLE_PRINT
#endif // _ENABLE_PRINT
#include "posix.h"

// for memory lead detect begin
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
// for memory lead detect end

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

//
// IP��ַ�ؼ�
//
#define IPA_SetAddress(hwndCtl, dwAddr)  \
	SendMessage((hwndCtl), IPM_SETADDRESS, 0, (LPARAM)dwAddr)

// IPM_GETADDRESS���ص�lpdwAddr,���IP��ַ��192.168.0.152,dwAddr��0xc0a80098
#define IPA_GetAddress(hwndCtl, lpdwAddr)			\
	SendMessage((hwndCtl), IPM_GETADDRESS, 0, (LPARAM)(lpdwAddr))

//
// �������ؼ�
//
#define ToolBar_ButtonStructSize(hwndCtl, cb) \
	SendMessage((hwndCtl), TB_BUTTONSTRUCTSIZE, (WPARAM)(cb), 0)

#define ToolBar_EnableButton(hwndCtl, id, bEnable)	\
    SendMessage((hwndCtl), TB_ENABLEBUTTON, id, MAKELPARAM(bEnable, 0))

#define ToolBar_CheckButton(hwndCtl, id, bCheck)	\
	SendMessage((hwndCtl), TB_CHECKBUTTON, id, MAKELPARAM(bCheck, 0))

#define ToolBar_AddButtons(hwndCtl, uNumButtons, lpButtons) \
	SendMessage((hwndCtl), TB_ADDBUTTONS, (WPARAM)(uNumButtons), (LPARAM)(LPTBBUTTON)(lpButtons))

#define ToolBar_SetExtendedStyle(hwndCtl, dwExStyle) \
	SendMessage((hwndCtl), TB_SETEXTENDEDSTYLE, 0, (LPARAM)(TBSTYLE_EX_DRAWDDARROWS))

#define ToolBar_SetImageList(hwndCtl, himl, iImageList) \
	SendMessage((hwndCtl), TB_SETIMAGELIST, iImageList, (LPARAM)(himl))

#define ToolBar_SetDisabledImageList(hwndCtl, himl) \
    SendMessage((hwndCtl), TB_SETDISABLEDIMAGELIST, 0, (LPARAM)(himl));

#define ToolBar_AutoSize(hwndCtl) \
	SendMessage((hwndCtl), TB_AUTOSIZE, 0, 0)

#define ToolBar_SetButtonSize(hwndCtl, dxButton, dyButton) \
	SendMessage((hwndCtl), TB_SETBUTTONSIZE, 0, (LPARAM)MAKELONG(dxButton, dyButton))

#define ToolBar_GetState(hwndCtl, idButton) \
	SendMessage((hwndCtl), TB_GETSTATE, (WPARAM)(idButton), 0)

//
// ״̬���ؼ�
//
#define StatusBar_SetText(hwndCtl, iPart, text) \
	SendMessage((hwndCtl), SB_SETTEXT, (WPARAM)(iPart), (LPARAM)(text))

// �������ؼ�
#define ProgressBar_SetPos(hwndCtl, pos)			SendMessage((hwndCtl), PBM_SETPOS, (WPARAM)(pos), 0)
#define ProgressBar_GetPos(hwndCtl)                 SendMessage((hwndCtl), PBM_GETPOS, 0, 0)

#define ProgressBar_SetRange(hwndCtl, posMin, posMax)    SendMessage((hwndCtl), PBM_SETRANGE, 0, (LPARAM)MAKELONG(posMin, posMax))
#define ProgressBar_GetRange(hwndCtl, lpposMin, lpposMax) do { \
    PBRANGE		range; \
	SendMessage((hwndCtl), PBM_GETRANGE, (WAPRAM)TRUE, (LPARAM)&range);	\
	*lpposMin = range.iLow;	\
	*lpposMax = range.iHigh;	\
} while(0)	

#define ProgressBar_SetBarColor(hwndCtl, clrBar)	\
	SendMessage((hwndCtl), PBM_SETBARCOLOR, 0, (LPARAM)(clrBar))


// �������ؼ�
#define TrackBar_SetPos(hwndCtl, pos, fRedraw)     SendMessage((hwndCtl), TBM_SETPOS, (WPARAM)(fRedraw), (LPARAM)(pos))
#define TrackBar_GetPos(hwndCtl)                   SendMessage((hwndCtl), TBM_GETPOS, 0, 0)

#define TrackBar_SetRange(hwndCtl, posMin, posMax, fRedraw)    SendMessage((hwndCtl), TBM_SETRANGE, (WPARAM)(fRedraw), (LPARAM)MAKELONG(posMin, posMax))
#define TrackBar_GetRange(hwndCtl, lpposMin, lpposMax)			\
	*lpposMin = SendMessage((hwndCtl), TBM_GETRANGEMIN, 0, 0);	\
	*lpposMax = SendMessage((hwndCtl), TBM_GETRANGEMAX, 0, 0)	

// ����ؼ�
#define UpDown_SetPos(hwndCtl, pos)	\
	SendMessage((hwndCtl), UDM_SETPOS32, (WPARAM)0, (LPARAM)(pos))
#define UpDown_GetPos(hwndCtl)	\
	SendMessage((hwndCtl), UDM_GETPOS32, 0, NULL)

#define UpDown_SetRange(hwndCtl, posMin, posMax)	\
	SendMessage((hwndCtl), UDM_SETRANGE32, (WPARAM)(posMin), (LPARAM)(posMax))
#define UpDown_GetRange(hwndCtl, lpposMin, lpposMax)			\
	SendMessage((hwndCtl), UDM_GETRANGE32, (WPARAM)(lpposMin), (LPARAM)(lpposMax))

#define UpDown_SetBuddy(hwndCtl, hwndBuddy)	\
	SendMessage((hwndCtl), UDM_SETBUDDY, (WPARAM)(hwndBuddy), (LPARAM)0)


// Zero out a structure. If fInitSize is TRUE, initialize the first int to
// the size of the structure. Many structures like WNDCLASSEX and STARTUPINFO
// require that their first member be set to the size of the structure itself.
#define chINITSTRUCT(structure, fInitSize)                           \
   (ZeroMemory(&(structure), sizeof(structure)),                     \
   fInitSize ? (*(int*) &(structure) = sizeof(structure)) : 0)


//////////////////////// Dialog Box Icon Setting Macro ////////////////////////


// The call to SetClassLong is for Windows NT 3.51 or less.  The WM_SETICON
// messages are for Windows NT 4.0 and Windows 95.
#define chSETDLGICONS(hwnd, idiLarge, idiSmall)                               \
   {                                                                          \
      OSVERSIONINFO VerInfo;                                                  \
      chINITSTRUCT(VerInfo, TRUE);                                            \
      GetVersionEx(&VerInfo);                                                 \
      if ((VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&                  \
          (VerInfo.dwMajorVersion <= 3 && VerInfo.dwMinorVersion <= 51)) {    \
         SetClassLong(hwnd, GCL_HICON, (LONG)                                 \
            LoadIcon(GetWindowInstance(hwnd), MAKEINTRESOURCE(idiLarge)));    \
      } else {                                                                \
         SendMessage(hwnd, WM_SETICON, TRUE,  (LPARAM)                        \
            LoadIcon(GetWindowInstance(hwnd), MAKEINTRESOURCE(idiLarge)));    \
         SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM)                        \
            LoadIcon(GetWindowInstance(hwnd), MAKEINTRESOURCE(idiSmall)));    \
      }                                                                       \
   }

//
// -------------------tree view(������ͼ) section-------------------------
//

//
// ������ͼ����˵��
//

// һ����ʼ��
// ��ʼ��������Ҫ��׼��ͼ���б�
// 1��������ͼ���б�ImageList_Create����
// 2�����������Ҫ��ͼ�꣨LoadIcon����������ͼ���б�ImageList_AddIcon����
// 3����ͼ���б�ҽ���Ի����пؼ���TreeView_SetImageList����

// �������ӡ��޸ġ�ɾ��Ҷ��
// TreeView_AddLeaf������һ��Ҷ�ӡ�
// TreeView_SetItem1���޸�һҶ�ӡ�TreeView_SetItem/SetItemEx����API
// TreeView_AddLeaf1�����ӣ�ͬʱ�޸�һҶ�ӣ���ͬTreeView_AddLeaf+TreeView_SetItem1��
// TreeView_SetItemParam���޸�һҶ�ӣ���ǿ��ʹ��Ҷ��ֻ��TVIF_PARAM����
// TreeView_DeleteItem(WinAPI)��ɾ��һҶ�ӡ���������lParam����Դ��
// TreeView_ReleaseItem��������Դ����ѡ��Ҫ��ҪɾҶ��


// ΪһЩ���ò��������еĺ���
// TreeView_Walk������Ҷ��(������ִ�еĲ�������ɾ������Ӱ�������ָ��������Ҷ�ӣ�

#define TreeView_GetItemParam(hwndTV, hti, param, type)	{	\
	TVITEMEX         _tvi;	\
	_tvi.mask = TVIF_HANDLE | TVIF_PARAM;	\
	_tvi.hItem = hti;	\
	if (TreeView_GetItem(hwndTV, &_tvi)) {	\
		param = (type)_tvi.lParam;		\
	} else {	\
		param = NULL;	\
	}	\
}

#define TreeView_GetItemImage(hctl, hti, iimg)	{	\
	TVITEMEX         _tvi;	\
	_tvi.mask = TVIF_HANDLE | TVIF_IMAGE;	\
	_tvi.hItem = hti;	\
	if (TreeView_GetItem(hctl, &_tvi)) {	\
		iimg = _tvi.iImage;		\
	} else {	\
		iimg = 0;	\
	}	\
}

#define TreeView_GetItemText(hctl, hti, text)	{	\
	TVITEMEX         _tvi;	\
	_tvi.mask = TVIF_HANDLE | TVIF_TEXT;	\
	_tvi.hItem = hti;	\
	_tvi.pszText = text;	\
	_tvi.cchTextMax = _MAX_PATH;	\
	TreeView_GetItem(hctl, &_tvi);	\
}
/*
#define TreeView_GetItem1(hctl, hti, tvi, mask, text)	{	\
	(tvi).mask = mask;	\
	(tvi).hItem = hti;	\
	(tvi).pszText = text;	\
	TreeView_GetItem(hctl, &(tvi));	\
}
*/

#define TreeView_HitTest1(hctl, point, htvi)	{	\
	TVHITTESTINFO	_tvhti;	\
	_tvhti.flags = TVHT_ONITEM | TVHT_ONITEMBUTTON;	\
	_tvhti.pt = point;	\
	MapWindowPoints(NULL, hctl, &_tvhti.pt, 1);	\
	TreeView_HitTest(hctl, &_tvhti);	\
	htvi = _tvhti.hItem;	\
}

void TreeView_GetItem1(HWND hctl, HTREEITEM hti, TVITEMEX *tvi, UINT mask, char *text);


HTREEITEM TreeView_AddLeaf(HWND hwndTV, HTREEITEM hTreeParent);
HTREEITEM TreeView_AddLeaf1(HWND hwndTV, HTREEITEM hTreeParent, UINT mask, LPARAM lParam, int iImage, int iSelectedImage, int cChildren, LPSTR lpszText, ...);
void TreeView_SetItem1(HWND hwndTV, HTREEITEM hTreeItem, UINT mask, LPARAM lParam, int iImage, int iSelectedImage, int cChildren, LPCSTR lpszText, ...);
void TreeView_SetItem2(HWND hwndTV, HTREEITEM hTreeItem, UINT mask, LPARAM lParam, int iImage, int iSelectedImage, int cChildren, LPSTR pszText);

void TreeView_ReleaseItem(HWND hctl, HTREEITEM hti, BOOL fDel);

// ����ʱ���õĻص�����
// 1.չ��֦Ҷ
void cb_treeview_walk_expand(HWND hctl, HTREEITEM hti, uint32_t* ctx);

// ����Ҷ�ӡ�ע�⣺lpfnTreeCallbackָ���Ĳ��������hTreeItem�������á�
// Callback function for walking TreeView items
typedef void (* fn_treeview_walk)(HWND hctl, HTREEITEM htvi, uint32_t *ctx);
void TreeView_Walk(HWND hctl, HTREEITEM htvi, BOOL fSubLeaf, fn_treeview_walk fn, uint32_t *ctx, BOOL fDelete = FALSE);

#define TreeView_SetItemParam(hwndTV, hti, param)	TreeView_SetItem1(hwndTV, hti, TVIF_PARAM, param, 0, 0, 0, NULL)
char* TreeView_FormPath(HWND hctl, HTREEITEM htvi, char *pathprefix);
void TreeView_FormVector(HWND hctl, HTREEITEM htvi, std::vector<std::string>& vec);
void TreeView_FormVector(HWND hctl, HTREEITEM htvi, std::vector<std::pair<LPARAM, std::string> >& vec);

void TreeView_SetChilerenByPath(HWND hctl, HTREEITEM htvi, char *path);
int CALLBACK fn_tvi_compare_sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

BOOL is_empty_dir(const char* dir);
BOOL is_directory(const char* fname);
BOOL is_file(const char *fname);
BOOL delfile(char *fname);
BOOL delfile1(const char *fname);
BOOL compare_dir(const std::string& dir1, const std::string& dir2);
bool copyfile(const char *src, const char *dst);
bool copy_root_files(const char* src, const char* dst);

//
// -------------------------------------------------------------------------
//

typedef BOOL (* fn_walk_dir)(char *name, uint32_t flags, uint64_t len, int64_t lastWriteTime, uint32_t *ctx);
#define walk_dir_win32_deepen(rootdir, subfolders, fn, ctx)		walk_dir_win32(rootdir, subfolders, 1, fn, ctx)
#define walk_dir_win32_fleeten(rootdir, subfolders, fn, ctx)	walk_dir_win32(rootdir, subfolders, 0, fn, ctx)
BOOL walk_dir_win32(const char* rootdir, int subfolders, int deepen, fn_walk_dir fn, uint32_t *ctx, int del = 0);
BOOL empty_directory(const char *path, fn_walk_dir fn, uint32_t *ctx);

void GetCurrentExePath(char *szCurExeDir);
char* GetBrowseFilePath(HWND hdlgP);
char *GetBrowseFileName(const char *strInintialDir, char *strFilter, BOOL fFileMustExist);
char* GetBrowseFileName_title(const char *strInintialDir, char *strFilter, BOOL fFileMustExist, char *title);

//
// ��windows�ṩ��API����/��дָ����key��value,�����һ�δ򿪹ر�һ���ļ�����ȷ̫��ʱ�䣬��������
//
// �ٶ�szKeyValue��������_MAX_PATH�ֽڳ�
// ����ֵ��
//	0���ɹ�
//  ��0��ʧ��
int CfgQueryValueWin(char *szFileName, char *szSection, char *szKeyName, char *szKeyValue);
// ����ֵ��
//	0���ɹ�
//  ��0��ʧ��
int CfgSetValueWin(char *szFileName, char *szSection, char *szKeyName, const char* szKeyValue);

BOOL MakeDirectory(const std::string& dd);

#endif //__WIN32X_H_