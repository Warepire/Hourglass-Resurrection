/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <cassert>
#include <cwchar>
#include <string>
#include <vector>

#include "shared/Alignment.h"

#include "Menu.h"

namespace
{
    enum : DWORD
    {
        IDC_STATIC = static_cast<DWORD>(-1),
    };

#pragma pack(push,1)
    struct MENUEX_TEMPLATE_HEADER
    {
        WORD wVersion;
        WORD wOffset;
        DWORD dwHelpId;
    };
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
};

Menu::Menu() :
    m_menu_depth(0),
    m_loaded_menu(nullptr)
{
    m_menu.resize(sizeof(MENUEX_TEMPLATE_HEADER));
    auto header = reinterpret_cast<MENUEX_TEMPLATE_HEADER*>(m_menu.data());
    header->wVersion = 0x01;
    header->wOffset = 0x04;
}

Menu::~Menu()
{
    if (m_loaded_menu != nullptr)
    {
        DestroyMenu(m_loaded_menu);
    }
}

void Menu::BeginMenuCategory(const std::wstring& name, DWORD id, bool enabled)
{

    DWORD state = (enabled ? MFS_ENABLED : MFS_DISABLED);
    m_menu_depth++;
    AddMenuObject(name, id, MFT_STRING, state, 0x01);
}

void Menu::BeginSubMenu(const std::wstring& name, DWORD id, bool enabled)
{
    /*
     * It's the same code-segment, just call the Category function instead
     */
    BeginMenuCategory(name, id, enabled);
}

void Menu::AddMenuItem(const std::wstring& name, const std::wstring& shortcut, DWORD id, bool enabled, bool default_choice)
{
    DWORD state = (enabled ? MFS_ENABLED : MFS_DISABLED) | (default_choice ? MFS_DEFAULT : 0);
    AddMenuObject(name + L'\t' + shortcut, id, MFT_STRING, state, 0x00);
}

void Menu::AddCheckableMenuItem(const std::wstring& name, DWORD id,
                                bool enabled, bool checked)
{
    DWORD state = (enabled ? MFS_ENABLED : MFS_DISABLED) |
                  (checked ? MFS_CHECKED : MFS_UNCHECKED);
    AddMenuObject(name, id, MFT_STRING, state, 0x00);
}

void Menu::AddMenuItemSeparator()
{
    AddMenuObject(L"", IDC_STATIC, MFT_SEPARATOR, 0, 0);
}

void Menu::EndMenuCategory()
{
    /*
     * Having mismatched depth is a bug, and LoadMenuIndirectW will only tell you 0xD (Invalid data)
     */
    assert(m_menu_depth == 1);
    m_menu_depth--;
}

void Menu::EndSubMenu()
{
    /*
     * Having mismatched depth is a bug, and LoadMenuIndirectW will only tell you 0xD (Invalid data)
     */
    assert(m_menu_depth > 1);
    m_menu_depth--;
}


void Menu::AddMenuObject(const std::wstring& name, DWORD id, DWORD type, DWORD state, WORD res)
{
    /*
     * Unset the previous item at current depth as the last item.
     */
    if (m_menu_depth < m_depth_offsets.size())
    {
        auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data() + m_depth_offsets.top());
        item->bResInfo &= ~0x80;
        m_depth_offsets.pop();
    }

    SIZE_T old_size = m_menu.size();

    m_depth_offsets.emplace(old_size);
    DWORD new_size = sizeof(MENUEX_TEMPLATE_ITEM) + old_size;
    new_size += (sizeof(WCHAR) * name.size());
    new_size += ((res & 0x01) != 0x00) ? sizeof(DWORD) : 0;
    new_size = AlignValueTo<sizeof(DWORD)>(new_size);

    m_menu.resize(new_size);

    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_menu.data() + old_size);

    item->dwType = type;
    item->dwState = state;
    item->menuId = id;
    /*
     * Always assume it's the last item
     */
    item->bResInfo = res | 0x80;

    if (type != MFT_SEPARATOR)
    {
        wmemcpy(&item->szText, name.c_str(), name.size());
    }
}

/*
 * TODO: Make sure attaching a menu fixes the size of the dialog
 * -- Warepire
 */

bool Menu::AttachMenu(HWND window)
{
    m_loaded_menu = LoadMenuIndirectW(reinterpret_cast<LPMENUTEMPLATEW>(m_menu.data()));
    if (m_loaded_menu == nullptr)
    {
        return false;
    }
    if (SetMenu(window, m_loaded_menu) == FALSE)
    {
        DestroyMenu(m_loaded_menu);
        m_loaded_menu = nullptr;
        return false;
    }
    return true;
}
