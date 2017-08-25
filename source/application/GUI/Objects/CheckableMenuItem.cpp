/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>
#include <string>

#include "../Core/MenuItemBase.h"

#include "CheckableMenuItem.h"

class DlgBase;

CheckableMenuItem::CheckableMenuItem(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg) :
    MenuItemBase(title, shortcut, false, parent, dlg)
{
}

CheckableMenuItem& CheckableMenuItem::SetEnabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Unset);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::SetDisabled()
{
    SetUnsetStyleBits(MFS_DISABLED, SetBits::Set);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::SetChecked()
{
    SetUnsetStyleBits(MFS_CHECKED, SetBits::Set);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::SetUnchecked()
{
    SetUnsetStyleBits(MFS_CHECKED, SetBits::Unset);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::ChangeTitle(const std::wstring& title)
{
    MenuItemBase::ChangeTitle(title);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::ChangeShortcut(const std::wstring& shortcut)
{
    MenuItemBase::ChangeShortcut(shortcut);
    return *this;
}

CheckableMenuItem& CheckableMenuItem::RegisterOnClickHandler(std::function<bool(WORD)>& cb)
{
    RegisterWmCommandHandler(cb);
    return *this;
}
