#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/MenuItemBase.h"

class SubMenu : public MenuItemBase
{
public:
    SubMenu(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg);

    SubMenu& SetEnabled();
    SubMenu& SetDisabled();
};
