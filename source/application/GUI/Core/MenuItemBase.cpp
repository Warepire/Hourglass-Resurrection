/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "DlgBase.h"
#include "MenuItemBase.h"

namespace
{
#pragma pack(push, 1)
    struct MENUEX_TEMPLATE_ITEM
    {
        DWORD dwType;
        DWORD dwState;
        DWORD menuId;
        WORD bResInfo;
        /*
         * Variable length NULL terminated WCHAR array
         */
        WCHAR szText;
        /*
         * DWORD dwHelpId, only present when it's a popup menu.
         */
    };
#pragma pack(pop)
}

MenuItemBase::MenuItemBase(const std::wstring& title, const std::wstring& shortcut, bool submenu, MenuBase* parent, DlgBase* dlg) :
    MenuBase(sizeof(MENUEX_TEMPLATE_ITEM) + (submenu ? sizeof(DWORD) : 0))
{
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data());
    item->dwType = title.empty() ? MFT_SEPARATOR : 0;
    item->menuId = dlg->GetNextID();
    item->bResInfo = submenu ? 0x01 : 0x00;

    ChangeTitle(title);
    ChangeShortcut(shortcut);

    if (parent != nullptr)
    {
        parent->m_children.emplace_back(this);
    }
}

void MenuItemBase::SetUnsetStyleBits(DWORD style, SetBits set)
{
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data());
    if (set == SetBits::Set)
    {
        item->dwState |= style;
    }
    else
    {
        item->dwState &= ~(style);
    }
}

void MenuItemBase::ChangeText(const std::wstring& new_text)
{
    DWORD new_size = sizeof(MENUEX_TEMPLATE_ITEM) + (new_text.size() * sizeof(WCHAR));
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data());
    if (item->bResInfo & 0x01)
    {
        new_size += sizeof(DWORD);
    }
    m_menu.resize(new_size);

    /*
     * It may have been re-allocated
     */
    item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data());

    wmemcpy(&item->szText, new_text.data(), new_text.size());
    memset(&item->szText + new_text.size(), 0, m_menu.size() - (reinterpret_cast<DWORD>(&item->szText) + new_text.size()));
}

void MenuItemBase::ChangeTitle(const std::wstring& title)
{
    ChangeText(title + (m_shortcut.empty() ? L"" : L"\t" + m_shortcut));
    m_title = title;
}

void MenuItemBase::ChangeShortcut(const std::wstring& shortcut)
{
    ChangeText(m_title + (shortcut.empty() ? L"" : L"\t" + shortcut));
    m_shortcut = shortcut;
}

void MenuItemBase::RegisterWmCommandHandler(std::function<bool(WORD)>& cb)
{
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data());
    m_dlg->RegisterWmControlCallback(item->menuId, cb);
}

