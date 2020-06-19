#include "pch.h"
#include "ProcessSelectDlg.h"
#include <string>
#include <TlHelp32.h>
#include <algorithm>
#include "SortHelper.h"

int CProcessSelectDlg::GetSelectedProcess(CString& name) const {
	name = m_Name;
	return m_SelectedPid;
}

void CProcessSelectDlg::DoSort(const SortInfo* si) {
	std::sort(m_Items.begin(), m_Items.end(), [si](const auto& p1, const auto& p2) {
		return CompareItems(p1, p2, si->SortColumn, si->SortAscending);
		});
}

LRESULT CProcessSelectDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	CenterWindow(GetParent());
	m_List.Attach(GetDlgItem(IDC_PROCLIST));

	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_List.InsertColumn(0, L"Name", LVCFMT_LEFT, 170);
	m_List.InsertColumn(1, L"PID", LVCFMT_RIGHT, 130);
	m_List.InsertColumn(2, L"Session", LVCFMT_CENTER, 70);
	m_List.InsertColumn(3, L"CLR", LVCFMT_LEFT, 70);
	m_List.InsertColumn(4, L"Arch", LVCFMT_CENTER, 70);

	m_Images.Create(16, 16, ILC_COLOR32, 32, 16);
	m_Images.AddIcon(AtlLoadSysIcon(IDI_APPLICATION));
	m_List.SetImageList(m_Images, LVSIL_SMALL);

	InitProcessList();

	return TRUE;
}

LRESULT CProcessSelectDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	EndDialog(wID);
	return 0;
}

LRESULT CProcessSelectDlg::OnItemChanged(int, LPNMHDR, BOOL&) {
	auto selected = m_List.GetSelectedIndex();
	GetDlgItem(IDOK).EnableWindow(selected >= 0);
	if (selected >= 0) {
		m_SelectedPid = m_Items[selected].Id;
		m_Name = m_Items[selected].Name;
	}

	return 0;
}

LRESULT CProcessSelectDlg::OnGetDispInfo(int, LPNMHDR hdr, BOOL&) {
	auto di = reinterpret_cast<NMLVDISPINFO*>(hdr);
	auto& item = di->item;
	auto row = item.iItem;
	auto col = item.iSubItem;
	const auto& data = m_Items[row];

	if (item.mask & LVIF_TEXT) {
		switch (col) {
			case 0:		// name
				item.pszText = (PWSTR)(PCWSTR)data.Name;
				break;

			case 1:		// ID
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d (0x%X)", data.Id, data.Id);
				break;

			case 2:		// session
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.Session);
				break;

			case 3:		// CLR
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%s", data.Runtime == RuntimeType::DesktopClr ? L"Desktop" : L"Core");
				break;

			case 4:
				::StringCchCopy(item.pszText, item.cchTextMax, data.Arch == ProcessArch::Bit64 ? L"64 bit" : L"32 bit");
				break;
		}
	}
	if (item.mask & LVIF_IMAGE)
		item.iImage = data.Image;

	return 0;
}

LRESULT CProcessSelectDlg::OnDblClickItem(int, LPNMHDR, BOOL&) {
	auto selected = m_List.GetSelectedIndex();
	if (selected >= 0)
		EndDialog(IDOK);
	return 0;
}

void CProcessSelectDlg::InitProcessList() {
	EnumProcesses();
	m_List.SetItemCount(static_cast<int>(m_Items.size()));
}

void CProcessSelectDlg::EnumProcesses() {
	auto hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	ATLASSERT(hSnapshot != INVALID_HANDLE_VALUE);
	m_Items.clear();

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	Process32First(hSnapshot, &pe);
	WCHAR path[MAX_PATH];
	DWORD len;
	do {
		auto hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
		if (!hProcess)
			continue;

		RuntimeType rt;
		ProcessArch arch;
		if (IsManagedProcess(pe.th32ProcessID, rt, arch)) {
			ProcessInfo pi;
			pi.Name = pe.szExeFile;
			len = MAX_PATH;
			if (::QueryFullProcessImageName(hProcess, 0, path, &len))
				pi.Path = path;
			HICON hIcon;
			if (::ExtractIconEx(pi.Path, 0, nullptr, &hIcon, 1)) {
				pi.Image = m_Images.AddIcon(hIcon);
			}
			else
				pi.Image = 0;
			pi.Id = pe.th32ProcessID;
			::ProcessIdToSessionId(pi.Id, &pi.Session);
			pi.Arch = arch;
			pi.Runtime = rt;
			m_Items.push_back(std::move(pi));
		}
		::CloseHandle(hProcess);
	} while (::Process32Next(hSnapshot, &pe));
	::CloseHandle(hSnapshot);
}

bool CProcessSelectDlg::IsManagedProcess(DWORD pid, RuntimeType& rt, ProcessArch& arch) const {
	auto hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return false;

	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	Module32First(hSnapshot, &me);
	bool found = false;
	do {
		if (::_wcsicmp(L"clr.dll", me.szModule) == 0) {
			rt = RuntimeType::DesktopClr;
			found = true;
			break;
		}
		if (::_wcsicmp(L"coreclr.dll", me.szModule) == 0) {
			rt = RuntimeType::CoreClr;
			found = true;
			break;
		}
	} while (::Module32Next(hSnapshot, &me));
	if(found)
		arch = sizeof(void*) == 8 ? ProcessArch::Bit64 : ProcessArch::Bit32;
	::CloseHandle(hSnapshot);

	return found;
}

bool CProcessSelectDlg::CompareItems(const ProcessInfo& p1, const ProcessInfo& p2, int col, bool asc) {
	switch (col) {
		case 0:	return SortHelper::SortStrings(p1.Name, p2.Name, asc);
		case 1: return SortHelper::SortNumbers(p1.Id, p2.Id, asc);
		case 2: return SortHelper::SortNumbers(p1.Session, p2.Session, asc);
		case 3: return SortHelper::SortNumbers(p1.Runtime, p2.Runtime, asc);
		case 4: return SortHelper::SortNumbers(p1.Arch, p2.Arch, asc);
	}
	ATLASSERT(false);
	return false;
}
