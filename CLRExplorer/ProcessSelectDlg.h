#pragma once

#include "resource.h"
#include "VirtualListView.h"

class CProcessSelectDlg : 
	public CDialogImpl<CProcessSelectDlg>,
	public CVirtualListView<CProcessSelectDlg>,
	public CCustomDraw<CProcessSelectDlg>{
public:
	friend struct CVirtualListView<CProcessSelectDlg>;

	enum class RuntimeType {
		DesktopClr,
		CoreClr
	};

	enum class ProcessArch {
		Bit32,
		Bit64,
	};

	struct ProcessInfo {
		CString Name;
		CString Path;
		DWORD Id;
		DWORD Session;
		ProcessArch Arch;
		RuntimeType Runtime;
		int Image;
	};

	enum { IDD = IDD_PROCSELECT };

	int GetSelectedProcess(CString& name) const;

	BEGIN_MSG_MAP(CProcessSelectDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClickItem)
		CHAIN_MSG_MAP(CVirtualListView<CProcessSelectDlg>)
		CHAIN_MSG_MAP(CCustomDraw<CProcessSelectDlg>)
	END_MSG_MAP()

protected:
	void DoSort(const SortInfo* si);

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnDblClickItem(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void InitProcessList();
	void EnumProcesses();
	bool IsManagedProcess(DWORD pid, RuntimeType& rt, ProcessArch& arch) const;
	static bool CompareItems(const ProcessInfo& p1, const ProcessInfo& p2, int col, bool asc);

private:
	std::vector<ProcessInfo> m_Items;
	int m_SelectedPid = -1;
	CString m_Name;
	CListViewCtrl m_List;
	CImageList m_Images;
};

