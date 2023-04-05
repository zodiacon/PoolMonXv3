// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "AboutDlg.h"
#include "MainFrm.h"
#include "Helpers.h"
#include <fstream>
#include <ThemeHelper.h>
#include <SortHelper.h>
#include <ToolbarHelper.h>
#include <ClipboardHelper.h>
#include <ListViewhelper.h>

#pragma comment(lib, "ntdll")

enum class SystemInformationClass {
	SystemPoolTagInformation = 22,
};

extern "C"
NTSTATUS NTAPI NtQuerySystemInformation(
	_In_ SystemInformationClass SystemInformationClass,
	_Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
);

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

CString CMainFrame::GetColumnText(HWND h, int row, int col) const {
	auto& info = m_PoolTags[row];
	switch (GetColumnManager(h)->GetColumnTag<ColumnType>(col)) {
		case ColumnType::TagName: return GetTagAsString(info.TagUlong);
		case ColumnType::TagValue: return std::format(L"0x{:08X}", info.TagUlong).c_str();
		case ColumnType::PagedAllocs: return Helpers::FormatWithCommas(info.PagedAllocs);
		case ColumnType::PagedFrees: return Helpers::FormatWithCommas(info.PagedFrees);
		case ColumnType::PagedUsage: return Helpers::FormatWithCommas(info.PagedUsed);
		case ColumnType::PagedDiff: return Helpers::FormatWithCommas(info.PagedAllocs - info.PagedFrees);
		case ColumnType::NonPagedAllocs: return Helpers::FormatWithCommas(info.NonPagedAllocs);
		case ColumnType::NonPagedFrees: return Helpers::FormatWithCommas(info.NonPagedFrees);
		case ColumnType::NonPagedUsage: return Helpers::FormatWithCommas(info.NonPagedUsed);
		case ColumnType::NonPagedDiff: return Helpers::FormatWithCommas(info.NonPagedAllocs - info.NonPagedFrees);
		case ColumnType::SourceName: return GetTagSource(info.TagUlong).c_str();
		case ColumnType::SourceDescription: return GetTagDesc(info.TagUlong).c_str();
	}
	return CString();
}

void CMainFrame::DoSort(SortInfo const* si) {
	auto col = GetColumnManager(m_List)->GetColumnTag<ColumnType>(si->SortColumn);
	auto asc = si->SortAscending;

	auto compare = [&](auto& t1, auto& t2) {
		switch (col) {
			case ColumnType::TagName: return SortHelper::Sort(CString((const char*)t1.Tag, 4), CString((const char*)t2.Tag, 4), asc);
			case ColumnType::TagValue: return SortHelper::Sort(t1.TagUlong, t2.TagUlong, asc);
			case ColumnType::PagedAllocs: return SortHelper::Sort(t1.PagedAllocs, t2.PagedAllocs, asc);
			case ColumnType::PagedFrees: return SortHelper::Sort(t1.PagedFrees, t2.PagedFrees, asc);
			case ColumnType::PagedUsage: return SortHelper::Sort(t1.PagedUsed, t2.PagedUsed, asc);
			case ColumnType::PagedDiff: return SortHelper::Sort((LONG64)t1.PagedAllocs - (LONG64)t1.PagedFrees, (LONG64)t2.PagedAllocs - (LONG64)t2.PagedFrees, asc);
			case ColumnType::NonPagedAllocs: return SortHelper::Sort(t1.NonPagedAllocs, t2.NonPagedAllocs, asc);
			case ColumnType::NonPagedFrees: return SortHelper::Sort(t1.NonPagedFrees, t2.NonPagedFrees, asc);
			case ColumnType::NonPagedUsage: return SortHelper::Sort(t1.NonPagedUsed, t2.NonPagedUsed, asc);
			case ColumnType::NonPagedDiff: return SortHelper::Sort((LONG64)t1.NonPagedAllocs - (LONG64)t1.NonPagedFrees, (LONG64)t2.NonPagedAllocs - (LONG64)t2.NonPagedFrees, asc);
			case ColumnType::SourceName: return SortHelper::Sort(GetTagSource(t1.TagUlong), GetTagSource(t2.TagUlong), asc);
			case ColumnType::SourceDescription: return SortHelper::Sort(GetTagDesc(t1.TagUlong), GetTagDesc(t2.TagUlong), asc);
		}
		return false;
	};

	m_PoolTags.Sort(compare);
}

int CMainFrame::GetSaveColumnRange(HWND, int&) const {
	return 1;
}

void CMainFrame::OnStateChanged(HWND, int from, int to, UINT oldState, UINT newState) {
	if ((oldState & LVIS_SELECTED) || (newState & LVIS_SELECTED))
		UpdateUI();
}

DWORD CMainFrame::OnPrePaint(int, LPNMCUSTOMDRAW cd) {
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CMainFrame::OnItemPrePaint(int, LPNMCUSTOMDRAW cd) {
	if (cd->hdr.hwndFrom != m_List) {
		SetMsgHandled(FALSE);
		return 0;
	}

	auto lv = (NMLVCUSTOMDRAW*)cd;
	int row = (int)cd->dwItemSpec;
	auto& tag = m_PoolTags[row].TagUlong;

	if (auto it = m_Changes.find(tag | (int64_t)ColumnType::All << 32); it != m_Changes.end()) {
		lv->clrTextBk = it->second.BackColor;
		return CDRF_DODEFAULT;
	}
	else {
		lv->clrTextBk = CLR_INVALID;
	}

	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CMainFrame::OnSubItemPrePaint(int, LPNMCUSTOMDRAW cd) {
	auto lv = (NMLVCUSTOMDRAW*)cd;
	int row = (int)cd->dwItemSpec;
	auto& tag = m_PoolTags[row].TagUlong;

	if (auto it = m_Changes.find(tag | (int64_t)GetColumnManager(m_List)->GetColumnTag(lv->iSubItem) << 32); it != m_Changes.end()) {
		lv->clrTextBk = it->second.BackColor;
	}
	else {
		lv->clrTextBk = CLR_INVALID;
	}
	if (lv->iSubItem == 0)
		m_hOldFont = (HFONT)::SelectObject(cd->hdc, m_MonoFont);
	else if (m_hOldFont) {
		::SelectObject(cd->hdc, m_hOldFont);
		m_hOldFont = nullptr;
	}

	return CDRF_DODEFAULT;
}

void CMainFrame::Refresh() {
	if (::NtQuerySystemInformation(SystemInformationClass::SystemPoolTagInformation, m_Buffer, 1 << 22, nullptr))
		return;

	auto tags = static_cast<SYSTEM_POOLTAG_INFORMATION*>(m_Buffer);
	std::vector<SYSTEM_POOLTAG> vtags(tags->TagInfo, tags->TagInfo + tags->Count);
	if (m_PoolTags.empty()) {
		m_PoolTags = std::move(vtags);
		for (auto& tag : m_PoolTags)
			m_PoolTagsMap.insert({ tag.TagUlong, tag });
	}
	else {
		m_PoolTags.clear();
		auto tick = ::GetTickCount64();
		for (DWORD i = 0; i < tags->Count; i++) {
			auto& tag = tags->TagInfo[i];
			if (!m_PoolTagsMap.contains(tag.TagUlong)) {
				//
				// new tag appears
				//
				m_PoolTags.push_back(tag);
				m_PoolTagsMap.insert({ tag.TagUlong, tag });
				Change c;
				c.BackColor = GetNewColor();
				c.Type = ColumnType::All;
				c.TargetTime = tick + m_Delay;
				c.New = true;
				m_Changes.insert({ tag.TagUlong | ((int64_t)c.Type << 32), c });
			}
			else {
				auto& t = m_PoolTagsMap.at(tag.TagUlong);
				AddChanges(t, tag);
				m_PoolTags.push_back(tag);
				t = tag;
			}
		}
	}
	m_List.SetItemCountEx((int)m_PoolTags.size(), LVSICF_NOSCROLL);
	Sort(m_List);
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
}

void CMainFrame::UpdateUI() {
	UIEnable(ID_EDIT_COPY, m_List.GetSelectedCount() > 0);
}

bool CMainFrame::LoadPoolTags() {
	//
	// look for pooltag.txt
	//
	PWSTR path;
	std::wstring spath;
	if (S_OK == ::SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, nullptr, &path)) {
		spath = path;
		spath += LR"(\Windows Kits\10\Debuggers\x86\triage\pooltag.txt)";
		::CoTaskMemFree(path);
		if (::GetFileAttributes(spath.c_str()) == INVALID_FILE_ATTRIBUTES)
			spath.clear();
	}
	if (spath.empty()) {
		//
		// no file found, get from resource
		//
		spath = Helpers::ExtractResource(IDR_TAGS, L"TXT", L"tags");
	}
	if (spath.empty())
		return false;

	std::ifstream stm;
	stm.open(spath.c_str());
	if (!stm.good())
		return false;

	char line[256];
	while (!stm.eof()) {
		stm.getline(line, sizeof(line));
		if (strlen(line) == 0 || strncmp(line, "//", 2) == 0 || strncmp(line, "rem", 3) == 0)
			continue;

		//
		// parse line
		//
		KnownPoolTag pt;
		pt.TagAsString.assign(line, line + 4);
		pt.Tag = 0;
		int shift = 0;
		for (int i = 0; i < 4; i++) {
			pt.Tag |= line[i] == ' ' ? 0 : (line[i] << shift);
			shift += 8;
		}

		auto dash = strchr(line + 5, '-');
		if (!dash)
			continue;
		auto space = strchr(dash + 2, ' ');
		if (!space)
			continue;
		pt.Name.assign(dash + 2, space);
		auto dash2 = strchr(space + 1, '-');
		if (dash2) {
			dash2++;
			while (*++dash2 == ' ')
				;
			pt.Desc = (PCWSTR)CString(dash2);
		}
		m_KnownTags.insert({ pt.Tag, std::move(pt) });
	}

	return false;
}

CString const& CMainFrame::GetTagAsString(ULONG tag) const {
	if (auto it = m_TagToString.find(tag); it != m_TagToString.end())
		return it->second;

	m_TagToString.insert({ tag, CString((const char*)&tag, 4) });
	return m_TagToString.at(tag);
}

std::wstring const& CMainFrame::GetTagSource(ULONG tag) const {
	if (auto it = m_KnownTags.find(tag); it != m_KnownTags.end())
		return it->second.Name;
	static std::wstring empty;
	return empty;
}

std::wstring const& CMainFrame::GetTagDesc(ULONG tag) const {
	if (auto it = m_KnownTags.find(tag); it != m_KnownTags.end())
		return it->second.Desc;
	static std::wstring empty;
	return empty;
}

COLORREF CMainFrame::GetNewColor() const {
	return ThemeHelper::IsDefault() ? RGB(0, 255, 0) : RGB(0, 128, 0);
}

COLORREF CMainFrame::GetOldColor() const {
	return ThemeHelper::IsDefault() ? RGB(255, 96, 0) : RGB(128, 0, 0);
}

int CMainFrame::AddChanges(SYSTEM_POOLTAG const& current, SYSTEM_POOLTAG const& next) {
	ATLASSERT(current.TagUlong == next.TagUlong);
	int changes = 0;
	changes += AddChange(current.TagUlong, current.PagedAllocs, next.PagedAllocs, ColumnType::PagedAllocs);
	changes += AddChange(current.TagUlong, current.PagedFrees, next.PagedFrees, ColumnType::PagedFrees);
	changes += AddChange(current.TagUlong, current.PagedUsed, next.PagedUsed, ColumnType::PagedUsage);
	changes += AddChange(current.TagUlong, (LONG64)current.PagedAllocs - (LONG64)current.PagedFrees, (LONG64)next.PagedAllocs - (LONG64)next.PagedFrees, ColumnType::PagedDiff);

	changes += AddChange(next.TagUlong, current.NonPagedAllocs, next.NonPagedAllocs, ColumnType::NonPagedAllocs);
	changes += AddChange(next.TagUlong, current.NonPagedFrees, next.NonPagedFrees, ColumnType::NonPagedFrees);
	changes += AddChange(next.TagUlong, current.NonPagedUsed, next.NonPagedUsed, ColumnType::NonPagedUsage);
	changes += AddChange(next.TagUlong, (LONG64)current.NonPagedAllocs - (LONG64)current.NonPagedFrees, (LONG64)next.NonPagedAllocs - (LONG64)next.NonPagedFrees, ColumnType::NonPagedDiff);

	return changes;
}

int CMainFrame::AddChange(ULONG tag, LONG64 current, LONG64 next, ColumnType type) {
	if (current == next)
		return 0;

	Change c;
	c.New = next > current;
	c.Type = type;
	c.BackColor = c.New ? GetNewColor() : GetOldColor();
	c.TargetTime = ::GetTickCount64() + m_Delay;
	m_Changes.insert({ tag | ((int64_t)type << 32), c });

	return 1;
}

bool CMainFrame::DoSave(bool all, PCWSTR path) const {
	std::ofstream stm;
	stm.open(path, std::ios::out);
	if (!stm.good())
		return false;

	auto text = all ? ListViewHelper::GetAllRowsAsString(m_List, L",", L"\n") : 
		ListViewHelper::GetSelectedRowsAsString(m_List, L",", L"\n");
	stm.write(CStringA(text), text.GetLength());
	stm.close();

	return true;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CreateSimpleStatusBar();
	m_StatusBar.SubclassWindow(m_hWndStatusBar);
	int parts[] = { 100, 300, 500 };
	m_StatusBar.SetParts(_countof(parts), parts);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	ToolBarButtonInfo const buttons[] = {
		{ ID_VIEW_RUN, IDI_RUN },
		{ ID_VIEW_PAUSE, IDI_PAUSE },
		{ 0 },
		{ ID_EDIT_COPY, IDI_COPY },
		{ 0 },
		{ ID_EDIT_FIND, IDI_FIND },
	};
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	auto tb = ToolbarHelper::CreateAndInitToolBar(m_hWnd, buttons, _countof(buttons));
	AddSimpleReBarBand(tb);
	UIAddToolBar(tb);
	
	CRect rc(0, 0, 0, 20);
	m_QuickEdit.Create(m_hWnd, rc, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER);
	AddSimpleReBarBand(m_QuickEdit, nullptr, 0, 200, 1);
	SizeSimpleReBarBands();
	m_QuickEdit.SetWindowPos(nullptr, 0, 0, 200, 20, SWP_NOMOVE | SWP_NOREPOSITION);

	m_hWndClient = m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_QuickEdit.SetFont(m_List.GetFont());

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"Tag", LVCFMT_LEFT, 60, ColumnType::TagName);
	cm->AddColumn(L"Value", LVCFMT_RIGHT, 90, ColumnType::TagValue);
	cm->AddColumn(L"Paged Allocs", LVCFMT_RIGHT, 90, ColumnType::PagedAllocs);
	cm->AddColumn(L"Paged Frees", LVCFMT_RIGHT, 90, ColumnType::PagedFrees);
	cm->AddColumn(L"Paged Diff", LVCFMT_RIGHT, 90, ColumnType::PagedDiff);
	cm->AddColumn(L"Paged Usage", LVCFMT_RIGHT, 90, ColumnType::PagedUsage);
	cm->AddColumn(L"NPaged Allocs", LVCFMT_RIGHT, 90, ColumnType::NonPagedAllocs);
	cm->AddColumn(L"NPaged Frees", LVCFMT_RIGHT, 90, ColumnType::NonPagedFrees);
	cm->AddColumn(L"NPaged Diff", LVCFMT_RIGHT, 90, ColumnType::NonPagedDiff);
	cm->AddColumn(L"NPaged Usage", LVCFMT_RIGHT, 90, ColumnType::NonPagedUsage);
	cm->AddColumn(L"Source", LVCFMT_LEFT, 150, ColumnType::SourceName);
	cm->AddColumn(L"Description", LVCFMT_LEFT, 300, ColumnType::SourceDescription);

	// register object for message filtering and idle updates
	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	LoadPoolTags();

	auto menu = GetMenu();
	InitMenu();
	SetCheckIcon(IDI_CHECK, IDI_RADIO);

	AddMenu(menu);
	UIAddMenu(menu);
	UISetCheck(ID_VIEW_RUN, TRUE);

	LOGFONT lf;
	((CFontHandle)m_List.GetFont()).GetLogFont(lf);
	wcscpy_s(lf.lfFaceName, L"Consolas");
	lf.lfWeight = FW_BOLD;
	m_MonoFont.CreateFontIndirect(&lf);

	m_Buffer = ::VirtualAlloc(nullptr, 1 << 22, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!m_Buffer) {
		AtlMessageBox(m_hWnd, L"Out of memory!", IDR_MAINFRAME, MB_ICONERROR);
		return -1;
	}

	Refresh();
	SetTimer(1, m_Interval);
	UpdateUI();

	return 0;
}

void CMainFrame::InitMenu() {
	struct {
		UINT id, icon;
	} cmds[] = {
		{ ID_EDIT_COPY, IDI_COPY },
		{ ID_EDIT_FIND, IDI_FIND },
		{ ID_FILE_SAVE, IDI_SAVE },
		{ ID_FILE_OPEN, IDI_OPEN },
		{ ID_VIEW_RUN, IDI_RUN },
		{ ID_VIEW_PAUSE, IDI_PAUSE },
		{ ID_OPTIONS_ALWAYSONTOP, IDI_PIN },
	};
	for (auto& cmd : cmds) {
		AddCommand(cmd.id, cmd.icon);
	}
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	::VirtualFree(m_Buffer, 0, MEM_RELEASE);

	auto pLoop = _Module.GetMessageLoop();
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;

	return 1;
}

LRESULT CMainFrame::OnTimer(UINT, WPARAM id, LPARAM, BOOL& bHandled) {
	if (id == 1) {
		auto tick = ::GetTickCount64();
		std::vector<ULONG64> tags;
		for (auto& [t, c] : m_Changes)
			if (tick > c.TargetTime) {
				tags.push_back(t);
			}
		for (auto& t : tags)
			m_Changes.erase(t);

		Refresh();
	}
	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnAlwaysOnTop(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto onTop = !(GetExStyle() & WS_EX_TOPMOST);
	SetWindowPos(onTop ? HWND_TOPMOST : HWND_NOTOPMOST, &rcDefault, SWP_NOSIZE | SWP_NOMOVE);
	UISetCheck(id, onTop);
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnFileSave(WORD, WORD, HWND, BOOL&) {
	auto running = m_IsRunning;
	if(running)
		SendMessage(WM_COMMAND, ID_VIEW_PAUSE);
	CSimpleFileDialog dlg(FALSE, L"csv", nullptr, OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
		L"CSV Files (*.csv)\0*.csv\0Text Files (*.txt)\0*.txt\0All Files\0*.*\0", m_hWnd);
	ThemeHelper::Suspend();
	auto ok = IDOK == dlg.DoModal();
	ThemeHelper::Resume();
	if (ok) {
		if (!DoSave(true, dlg.m_szFileName))
			AtlMessageBox(m_hWnd, L"Failed to save file.", IDR_MAINFRAME, MB_ICONERROR);
	}
	if (running)
		SendMessage(WM_COMMAND, ID_VIEW_RUN);
	return LRESULT();
}

LRESULT CMainFrame::OnEditFind(WORD, WORD, HWND, BOOL&) {
	return LRESULT();
}

LRESULT CMainFrame::OnViewPause(WORD src, WORD id, HWND, BOOL& handled) {
	if (!m_IsRunning) {
		if (src)
			SendMessage(WM_COMMAND, ID_VIEW_RUN);
		handled = FALSE;
		return 0;
	}
	KillTimer(1);
	UISetCheck(ID_VIEW_RUN, FALSE);
	UISetCheck(ID_VIEW_PAUSE, TRUE);
	m_IsRunning = false;

	return 0;
}

LRESULT CMainFrame::OnViewRun(WORD, WORD, HWND, BOOL& handled) {
	if (m_IsRunning) {
		handled = FALSE;
		return 0;
	}
	SetTimer(1, m_Interval);
	UISetCheck(ID_VIEW_RUN, TRUE);
	UISetCheck(ID_VIEW_PAUSE, FALSE);
	m_IsRunning = true;

	return 0;
}

LRESULT CMainFrame::OnEditCopy(WORD, WORD, HWND, BOOL&) {
	auto text = ListViewHelper::GetSelectedRowsAsString(m_List, L",");
	ClipboardHelper::CopyText(m_hWnd, text);

	return 0;
}

LRESULT CMainFrame::OnEditSelectAll(WORD, WORD, HWND, BOOL&) {
	m_List.SelectAllItems(true);

	return 0;
}
