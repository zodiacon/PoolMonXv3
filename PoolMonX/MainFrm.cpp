// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "AboutDlg.h"
#include "MainFrm.h"
#include "Helpers.h"
#include <fstream>

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

void CMainFrame::Refresh() {
	if (::NtQuerySystemInformation(SystemInformationClass::SystemPoolTagInformation, m_Buffer, 1 << 22, nullptr))
		return;

	auto tags = static_cast<SYSTEM_POOLTAG_INFORMATION*>(m_Buffer);
	std::vector<SYSTEM_POOLTAG> vtags(tags->TagInfo, tags->TagInfo + tags->Count);
	if (m_PoolTags.empty()) {
		m_PoolTags = std::move(vtags);
	}
	else {
	}
	m_List.SetItemCountEx((int)m_PoolTags.size(), LVSICF_NOSCROLL);
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

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CreateSimpleStatusBar();
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	m_hWndClient = m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

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

	m_Buffer = ::VirtualAlloc(nullptr, 1 << 22, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!m_Buffer) {
		AtlMessageBox(m_hWnd, L"Out of memory!", IDR_MAINFRAME, MB_ICONERROR);
		return -1;
	}

	Refresh();

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	::VirtualFree(m_Buffer, 0, MEM_RELEASE);

	auto pLoop = _Module.GetMessageLoop();
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;

	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnAlwaysOnTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	// TODO: add code to initialize document

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
