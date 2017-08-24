#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>
#include <string>
#include <vector>

class DlgBase;

class MenuItemBase
{
protected:
    enum class SetBits
    {
        Set,
        Unset,
    };

    MenuItemBase(const std::wstring& title, const std::wstring& shortcut, bool submenu, MenuItemBase* parent, DlgBase* dlg);

    void SetUnsetStyleBits(DWORD style, SetBits set);

    void ChangeTitle(const std::wstring& title);
    void ChangeShortcut(const std::wstring& shortcut);

    void RegisterWmCommandHandler(std::function<bool(WORD)>& cb);
private:
    void ChangeText(const std::wstring& new_text);

    std::vector<BYTE> m_menu;
    std::vector<MenuItemBase*> m_children;

    std::wstring m_title;
    std::wstring m_shortcut;

    DlgBase* m_dlg;
};
