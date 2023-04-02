// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <VirtualListView.h>
#include <OwnerDrawnMenu.h>

struct SYSTEM_POOLTAG {
	union {
		UCHAR Tag[4];
		ULONG TagUlong;
	};
	ULONG PagedAllocs;
	ULONG PagedFrees;
	SIZE_T PagedUsed;
	ULONG NonPagedAllocs;
	ULONG NonPagedFrees;
	SIZE_T NonPagedUsed;
};

struct SYSTEM_POOLTAG_INFORMATION {
	ULONG Count;
	SYSTEM_POOLTAG TagInfo[1];
};

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>, 
	public CAutoUpdateUI<CMainFrame>,
	public CVirtualListView<CMainFrame>,
	public CCustomDraw<CMainFrame>,
	public COwnerDrawnMenu<CMainFrame>,
	public CMessageFilter, 
	public CIdleHandler {
public:
	DECLARE_FRAME_WND_CLASS(nullptr, IDR_MAINFRAME)

	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnIdle() override;

	CString GetColumnText(HWND, int row, int col) const;

protected:
	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		CHAIN_MSG_MAP(CVirtualListView<CMainFrame>)
		CHAIN_MSG_MAP(CCustomDraw<CMainFrame>)
		CHAIN_MSG_MAP(COwnerDrawnMenu<CMainFrame>)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

private:
	enum class ColumnType {
		TagName,
		TagValue,
		PagedAllocs,
		PagedFrees,
		PagedDiff,
		PagedUsage,

		NonPagedAllocs,
		NonPagedFrees,
		NonPagedDiff,
		NonPagedUsage,

		SourceName,
		SourceDescription,
	};

	void Refresh();
	bool LoadPoolTags();
	CString const& GetTagAsString(ULONG tag) const;
	std::wstring const& GetTagSource(ULONG tag) const;
	std::wstring const& GetTagDesc(ULONG tag) const;

	struct KnownPoolTag {
		ULONG Tag;
		std::string TagAsString;
		std::wstring Name;
		std::wstring Desc;
	};

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAlwaysOnTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CListViewCtrl m_List;
	std::unordered_map<ULONG, KnownPoolTag> m_KnownTags;
	std::vector<SYSTEM_POOLTAG> m_PoolTags;
	mutable std::unordered_map<ULONG, CString> m_TagToString;

	void* m_Buffer;
};
