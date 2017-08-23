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
class Menu;

class DlgBase
{
protected:
    enum class DlgMode
    {
        PROMPT,
        INDIRECT,
    };

    DlgBase(const std::wstring& caption, SHORT x, SHORT y, SHORT w, SHORT h);
    ~DlgBase();

    //void AddUpDownControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    //void AddDropDownList(DWORD id, SHORT x, SHORT y, SHORT w, SHORT drop_distance);
    //void AddListView(DWORD id,
    //                 SHORT x, SHORT y,
    //                 SHORT w, SHORT h,
    //                 bool single_selection, bool owner_data);
    //void AddIPEditControl(DWORD id, SHORT x, SHORT y);

    INT_PTR SpawnDialogBox(const DlgBase* parent, const DlgMode mode);

    LRESULT SendMessage(UINT msg, WPARAM w_param, LPARAM l_param);
    LRESULT SendDlgItemMessage(int item_id, UINT msg, WPARAM w_param, LPARAM l_param);
    BOOL ShowDialogBox(int show_window_cmd);
    BOOL UpdateWindow();
    void SetReturnCode(INT_PTR return_code);
    BOOL DestroyDialog();

    DWORD GetNextID();
    SIZE_T AddObject(const std::vector<BYTE>& object);
    void SetNewStyle(SIZE_T obj_offset, DWORD ex_style, DWORD style);

    void RegisterCreateEventCallback(std::function<bool()> cb);
    void RegisterCloseEventCallback(std::function<bool()> cb);

    void RegisterWmControlCallback(DWORD id, std::function<bool(WORD)> cb);

private:
    bool SetMenu(const Menu* menu) const;

    bool DestroyCallback();
    bool NcDestroyCallback();

    static DWORD ms_ref_count;

    std::vector<BYTE> m_window;
    HWND m_handle;
    DlgMode m_mode;
    bool m_return_code_set;
    INT_PTR m_return_code;

    DWORD m_next_id;

    std::map<UINT, std::unique_ptr<CallbackBase>> m_wm_command_callbacks;
    std::map<UINT, std::unique_ptr<CallbackBase>> m_message_callbacks;

    INT_PTR DlgCallback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
    /*
     * Due to DLGPROC types being a C-style function pointer, we cannot std::bind it.
     * Instead we have to use this generic callback, which only purpose is to call registered callbacks.
     */
    static INT_PTR CALLBACK BaseCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    static std::map<HWND, DlgBase*> ms_hwnd_dlgbase_map;

    friend class Menu;

    template<std::size_t>
    friend class ObjBase;
};
