#pragma once

#include <VirtualListView.h>
#include <OwnerDrawnMenu.h>
#include <SortedFilteredVector.h>
#include <QuickFindEdit.h>

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
	void DoSort(SortInfo const* si);
	int GetSaveColumnRange(HWND, int&) const;
	void OnStateChanged(HWND, int from, int to, UINT oldState, UINT newState);
	bool OnRightClickList(HWND, int row, int col, POINT const& pt);

	DWORD OnPrePaint(int, LPNMCUSTOMDRAW cd);
	DWORD OnItemPrePaint(int, LPNMCUSTOMDRAW cd);
	DWORD OnSubItemPrePaint(int, LPNMCUSTOMDRAW cd);

protected:
	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_CODE_HANDLER(EN_DELAYCHANGE, OnSearchTextChanged)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnEditFind)
		COMMAND_ID_HANDLER(ID_EDIT_FIND_NEXT, OnEditFindNext)
		COMMAND_ID_HANDLER(ID_VIEW_RUN, OnViewRun)
		COMMAND_ID_HANDLER(ID_VIEW_PAUSE, OnViewPause)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnEditSelectAll)
		COMMAND_ID_HANDLER(ID_OPTIONS_ALWAYSONTOP, OnAlwaysOnTop)
		COMMAND_ID_HANDLER(ID_EDIT_QUICKFIND, OnQuickFind)
		MESSAGE_HANDLER(CFindReplaceDialog::GetFindReplaceMsg(), OnFind)
		COMMAND_RANGE_HANDLER(ID_UPDATEINTERVAL_0, ID_UPDATEINTERVAL_5SECONDS, OnUpdateInterval)
		MESSAGE_HANDLER(WM_MENUSELECT, [](auto, auto, auto, auto) { return 0; })
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
		COMMAND_ID_HANDLER(ID_FILE_SAVESELECTION, OnFileSaveSelected)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_HELP_ABOUTWINDOWS, OnAboutWindows)
		CHAIN_MSG_MAP(CCustomDraw<CMainFrame>)
		CHAIN_MSG_MAP(CVirtualListView<CMainFrame>)
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
		All
	};

	void InitMenu();
	void Refresh();
	void UpdateUI();
	bool LoadPoolTags();
	CString const& GetTagAsString(ULONG tag) const;
	std::wstring const& GetTagSource(ULONG tag) const;
	std::wstring const& GetTagDesc(ULONG tag) const;
	COLORREF GetNewColor() const;
	COLORREF GetOldColor() const;
	int AddChanges(SYSTEM_POOLTAG const& current, SYSTEM_POOLTAG const& next);
	int AddChange(ULONG tag, LONG64 current, LONG64 next, ColumnType type);
	bool DoSave(bool all, PCWSTR path) const;
	void SaveCommon(bool all);
	void UpdateIntervalText();

	struct KnownPoolTag {
		ULONG Tag;
		std::string TagAsString;
		std::wstring Name;
		std::wstring Desc;
	};
	
	struct Change {
		ColumnType Type;
		COLORREF BackColor;
		DWORD64 TargetTime;
		bool New;
	};

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAlwaysOnTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAboutWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSaveSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnQuickFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSearchTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnUpdateInterval(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CListViewCtrl m_List;
	CMultiPaneStatusBarCtrl m_StatusBar;
	CQuickFindEdit m_QuickEdit;
	CFindReplaceDialog* m_pFindDlg{ nullptr };
	std::unordered_map<ULONG, KnownPoolTag> m_KnownTags;
	SortedFilteredVector<SYSTEM_POOLTAG> m_PoolTags;
	std::unordered_map<ULONG, SYSTEM_POOLTAG> m_PoolTagsMap;
	mutable std::unordered_map<ULONG, CString> m_TagToString;
	std::unordered_map<ULONG64, Change> m_Changes;
	void* m_Buffer;
	int m_Interval{ 1000 };
	int m_Delay{ 1000 };
	CFont m_MonoFont;
	HFONT m_hOldFont{ nullptr };
	int m_IntervalIndex{ 1 };
	std::function<bool(SYSTEM_POOLTAG const&, int)> m_Filter;
	mutable bool m_IsRunning{ false };
	CString m_SearchText;
	inline static int s_Intervals[]{ 500, 1000, 2000, 5000 };
};
