/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stack>
#include <string>
#include <vector>

/*
 * ------------------------------------------------------------------------------------------------
 *                                            USAGE
 * ------------------------------------------------------------------------------------------------
 *
 * Usage is simple. Just remember that everything must happen in sequence and that AttachMenu shall
 * be called from the Dialog Init message.
 */

class Menu
{
public:
    Menu();
    ~Menu();

    void BeginMenuCategory(const std::wstring& name, DWORD id, bool enabled);
    void BeginSubMenu(const std::wstring& name, DWORD id, bool enabled);
    void AddMenuItem(const std::wstring& name, const std::wstring& shortcut, DWORD id, bool enabled, bool default_choice);
    void AddCheckableMenuItem(const std::wstring& name, DWORD id,
                              bool enabled, bool checked);
    void AddMenuItemSeparator();

    void EndMenuCategory();
    void EndSubMenu();

    bool AttachMenu(HWND window);
private:

    void AddMenuObject(const std::wstring& name, DWORD id, DWORD type, DWORD state, WORD res);
    std::vector<BYTE> m_menu;
    DWORD m_menu_depth;
    std::stack<DWORD> m_depth_offsets;

    HMENU m_loaded_menu;
};
