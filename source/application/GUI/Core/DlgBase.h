/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

/*
 * A lot of object types are missing, please add them as necessary.
 * Refer to the currently implemented objects when creating new ones.
 * -- Warepire
 */

/*
 * ------------------------------------------------------------------------------------------------
 *                                   Implementation notes
 * ------------------------------------------------------------------------------------------------
 *
 * Usage is simple. Just inherit DlgBase and remember that everything is in DialogPoints instead of
 * pixels.
 *
 * This construction allows for enum IDs for the object IDs, and it is encouraged to use this
 * method to declare them in order to avoid ID collisions. It is also encouraged to keep the IDs
 * private to the class inheriting DlgBase.
 *
 * When creating an Indirect dialog, SpawnDialogBox() will return -1 on creation failure and 0 on
 * success. When creating a Prompt dialog, SpawnDialogBox() will return the result of the dialog
 * box when it exits.
 *
 * The purpose of the overloaded WinAPI calls are to send messages to the window and it's controls,
 * without needing to know the HWND.
 * If you provide an external interface to your class, don't just proxy all paramterized calls,
 * wrap them where it makes sense.
 * You should have class members that look like this (example code, do not actually use):
 * bool SendCloseMessage()
 * {
 *    SendMessage(WM_CLOSE, 0, 0);
 *    return true;
 * }
 * These overloaded SendMessage and SendDlgItemMessage functions can now fail with LRESULT -1 and
 * ERROR_CAN_NOT_COMPLETE, this is not a standard error for these functions.
 *
 * Note:
 * Some items like the DropDownList and ListView need further initialization during WM_INITDIALOG
 * as some of their settings are not exposed as style flags, but need setting using
 * SendDlgItemMessage().
 * Use a WM_INITDIALOG callback and perform the init steps in that.
 */

/*
 * Undefine some Windows defines to make sure calling these functions sends the call to the DlgBase
 * implementation.
 */
#undef SendMessage
#undef SendDlgItemMessage

class CallbackBase;

class DlgBase
{
protected:
    enum class DlgType
    {
        NORMAL,
        TAB_PAGE,
    };

    enum class DlgMode
    {
        PROMPT,
        INDIRECT,
    };

    DlgBase(const std::wstring& caption, SHORT x, SHORT y, SHORT w, SHORT h, DlgType type);
    ~DlgBase();

    /*
     * TODO: Categorize.
     * -- Warepire
     */

    void AddPushButton(const std::wstring& caption,
                       DWORD id,
                       SHORT x, SHORT y,
                       SHORT w, SHORT h,
                       bool disable,
                       bool default_choice);
    void AddCheckbox(const std::wstring& caption,
                     DWORD id,
                     SHORT x, SHORT y,
                     SHORT w, SHORT h,
                     bool right_hand);
    void AddRadioButton(const std::wstring& caption,
                        DWORD id,
                        SHORT x, SHORT y,
                        SHORT w, SHORT h,
                        bool right_hand,
                        bool group_with_prev);
    void AddUpDownControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddEditControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h, bool disabled, bool multi_line);
    void AddStaticText(const std::wstring& caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddStaticPanel(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddGroupBox(const std::wstring& caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddDropDownList(DWORD id, SHORT x, SHORT y, SHORT w, SHORT drop_distance);
    void AddListView(DWORD id,
                     SHORT x, SHORT y,
                     SHORT w, SHORT h,
                     bool single_selection, bool owner_data);
    void AddIPEditControl(DWORD id, SHORT x, SHORT y);
    void AddTabControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);


    INT_PTR SpawnDialogBox(const DlgBase* parent, const DlgMode mode);

    LRESULT SendMessage(UINT msg, WPARAM w_param, LPARAM l_param);
    LRESULT SendDlgItemMessage(int item_id, UINT msg, WPARAM w_param, LPARAM l_param);
    BOOL ShowDialogBox(int show_window_cmd);
    BOOL UpdateWindow();
    void SetReturnCode(INT_PTR return_code);
    BOOL DestroyDialog();
    bool AddTabPageToTabControl(HWND tab_control, unsigned int pos, LPTCITEMW data);

    void RegisterControlEventCallback(DWORD id, std::function<bool(WORD)> cb);

private:
    void AddObject(DWORD ex_style, DWORD style,
                   const std::wstring& window_class,
                   const std::wstring& caption,
                   DWORD id,
                   SHORT x, SHORT y,
                   SHORT w, SHORT h);

    bool DestroyCallback();
    bool NcDestroyCallback();

    static DWORD ms_ref_count;

    std::vector<BYTE> m_window;
    HWND m_handle;
    DlgMode m_mode;
    bool m_return_code_set;
    INT_PTR m_return_code;

    std::map<UINT, std::unique_ptr<CallbackBase>> m_wm_command_callbacks;
    std::map<UINT, std::unique_ptr<CallbackBase>> m_message_callbacks;

    INT_PTR DlgCallback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
    /*
     * Due to DLGPROC types being a C-style function pointer, we cannot std::bind it.
     * Instead we have to use this generic callback, which only purpose is to call registered callbacks.
     */
    static INT_PTR CALLBACK BaseCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    static std::map<HWND, DlgBase*> ms_hwnd_dlgbase_map;
};
