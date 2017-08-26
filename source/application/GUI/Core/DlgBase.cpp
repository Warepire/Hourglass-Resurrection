/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>
#include <CommCtrl.h>

#include <cassert>
#include <cwchar>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "shared/Alignment.h"

#include "Menu.h"

#include "DlgBase.h"

/*
 * wmemcpy is used over wcscpy, swprintf etc in this code. This is due to these methods end up
 * corrupting the DLGTEMPLATE structures causing Windows to read memory continously until it
 * reaches a memory access violation exception (and thus the program crashes).
 * -- Warepire
 */

/*
 * Reference DLGTEMPLATEEX structure:
 * struct DLGTEMPLATEEX
 * {
 *     WORD      dlgVer;
 *     WORD      signature;
 *     DWORD     helpID;
 *     DWORD     exStyle;
 *     DWORD     style;
 *     WORD      cDlgItems;            --> Caps out at 0xFFFF items.
 *     short     x;
 *     short     y;
 *     short     cx;
 *     short     cy;
 *     sz_Or_Ord menu;                 --> Does not accept a HMENU, Use "SetMenu"
 *     sz_Or_Ord windowClass;
 *     WCHAR     title[titleLen];
 *     WORD      pointsize;
 *     WORD      weight;
 *     BYTE      italic;
 *     BYTE      charset;
 *     WCHAR     typeface[stringLen];  --> "MS Shell Dlg" pre-defined
 * };
 *
 * Reference DLGITEMTEMPLATEEX structure:
 * struct DLGITEMTEMPLATEEX
 * {
 *     DWORD     helpID;
 *     DWORD     exStyle;
 *     DWORD     style;
 *     short     x;
 *     short     y;
 *     short     cx;
 *     short     cy;
 *     DWORD     id;
 *     sz_Or_Ord windowClass;
 *     sz_Or_Ord title[titleLen];
 *     WORD      extraCount;
 * };
 */

namespace
{
    enum : int
    {
        IDC_DLGBASE_INTERNAL_MENU_ANCHOR = -1,
    };
    enum : DWORD
    {
        /*
         * The title must always contain at least a NULL character.
         * The typeface is the string L"MS Shell Dlg\0".
         */
        DLGTEMPLATEEX_BASE_SIZE = (sizeof(WORD) * 2) +
                                  (sizeof(DWORD) * 3) +
                                  (sizeof(WORD) * 1) +
                                  (sizeof(SHORT) * 4) +
                                  (sizeof(WORD) * 1) +
                                  (sizeof(WORD) * 1) +
                                  (sizeof(WCHAR) * 1) +
                                  (sizeof(WORD) * 2) +
                                  (sizeof(BYTE) * 2) +
                                  (sizeof(WCHAR) * 13),

        CDLGITEMS_OFFSET = (sizeof(WORD) * 2) +
                           (sizeof(DWORD) * 3),

        /*
         * The windowClass will be evaluated during object construction.
         * The title must always contain at least a NULL character.
         */
        DLGITEMTEMPLATEEX_BASE_SIZE = (sizeof(DWORD) * 3) +
                                      (sizeof(SHORT) * 4) +
                                      (sizeof(DWORD) * 1) +
                                      (sizeof(WCHAR) * 0) +
                                      (sizeof(WCHAR) * 1) +
                                      (sizeof(WORD) * 1),
    };
}

/*
 * These classes provide type erasure on callback functions to simplify
 * the call process from the message loop to the event handlers.
 */
class CallbackBase
{
public:
    virtual ~CallbackBase()
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam) = 0;
};

class Callback0 :
    public CallbackBase
{
public:
    Callback0(std::function<bool()> cb) :
        callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return callback();
    }
private:
    std::function<bool()> callback;
};

template<typename T>
class CallbackLparam : public CallbackBase
{
public:
    CallbackLparam(std::function<bool(T)> cb) :
        callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return callback(reinterpret_cast<T>(lparam));
    }
private:
    std::function<bool(T)> callback;
};

template<typename T>
class CallbackWparam : public CallbackBase
{
public:
    CallbackWparam(std::function<bool(T)> cb) :
        callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return callback(reinterpret_cast<T>(wparam));
    }
private:
    std::function<bool(T)> callback;
};

template<typename T, typename U>
class CallbackBoth : public CallbackBase
{
public:
    CallbackBoth(std::function<bool(T, U)> cb) :
        callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return callback(reinterpret_cast<T>(lparam), reinterpret_cast<U>(wparam));
    }
private:
    std::function<bool(T, U)> callback;
};

class CallbackWmCommand : public CallbackBase
{
public:
    CallbackWmCommand(std::function<bool(WORD)> cb) :
        callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return callback(HIWORD(wparam));
    }
private:
    std::function<bool(WORD)> callback;
};

std::map<HWND, DlgBase*> DlgBase::ms_hwnd_dlgbase_map;

DWORD DlgBase::ms_ref_count = 0;

namespace
{
    HINSTANCE gs_instance;
}

DlgBase::DlgBase(const std::wstring& caption, SHORT x, SHORT y, SHORT w, SHORT h, DlgType type) :
    m_handle(nullptr),
    m_mode(DlgMode::INDIRECT),
    m_return_code_set(false),
    m_return_code(0)
{
    DWORD iterator = 0;
    std::vector<BYTE>::size_type struct_size = DLGTEMPLATEEX_BASE_SIZE;

    /*
     * A bit hacky to avoid a special init function just to pass the isntance to us.
     * -- Warepire
     */
    if (ms_ref_count == 0)
    {
        gs_instance = GetModuleHandleW(nullptr);
    }

    ms_ref_count++;

    m_message_callbacks.emplace(WM_DESTROY, std::make_unique<Callback0>(std::bind(&DlgBase::DestroyCallback, this)));
    m_message_callbacks.emplace(WM_NCDESTROY, std::make_unique<Callback0>(std::bind(&DlgBase::NcDestroyCallback, this)));

    /*
     * Add the size of the caption
     */
    struct_size += (sizeof(WCHAR) * caption.size());

    /*
     * Make sure the size is DWORD aligned to put the first child objects at the right offset.
     */
    struct_size = AlignValueTo<sizeof(DWORD)>(struct_size);

    m_window.resize(struct_size);

    /*
     * Required values
     */
    *reinterpret_cast<LPWORD>(&(m_window[iterator])) = 0x0001;
    iterator += sizeof(WORD);
    *reinterpret_cast<LPWORD>(&(m_window[iterator])) = 0xFFFF;
    iterator += sizeof(WORD);

    /*
     * Leave HelpID and exStyle alone
     */
    iterator += (sizeof(DWORD) * 2);

    *reinterpret_cast<LPDWORD>(&(m_window[iterator])) = DS_SETFONT | DS_FIXEDSYS;
    switch (type)
    {
    case DlgType::NORMAL:
        *reinterpret_cast<LPDWORD>(&(m_window[iterator])) |= DS_MODALFRAME | WS_POPUP |
                                                             WS_CAPTION | WS_SYSMENU |
                                                             WS_MINIMIZEBOX;
        break;
    case DlgType::TAB_PAGE:
        *reinterpret_cast<LPDWORD>(&(m_window[iterator])) |= WS_CHILD | WS_CLIPSIBLINGS |
                                                             WS_VISIBLE;
        break;
    default:
        break;
    }
    iterator += sizeof(DWORD);

    /*
     * We'll set cDlgItems incrementally with the children.
     */
    iterator += sizeof(WORD);

    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = x;
    iterator += sizeof(SHORT);
    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = y;
    iterator += sizeof(SHORT);
    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = w;
    iterator += sizeof(SHORT);
    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = h;
    iterator += sizeof(SHORT);

    /*
     * Leave menu and windowClass alone
     */
    iterator += (sizeof(WORD) * 2);

    if (caption.empty() == false)
    {
        wmemcpy(reinterpret_cast<LPWSTR>(&(m_window[iterator])),
                caption.c_str(),
                caption.size());
        iterator += (sizeof(WCHAR) * caption.size());
    }
    iterator += sizeof(WCHAR);

    /*
     * pointsize 8
     */
    *reinterpret_cast<LPWORD>(&(m_window[iterator])) = 0x0008;
    iterator += sizeof(WORD);

    *reinterpret_cast<LPWORD>(&(m_window[iterator])) = static_cast<WORD>(FW_NORMAL);
    iterator += sizeof(WORD);
    m_window[iterator] = static_cast<BYTE>(FALSE);
    iterator += sizeof(BYTE);
    m_window[iterator] = 0x01;
    iterator += sizeof(BYTE);

    wmemcpy(reinterpret_cast<LPWSTR>(&(m_window[iterator])),
            L"MS Shell Dlg",
            ARRAYSIZE(L"MS Shell Dlg") - 1);

    /*
     * Hack to resize windows when adding menus.
     * -- Warepire
     */
    AddStaticText(L"", IDC_DLGBASE_INTERNAL_MENU_ANCHOR, 0, 0, 1, 1);
}

DlgBase::~DlgBase()
{
    ms_ref_count--;
}

void DlgBase::AddPushButton(const std::wstring& caption,
                            DWORD id,
                            SHORT x, SHORT y,
                            SHORT w, SHORT h,
                            bool disable,
                            bool default_choice)
{
    assert(!(disable && default_choice));
    DWORD style = (disable ? WS_DISABLED : 0);
    style |= (default_choice ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON);
    AddObject(0,
              WS_GROUP | WS_TABSTOP | style,
              L"\xFFFF\x0080",
              caption,
              id,
              x, y,
              w, h);
}

void DlgBase::AddCheckbox(const std::wstring& caption,
                          DWORD id,
                          SHORT x, SHORT y,
                          SHORT w, SHORT h,
                          bool right_hand)
{
    AddObject(0,
              WS_GROUP | WS_TABSTOP | BS_AUTOCHECKBOX | (right_hand ? BS_RIGHTBUTTON : 0),
              L"\xFFFF\x0080",
              caption,
              id,
              x, y,
              w, h);
}

void DlgBase::AddRadioButton(const std::wstring& caption,
                             DWORD id,
                             SHORT x, SHORT y,
                             SHORT w, SHORT h,
                             bool right_hand,
                             bool group_with_prev)
{
    DWORD style = (group_with_prev ? 0 : WS_GROUP) | (right_hand ? BS_RIGHTBUTTON : 0);
    AddObject(0,
              WS_TABSTOP | BS_AUTORADIOBUTTON | style,
              L"\xFFFF\x0080",
              caption,
              id,
              x, y,
              w, h);
}

void DlgBase::AddUpDownControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(0, WS_TABSTOP, L"msctls_updown32", L"", id, x, y, w, h);
}

void DlgBase::AddEditControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h, bool disabled, bool multi_line)
{
    DWORD style = (multi_line ? WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN : 0);
    style |= (disabled ? ES_READONLY : WS_TABSTOP);

    AddObject(0,
              WS_GROUP | WS_BORDER | ES_AUTOHSCROLL | style,
              L"\xFFFF\x0081",
              L"",
              id,
              x, y,
              w, h);
}

void DlgBase::AddStaticText(const std::wstring& caption, DWORD id,
                            SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(0, SS_LEFT, L"\xFFFF\x0082", caption, id, x, y, w, h);
}

void DlgBase::AddStaticPanel(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(0, SS_GRAYRECT, L"\xFFFF\x0082", L"", id, x, y, w, h);
}

void DlgBase::AddGroupBox(const std::wstring& caption, DWORD id,
                          SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(WS_EX_TRANSPARENT, BS_GROUPBOX, L"\xFFFF\x0080", caption, id, x, y, w, h);
}

void DlgBase::AddDropDownList(DWORD id, SHORT x, SHORT y, SHORT w, SHORT drop_distance)
{
    AddObject(0,
              WS_GROUP | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
              L"\xFFFF\x0085",
              L"",
              id,
              x, y,
              w,
              drop_distance);
}

void DlgBase::AddListView(DWORD id,
                          SHORT x, SHORT y,
                          SHORT w, SHORT h,
                          bool single_selection, bool owner_data)
{
    DWORD style = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;
    style |= (single_selection ? LVS_SINGLESEL : 0);
    style |= (owner_data ? LVS_OWNERDATA : 0);
    AddObject(0,
              WS_GROUP | WS_BORDER | WS_TABSTOP | style,
              L"SysListView32",
              L"",
              id,
              x, y,
              w, h);
}

void DlgBase::AddIPEditControl(DWORD id, SHORT x, SHORT y)
{
    AddObject(0, WS_GROUP | WS_TABSTOP, L"SysIPAddress32", L"", id, x, y, 100, 15);
}

void DlgBase::AddTabControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(0, WS_GROUP, L"SysTabControl32", L"", id, x, y, w, h);
}

void DlgBase::AddObject(DWORD ex_style, DWORD style,
                        const std::wstring& window_class,
                        const std::wstring& caption,
                        DWORD id,
                        SHORT x, SHORT y,
                        SHORT w, SHORT h)
{
    DWORD iterator = m_window.size();
    std::vector<BYTE>::size_type new_size = DLGITEMTEMPLATEEX_BASE_SIZE;
    new_size += (sizeof(WCHAR) * window_class.size());
    /*
     * Non-atomic class, also add space for the NULL.
     */
    if (window_class[0] != L'\xFFFF')
    {
        new_size += sizeof(WCHAR);
    }

    new_size += (sizeof(WCHAR) * caption.size());
    new_size += iterator;

    new_size = AlignValueTo<sizeof(DWORD)>(new_size);

    m_window.resize(new_size);

    /*
     * Leave helpID alone
     */
    iterator += sizeof(DWORD);

    *reinterpret_cast<LPDWORD>(&(m_window[iterator])) = ex_style;
    iterator += sizeof(DWORD);
    *reinterpret_cast<LPDWORD>(&(m_window[iterator])) = WS_CHILD | WS_VISIBLE | style;
    iterator += sizeof(DWORD);

    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = x;
    iterator += sizeof(SHORT);
    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = y;
    iterator += sizeof(SHORT);
    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = w;
    iterator += sizeof(SHORT);
    *reinterpret_cast<PSHORT>(&(m_window[iterator])) = h;
    iterator += sizeof(SHORT);

    *reinterpret_cast<LPDWORD>(&(m_window[iterator])) = id;
    iterator += sizeof(DWORD);

    wmemcpy(reinterpret_cast<LPWSTR>(&(m_window[iterator])),
            window_class.c_str(),
            window_class.size());
    iterator += (sizeof(WCHAR) * window_class.size());
    if (window_class[0] != L'\xFFFF')
    {
        iterator += sizeof(WCHAR);
    }

    if (caption.empty() == false)
    {
        wmemcpy(reinterpret_cast<LPWSTR>(&(m_window[iterator])),
                caption.c_str(),
                caption.size());
        iterator += (sizeof(WCHAR) * caption.size());
    }
    iterator += sizeof(WCHAR);

    *reinterpret_cast<LPWORD>(&(m_window[CDLGITEMS_OFFSET])) += 1;
}

INT_PTR DlgBase::SpawnDialogBox(const DlgBase* parent, const DlgMode mode)
{
    SetLastError(ERROR_SUCCESS);
    INT_PTR result;
    HWND parent_hwnd = (parent != nullptr ? parent->m_handle : nullptr);

    if (m_handle == nullptr)
    {
        m_mode = mode;
        switch (mode)
        {
        case DlgMode::PROMPT:
            result = DialogBoxIndirectParamW(GetModuleHandleW(nullptr),
                                             reinterpret_cast<LPCDLGTEMPLATEW>(m_window.data()),
                                             parent_hwnd,
                                             BaseCallback,
                                             reinterpret_cast<LPARAM>(this));
            return result;
        case DlgMode::INDIRECT:
            /*
             * Don't save the HWND directly here, save it from the callback to unify the saving
             * process with the PROMPT dialogs. CreateDialog[X] will not return until the window
             * has processed the WM_INITDIALOG message.
             */
            HWND ret = CreateDialogIndirectParamW(GetModuleHandleW(nullptr),
                                                  reinterpret_cast<LPCDLGTEMPLATEW>(m_window.data()),
                                                  parent_hwnd,
                                                  BaseCallback,
                                                  reinterpret_cast<LPARAM>(this));
            if (ret == nullptr)
            {
                return -1;
            }
            return 0;
        }
    }

    SetLastError(ERROR_CAN_NOT_COMPLETE);
    return -1;
}

LRESULT DlgBase::SendMessage(UINT msg, WPARAM w_param, LPARAM l_param)
{
    SetLastError(ERROR_SUCCESS);
    if (m_handle == nullptr)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return -1;
    }
    return ::SendMessageW(m_handle, msg, w_param, l_param);
}

LRESULT DlgBase::SendDlgItemMessage(int item_id, UINT msg, WPARAM w_param, LPARAM l_param)
{
    SetLastError(ERROR_SUCCESS);
    if (m_handle == nullptr)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return -1;
    }
    return ::SendDlgItemMessageW(m_handle, item_id, msg, w_param, l_param);
}

BOOL DlgBase::ShowDialogBox(int show_window_cmd)
{
    SetLastError(ERROR_SUCCESS);
    if (m_handle == nullptr)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }
    return ::ShowWindow(m_handle, show_window_cmd);
}

BOOL DlgBase::UpdateWindow()
{
    SetLastError(ERROR_SUCCESS);
    if (m_handle == nullptr)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }
    return ::UpdateWindow(m_handle);
}

void DlgBase::SetReturnCode(INT_PTR return_code)
{
    m_return_code_set = true;
    m_return_code = return_code;
}

BOOL DlgBase::DestroyDialog()
{
    SetLastError(ERROR_SUCCESS);
    if (m_handle == nullptr)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }
    if (m_mode == DlgMode::INDIRECT)
    {
        return ::DestroyWindow(m_handle);
    }
    else if (m_mode == DlgMode::PROMPT)
    {
        if (m_return_code_set == false)
        {
            SetLastError(ERROR_CAN_NOT_COMPLETE);
            return FALSE;
        }
        return ::EndDialog(m_handle, m_return_code);
    }
    return FALSE;
}

bool DlgBase::AddTabPageToTabControl(HWND tab_control, unsigned int pos, LPTCITEMW data)
{
    if (m_handle == nullptr)
    {
        return false;
    }
    /*
     * TODO: More error handling
     * -- Warepire
     */
    ::SendMessageW(tab_control, TCM_INSERTITEM, pos, reinterpret_cast<LPARAM>(data));
    /*
     * Re-position the window to fit within the tab space.
     */
    RECT new_pos;
    ::GetClientRect(m_handle, &new_pos);
    ::SendMessageW(tab_control, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&new_pos));
    ::SetWindowPos(m_handle, NULL, new_pos.left, new_pos.top, NULL, NULL, SWP_NOSIZE);

    return true;
}

bool DlgBase::SetMenu(const Menu* menu) const
{
    HWND anchor_hwnd = GetDlgItem(m_handle, IDC_DLGBASE_INTERNAL_MENU_ANCHOR);
    RECT old_rect = {};
    RECT new_rect = {};
    GetWindowRect(anchor_hwnd, &old_rect);

    BOOL rv = ::SetMenu(m_handle, menu->m_loaded_menu);

    /*
     * If setting the menu pushed the window contents down,
     * extend the bottom of the window by the same amount.
     */
    GetWindowRect(anchor_hwnd, &new_rect);
    int change = new_rect.top - old_rect.top;
    if (change != 0)
    {
        RECT rect;
        GetWindowRect(m_handle, &rect);
        SetWindowPos(m_handle, nullptr, 0, 0,
                     rect.right - rect.left,
                     change + rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }

    return (rv == TRUE) ? true : false;
}

void DlgBase::RegisterCreateEventCallback(std::function<bool()> cb)
{
    m_message_callbacks.emplace(WM_INITDIALOG, std::make_unique<Callback0>(cb));
}

void DlgBase::RegisterCloseEventCallback(std::function<bool()> cb)
{
    m_message_callbacks.emplace(WM_CLOSE, std::make_unique<Callback0>(cb));
}

void DlgBase::RegisterControlEventCallback(DWORD id, std::function<bool(WORD)> cb)
{
    m_wm_command_callbacks.emplace(id, std::make_unique<CallbackWmCommand>(cb));
}

bool DlgBase::DestroyCallback()
{
    /*
     * Post Quit message regardless of m_return_code_set, assume everything is ok (return 0)
     */
    if (ms_ref_count == 1)
    {
        PostQuitMessage(static_cast<int>(m_return_code));
    }
    return true;
}

bool DlgBase::NcDestroyCallback()
{
    m_handle = nullptr;
    m_message_callbacks.clear();
    m_wm_command_callbacks.clear();
    return true;
}

INT_PTR DlgBase::DlgCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    /*
     * TODO: No support for WM_NOTIFY
     */
    bool rv = false;
    auto& cb_map = message == WM_COMMAND ? m_wm_command_callbacks : m_message_callbacks;
    auto& cb = cb_map.find(message == WM_COMMAND ? LOWORD(wparam) : message);
    if (cb != cb_map.end())
    {
        rv = cb->second->Callback(wparam, lparam);
    }

    return rv ? TRUE : FALSE;
}

INT_PTR CALLBACK DlgBase::BaseCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_INITDIALOG)
    {
        /*
         * Window init special case: Map the DlgBase pointer to the window handle,
         * And set the m_handle member of DlgBase. Ugly hack that can't be avoided.
         */
        ms_hwnd_dlgbase_map[window] = reinterpret_cast<DlgBase*>(lparam);
        ms_hwnd_dlgbase_map[window]->m_handle = window;
    }

    auto dlg_from_hwnd = ms_hwnd_dlgbase_map.find(window);
    if (dlg_from_hwnd == ms_hwnd_dlgbase_map.end())
    {
        return FALSE;
    }
    INT_PTR rv = dlg_from_hwnd->second->DlgCallback(window, message, wparam, lparam);

    if (message == WM_NCDESTROY)
    {
        ms_hwnd_dlgbase_map.erase(window);
    }

    return rv;
}
